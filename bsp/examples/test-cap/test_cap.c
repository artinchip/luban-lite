/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include "boot_param.h"

#define WATER_MARK      50

struct aic_cap_usr {
    rt_uint8_t id;
    rt_uint32_t freq;
    float duty;
};

/* callback function */
static rt_err_t cap_cb(rt_device_t dev, rt_size_t size)
{
    struct rt_inputcapture_data inputcap_data[WATER_MARK];
    rt_device_read(dev, 0, (void *)inputcap_data, size);
#ifdef ULOG_USING_ISR_LOG
    struct aic_cap_usr *data = (struct aic_cap_usr *)dev->user_data;

    rt_kprintf("cap%d: freq:%dHz, duty:%d.%02d%%\n",
        data->id, data->freq, (rt_uint32_t)data->duty, (rt_uint32_t)(data->duty * 100) % 100);

    for (int i = 0; i < size; i++)
        rt_kprintf("%s: pulsewidth:%d us\n", &dev->parent.name, inputcap_data[i].pulsewidth_us);
#endif
    return RT_EOK;
}

int test_cap(int argc, char **argv)
{
    rt_uint32_t watermark = WATER_MARK;
    rt_device_t cap_dev = RT_NULL;
    char device_name[8] = {"cap"};
    int ret;

    if (argc != 2) {
        rt_kprintf("Usage: test_cap <channel>\n");
        return -RT_ERROR;
    }

    strcat(device_name, argv[1]);

    cap_dev =  rt_device_find(device_name);
    if (cap_dev == RT_NULL) {
        rt_kprintf("Can't find %s device!\n", device_name);
        return -RT_ERROR;
    }

    /* set callback function */
    rt_device_set_rx_indicate(cap_dev, cap_cb);

    ret = rt_device_control(cap_dev, INPUTCAPTURE_CMD_SET_WATERMARK, &watermark);
    if (ret != RT_EOK) {
        rt_kprintf("Failed to set %s device watermark!\n", device_name);
        return ret;
    }

    ret = rt_device_open(cap_dev, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK) {
        rt_kprintf("Failed to open %s device!\n", device_name);
        return ret;
    }

    rt_kprintf("cap%d open.\n", atoi(argv[1]));

    return RT_EOK;
}
MSH_CMD_EXPORT_ALIAS(test_cap, test_cap, Test the cap);
