/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <string.h>
#include <drivers/mmcsd_core.h>
#include <drivers/sdio.h>

#define LOG_TAG         "SDMC"

#include "aic_core.h"
#include "aic_hal.h"

#include "bouncebuf.h"
#include "hal_sdmc.h"

#define MSEC_PER_SEC        1000

struct aic_sdmc *g_host = NULL;

struct aic_sdmc_pdata {
    ulong base;
    int irq;
    int clk;
    u32 is_sdio;
    u8 id;
    u8 buswidth;
    u8 drv_phase;
    u8 smp_phase;
};

/**
 * struct aic_sdmc - Information about a ArtInChip SDMC host
 *
 * @quirks:     Quick flags - see SDMC_QUIRK_...
 * @caps:       Capabilities - see MMC_MODE_...
 * @sclk_rate:  The rate of SDMC clk in Hz. It's the basis clk for SDMC.
 * @div:        Clock divider value for use by controller
 * @buswidth:   Bus width in bits (8 or 4)
 */
struct aic_sdmc {
    struct rt_mmcsd_host *rthost;
    struct rt_mmcsd_req *req;
    struct rt_mmcsd_cmd *cmd;
    struct aic_sdmc_host host;

    rt_uint32_t *buf;
    u32 clk;
    u32 irq;
    u32 index;

    unsigned int quirks;
    unsigned int caps;
    unsigned int version;
    unsigned int clock;
    unsigned int sclk_rate;
    unsigned int div;
    int buswidth;
    int ddr_mode;

    /* use fifo mode to read and write data */
    int fifo_mode;

    struct aic_sdmc_pdata *pdata;

    u8 is_enable;
};

static inline int resp_crc_type(struct rt_mmcsd_cmd *cmd)
{
    u32 type = resp_type(cmd);

    if ((type == RESP_R1) || (type == RESP_R1B) || (type == RESP_R2) || \
         (type == RESP_R5) || (type == RESP_R6) || (type == RESP_R7))
        return 1;
    else
        return 0;
}

static u32 aic_sdmc_buswidth(u32 type)
{
    switch (type) {
    case SDMC_CTYPE_8BIT:
        return 8;
    case SDMC_CTYPE_4BIT:
        return 4;
    case SDMC_CTYPE_1BIT:
        return 1;
    default:
    case SDMC_CTYPE_RESERVED:
        pr_warn("Invalid Card type %d\n", type);
        return 1;
    }
}

#ifndef AIC_SDMC_IRQ_MODE
static u32 aic_sdmc_get_timeout(struct aic_sdmc *host, const u32 size)
{
    unsigned int timeout;

    timeout = size * 8;
    timeout /= aic_sdmc_buswidth(host->buswidth);
    timeout *= 10;      /* wait 10 times as long */
    timeout /= (host->clock / MSEC_PER_SEC);
    timeout /= host->ddr_mode ? 2 : 1;
    timeout = (timeout < MSEC_PER_SEC) ? MSEC_PER_SEC : timeout;
    return timeout;
}

static int aic_sdmc_data_transfer(struct aic_sdmc *host,
                                  struct rt_mmcsd_data *data)
{
    int ret = 0;
    u32 mask, size, total;
    rt_tick_t timeout, end, start = rt_tick_get_millisecond();

    total = data->blksize * data->blks;
    timeout = aic_sdmc_get_timeout(host, total);

