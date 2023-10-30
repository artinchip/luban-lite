/* 
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_RTC_H_
#define _ARTINCHIP_HAL_RTC_H_

#include "aic_common.h"

typedef s32 (*rtc_callback_t)(void);

s32 hal_rtc_init(void);
s32 hal_rtc_deinit(void);
void hal_rtc_read_time(u32 *sec);
void hal_rtc_set_time(u32 sec);
s32 hal_rtc_read_alarm(u32 *sec);
void hal_rtc_set_alarm(u32 sec);
void hal_rtc_alarm_io_output(void);
void hal_rtc_32k_clk_output(void);
irqreturn_t hal_rtc_irq(int irq, void *arg);

void hal_rtc_cali(s32 clk_rate);

s32 hal_rtc_register_callback(rtc_callback_t callback);

void aic_rtc_aicupg(void);

#endif // end of _ARTINCHIP_HAL_RTC_H_
