/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "aic_core.h"
#include "aic_dma_id.h"

#include "hal_dma_reg.h"
#include "hal_dma.h"

static void hal_dma_reg_dump(struct aic_dma_chan *chan)
{
    struct aic_dma_dev *aich_dma;

    aich_dma = get_aic_dma_dev();
    printf("Common register: \n"
            "    IRQ_EN 0x%x, \tIRQ_STA 0x%x, \tCH_STA 0x%x, GATE 0x%x\n",
            readl(aich_dma->base + DMA_IRQ_EN_REG(0)),
            readl(aich_dma->base + DMA_IRQ_STA_REG(0)),
            readl(aich_dma->base + DMA_CH_STA_REG),
            readl(aich_dma->base + DMA_GATE_REG));

    printf("Ch%d register: \n"
            "    Enable 0x%x, \tMode 0x%x, \tPause 0x%x\n"
            "    Task 0x%x, \tConfig 0x%x, \tSrc 0x%x, \tSink 0x%x\n"
            "    Left 0x%x, \tPackage_cnt %d\n",
            chan->ch_nr,
            readl(chan->base + DMA_CH_EN_REG),
            readl(chan->base + DMA_CH_MODE_REG),
            readl(chan->base + DMA_CH_PAUSE_REG),
            readl(chan->base + DMA_CH_TASK_REG),
            readl(chan->base + DMA_CH_CFG_REG),
            readl(chan->base + DMA_CH_SRC_REG),
            readl(chan->base + DMA_CH_SINK_REG),
            readl(chan->base + DMA_CH_LEFT_REG),
            readl(chan->base + DMA_CH_PKG_NUM_REG));
}

static void hal_dma_task_dump(struct aic_dma_chan *chan)
{
    struct aic_dma_task *task;

    printf("DMA Ch%d: desc = 0x%lx\n", chan->ch_nr, (unsigned long)chan->desc);

    for (task = chan->desc; task != NULL; task = task->v_next)
    {
        printf(" task (0x%lx):\n"
                "\tcfg - 0x%x, src - 0x%x, dst - 0x%x, len - 0x%x\n"
                "\tdelay - 0x%x, p_next - 0x%x, mode - 0x%x, v_next - 0x%lx\n",
                (unsigned long)task,
                task->cfg, task->src, task->dst, task->len,
                task->delay, task->p_next, task->mode,
                (unsigned long)task->v_next);
    }
}

int hal_dma_chan_dump(int ch_nr)
{
    struct aic_dma_chan *chan;
    struct aic_dma_dev *aich_dma;

    aich_dma = get_aic_dma_dev();

    CHECK_PARAM(ch_nr < AIC_DMA_CH_NUM, -EINVAL);

    chan = &aich_dma->dma_chan[ch_nr];

    hal_dma_task_dump(chan);
    hal_dma_reg_dump(chan);

    return 0;
}

irqreturn_t hal_dma_irq(int irq, void *arg)
{
    int i;
    u32 status;
    struct aic_dma_chan *chan;
    struct aic_dma_dev *aich_dma;
    dma_async_callback cb = NULL;
    void *cb_data = NULL;

    aich_dma = get_aic_dma_dev();

    /* get dma irq pending */
    status = readl(aich_dma->base + DMA_IRQ_STA_REG(0));
    if (!status) {
        /* none irq trigger */
        return IRQ_NONE;
    }
    pr_debug("IRQ status: 0x%x\n", status);

    /* clear irq pending */
    writel(status, aich_dma->base + DMA_IRQ_STA_REG(0));

    /* process irq for every dma channel */
    for (i = 0; (i < AIC_DMA_CH_NUM) && status; i++, status >>= DMA_IRQ_CH_WIDTH) {
        chan = &aich_dma->dma_chan[i];

        if ((!chan->used) || !(status & chan->irq_type))
            continue;

        cb = chan->callback;
        cb_data = chan->callback_param;
        if (cb)
        {
            cb(cb_data);
        }
    }

    return IRQ_HANDLED;
}