    size = total / 4;
    for (;;) {
        mask = hal_sdmc_int_stat(&host->host);
        if (mask & (SDMC_DATA_ERR | SDMC_DATA_TOUT)) {
            pr_err("Data error! size %d/%d, mode %s-%s, status 0x%x\n",
                   size * 4, total,
                   host->fifo_mode ? "FIFO" : "IDMA",
                   data->flags == DATA_DIR_READ ? "Read" : "Write", mask);
            ret = -EINVAL;
            break;
        }

        // mmcsd_dbg("%s: mask %#x, size %d, timeout %d\n",
        //           data->flags == DATA_DIR_READ ? "Read" : "Write",
        //           mask, size, timeout);
        if (host->fifo_mode && size) {
            if (data->flags == DATA_DIR_READ &&
                    (mask & (SDMC_INT_RXDR | SDMC_INT_DAT_DONE)))
                size = hal_sdmc_data_rx(&host->host, (u32 *)data->buf, size);
            else if (data->flags == DATA_DIR_WRITE &&
                     (mask & SDMC_INT_TXDR))
                size = hal_sdmc_data_tx(&host->host, (u32 *)data->buf, size);
        }

        if (mask & SDMC_INT_DAT_DONE) {
            ret = 0;
            break;
        }

        end = rt_tick_get_millisecond();
        if (end - start > timeout) {
            LOG_W("Data timeout %d - %d > %d! size %d/%d, mode %s-%s, status 0x%x\n",
                    end, start,
                    timeout, size * 4, total,
                    host->fifo_mode ? "FIFO" : "IDMA",
                    data->flags == DATA_DIR_READ ? "Read" : "Write", mask);
            data->err = -RT_ETIMEOUT;
            ret = -RT_ETIMEOUT;
            break;
        }
    }

    hal_sdmc_int_clr(&host->host, mask);
    return ret;
}
#endif

static int aic_sdmc_set_transfer_mode(struct aic_sdmc *host,
                                      struct rt_mmcsd_data *data)
{
    unsigned long mode = SDMC_CMD_DAT_EXP;

    if (data->flags & DATA_DIR_WRITE)
        mode |= SDMC_CMD_DAT_WR;

    return mode;
}

static void aic_sdmc_request(struct rt_mmcsd_host *rthost,
                             struct rt_mmcsd_req *req)
{
    struct aic_sdmc *host;
    struct rt_mmcsd_cmd *cmd = req->cmd;
    struct rt_mmcsd_data *data = req->data;
    s32 ret = 0, flags = 0, i;
    u32 retry = SDMC_TIMEOUT;
    u32 mask;
    rt_tick_t timeout = 500;
    rt_tick_t start = rt_tick_get_millisecond();
    struct bounce_buffer bbstate;
    ALLOC_CACHE_ALIGN_BUFFER(struct aic_sdmc_idma_desc, cur_idma,
                             data ? DIV_ROUND_UP(data->blks, 8) : 0);

    RT_ASSERT(rthost != RT_NULL);
    RT_ASSERT(req != RT_NULL);

    host = (struct aic_sdmc *)rthost->private_data;
    RT_ASSERT(host != RT_NULL);

    cmd = req->cmd;
    RT_ASSERT(cmd != RT_NULL);

    if (cmd->cmd_code != STOP_TRANSMISSION) {
        while (hal_sdmc_is_busy(&host->host)) {
            if (rt_tick_get_millisecond() - start > timeout) {
                pr_warn("Data transfer is busy\n");
                cmd->err = -RT_EBUSY;
                goto out;
            }
        }
    }

    // pr_debug("cmd_code: %02d, arg: %08x, flags: %08x --> ", cmd->cmd_code, cmd->arg, cmd->flags);

    hal_sdmc_int_clr(&host->host, SDMC_INT_ALL);
    if (data) {
    #ifdef AIC_SDMC_IRQ_MODE    /* SDIO card always enable AIC_SDMC_IRQ_MODE */
        host->fifo_mode = 0;
    #else
        if ((data->blksize * data->blks) < 512)
            host->fifo_mode = 1;
        else
            host->fifo_mode = 0;
    #endif

        hal_sdmc_set_blk(&host->host, data->blksize, data->blks);
        hal_sdmc_reset(&host->host, SDMC_HCTRL1_FIFO_RESET);
        if (!host->fifo_mode) {
            ret = hal_sdmc_idma_start(&host->host, data->blksize * data->blks,
                                      data->flags == DATA_DIR_READ,
                                      (u32 *)data->buf, &bbstate);
            if (ret) {
                cmd->err = -RT_ERROR;
                goto out;
            }

            hal_sdmc_idma_prepare(&host->host, data->blksize, data->blks,
                                  cur_idma, bbstate.bounce_buffer);
        } else {
            if (hal_sdmc_get_idma_status(&host->host))
                hal_sdmc_idma_disable(&host->host);
        }
    }
    if (data)
        mmcsd_dbg("fifo_mode: %d, block: %d x %d\n", host->fifo_mode,
                  data->blksize, data->blks);
    else
        mmcsd_dbg("fifo_mode: %d\n", host->fifo_mode);

