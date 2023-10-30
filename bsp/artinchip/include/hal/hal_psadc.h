/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_PSADC_H_
#define _ARTINCHIP_HAL_PSADC_H_

#include "aic_osal.h"

#define AIC_PSADC_FIFO1_NUM_BITS   20
#define AIC_PSADC_FIFO2_NUM_BITS   12
#define AIC_PSADC_TIMEOUT    1000 /* 1000 ms */

enum aic_psadc_mode {
    AIC_PSADC_MODE_SINGLE = 0,
    AIC_PSADC_MODE_PERIOD = 1
};

struct aic_psadc_ch {
    u8 id;
    u8 available;
    enum aic_psadc_mode mode;
    u8 fifo_depth;

    aicos_sem_t complete;
};

void aich_psadc_enable(int enable);
void aic_psadc_single_queue_mode(int enable);
void aich_psadc_qc_irq_enable(int enable);
int aich_psadc_ch_init(struct aic_psadc_ch *chan, u32 pclk);
irqreturn_t aich_psadc_isr(int irq, void *arg);
int aich_psadc_read(struct aic_psadc_ch *chan, u32 *val, u32 timeout);
struct aic_psadc_ch *hal_psadc_ch_is_valid(u32 ch);
void hal_psadc_set_ch_num(u32 num);
void aich_psadc_status_show(struct aic_psadc_ch *chan);

#endif
