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
    int i;
    struct aic_dma_dev *aich_dma;

    aich_dma = get_aic_dma_dev();

    printf("\nCommon register: \n");
    for (i = 0; i < AIC_DMA_CH_NUM / DMA_IRQ_CHAN_NR; i++) {
    printf( "    IRQ_EN(%d) 0x%x, \tIRQ_STA(%d) 0x%x\n",
            i, readl(aich_dma->base + DMA_IRQ_EN_REG(i)),
            i, readl(aich_dma->base + DMA_IRQ_STA_REG(i)));
    }

    printf("Ch%d register: ID:%x\n"
            "    Enable 0x%x, \tPause 0x%x, Task 0x%x\n"
            "    Config1 0x%x, \tConfig2 0x%x, \tSrc 0x%x, \tDST 0x%x\n"
            "    Byte Counter 0x%d\n",
            chan->ch_nr,
            readl(chan->base + DMA_LINK_ID_REG),
            readl(chan->base + DMA_CH_EN_REG),
            readl(chan->base + DMA_CH_PAUSE_REG),
            readl(chan->base + DMA_CH_TASK_REG),
            readl(chan->base + DMA_TASK_CFG1_REG),
            readl(chan->base + DMA_TASK_CFG2_REG),
            readl(chan->base + DMA_SRC_ADDR_REG),
            readl(chan->base + DMA_DST_ADDR_REG),
            readl(chan->base + DMA_CH_TASK_BCNT_REG));
}