    hal_sdmc_set_arg(&host->host, cmd->arg);
    if (data)
        flags = aic_sdmc_set_transfer_mode(host, data);

    // if (resp_type(cmd) == RESP_R2) && (cmd->resp_type & MMC_RSP_BUSY))
    //     return -1;

    if (cmd->cmd_code == GO_IDLE_STATE)
        flags |= SDMC_CMD_INIT;

    if (cmd->cmd_code == STOP_TRANSMISSION)
        flags |= SDMC_CMD_STOP;
    else
        flags |= SDMC_CMD_PRV_DAT_WAIT;

    if (resp_type(cmd) != RESP_NONE) {
        flags |= SDMC_CMD_RESP_EXP;
        if (resp_type(cmd) == RESP_R2)
            flags |= SDMC_CMD_RESP_LEN;
    }

    if (resp_crc_type(cmd))
        flags |= SDMC_CMD_RESP_CRC;

    flags |= (cmd->cmd_code | SDMC_CMD_START | SDMC_CMD_USE_HOLD_REG);
    hal_sdmc_set_cmd(&host->host, flags);

    for (i = 0; i < retry; i++) {
        mask = hal_sdmc_int_stat(&host->host);
        if (mask & SDMC_INT_CMD_DONE) {
            if (!data)
                hal_sdmc_int_clr(&host->host, mask);
            break;
        }
    }
    mmcsd_dbg("Flags: 0x%x, mask: 0x%x\n", flags, mask);

    if (i == retry) {
        LOG_W("CMD%d done timeout.\n", cmd->cmd_code);
        cmd->err = -RT_ETIMEOUT;
        goto out;
    }

    if (mask & SDMC_INT_RTO) {
        pr_debug("CMD%d Response Timeout.\n", cmd->cmd_code);
        cmd->err = -RT_ETIMEOUT;
        goto out;
    } else if (mask & SDMC_INT_RESP_ERR) {
        pr_err("CMD%d Response Error.\n", cmd->cmd_code);
        cmd->err = -RT_EIO;
        goto out;
    } else if (resp_crc_type(cmd) && (mask & SDMC_INT_RCRC)) {
        pr_err("CMD%d Response CRC Error.\n", cmd->cmd_code);
        cmd->err = -RT_EINVAL;
        goto out;
    }

    if (resp_type(cmd) != RESP_NONE)
        hal_sdmc_get_rsp(&host->host, (u32 *)cmd->resp, resp_type(cmd) == RESP_R2);

    if (data) {
#ifndef AIC_SDMC_IRQ_MODE
        aic_sdmc_data_transfer(host, data);
        if (!(host->fifo_mode))
            hal_sdmc_idma_stop(&host->host, &bbstate,
                               data->flags == DATA_DIR_READ);
#else
        hal_sdmc_idma_stop(&host->host, &bbstate,data->flags == DATA_DIR_READ);
#endif
    }

out:
    mmcsd_req_complete(rthost);
}

static u32 aic_sdmc_get_best_div(u32 sclk, u32 target_freq)
{
    u32 down, up, f_down, f_up;

    down = sclk / target_freq;
    up = DIV_ROUND_UP(sclk, target_freq);

    f_down = down == 0 ? sclk : sclk / down;
    f_up = up == 0 ? sclk : sclk / up;

    /* Select the closest div parameter */
    if ((f_down - target_freq) < (target_freq - f_up))
        return down;
    return up;
}

