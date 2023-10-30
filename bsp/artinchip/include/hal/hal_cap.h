/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_PWMCS_H_
#define _ARTINCHIP_HAL_PWMCS_H_

#include "aic_common.h"

#ifdef CONFIG_FPGA_BOARD_ARTINCHIP
#define PWMCS_CLK_RATE      24000000 /* 24 MHz */
#else
#define PWMCS_CLK_RATE      200000000 /* 200 MHz */
#endif

#define CAP_MAX_FREQ        1000000 /* 1MHz */

void hal_cap_ch_init(u32 ch);
void hal_cap_ch_deinit(u32 ch);

void hal_cap_int_enable(u32 ch, int enable);
u32 hal_cap_is_pending(u32 ch);
int hal_cap_set_freq(u32 ch, u32 freq);
int hal_cap_set_cnt(u32 ch, u32 cnt);
int hal_cap_get(u32 ch);
int hal_cap_enable(u32 ch);
int hal_cap_disable(u32 ch);
void hal_cap_cnt_start(u32 ch);
void hal_cap_cnt_stop(u32 ch);

int hal_cap_init(void);
int hal_cap_deinit(void);

void hal_cap_status_show(void);

#endif // end of _ARTINCHIP_HAL_PWMCS_H_
