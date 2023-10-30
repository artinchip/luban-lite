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


static struct aic_dma_dev aich_dma __ALIGNED(CACHE_LINE_SIZE) = {
    .base = DMA_BASE,
    .burst_length = BIT(1) | BIT(4) | BIT(8) | BIT(16),
    .addr_widths = AIC_DMA_BUS_WIDTH,
};

struct aic_dma_dev *get_aic_dma_dev(void)
{
    return &aich_dma;
}

static inline s8 convert_burst(u32 maxburst)
{
    switch (maxburst) {
    case 1:
        return 0;
    case 4:
        return 1;
    case 8:
        return 2;
    case 16:
        return 3;
    default:
        return -EINVAL;
    }
}

static inline s8 convert_buswidth(enum dma_slave_buswidth addr_width)
{
    switch (addr_width) {
    case DMA_SLAVE_BUSWIDTH_2_BYTES:
        return 1;
    case DMA_SLAVE_BUSWIDTH_4_BYTES:
        return 2;
    case DMA_SLAVE_BUSWIDTH_8_BYTES:
        return 3;
    #ifdef AIC_DMA_DRV_V20
    case DMA_SLAVE_BUSWIDTH_16_BYTES:
        return 4;
    #endif
    default:
        /* For 1 byte width or fallback */
        return 0;
    }
}

int aic_set_burst(struct dma_slave_config *sconfig,
                             enum dma_transfer_direction direction,
                             u32 *p_cfg)
{
    enum dma_slave_buswidth src_addr_width, dst_addr_width;
    u32 src_maxburst, dst_maxburst;
    s8 src_width, dst_width, src_burst, dst_burst;

    src_addr_width = sconfig->src_addr_width;
    dst_addr_width = sconfig->dst_addr_width;
    src_maxburst = sconfig->src_maxburst;
    dst_maxburst = sconfig->dst_maxburst;

    switch (direction) {
    case DMA_MEM_TO_DEV:
        #if defined(AIC_DMA_DRV_V10) || defined(AIC_DMA_DRV_V11)
        if (src_addr_width == DMA_SLAVE_BUSWIDTH_UNDEFINED)
            src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
        src_maxburst = src_maxburst ? src_maxburst : 8;
        #endif
        #ifdef AIC_DMA_DRV_V20
        if (src_addr_width == DMA_SLAVE_BUSWIDTH_UNDEFINED)
            src_addr_width = DMA_SLAVE_BUSWIDTH_16_BYTES;
        src_maxburst = src_maxburst ? src_maxburst : 16;
        #endif
        break;
    case DMA_DEV_TO_MEM:
        #if defined(AIC_DMA_DRV_V10) || defined(AIC_DMA_DRV_V11)
        if (dst_addr_width == DMA_SLAVE_BUSWIDTH_UNDEFINED)
            dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
        dst_maxburst = dst_maxburst ? dst_maxburst : 8;
        #endif
        #ifdef AIC_DMA_DRV_V20
        if (dst_addr_width == DMA_SLAVE_BUSWIDTH_UNDEFINED)
            dst_addr_width = DMA_SLAVE_BUSWIDTH_16_BYTES;
        dst_maxburst = dst_maxburst ? dst_maxburst : 16;
        #endif
        break;

    default:
        return -EINVAL;
    }

    if (!(BIT(src_addr_width) & aich_dma.addr_widths))
        return -EINVAL;
    if (!(BIT(dst_addr_width) & aich_dma.addr_widths))
        return -EINVAL;
    if (!(BIT(src_maxburst) & aich_dma.burst_length))
        return -EINVAL;
    if (!(BIT(dst_maxburst) & aich_dma.burst_length))
        return -EINVAL;

    src_width = convert_buswidth(src_addr_width);
    dst_width = convert_buswidth(dst_addr_width);
    dst_burst = convert_burst(dst_maxburst);
    src_burst = convert_burst(src_maxburst);

    *p_cfg = (src_width << SRC_WIDTH_BITSHIFT) |
         (dst_width << DST_WIDTH_BITSHIFT) |
         (src_burst << SRC_BURST_BITSHIFT) |
         (dst_burst << DST_BURST_BITSHIFT);

    return 0;
}

struct aic_dma_task *aic_dma_task_alloc(void)
{
    struct aic_dma_task *task;

    /* Remove the QH structure from the freelist */

    task = aich_dma.freetask;
    if (task) {
        aich_dma.freetask = task->v_next;
        memset(task, 0, sizeof(struct aic_dma_task));
    }

    return task;
}

static void aic_dma_task_free(struct aic_dma_task *task)
{
    CHECK_PARAM_RET(task != NULL);

    task->v_next = aich_dma.freetask;
    aich_dma.freetask = task;
}

void *aic_dma_task_add(struct aic_dma_task *prev,
                                 struct aic_dma_task *next,
                                 struct aic_dma_chan *chan)
{
    CHECK_PARAM((chan != NULL || prev != NULL) && next != NULL, NULL);

    if (!prev)
    {
        chan->desc = next;
    }
    else
    {
        prev->p_next = __pa((unsigned long)next);
        prev->v_next = next;
    }

    next->p_next = DMA_LINK_END_FLAG;
    next->v_next = NULL;

