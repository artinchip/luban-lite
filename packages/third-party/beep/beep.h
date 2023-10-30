/*
 * Copyright (c) 2006-2020, Sunwancn(bwsheng2000@163.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-02-14     Sunwancn     the first version
 */

#ifndef __LPKG_BEEP_H__
#define __LPKG_BEEP_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 线程栈相关配置 */
#define LPKG_BEEP_THREAD_PRIORITY        ((RT_THREAD_PRIORITY_MAX - 1) * 7 / 8)
#define LPKG_BEEP_THREAD_STACK_SIZE      2048
#define LPKG_BEEP_THREAD_TIMESLICE       (10U * RT_TICK_PER_SECOND / 1000U)  // 10ms
#define LPKG_BEEP_WAIT_SEM_MAX_TICK      (10U * RT_TICK_PER_SECOND)

/* 硬件参数配置 */
#ifndef LPKG_BEEP_ACTIVE_BUZZER
#define LPKG_BEEP_PASSIVE_BUZZER                 // 无源蜂鸣器
#endif

#ifdef LPKG_BEEP_PASSIVE_BUZZER
#ifndef LPKG_BEEP_PWM_DEV_NAME
#define LPKG_BEEP_PWM_DEV_NAME           "pwm1"
#endif
#ifndef LPKG_BEEP_PWM_DEV_CHANNEL
#define LPKG_BEEP_PWM_DEV_CHANNEL        4
#endif
#ifndef LPKG_BEEP_DEFAULT_FREQ
#define LPKG_BEEP_DEFAULT_FREQ           2300U   // 蜂鸣器最佳发声频率 Hz
#endif
#endif /* LPKG_BEEP_PASSIVE_BUZZER */

/* 低功耗模式相关 */
#if defined(RT_USING_PM) && defined(LPKG_BEEP_PASSIVE_BUZZER)
#ifndef LPKG_BEEP_BLOCK_POWER_STOP
#define LPKG_BEEP_BLOCK_POWER_STOP               // 无源蜂鸣器鸣叫时阻止MCU进入STOP低功耗模式
#endif

#define LPKG_BEEP_REQUEST_PM_MODE        PM_SLEEP_MODE_IDLE

#ifndef LPKG_BEEP_SUPPORT_PM_RUN_FREQ_CHANGE
#define LPKG_BEEP_SUPPORT_PM_RUN_FREQ_CHANGE     // 在MCU运行频率改变时，无源蜂鸣器发声频率不变
#endif
#endif /* defined(RT_USING_PM) && defined(LPKG_BEEP_PASSIVE_BUZZER) */

/* 极限参数范围配置 */
#define LPKG_BEEP_PERIOD_MIN             10U     // 10ms
#define LPKG_BEEP_PERIOD_MAX             100000U // 100000ms
#define LPKG_BEEP_FREQ_MIN               500U    // 500Hz
#define LPKG_BEEP_FREQ_MAX               10000U  // 10KHz

struct beep_struct
{
#ifdef LPKG_BEEP_PASSIVE_BUZZER
    struct rt_device_pwm *pwm_dev;
    rt_uint32_t freq;
#else
    rt_base_t pin;
    rt_base_t pin_reset_level;
#endif
    rt_uint32_t nums;
    rt_uint32_t pulse;
    rt_uint32_t npulse;
    rt_bool_t inited;
};

extern void beep_init(rt_base_t pin, rt_base_t reset_level);
extern void beep_deinit(void);
extern void beep_stop(void);
extern void beep(rt_uint32_t beep_nums, rt_uint32_t period, rt_uint32_t percent, rt_uint32_t freq);

#ifdef __cplusplus
}
#endif

#endif /* __LPKG_BEEP_H__ */
