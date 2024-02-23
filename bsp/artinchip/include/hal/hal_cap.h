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

#ifdef AIC_HRTIMER_DRV
#define AIC_CAP_CH_NUM      AIC_HRTIMER_CH_NUM
#endif

#ifdef AIC_CAP_DRV
#define AIC_CAP_CH_NUM      AIC_CAPS_CH_NUM
#endif

#define CAP_EVENT3_FLG      BIT(4)

struct aic_cap_data {
    u8 id;
    u32 freq;
    float duty;
};

void hal_cap_ch_init(u32 ch);
void hal_cap_ch_deinit(u32 ch);

void hal_cap_int_enable(u32 ch, int enable);
u32 hal_cap_int_sta(void);
u32 hal_cap_is_pending(u32 ch);
int hal_cap_set_freq(u32 ch, u32 freq);
int hal_cap_set_cnt(u32 ch, u32 cnt);
int hal_cap_get(u32 ch);
int hal_cap_in_config(u32 ch);
u32 hal_cap_reg0(u32 ch);
u32 hal_cap_reg1(u32 ch);
u32 hal_cap_reg2(u32 ch);
u32 hal_cap_int_flg(u32 ch);
void hal_cap_clr_flg(u32 ch, u32 stat);
int hal_cap_enable(u32 ch);
int hal_cap_disable(u32 ch);
void hal_cap_cnt_start(u32 ch);
void hal_cap_cnt_stop(u32 ch);

int hal_cap_init(void);
int hal_cap_deinit(void);

void hal_cap_status_show(void);

#endif // end of _ARTINCHIP_HAL_PWMCS_H_
