/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_PWM_H_
#define _ARTINCHIP_HAL_PWM_H_

#include "aic_common.h"

#define AIC_PWM_NAME        "aic-pwm"

#ifdef CONFIG_FPGA_BOARD_ARTINCHIP
#define PWM_CLK_RATE        24000000 /* 24 MHz */
#else
#define PWM_CLK_RATE        48000000 /* 48 MHz */
#endif
#define PWM_TB_CLK_RATE     24000000 /* 24 MHz */

enum pwm_polarity {
    PWM_POLARITY_NORMAL,
    PWM_POLARITY_INVERSED,
};

enum aic_pwm_mode {
    PWM_MODE_UP_COUNT = 0,
    PWM_MODE_DOWN_COUNT,
    PWM_MODE_UP_DOWN_COUNT,
    PWM_MODE_NUM
};

enum aic_pwm_action_type {
    PWM_ACT_NONE = 0,
    PWM_ACT_LOW,
    PWM_ACT_HIGH,
    PWM_ACT_INVERSE,
    PWM_ACT_NUM
};

struct aic_pwm_action {
    enum aic_pwm_action_type CBD;
    enum aic_pwm_action_type CBU;
    enum aic_pwm_action_type CAD;
    enum aic_pwm_action_type CAU;
    enum aic_pwm_action_type PRD;
    enum aic_pwm_action_type ZRO;
};

struct aic_pwm_arg {
    u16 available;
    u16 id;
    enum aic_pwm_mode mode;
    u32 tb_clk_rate;
    u32 freq;
    struct aic_pwm_action action0;
    struct aic_pwm_action action1;
    u32 period;
    u32 duty;
    s32 def_level;
    enum pwm_polarity polarity;
};

enum aic_pwm_int_event {
    PWM_CMPA_UP = 0,
    PWM_CMPA_DOWN,
    PWM_CMPB_UP,
    PWM_CMPB_DOWN
};

struct aic_pwm_pulse_para {
    u32 prd_ns;
    u32 duty_ns;
    u32 pulse_cnt;
};

void hal_pwm_ch_init(u32 ch, enum aic_pwm_mode mode, u32 default_level,
                     struct aic_pwm_action *a0, struct aic_pwm_action *a1);
int hal_pwm_set(u32 ch, u32 duty_ns, u32 period_ns);
int hal_pwm_get(u32 ch, u32 *duty_ns, u32 *period_ns);
int hal_pwm_set_polarity(u32 ch, enum pwm_polarity polarity);
int hal_pwm_enable(u32 ch);
int hal_pwm_disable(u32 ch);
u32 hal_pwm_int_sts(void);
void hal_pwm_clr_int(u32 stat);
void hal_pwm_int_config(u32 ch, u8 irq_mode, u8 enable);

int hal_pwm_init(void);
int hal_pwm_deinit(void);

void hal_pwm_status_show(void);

#endif // end of _ARTINCHIP_HAL_PWM_H_