#ifndef AIC_DMA_DRV_V10
int hal_dma_chan_prep_memset(struct aic_dma_chan *chan,
                             u32 p_dest, u32 value, u32 len)
{
    struct aic_dma_task *task;

    CHECK_PARAM(chan != NULL && len != 0, -EINVAL);

    CHECK_PARAM((p_dest % AIC_DMA_ALIGN_SIZE) == 0, -EINVAL);

    task = aic_dma_task_alloc();
    CHECK_PARAM(task != NULL, -ENOMEM);

    task->src = p_dest;
    task->dst = p_dest;
    task->len = len;
    task->cfg = (ADDR_LINEAR_MODE << DST_ADDR_BITSHIFT) |
             (ADDR_LINEAR_MODE << SRC_ADDR_BITSHIFT) |
             (3 << DST_BURST_BITSHIFT) | (3 << SRC_BURST_BITSHIFT) |
             (2 << DST_WIDTH_BITSHIFT) | (2 << SRC_WIDTH_BITSHIFT) |
             (DMA_ID_DRAM << DST_PORT_BITSHIFT) |
             (DMA_ID_DRAM << SRC_PORT_BITSHIFT);
    task->delay = DELAY_DEF_VAL;
    task->mode = DMA_S_WAIT_D_WAIT;

    aicos_dcache_clean_invalid_range((void *)(unsigned long)task->dst, task->len);

    aic_dma_task_add(NULL, task, chan);

    writel(value, chan->base + DMA_CH_MEMSET_VAL_REG);
    chan->memset = true;

#ifdef AIC_DMA_DRV_DEBUG
    hal_dma_task_dump(chan);
#endif
    return 0;
}

#endif

int hal_dma_chan_prep_memcpy(struct aic_dma_chan *chan,
                             u32 p_dest, u32 p_src, u32 len)
{
    struct aic_dma_task *task;

    CHECK_PARAM(chan != NULL && len != 0, -EINVAL);

    CHECK_PARAM((p_dest % AIC_DMA_ALIGN_SIZE) == 0
            && (p_src % AIC_DMA_ALIGN_SIZE) == 0, -EINVAL);

    task = aic_dma_task_alloc();
    CHECK_PARAM(task != NULL, -ENOMEM);

    task->src = p_src;
    task->dst = p_dest;
    task->len = len;
    task->cfg = (ADDR_LINEAR_MODE << DST_ADDR_BITSHIFT) |
             (ADDR_LINEAR_MODE << SRC_ADDR_BITSHIFT) |
             (3 << DST_BURST_BITSHIFT) | (3 << SRC_BURST_BITSHIFT) |
             (2 << DST_WIDTH_BITSHIFT) | (2 << SRC_WIDTH_BITSHIFT) |
             (DMA_ID_DRAM << DST_PORT_BITSHIFT) |
             (DMA_ID_DRAM << SRC_PORT_BITSHIFT);
    task->delay = DELAY_DEF_VAL;
    task->mode = DMA_S_WAIT_D_WAIT;

    aicos_dcache_clean_range((void *)(unsigned long)task->src, task->len);
    aicos_dcache_clean_invalid_range((void *)(unsigned long)task->dst, task->len);

    aic_dma_task_add(NULL, task, chan);

#ifdef AIC_DMA_DRV_DEBUG
    hal_dma_task_dump(chan);
#endif
    return 0;
}

int hal_dma_chan_prep_device(struct aic_dma_chan *chan,
                             u32 p_dest, u32 p_src, u32 len,
                             enum dma_transfer_direction dir)
{
    struct aic_dma_task *task;
    u32 task_cfg;
    int ret;

    CHECK_PARAM(chan != NULL && len != 0, -EINVAL);

    CHECK_PARAM((p_dest%AIC_DMA_ALIGN_SIZE) == 0 && (p_src%AIC_DMA_ALIGN_SIZE) == 0, -EINVAL);

    ret = aic_set_burst(&chan->cfg, dir, &task_cfg);
    if (ret) {
        hal_log_err("Invalid DMA configuration\n");
        return -EINVAL;
    }

    task = aic_dma_task_alloc();
    CHECK_PARAM(task != NULL, -ENOMEM);

    task->delay = DELAY_DEF_VAL;
    task->len = len;
    task->src = p_src;
    task->dst = p_dest;
    task->cfg = task_cfg;
    if (dir == DMA_MEM_TO_DEV) {
        task->cfg |= (DMA_ID_DRAM << SRC_PORT_BITSHIFT) |
                  ((chan->cfg.slave_id & DMA_DRQ_PORT_MASK) << DST_PORT_BITSHIFT) |
                  (ADDR_LINEAR_MODE << SRC_ADDR_BITSHIFT) |
                  (ADDR_FIXED_MODE << DST_ADDR_BITSHIFT);
        task->mode = DMA_S_WAIT_D_HANDSHAKE;
        aicos_dcache_clean_range((void *)(unsigned long)task->src, task->len);
    } else {
        task->cfg |= (DMA_ID_DRAM << DST_PORT_BITSHIFT) |
                  ((chan->cfg.slave_id & DMA_DRQ_PORT_MASK) << SRC_PORT_BITSHIFT) |
                  (ADDR_LINEAR_MODE << DST_ADDR_BITSHIFT) |
                  (ADDR_FIXED_MODE << SRC_ADDR_BITSHIFT);
        task->mode = DMA_S_HANDSHAKE_D_WAIT;
        aicos_dcache_clean_invalid_range((void *)(unsigned long)task->dst, task->len);
    }

