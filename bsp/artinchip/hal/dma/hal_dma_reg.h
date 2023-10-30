/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_HAL_DMA_REG_H_
#define _ARTINCHIP_HAL_DMA_REG_H_

#include "aic_core.h"
#include "hal_dma.h"
#include "aic_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TASK_MAX_NUM            24

#define DELAY_DEF_VAL           0x40

#define DMA_IRQ_CHAN_NR         4

#define DMA_IRQ_EN_REG(x)       ((x) * 0x04 + 0x00)

#if defined(AIC_DMA_DRV_V10) || defined(AIC_DMA_DRV_V11)
#define DMA_CHAN_OFFSET         (0x40)
#define DMA_IRQ_STA_REG(x)      ((x) * 0x04 + 0x10)
#define DMA_CH_STA_REG          (0x0030)
#define DMA_LINK_END_FLAG       0xFFFFF800
#endif

#ifdef AIC_DMA_DRV_V20
#define DMA_CHAN_OFFSET         (0x80)
#define DMA_IRQ_STA_REG(x)      ((x) * 0x04 + 0x40)
#define DMA_CH_STA_REG          (0x00A0)
#define DMA_LINK_ID_DEF         0xa1c86688
#define DMA_LINK_END_FLAG       0xFFFFFFFC
#endif
/*
 * define dma_v1.x register list
 */
#define DMA_GATE_REG            (0x0028)
#define DMA_CH_EN_REG           (0x0000)
#define DMA_CH_PAUSE_REG        (0x0004)
#define DMA_CH_TASK_REG         (0x0008)
#define DMA_CH_CFG_REG          (0x000C)
#define DMA_CH_SRC_REG          (0x0010)
#define DMA_CH_SINK_REG         (0x0014)
#define DMA_CH_LEFT_REG         (0x0018)
#define DMA_CH_MODE_REG         (0x0028)
#define DMA_CH_PKG_NUM_REG      (0x0030)
#define DMA_CH_MEMSET_VAL_REG   (0x0034)

/*
 * define dma_v2.x register list
 */
#define DMA_BUS_CFG_REG         (0x80)
#define DMA_SET_LINK_ID_REG     (0x88)
#define DMA_FIFO_SIZE_REG       (0x90)
/* channel config */
#define DMA_CH_CTL1_REG         (0x00)
#define DMA_CH_CTL2_REG         (0x04)
#define DMA_CH_TASK_ADD1_REG    (0x08)
#define DMA_CH_TASK_ADD2_REG    (0x0C)
#define DMA_CH_CTL3_REG         (0x10)
#define DMA_CH_CTL4_REG         (0x14)
#define DMA_CH_MEM_SET_REG      (0x18)
#define DMA_CH_TASK_BCNT_REG    (0x1c)
#define DMA_LINK_ID_REG         (0x20)
/* task config */
#define DMA_TASK_CFG1_REG       (0x24)
#define DMA_BLOCK_LEN_REG       (0x28)
#define DMA_SRC_ADDR_REG        (0x2C)
#define DMA_DST_ADDR_REG        (0x30)
#define DMA_TASK_LEN_REG        (0x34)
#define DMA_TASK_CFG2_REG       (0x38)
#define DMA_NEXT_TASK_ADDR_REG  (0x3c)
#define DMA_SRC_WB_ADDR_SET_REG (0x40)
#define DMA_DST_WB_ADDR_SET_REG (0x44)
#define DMA_SRC_WB_DATA_REG     (0x48)
#define DMA_DST_WB_DATA_REG     (0x4C)
#define DMA_DEBUG_REG           (0x60)

/*
 * define macro for access register for specific channel
 */
#if defined(AIC_DMA_DRV_V10) || defined(AIC_DMA_DRV_V11)
#define DMA_IRQ_HALF_TASK   BIT(0)
#define DMA_IRQ_ONE_TASK    BIT(1)
#define DMA_IRQ_ALL_TASK    BIT(2)
#define DMA_IRQ_CH_WIDTH    (4)
#define DMA_IRQ_MASK(ch)    (GENMASK(2, 0) << DMA_IRQ_SHIFT(ch))
#endif

