/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */

#include <stdio.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <string.h>
#include <aic_osal.h>

rt_timer_t touch_timer;

static void touch_timer_timeout(void *parameter)
{
    rt_uint8_t sleep_mode;

    sleep_mode = rt_pm_get_sleep_mode();

    if (sleep_mode == PM_SLEEP_MODE_NONE)
    {
        rt_pm_module_release(PM_POWER_ID, PM_SLEEP_MODE_NONE);
        wakeup_triggered = 0;
    }
}

int touch_timer_init(void)
{
    rt_tick_t timeout;

    if (!AIC_PM_POWER_TOUCH_TIME_SLEEP)
        timeout = RT_TICK_MAX / 2 - 1;
    else
        timeout = AIC_PM_POWER_TOUCH_TIME_SLEEP * RT_TICK_PER_SECOND;

    touch_timer = rt_timer_create("tp_timer", touch_timer_timeout, RT_NULL,
                                  timeout, RT_TIMER_FLAG_PERIODIC);

    if (touch_timer)
        rt_timer_start(touch_timer);

    return 0;
}

INIT_DEVICE_EXPORT(touch_timer_init);