static int aic_sdmc_setup_bus(struct aic_sdmc *host, u32 freq)
{
    u32 mux, div, sclk = host->sclk_rate;

    if ((freq == host->clock) || (freq == 0))
        return 0;

    if (sclk == freq) {
        /* bypass mode */
        mux = 1;
        div = 0;
    } else {
        div = aic_sdmc_get_best_div(sclk, freq);
        if (div <= 4) {
            mux = DIV_ROUND_UP(div, 2);
        } else {
            if (div % 8)
                mux = 2;
            else
                mux = 4;
        }
        div /= mux * 2;
        if (div > SDMC_CLKCTRL_DIV_MAX)
            div = SDMC_CLKCTRL_DIV_MAX;
    }
    aic_sdmc_set_ext_clk_mux(&host->host, mux);
    LOG_I("SDMC%d BW %d, sclk %d KHz, clk %d KHz(%d KHz), div %d-%d\n",
            host->index, aic_sdmc_buswidth(host->buswidth),
            sclk / 1000, freq / 1000,
            div ? sclk / mux / div / 2 / 1000 : sclk / mux / 1000,
            mux, div * 2);

    hal_sdmc_set_div(&host->host, div);
    hal_sdmc_set_cmd(&host->host,
                    SDMC_CMD_PRV_DAT_WAIT | SDMC_CMD_UPD_CLK | SDMC_CMD_START);

    if (hal_sdmc_wait_cmd_started(&host->host))
        return -RT_ETIMEOUT;

    hal_sdmc_clk_enable(&host->host);
    hal_sdmc_set_cmd(&host->host,
                    SDMC_CMD_PRV_DAT_WAIT | SDMC_CMD_UPD_CLK | SDMC_CMD_START);

    if (hal_sdmc_wait_cmd_started(&host->host))
        return -RT_ETIMEOUT;

    host->clock = freq;
    return 0;
}

static int aic_sdmc_init(struct aic_sdmc *host)
{
    if (hal_sdmc_reset(&host->host, SDMC_HCTRL1_RESET_ALL)) {
        pr_err("Failed to reset!\n");
        return -RT_EIO;
    }

    aic_sdmc_setup_bus(host, host->rthost->freq_min);

    hal_sdmc_init(&host->host);
    hal_sdmc_fifo_init(&host->host, &host->host.fifoth_val);
    hal_sdmc_set_phase(&host->host,
                       host->pdata->drv_phase, host->pdata->smp_phase);
    hal_sdmc_clk_enable(&host->host);
    return 0;
}

static void aic_sdmc_set_iocfg(struct rt_mmcsd_host *rthost,
                               struct rt_mmcsd_io_cfg *io_cfg)
{
    struct aic_sdmc *host;

    RT_ASSERT(rthost != RT_NULL);
    RT_ASSERT(rthost->private_data != RT_NULL);
    RT_ASSERT(io_cfg != RT_NULL);

    host = (struct aic_sdmc *)rthost->private_data;

    switch (io_cfg->bus_width) {
    case MMCSD_DDR_BUS_WIDTH_8:
        // host->ddr_mode = 1;
    case MMCSD_BUS_WIDTH_8:
        host->buswidth = SDMC_CTYPE_8BIT;
        break;
    case MMCSD_DDR_BUS_WIDTH_4:
        // host->ddr_mode = 1;
    case MMCSD_BUS_WIDTH_4:
        host->buswidth = SDMC_CTYPE_4BIT;
        break;
    default:
        host->buswidth = SDMC_CTYPE_1BIT;
        break;
    }
    if (host->buswidth != SDMC_CTYPE_1BIT)
        pr_info("SDMC%d Buswidth %d, DDR mode %d, Current clock: %d KHz\n",
            host->index,
            aic_sdmc_buswidth(host->buswidth), host->ddr_mode,
            io_cfg->clock / 1000);

    hal_sdmc_set_buswidth(&host->host, host->buswidth);
    hal_sdmc_set_ddrmode(&host->host, host->ddr_mode);

    switch (io_cfg->power_mode) {
    case MMCSD_POWER_UP:
        break;
    case MMCSD_POWER_ON:
        if (!host->is_enable) {
            host->is_enable = 1;
            hal_sdmc_reset(&host->host, SDMC_HCTRL1_RESET_ALL);
            host->clock = 0;
            aic_sdmc_setup_bus(host, io_cfg->clock);
        } else {
            aic_sdmc_setup_bus(host, io_cfg->clock);
        }
        break;
    case MMCSD_POWER_OFF:
        host->is_enable = 0;
        hal_sdmc_clk_disable(&host->host);
        hal_sdmc_set_cmd(&host->host,
                    SDMC_CMD_PRV_DAT_WAIT | SDMC_CMD_UPD_CLK | SDMC_CMD_START);
        break;
    }
}

