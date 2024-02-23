/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo.Dong <guojun.dong@artinchip.com>
 */
#include <finsh.h>
#include <drivers/rtc.h>
#include "aic_core.h"

static rt_err_t cmd_test_rtc(int argc, char **argv)
{
    rt_err_t ret = RT_EOK;
    time_t now;
    struct tm *local_time;

    // set date with local timezone
    ret = set_date(2023, 11, 8);
    if (ret != RT_EOK)
    {
        rt_kprintf("set RTC date failed");
        return ret;
    }

    // set time with local timezone
    ret = set_time(20, 22, 33);
    if (ret != RT_EOK)
    {
        rt_kprintf("set RTC time failed");
        return ret;
    }

    // must delay 1 ms at least for rtc sync
    rt_thread_mdelay(1);

    now = time(RT_NULL);
    local_time = localtime(&now);
    rt_kprintf("date: %04d-%02d-%02d\n", local_time->tm_year+1900, local_time->tm_mon+1, local_time->tm_mday);
    rt_kprintf("time: %02d:%02d:%02d\n", local_time->tm_hour, local_time->tm_min, local_time->tm_sec);

    return ret;
}
MSH_CMD_EXPORT_ALIAS(cmd_test_rtc, test_rtc, test RTC);