static void hal_dma_task_dump(struct aic_dma_chan *chan)
{
    struct aic_dma_task *task;

    printf("\nDMA Ch%d: desc = 0x%lx\n", chan->ch_nr, (unsigned long)chan->desc);

    for (task = chan->desc; task != NULL; task = task->v_next)
    {
        printf(" task_id (0x%x):\n"
                "\tcfg1 - 0x%x,\tblock_len - 0x%x,\tsrc - 0x%x,\tdst - 0x%x,\tlen - 0x%x\n"
                "\tcfg2 - 0x%x,\tdata_src - 0x%x,\tdata_dst - 0x%x\n"
                "\tp_next - 0x%x, mode - 0x%x, v_next - 0x%lx\n",
                task->link_id,
                task->cfg1, task->block_len, task->src, task->dst, task->len,
                task->cfg2, task->data_src, task->data_dst,
                task->p_next, task->mode,(unsigned long)task->v_next);
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
    int i, j;
    u32 status;
    struct aic_dma_chan *chan;
    struct aic_dma_dev *aich_dma;
    dma_async_callback cb = NULL;
    void *cb_data = NULL;

    aich_dma = get_aic_dma_dev();

    for (i = 0; i < AIC_DMA_CH_NUM / DMA_IRQ_CHAN_NR; i++) {

        /* get dma irq pending */
        status = readl(aich_dma->base + DMA_IRQ_STA_REG(i));
        if (!status) {
            /* none irq trigger */
            continue;
        }

        pr_debug("IRQ status:0x%x->0x%x\n",
                aich_dma->base + DMA_IRQ_STA_REG(i), status);

        /* clear irq pending */
        writel(status, aich_dma->base + DMA_IRQ_STA_REG(i));

        /* process irq for every dma channel */
        for (j = 0; (j < DMA_IRQ_CHAN_NR) && status; j++, status >>= DMA_IRQ_CH_WIDTH) {
            chan = &aich_dma->dma_chan[j + i * DMA_IRQ_CHAN_NR];

            if ((!chan->used) || !(status & chan->irq_type))
                continue;

            cb = chan->callback;
            cb_data = chan->callback_param;
            if (cb)
            {
                cb(cb_data);
            }
        }


    }
    return IRQ_HANDLED;
}

int hal_dma_chan_prep_memset(struct aic_dma_chan *chan,
                             u32 p_dest, u32 value, u32 len)
{
    struct aic_dma_task *task;
    CHECK_PARAM(chan != NULL && len != 0, -EINVAL);

    CHECK_PARAM((p_dest % AIC_DMA_ALIGN_SIZE) == 0, -EINVAL);

    task = aic_dma_task_alloc();
    CHECK_PARAM(task != NULL, -ENOMEM);

    task->link_id = DMA_LINK_ID_DEF;
    task->block_len = DMA_FIFO_SIZE / 2;
    task->src = p_dest;
    task->dst = p_dest;
    task->len = len;
    task->cfg1 =
        (DMA_ID_DRAM << SRC_PORT_BITSHIFT) | (DMA_ID_DRAM << DST_PORT_BITSHIFT) |
        (TYPE_MEMORYSET << SRC_TYPE_BITSHIFT) | (TYPE_MEMORYSET << DST_TYPE_BITSHIFT) |
        (3 << SRC_BURST_BITSHIFT) | (3 << DST_BURST_BITSHIFT)|
        (4 << SRC_WIDTH_BITSHIFT) | (4 << DST_WIDTH_BITSHIFT);
    task->cfg2 = 0;

    task->mode = DMA_S_WAIT_D_WAIT;

    aicos_dcache_clean_invalid_range((void *)(unsigned long)task->dst, task->len);

    aic_dma_task_add(NULL, task, chan);

    writel(value, chan->base + DMA_CH_MEM_SET_REG);
    chan->memset = true;

#ifdef AIC_DMA_DRV_DEBUG
    hal_dma_task_dump(chan);
#endif
    return 0;
}


int hal_dma_chan_prep_memcpy(struct aic_dma_chan *chan,
                             u32 p_dest, u32 p_src, u32 len)
{
    struct aic_dma_task *task;

    CHECK_PARAM(chan != NULL && len != 0, -EINVAL);

    CHECK_PARAM((p_dest%AIC_DMA_ALIGN_SIZE) == 0 && (p_src%AIC_DMA_ALIGN_SIZE) == 0, -EINVAL);

    task = aic_dma_task_alloc();
    CHECK_PARAM(task != NULL, -ENOMEM);

    task->link_id = DMA_LINK_ID_DEF;
    task->block_len = DMA_FIFO_SIZE / 2;
    task->src = p_src;
    task->dst = p_dest;
    task->len = len;
	task->cfg1 =
        (DMA_ID_DRAM << SRC_PORT_BITSHIFT)  | (DMA_ID_DRAM << DST_PORT_BITSHIFT)  |
		(TYPE_MEMORY << SRC_TYPE_BITSHIFT)  | (TYPE_MEMORY << DST_TYPE_BITSHIFT)  |
		(3 << SRC_BURST_BITSHIFT) | (3 << DST_BURST_BITSHIFT)|
		(4 << SRC_WIDTH_BITSHIFT) | (4 << DST_WIDTH_BITSHIFT);

    task->cfg2 = 0;

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
    struct dma_slave_config *sconfig = &chan->cfg;
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
    task->link_id = DMA_LINK_ID_DEF;
    task->len = len;
    task->src = p_src;
    task->dst = p_dest;
    task->cfg1 = task_cfg;
    if (dir == DMA_MEM_TO_DEV) {
        task->block_len = sconfig->dst_addr_width * sconfig->dst_maxburst;
        task->cfg1 |= (DMA_ID_DRAM << SRC_PORT_BITSHIFT) |
                    ((chan->cfg.slave_id & DMA_DRQ_PORT_MASK) << DST_PORT_BITSHIFT) |
                    (TYPE_MEMORY << SRC_TYPE_BITSHIFT);
        if (sconfig->dst_maxburst != 1)
            task->cfg1 |= (TYPE_BURST << DST_TYPE_BITSHIFT);
        else
            task->cfg1 |= (TYPE_IO_SINGLE << DST_TYPE_BITSHIFT);
        task->mode = DMA_S_WAIT_D_HANDSHAKE;
        aicos_dcache_clean_range((void *)(unsigned long)task->src, task->len);
    } else {
        task->block_len = sconfig->src_addr_width * sconfig->src_maxburst;
        task->cfg1 |= (DMA_ID_DRAM << DST_PORT_BITSHIFT) |
                    ((chan->cfg.slave_id & DMA_DRQ_PORT_MASK) << SRC_PORT_BITSHIFT) |
                    (TYPE_MEMORY << DST_TYPE_BITSHIFT);
        if (sconfig->src_maxburst != 1)
            task->cfg1 |= (TYPE_BURST << SRC_TYPE_BITSHIFT);
        else
            task->cfg1 |= (TYPE_IO_SINGLE << SRC_TYPE_BITSHIFT);
        task->mode = DMA_S_HANDSHAKE_D_WAIT;
        aicos_dcache_clean_invalid_range((void *)(unsigned long)task->dst, task->len);
    }
    task->cfg2 = 0;

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
    struct dma_slave_config *sconfig = &chan->cfg;
    u32 task_cfg;
    u32 periods;
    u32 i;
    int ret;

    CHECK_PARAM(chan != NULL && buf_len != 0 && period_len != 0, -EINVAL);

    CHECK_PARAM((p_buf_addr % AIC_DMA_ALIGN_SIZE) == 0
                && (buf_len % AIC_DMA_ALIGN_SIZE) == 0, -EINVAL);

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

        task->link_id = DMA_LINK_ID_DEF;
        task->block_len = 0;
        task->len = period_len;
        task->cfg1 = task_cfg;
        if (dir == DMA_MEM_TO_DEV) {
            task->src = p_buf_addr + period_len * i;
            task->dst = chan->cfg.dst_addr;
            task->block_len = sconfig->dst_addr_width * sconfig->dst_maxburst;
            task->cfg1 |= (DMA_ID_DRAM << SRC_PORT_BITSHIFT) |
                    ((chan->cfg.slave_id & DMA_DRQ_PORT_MASK) << DST_PORT_BITSHIFT) |
                    (TYPE_MEMORY << SRC_TYPE_BITSHIFT);
            if (sconfig->dst_maxburst != 1)
                task->cfg1 |= (TYPE_BURST << DST_TYPE_BITSHIFT);
            else
                task->cfg1 |= (TYPE_IO_SINGLE << DST_TYPE_BITSHIFT);
            task->mode = DMA_S_WAIT_D_HANDSHAKE;
            aicos_dcache_clean_range((void *)(unsigned long)task->src, task->len);
        } else {
            task->src = chan->cfg.src_addr;
            task->dst = p_buf_addr + period_len * i;
            task->block_len = sconfig->dst_addr_width * sconfig->dst_maxburst;
            task->cfg1 |= (DMA_ID_DRAM << SRC_PORT_BITSHIFT) |
                    ((chan->cfg.slave_id & DMA_DRQ_PORT_MASK) << DST_PORT_BITSHIFT)|
                    (TYPE_MEMORY << SRC_TYPE_BITSHIFT);
            if (sconfig->src_maxburst != 1)
                task->cfg1 |= (TYPE_BURST << SRC_TYPE_BITSHIFT);
            else
                task->cfg1 |= (TYPE_IO_SINGLE << SRC_TYPE_BITSHIFT);
            task->mode = DMA_S_HANDSHAKE_D_WAIT;
            aicos_dcache_clean_invalid_range((void *)(unsigned long)task->dst, task->len);
        }
        task->cfg2 = 0;

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
    u32 irq_reg, irq_offset;
    struct aic_dma_task *task;
    struct aic_dma_dev *aich_dma;

    aich_dma = get_aic_dma_dev();

    CHECK_PARAM(chan != NULL && chan->desc != NULL, -EINVAL);

    for (task = chan->desc; task != NULL; task = task->v_next)
        aicos_dcache_clean_range((void *)(unsigned long)task, sizeof(*task));

    chan->irq_type = chan->cyclic ? DMA_IRQ_ONE_TASK : DMA_IRQ_LINK_TASK;
    chan->irq_type |= DMA_IRQ_ID_ERR | DMA_IRQ_ADDR_ERR | DMA_IRQ_RD_AHB_ERR |
                        DMA_IRQ_WT_AHB_ERR | DMA_IRQ_WT_AXI_ERR;

    irq_reg = chan->ch_nr / DMA_IRQ_CHAN_NR;
    irq_offset = chan->ch_nr % DMA_IRQ_CHAN_NR;

    value = readl(aich_dma->base + DMA_IRQ_EN_REG(irq_reg));
    value &= ~(DMA_IRQ_MASK(irq_offset));
    value |= chan->irq_type << DMA_IRQ_SHIFT(irq_offset);
    writel(value, aich_dma->base + DMA_IRQ_EN_REG(irq_reg));

    writel((u32)(unsigned long)(chan->desc), chan->base + DMA_CH_TASK_ADD1_REG);

    hal_dma_chan_resume(chan);
    writel(0x1, chan->base + DMA_CH_CTL1_REG);
    #ifdef AIC_DMA_DRV_DEBUG
    hal_dma_reg_dump(chan);
    #endif

    return 0;
}

void hal_dma_linkid_set(u32 id)
{
    struct aic_dma_dev *aich_dma;
    aich_dma = get_aic_dma_dev();
    writel(id, aich_dma->base + DMA_LINK_ID_REG);
}

void hal_dma_linkid_rst(void)
{
    struct aic_dma_dev *aich_dma;
    aich_dma = get_aic_dma_dev();
    writel(DMA_LINK_ID_DEF, aich_dma->base + DMA_LINK_ID_REG);
}