#ifdef AIC_DMA_DRV_V20
#define DMA_IRQ_HALF_TASK   BIT(0)
#define DMA_IRQ_ONE_TASK    BIT(1)
#define DMA_IRQ_LINK_TASK   BIT(2)
#define DMA_IRQ_ID_ERR      BIT(3)
#define DMA_IRQ_ADDR_ERR    BIT(4)
#define DMA_IRQ_RD_AHB_ERR  BIT(5)
#define DMA_IRQ_WT_AHB_ERR  BIT(6)
#define DMA_IRQ_WT_AXI_ERR  BIT(7)
#define DMA_IRQ_CH_WIDTH    (8)
#define DMA_IRQ_MASK(ch)    (GENMASK(7, 0) << DMA_IRQ_SHIFT(ch))
#endif

#define DMA_IRQ_SHIFT(ch)   (DMA_IRQ_CH_WIDTH * (ch))
#define AIC_DMA_BUS_WIDTH                                                   \
    (BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) | BIT(DMA_SLAVE_BUSWIDTH_2_BYTES)  |    \
     BIT(DMA_SLAVE_BUSWIDTH_4_BYTES) | BIT(DMA_SLAVE_BUSWIDTH_8_BYTES) |    \
     BIT(DMA_SLAVE_BUSWIDTH_16_BYTES))
/*
 * define bit index in channel configuration register
 */
#if defined(AIC_DMA_DRV_V10) || defined(AIC_DMA_DRV_V11)
/* dma_v1.x task  config */
#define DST_WIDTH_BITSHIFT  25
#define DST_ADDR_BITSHIFT   24
#define DST_BURST_BITSHIFT  22
#define DST_PORT_BITSHIFT   16
#define SRC_WIDTH_BITSHIFT  9
#define SRC_ADDR_BITSHIFT   8
#define SRC_BURST_BITSHIFT  6
#define SRC_PORT_BITSHIFT   0
#define ADDR_LINEAR_MODE    0
#define ADDR_FIXED_MODE     1
#endif

#ifdef AIC_DMA_DRV_V20
/* dma_v2.x task config */
#define SRC_PORT_BITSHIFT   0
#define SRC_BURST_BITSHIFT  6
#define SRC_TYPE_BITSHIFT   8
#define SRC_WIDTH_BITSHIFT  12
#define DST_PORT_BITSHIFT   16
#define DST_BURST_BITSHIFT  22
#define DST_TYPE_BITSHIFT   24
#define DST_WIDTH_BITSHIFT  28
#define TYPE_IO_SINGLE      0
#define TYPE_BURST          1
#define TYPE_MEMORY         2
#define TYPE_MEMORYSET      3
#endif

#define DMA_DRQ_PORT_MASK   0x3F
#define DMA_WAIT_MODE       0
#define DMA_HANDSHAKE_MODE  1
#define DMA_DST_MODE_SHIFT  3
#define DMA_SRC_MODE_SHIFT  2
#define DMA_S_WAIT_D_HANDSHAKE  (DMA_HANDSHAKE_MODE << DMA_DST_MODE_SHIFT)
#define DMA_S_HANDSHAKE_D_WAIT  (DMA_HANDSHAKE_MODE << DMA_SRC_MODE_SHIFT)
#define DMA_S_WAIT_D_WAIT   (DMA_WAIT_MODE)
#define DMA_FIFO_SIZE       0x200

/*
 * define bit index in channel pause register
 */
#if defined(AIC_DMA_DRV_V10) || defined(AIC_DMA_DRV_V11)
#define  DMA_CH_RESUME      0x00
#define  DMA_CH_PAUSE       0x10
#endif

#ifdef AIC_DMA_DRV_V20
#define DMA_CH_RESUME       0x00
#define DMA_CH_PAUSE        0x01
#define DMA_CH_BYTEMODE     0x20
#endif

#define DMA_CH_MEMSET       0x10

#ifdef __cplusplus
}
#endif

struct aic_dma_dev {
    struct aic_dma_task task[TASK_MAX_NUM];
    s32 inited;
    unsigned long base;
    u32 burst_length; /* burst length capacity */
    u32 addr_widths; /* address width support capacity */
    struct aic_dma_chan dma_chan[AIC_DMA_CH_NUM];
    struct aic_dma_task *freetask;
};

struct aic_dma_dev *get_aic_dma_dev(void);
void *aic_dma_task_add(struct aic_dma_task *prev,
                        struct aic_dma_task *next,
                        struct aic_dma_chan *chan);
int aic_set_burst(struct dma_slave_config *sconfig,
                    enum dma_transfer_direction direction,
                    u32 *p_cfg);
struct aic_dma_task *aic_dma_task_alloc(void);
void aic_dma_free_desc(struct aic_dma_chan *chan);

#endif /*_ARTINCHIP_HAL_DMA_REG_H_  */