static void aic_sdmc_enable_sdio_irq(struct rt_mmcsd_host *rthost,
                               rt_int32_t en)
{
    struct aic_sdmc *host;

    RT_ASSERT(rthost != RT_NULL);
    RT_ASSERT(rthost->private_data != RT_NULL);

    host = (struct aic_sdmc *)rthost->private_data;

    hal_sdmc_sdio_irq_enable(&host->host, en);
}

static const struct rt_mmcsd_host_ops ops =
{
    aic_sdmc_request,
    aic_sdmc_set_iocfg,
    RT_NULL,//_mmc_get_card_status,
    aic_sdmc_enable_sdio_irq,
};

/* Only enable two types of interrupt: SDIO and DTO */
irqreturn_t aic_sdmc_irq(int irq, void *arg)
{
#ifdef AIC_SDMC_IRQ_MODE
    struct aic_sdmc *host = (struct aic_sdmc *)arg;
    u32 stat = 0;

    stat = hal_sdmc_int_stat(&host->host);
    pr_debug("SDMC IRQ status: 0x%x\n", stat);

    if (stat & SDMC_INT_FROM_SDIO) {
        hal_sdmc_int_clr(&host->host, SDMC_INT_FROM_SDIO);
        sdio_irq_wakeup(host->rthost);
    }

    /* Then, check the DMA status */
    if (stat & SDMC_INT_DAT_DONE) {
        hal_sdmc_int_clr(&host->host, SDMC_INT_DAT_DONE);
        hal_sdmc_idma_update_intstat(&host->host);
        aicos_sem_give(host->host.complete);
    }
#endif
    return IRQ_HANDLED;
}

void aic_sdmc_setup_cfg(struct rt_mmcsd_host *rthost)
{
    struct aic_sdmc *host = (struct aic_sdmc *)rthost->private_data;

    rthost->ops = &ops;
    rthost->freq_min = SDMC_CLOCK_MIN;
    rthost->freq_max = SDMC_CLOCK_MAX;
    rthost->valid_ocr = VDD_32_33 | VDD_33_34;
    rthost->flags = MMCSD_MUTBLKWRITE | \
                  MMCSD_SUP_HIGHSPEED | MMCSD_SUP_SDIO_IRQ;

    if (host->pdata->buswidth == SDMC_CTYPE_4BIT)
        rthost->flags |= MMCSD_BUSWIDTH_4;
    else if (host->pdata->buswidth == SDMC_CTYPE_8BIT)
        rthost->flags |= MMCSD_BUSWIDTH_8;

    rthost->max_seg_size = 4096;
    rthost->max_dma_segs = 256;
    rthost->max_blk_size = 512;
    rthost->max_blk_count = 65535;
}

s32 aic_sdmc_clk_init(struct aic_sdmc *host)
{
    s32 ret = 0;

    /* First, disable SDMC controller to reset FIFO status */
    hal_clk_disable_assertrst(host->clk);
    hal_clk_disable(host->clk);

    ret = hal_clk_get_freq(hal_clk_get_parent(host->clk));
    hal_clk_set_freq(host->clk, SDMC_CLOCK_MAX);
    host->sclk_rate = hal_clk_get_freq(host->clk) / 2;
    pr_info("SDMC%d sclk: %d KHz, parent clk %d KHz\n",
            host->index, host->sclk_rate / 1000, ret / 1000);

    ret = hal_clk_enable(host->clk);
    if (ret < 0) {
        pr_err("SDMC%d clk enable failed!\n", host->index);
        return -1;
    }

    ret = hal_clk_enable_deassertrst(host->clk);
    if (ret < 0) {
        pr_err("SDMC%d reset deassert failed!\n", host->index);
        return -1;
    }

    return 0;
}

