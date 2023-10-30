/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq ruqi.zheng<duanmt@artinchip.com>
 */

#include "aic_core.h"

#ifdef AIC_SD_USING_HOTPLUG

#include <drivers/mmcsd_core.h>
#include <dfs_fs.h>

#define SD_CHECK_PIN (rt_pin_get("PC.6"))

extern void aic_mmcsd_change(void);

static void sd_hotplug_detection_thread(void *parameter)
{
    rt_uint8_t re_sd_check_pin = 1;
    rt_device_t device;

    while (1)
    {
        rt_thread_mdelay(200);
        if (re_sd_check_pin && (re_sd_check_pin = rt_pin_read(SD_CHECK_PIN)) == 0) {
            printf("card insertion detected!\n");
            device = rt_device_find("sd0");
            if (device == NULL)
                aic_mmcsd_change();
        }
        if (!re_sd_check_pin && (re_sd_check_pin = rt_pin_read(SD_CHECK_PIN)) != 0) {
            printf("card removal detected!\n");
            aic_mmcsd_change();
        }
    }
}

int aic_sd_hotplug_detection(void)
{
    rt_thread_t tid;

    rt_pin_mode(SD_CHECK_PIN, PIN_MODE_INPUT_PULLUP);

    tid = rt_thread_create("sd_hotplug_detection", sd_hotplug_detection_thread, RT_NULL,
                           1024, RT_THREAD_PRIORITY_MAX - 2, 20);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    else
        printf("create sd_hotplug_detection thread err!\n");

    return RT_EOK;
}
INIT_APP_EXPORT(aic_sd_hotplug_detection);
#endif

