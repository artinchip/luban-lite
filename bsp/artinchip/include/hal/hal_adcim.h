/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_ADCIM_H_
#define _ARTINCHIP_HAL_ADCIM_H_

#include "aic_common.h"

int hal_adcim_calibration_set(unsigned int val);
s32 hal_adcim_probe(void);
int hal_adcim_auto_calibration(int adc_val, int st_voltage, int scale,
                               int adc_max_val);

#ifdef AIC_ADCIM_DM_DRV
void hal_dm_chan_show(void);
s32 hal_dm_chan_store(u32 val);
void hal_adcdm_rtp_down_store(u32 val);
ssize_t hal_adcdm_sram_write(int *buf, u32 offset, size_t count);
#endif


#endif