static struct aic_sdmc_pdata sdmc_pdata[] = {
#ifdef AIC_USING_SDMC0
    {
        .id = 0,
        .base = SDMC0_BASE,
        .irq = SDMC0_IRQn,
        .clk = CLK_SDMC0,
#ifdef AIC_SDMC0_BUSWIDTH1
        .buswidth = SDMC_CTYPE_1BIT,
#endif
#ifdef AIC_SDMC0_BUSWIDTH4
        .buswidth = SDMC_CTYPE_4BIT,
#endif
#ifdef AIC_SDMC0_BUSWIDTH8
        .buswidth = SDMC_CTYPE_8BIT,
#endif
        .drv_phase = AIC_SDMC0_DRV_PHASE,
        .smp_phase = AIC_SDMC0_SMP_PHASE,
    },
#endif
#ifdef AIC_USING_SDMC1
    {
        .id = 1,
        .base = SDMC1_BASE,
        .irq = SDMC1_IRQn,
        .clk = CLK_SDMC1,
#ifdef AIC_SDMC1_BUSWIDTH1
        .buswidth = SDMC_CTYPE_1BIT,
#endif
#ifdef AIC_SDMC1_BUSWIDTH4
        .buswidth = SDMC_CTYPE_4BIT,
#endif
#ifdef AIC_SDMC1_BUSWIDTH8
        .buswidth = SDMC_CTYPE_8BIT,
#endif
#ifdef AIC_SDMC1_IS_SDIO
        .is_sdio = 1,
#endif
        .drv_phase = AIC_SDMC1_DRV_PHASE,
        .smp_phase = AIC_SDMC1_SMP_PHASE,
    },
#endif
#ifdef AIC_USING_SDMC2
    {
        .id = 2,
        .base = SDMC2_BASE,
        .irq = SDMC2_IRQn,
        .clk = CLK_SDMC2,
#ifdef AIC_SDMC2_BUSWIDTH1
        .buswidth = SDMC_CTYPE_1BIT,
#endif
#ifdef AIC_SDMC2_BUSWIDTH4
        .buswidth = SDMC_CTYPE_4BIT,
#endif
#ifdef AIC_SDMC2_BUSWIDTH8
        .buswidth = SDMC_CTYPE_8BIT,
#endif
#ifdef AIC_SDMC2_IS_SDIO
        .is_sdio = 1,
#endif
        .drv_phase = AIC_SDMC2_DRV_PHASE,
        .smp_phase = AIC_SDMC2_SMP_PHASE,
    },
#endif
};

s32 aic_sdmc_probe(struct aic_sdmc_pdata *pdata)
{
    struct rt_mmcsd_host *rthost = NULL;
    struct aic_sdmc *host = NULL;

    rthost = mmcsd_alloc_host();
    if (!rthost)
        return -1;

    host = malloc(sizeof(struct aic_sdmc));
    if (!host) {
        pr_err("Failed to malloc(%d)\n", (u32)sizeof(struct aic_sdmc));
        goto err;
    }

    memset(host, 0, sizeof(struct aic_sdmc));
    host->index = pdata->id;
    host->irq = pdata->irq;
    host->clk = pdata->clk;
    host->host.base = (volatile void *)pdata->base;
    host->pdata = pdata;
    if (aic_sdmc_clk_init(host) < 0)
        goto err;

    aicos_request_irq(host->irq, aic_sdmc_irq, 0, NULL, host);

    host->host.fifoth_val = MSIZE(2) | RX_WMARK(7) | TX_WMARK(8);
    host->host.is_sdio = pdata->is_sdio;
    host->rthost = rthost;
    rthost->private_data = host;
    aic_sdmc_setup_cfg(rthost);

    aic_sdmc_init(host);
    pr_info("SDMC%d driver loaded\n", pdata->id);

    g_host = host;

    mmcsd_change(rthost);
    return 0;

err:

    if (host)
        free(host);

    if (rthost)
        mmcsd_free_host(rthost);

    return -RT_ENOMEM;
}

void aic_mmcsd_change(void)
{
    mmcsd_change(g_host->rthost);
}

static int drv_sdmc_init(void)
{
    int i;
    rt_err_t ret = RT_EOK;

    for (i = 0; i < ARRAY_SIZE(sdmc_pdata); i++) {
        ret = aic_sdmc_probe(&sdmc_pdata[i]);
        if (ret)
            return ret;
    }

    return 0;
}
INIT_DEVICE_EXPORT(drv_sdmc_init);