    aic_dma_task_add(NULL, task, chan);

#ifdef AIC_DMA_DRV_DEBUG
    hal_dma_task_dump(chan);
#endif
    return 0;
}

int hal_dma_chan_prep_cyclic(struct aic_dma_chan *chan,
                             u32 p_buf_addr, u32 buf_len, u32 period_len,
                             enum dma_transfer_direction dir)
{
    struct aic_dma_task *task = NULL;
    struct aic_dma_task *prev = NULL;
    u32 task_cfg;
    u32 periods;
    u32 i;
    int ret;

    CHECK_PARAM(chan != NULL && buf_len != 0 && period_len != 0, -EINVAL);

    CHECK_PARAM((p_buf_addr%AIC_DMA_ALIGN_SIZE) == 0 && (buf_len%AIC_DMA_ALIGN_SIZE) == 0, -EINVAL);

    ret = aic_set_burst(&chan->cfg, dir, &task_cfg);
    if (ret) {
        hal_log_err("Invalid DMA configuration\n");
        return -EINVAL;
    }

    periods = buf_len / period_len;

    for (i = 0; i < periods; i++) {
        task = aic_dma_task_alloc();
        if (task == NULL) {
            aic_dma_free_desc(chan);
            return -ENOMEM;
        }

        task->len = period_len;
        if (dir == DMA_MEM_TO_DEV) {
            task->src = p_buf_addr + period_len * i;
            task->dst = chan->cfg.dst_addr;
            task->cfg = task_cfg;
            task->cfg |= (DMA_ID_DRAM << SRC_PORT_BITSHIFT) |
                      ((chan->cfg.slave_id & DMA_DRQ_PORT_MASK) << DST_PORT_BITSHIFT) |
                      (ADDR_LINEAR_MODE << SRC_ADDR_BITSHIFT) |
                      (ADDR_FIXED_MODE << DST_ADDR_BITSHIFT);
            task->delay = DELAY_DEF_VAL;
            task->mode = DMA_S_WAIT_D_HANDSHAKE;
            aicos_dcache_clean_range((void *)(unsigned long)task->src, task->len);
        } else {
            task->src = chan->cfg.src_addr;
            task->dst = p_buf_addr + period_len * i;
            task->cfg = task_cfg;
            task->cfg |= (DMA_ID_DRAM << DST_PORT_BITSHIFT) |
                      ((chan->cfg.slave_id & DMA_DRQ_PORT_MASK) << SRC_PORT_BITSHIFT) |
                      (ADDR_LINEAR_MODE << DST_ADDR_BITSHIFT) |
                      (ADDR_FIXED_MODE << SRC_ADDR_BITSHIFT);
            task->delay = DELAY_DEF_VAL;
            task->mode = DMA_S_HANDSHAKE_D_WAIT;
            aicos_dcache_clean_invalid_range((void *)(unsigned long)task->dst, task->len);
        }

        prev = aic_dma_task_add(prev, task, chan);
    }

    prev->p_next = __pa((unsigned long)chan->desc);

    chan->cyclic = true;

#ifdef AIC_DMA_DRV_DEBUG
    hal_dma_task_dump(chan);
#endif
    return 0;
}

int hal_dma_chan_start(struct aic_dma_chan *chan)
{
    u32 value;
    struct aic_dma_task *task;
    struct aic_dma_dev *aich_dma;

    aich_dma = get_aic_dma_dev();

    CHECK_PARAM(chan != NULL && chan->desc != NULL, -EINVAL);

    for (task = chan->desc; task != NULL; task = task->v_next)
        aicos_dcache_clean_range((void *)(unsigned long)task, sizeof(*task));

    chan->irq_type = chan->cyclic ? DMA_IRQ_ONE_TASK : DMA_IRQ_ALL_TASK;

    value = readl(aich_dma->base + DMA_IRQ_EN_REG(0));
    value &= ~(DMA_IRQ_MASK(chan->ch_nr));
    value |= chan->irq_type << DMA_IRQ_SHIFT(chan->ch_nr);
    writel(value, aich_dma->base + DMA_IRQ_EN_REG(0));

    writel(chan->desc->mode, chan->base + DMA_CH_MODE_REG);
    writel((u32)(unsigned long)(chan->desc), chan->base + DMA_CH_TASK_REG);
    if (chan->memset)
        writel(DMA_CH_MEMSET, chan->base + DMA_CH_PAUSE_REG);
    else
        writel(0x00, chan->base + DMA_CH_PAUSE_REG);
    writel(0x01, chan->base + DMA_CH_EN_REG);

#ifdef AIC_DMA_DRV_DEBUG
    hal_dma_reg_dump(chan);
#endif

    return 0;
}
