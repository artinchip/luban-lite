/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_TIME_H__
#define __AIC_TIME_H__
#include <aic_common.h>

#ifdef __cplusplus
extern "C" {
#endif

void aic_udelay(u32 us);
void aic_mdelay(u32 ms);
u64 aic_get_ticks(void);
u64 aic_get_time_us(void);
u64 aic_get_time_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* __AIC_TIME_H__ */
