/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#define DBG_TAG  "tsen.cmd"

#include "aic_core.h"
#include "aic_log.h"
#include "sensor.h"

/* Global macro and variables */
#define AIC_TSEN_NAME      "temp_aic"

static void cmd_test_tsen(int argc, char **argv)
{
    int num = 0;
    rt_int32_t delay;
    rt_sensor_t sensor;
    rt_device_t tsen_dev;
    struct rt_sensor_data data;

    tsen_dev = rt_device_find(AIC_TSEN_NAME);
    sensor = (rt_sensor_t)tsen_dev;
    delay  = sensor->info.period_min > 100 ? sensor->info.period_min : 100;

    rt_device_open(tsen_dev, RT_DEVICE_FLAG_RDONLY);
    int k = 60;
    LOG_I("%3d.%d", k/10, k %10);
    for (num = 0; num < 10; num++) {
        if (rt_device_read(tsen_dev, 0, &data,1) == 1)
            LOG_I("num:%3d, temp:%3d.%d C, timestamp:%5d\n",
                  num, data.data.temp / 10,(rt_uint32_t)data.data.temp % 10,
                  data.timestamp);
        else
            rt_kprintf("read data failed!");
        rt_thread_mdelay(delay);
    }
    rt_device_close(tsen_dev);

}
MSH_CMD_EXPORT_ALIAS(cmd_test_tsen, test_tsen, tsen device sample);
