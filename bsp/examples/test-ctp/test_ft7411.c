/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-01-06        the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "ft7411.h"
#include "touch.h"
#include "aic_hal_gpio.h"
#include <rtdbg.h>

#define DBG_TAG "ft7411_example"
#define DBG_LVL DBG_LOG

static rt_thread_t ft7411_thread;
static rt_device_t ft7411;

static void ft7411_thread_entry(void *parameter)
{
    struct rt_touch_data *read_data;

    read_data = (struct rt_touch_data *)rt_calloc(1, sizeof(struct rt_touch_data));

    while(1)
    {
        rt_memset(read_data, 0, sizeof(struct rt_touch_data));
        rt_device_read(ft7411, 0, read_data, 1);

        switch(read_data->event)
        {
            case RT_TOUCH_EVENT_DOWN:
                rt_kprintf("down x: %03d y: %03d", read_data->x_coordinate, read_data->y_coordinate);
                rt_kprintf(" t: %d\n", read_data->timestamp);
                break;

            case RT_TOUCH_EVENT_UP:
                rt_kprintf("up x: %03d y: %03d", read_data->x_coordinate, read_data->y_coordinate);
                rt_kprintf(" t: %d\n", read_data->timestamp);
                break;

            case RT_TOUCH_EVENT_MOVE:
                rt_kprintf("move x: %03d y: %03d", read_data->x_coordinate, read_data->y_coordinate);
                rt_kprintf(" t: %d\n", read_data->timestamp);
                break;

            default:
                break;
        }
        rt_thread_delay(10);
    }

}

static int test_ft7411(void)
{
    ft7411 = rt_device_find("ft7411");

    rt_device_open(ft7411, RT_DEVICE_FLAG_RDONLY);

    struct rt_touch_info info;
    rt_device_control(ft7411, RT_TOUCH_CTRL_GET_INFO, &info);
    LOG_I("type       :%d", info.type);
    LOG_I("vendor     :%d", info.vendor);
    LOG_I("point_num  :%d", info.point_num);
    LOG_I("range_x    :%d", info.range_x);
    LOG_I("range_y    :%d", info.range_y);

    ft7411_thread = rt_thread_create("ft7411", ft7411_thread_entry, RT_NULL, 1600, 25, 20);
    if (ft7411_thread == RT_NULL)
    {
        LOG_D("create ft7411 thread err");

        return -RT_ENOMEM;
    }
    rt_thread_startup(ft7411_thread);

    return RT_EOK;
}
MSH_CMD_EXPORT(test_ft7411, test ft7411 sample);
