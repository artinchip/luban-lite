/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <rtthread.h>
#include "rtdevice.h"
#include <aic_core.h>

#define PM_TEST_HELP                                                \
    "test_pm TIME:\n"                                               \
    "This program is used to test pm sleep/wakeup,\n"               \
    "and will change the frequency of CPU.\n"                       \
    "The system will alternately wakeup and sleep TIME seconds\n"   \
    "Example:\n"                                                    \
    "test_pm 10\n"

void test_pm_usage()
{
    puts(PM_TEST_HELP);
}
static void pm_timer_timeout(void *parameter)
{
    rt_uint8_t sleep_mode;

    sleep_mode = rt_pm_get_sleep_mode();

    if (sleep_mode == PM_SLEEP_MODE_NONE)
    {
        rt_pm_run_enter(PM_RUN_MODE_NORMAL_SPEED);
        rt_pm_module_release(PM_POWER_ID, PM_SLEEP_MODE_NONE);
        wakeup_triggered = 0;
    }
    else
    {
        rt_pm_run_enter(PM_RUN_MODE_MEDIUM_SPEED);
        rt_pm_module_request(PM_POWER_ID, PM_SLEEP_MODE_NONE);
        wakeup_triggered = 1;
    }
}

int test_pm(int argc, char *argv[])
{
    rt_tick_t timeout;
    rt_timer_t timer;
    uint32_t seconds = 0;

    if (argc != 2) {
        test_pm_usage();
        return 1;
    }

    seconds = strtoul(argv[1], NULL, 10);
    rt_pm_default_set(PM_SLEEP_MODE_LIGHT);
    timeout = seconds * RT_TICK_PER_SECOND;
    timer = rt_timer_create("pm_test_timer", pm_timer_timeout, RT_NULL,
                            timeout, RT_TIMER_FLAG_PERIODIC);
    if (timer)
        rt_timer_start(timer);

    return 0;
}

MSH_CMD_EXPORT(test_pm, test pm stability);

