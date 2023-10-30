/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_dma.h"
#include "drv_dma.h"

void drv_dma_deinit(void)
{
    hal_clk_disable_assertrst(CLK_DMA);
    hal_clk_disable(CLK_DMA);
}

int drv_dma_init(void)
{
    s32 ret = 0;

    if (hal_clk_is_enabled(CLK_DMA))
        drv_dma_deinit();

    ret = hal_clk_enable(CLK_DMA);
    if (ret < 0) {
        pr_err("DMA BUS clk enable failed!");
        return -1;
    }

    ret = hal_clk_enable_deassertrst(CLK_DMA);
    if (ret < 0) {
        pr_err("DMA reset deassert failed!");
        return -1;
    }

    hal_dma_init();
    aicos_request_irq(DMA_IRQn, hal_dma_irq, 0, NULL, NULL);

    pr_info("ArtInChip DMA loaded\n");
    return 0;
}

#ifdef KERNEL_RTTHREAD
INIT_BOARD_EXPORT(drv_dma_init);
#endif

long drv_dma_ch_alloc(struct dma_chan *dma_ch, int8_t ch_id, int8_t ctrl_id)
{
    struct aic_dma_chan * chan;

    chan = hal_request_dma_chan();

    *(struct aic_dma_chan **)dma_ch = chan;

    return 0;
}

void drv_dma_ch_free(struct dma_chan *dma_ch)
{
    struct aic_dma_chan * chan = *(struct aic_dma_chan **)dma_ch;

    hal_release_dma_chan(chan);
}

long drv_dma_ch_config(struct dma_chan *dma_ch,
                           struct dma_slave_config *config)
{
    struct aic_dma_chan * chan = *(struct aic_dma_chan **)dma_ch;

    return hal_dma_chan_config(chan, config);
}

long drv_dma_ch_attach_callback(struct dma_chan *dma_ch, void *callback, void *arg)
{
    struct aic_dma_chan * chan = *(struct aic_dma_chan **)dma_ch;
    int ret;

    ret = hal_dma_chan_register_cb(chan, callback, arg);

    return ret;
}

void drv_dma_ch_detach_callback(struct dma_chan *dma_ch)
{
    dma_ch->chan.callback = NULL;
    dma_ch->chan.callback_param = NULL;
}

void drv_dma_ch_start(struct dma_chan *dma_ch, void *srcaddr, void *dstaddr, u32 length)
{
    struct aic_dma_chan * chan = *(struct aic_dma_chan **)dma_ch;
    int ret;

    if (DMA_MEM_TO_MEM == chan->cfg.direction){
        ret = hal_dma_chan_prep_memcpy(chan, (unsigned long)dstaddr,
                                        (unsigned long)srcaddr, length);
    } else {
        ret = hal_dma_chan_prep_device(chan, (unsigned long)dstaddr,
                                        (unsigned long)srcaddr, length, chan->cfg.direction);
    }
    if (ret)
        return;

    hal_dma_chan_start(chan);
}

void drv_dma_ch_stop(struct dma_chan *dma_ch)
{
    struct aic_dma_chan * chan = *(struct aic_dma_chan **)dma_ch;

    hal_dma_chan_stop(chan);
}

#if defined(RT_USING_FINSH)
#include <finsh.h>

static void cmd_dma_dump(int argc, char **argv)
{
    u32 ch_nr = 0;

    if ((argc != 2) || !aic_isdigit(argv[1][0])) {
        pr_err("Invalid parameter\n");
        return;
    }

    ch_nr = atoi(argv[1]);
    if (ch_nr >= AIC_DMA_CH_NUM) {
        pr_err("Invalid channel No.%d\n", ch_nr);
        return;
    }

    hal_dma_chan_dump(ch_nr);
}
MSH_CMD_EXPORT_ALIAS(cmd_dma_dump, dma_dump,
                     Dump DMA register. Argument: channel_num);

#endif
