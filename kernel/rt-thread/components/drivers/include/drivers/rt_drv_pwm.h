/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-07     aozima       the first version
 */

#ifndef __DRV_PWM_H_INCLUDE__
#define __DRV_PWM_H_INCLUDE__

#include <rtthread.h>

#define PWM_CMD_ENABLE      (RT_DEVICE_CTRL_BASE(PWM) + 0)
#define PWM_CMD_DISABLE     (RT_DEVICE_CTRL_BASE(PWM) + 1)
#define PWM_CMD_SET         (RT_DEVICE_CTRL_BASE(PWM) + 2)
#define PWM_CMD_GET         (RT_DEVICE_CTRL_BASE(PWM) + 3)
#define PWMN_CMD_ENABLE     (RT_DEVICE_CTRL_BASE(PWM) + 4)
#define PWMN_CMD_DISABLE    (RT_DEVICE_CTRL_BASE(PWM) + 5)
#define PWM_CMD_SET_PERIOD  (RT_DEVICE_CTRL_BASE(PWM) + 6)
#define PWM_CMD_SET_PULSE   (RT_DEVICE_CTRL_BASE(PWM) + 7)
#define PWM_CMD_SET_FIFO    (RT_DEVICE_CTRL_BASE(PWM) + 8)
#define PWM_CMD_SET_FIFO_NUM (RT_DEVICE_CTRL_BASE(PWM) + 9)
#define PWM_CMD_GET_FIFO    (RT_DEVICE_CTRL_BASE(PWM) + 10)
#define PWM_CMD_SET_PUL     (RT_DEVICE_CTRL_BASE(PWM) + 11)

struct rt_pwm_configuration
{
    rt_uint32_t channel; /* 1-n or 0-n, which depends on specific MCU requirements */
    rt_uint32_t period;  /* unit:ns 1ns~4.29s:1Ghz~0.23hz */
    rt_uint32_t pulse;   /* unit:ns (pulse<=period) */

    /*
     * RT_TRUE  : The channel of pwm is complememtary.
     * RT_FALSE : The channel of pwm is nomal.
    */
    rt_bool_t  complementary;

#if defined(AIC_PWM_DRV) || defined(AIC_EPWM_DRV)
    rt_uint32_t irq_mode;
    rt_uint32_t pul_cnt;
#endif
#ifdef AIC_XPWM_DRV
    rt_uint32_t pulse_cnt; /* 0:PWM mode, 1-n:XPWM pulse cnt */
    rt_uint32_t fifo_num;
    rt_uint32_t fifo_index;
    rt_uint32_t pul_num;
    rt_uint32_t pul_prd;
    rt_uint32_t pul_cmp;
#ifdef AIC_DMA_DRV
    rt_uint32_t *buf;
    rt_uint32_t buf_len;
#endif
#endif
};

struct rt_device_pwm;
struct rt_pwm_ops
{
    rt_err_t (*control)(struct rt_device_pwm *device, int cmd, void *arg);
};

struct rt_device_pwm
{
    struct rt_device parent;
    const struct rt_pwm_ops *ops;
};

rt_err_t rt_device_pwm_register(struct rt_device_pwm *device, const char *name, const struct rt_pwm_ops *ops, const void *user_data);

rt_err_t rt_pwm_enable(struct rt_device_pwm *device, int channel);
rt_err_t rt_pwm_disable(struct rt_device_pwm *device, int channel);
#if defined(AIC_PWM_DRV) || defined(AIC_EPWM_DRV)
rt_err_t rt_pwm_set_pul(struct rt_device_pwm *device, int channel, rt_uint32_t irq_mode, rt_uint32_t period, rt_uint32_t pulse, rt_uint32_t pul_cnt);
#endif
#ifdef AIC_XPWM_DRV
rt_err_t rt_pwm_set(struct rt_device_pwm *device, int channel, rt_uint32_t period, rt_uint32_t pulse, rt_uint32_t pulse_cnt);
#else
rt_err_t rt_pwm_set(struct rt_device_pwm *device, int channel, rt_uint32_t period, rt_uint32_t pulse);
#endif
rt_err_t rt_pwm_set_period(struct rt_device_pwm *device, int channel, rt_uint32_t period);
rt_err_t rt_pwm_set_pulse(struct rt_device_pwm *device, int channel, rt_uint32_t pulse);

#endif /* __DRV_PWM_H_INCLUDE__ */
