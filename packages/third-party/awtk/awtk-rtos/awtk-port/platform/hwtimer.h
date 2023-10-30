/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

/**
 * History:
 * ================================================================
 * 2023-9-14 Zequan Liang <zequan.liang@artinchip.com> created
 *
 */


#ifndef TK_AIC_HW_TIMER_H
#define TK_AIC_HW_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mpp_decoder.h"

int hwtimer_init(void);
uint64_t hw_get_time_us64(void);
uint64_t hw_get_time_ms64(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /*TK_AIC_HW_TIMER_H*/
