/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_GPAI_H_
#define _ARTINCHIP_HAL_GPAI_H_

#include "aic_osal.h"

#define AIC_GPAI_NUM_BITS   12
#define AIC_GPAI_TIMEOUT    1000 /* 1000 ms */

enum aic_gpai_mode {
    AIC_GPAI_MODE_SINGLE = 0,
    AIC_GPAI_MODE_PERIOD = 1
};

enum aic_gpai_obtain_data_mode {
    AIC_GPAI_OBTAIN_DATA_BY_CPU = 1,
    AIC_GPAI_OBTAIN_DATA_BY_DMA = 2,
    AIC_GPAI_OBTAIN_DATA_BY_DO = 3
};

typedef void (*dma_callback)(void *dma_param);

struct aic_dma_transfer_info
{
    u32 chan_id;
    struct aic_dma_chan *dma_chan;

    void *buf;
    int buf_size;
    void *callback_param;
    dma_callback callback;
};

struct aic_gpai_ch {
    u8 id;
    u8 available;
    enum aic_gpai_mode mode;
    enum aic_gpai_obtain_data_mode obtain_data_mode;
    u32 smp_period;
    u32 pclk_rate;
    u16 latest_data;
    u8 fifo_depth;
    u8 fifo_thd;

    u8 hla_enable; // high-level alarm
    u8 lla_enable; // low-level alarm
    u16 hla_thd;
    u16 hla_rm_thd;
    u16 lla_thd;
    u16 lla_rm_thd;

    u8 irq_count;
    u8 dma_port_id;
    struct aic_dma_transfer_info dma_rx_info;

    aicos_sem_t complete;
};

void aich_gpai_enable(int enable);
void aich_gpai_ch_enable(u32 ch, int enable);
int aich_gpai_ch_init(struct aic_gpai_ch *chan, u32 pclk);

irqreturn_t aich_gpai_isr(int irq, void *arg);

int aich_gpai_read(struct aic_gpai_ch *chan, u32 *val, u32 timeout);
s32 aich_gpai_data2vol(u16 data);

struct aic_gpai_ch *hal_gpai_ch_is_valid(u32 ch);
void hal_gpai_set_ch_num(u32 num);

void aich_gpai_status_show(struct aic_gpai_ch *chan);
s32 hal_gpai_clk_init(void);
void hal_gpai_clk_get(struct aic_gpai_ch *chan);
#if defined(AIC_GPAI_DRV_V20) && defined(AIC_DMA_DRV)
void hal_gpai_config_dma(struct aic_gpai_ch *chan);
void hal_gpai_start_dma(struct aic_gpai_ch *chan);
#endif
#endif // end of _ARTINCHIP_HAL_GPAI_H_