    return next;
}

void aic_dma_free_desc(struct aic_dma_chan *chan)
{
    struct aic_dma_task *task;
    struct aic_dma_task *next;

    CHECK_PARAM_RET(chan != NULL);

    task = chan->desc;
    chan->desc = NULL;

    while (task)
    {
        next = task->v_next;
        aic_dma_task_free(task);
        task = next;
    }

    chan->callback = NULL;
    chan->callback_param = NULL;
}

enum dma_status hal_dma_chan_tx_status(struct aic_dma_chan *chan,
                                       u32 *left_size)
{
    CHECK_PARAM(chan != NULL && left_size != NULL, -EINVAL);

    if (!(readl(aich_dma.base + DMA_CH_STA_REG) & BIT(chan->ch_nr)))
        return DMA_COMPLETE;

    *left_size = readl(chan->base + DMA_CH_LEFT_REG);
        return DMA_IN_PROGRESS;

    return DMA_COMPLETE;
}

int hal_dma_chan_stop(struct aic_dma_chan *chan)
{
    u32 value;
    u32 irq_reg, irq_offset;

    CHECK_PARAM(chan != NULL, -EINVAL);
    irq_reg = chan->ch_nr / DMA_IRQ_CHAN_NR;
    irq_offset = chan->ch_nr % DMA_IRQ_CHAN_NR;

    /* disable irq */
    value = readl(aich_dma.base + DMA_IRQ_EN_REG(irq_reg));
    value &= ~(DMA_IRQ_MASK(irq_offset));
    writel(value, aich_dma.base + DMA_IRQ_EN_REG(irq_reg));

    /* pause */
    hal_dma_chan_pause(chan);
    /* stop */
    writel(0x00, chan->base + DMA_CH_EN_REG);
    /* resume */
    hal_dma_chan_resume(chan);

    chan->cyclic = false;
    chan->memset = false;

    /* free task list */
    aic_dma_free_desc(chan);
    return 0;
}

int hal_dma_chan_pause(struct aic_dma_chan *chan)
{
    u32 val;

    CHECK_PARAM(chan != NULL, -EINVAL);

    /* pause */
    val = readl(chan->base + DMA_CH_PAUSE_REG);
    val |= DMA_CH_PAUSE;
    writel(val, chan->base + DMA_CH_PAUSE_REG);

    return 0;
}

int hal_dma_chan_resume(struct aic_dma_chan *chan)
{
    u32 val;

    CHECK_PARAM(chan != NULL, -EINVAL);

    /* resume */
    val = readl(chan->base + DMA_CH_PAUSE_REG);
    val &= ~ DMA_CH_PAUSE;
    writel(DMA_CH_RESUME, chan->base + DMA_CH_PAUSE_REG);

    return 0;
}

int hal_dma_chan_terminate_all(struct aic_dma_chan *chan)
{
    CHECK_PARAM(chan != NULL, -EINVAL);

    hal_dma_chan_stop(chan);

    return 0;
}

int hal_dma_chan_register_cb(struct aic_dma_chan *chan,
                                     dma_async_callback callback,
                                     void *callback_param)
{
    CHECK_PARAM(chan != NULL && callback != NULL && callback_param != NULL, -EINVAL);

    chan->callback = callback;
    chan->callback_param = callback_param;

    return 0;
}

int hal_dma_chan_config(struct aic_dma_chan *chan,
                               struct dma_slave_config *config)
{

    CHECK_PARAM(chan != NULL && config != NULL, -EINVAL);

    memcpy(&chan->cfg, config, sizeof(*config));

    return 0;
}

int hal_release_dma_chan(struct aic_dma_chan *chan)
{
    CHECK_PARAM(chan != NULL && chan->used != 0, -EINVAL);

    /* free task list */
    aic_dma_free_desc(chan);

    chan->used = 0;

    return 0;
}

struct aic_dma_chan *hal_request_dma_chan(void)
{
    int i = 0;
    struct aic_dma_chan *chan;

    for (i = 0; i < AIC_DMA_CH_NUM; i++)
    {
        chan = &aich_dma.dma_chan[i];
        if (chan->used == 0)
        {
            chan->used = 1;
            chan->cyclic = false;
            chan->memset = false;
            chan->irq_type = 0;
            chan->callback = NULL;
            chan->callback_param = NULL;
            chan->desc = NULL;
            return chan;
        }
    }

    return NULL;
}

int hal_dma_init(void)
{
    int i;

    aich_dma.base = DMA_BASE;

    for (i = 0; i < AIC_DMA_CH_NUM; i++) {
        aich_dma.dma_chan[i].ch_nr = i;
        aich_dma.dma_chan[i].base = aich_dma.base + 0x100
                                    + i * DMA_CHAN_OFFSET;
    }

    aich_dma.freetask = NULL;
    for (i = 0; i < TASK_MAX_NUM; i++)
        aic_dma_task_free(&aich_dma.task[i]);


    for (i = 0; i < AIC_DMA_CH_NUM / DMA_IRQ_CHAN_NR; i++) {
        writel(0x0, aich_dma.base + DMA_IRQ_EN_REG(i));
    }

    return 0;
}
