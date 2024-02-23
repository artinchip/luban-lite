/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-01-10        the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "gsl1680.h"
#include "touch.h"
#include <rtdbg.h>

#define DBG_TAG "gsl1680_example"
#define DBG_LVL DBG_LOG

rt_thread_t gsl1680_thread;
rt_device_t gsl1680;

void gsl1680_thread_entry(void *parameter)
{
    struct rt_touch_data *read_data;

    read_data = (struct rt_touch_data *)rt_calloc(1, sizeof(struct rt_touch_data));

    while(1)
    {
        rt_memset(read_data, 0, sizeof(struct rt_touch_data));
        rt_device_read(gsl1680, 0, read_data, 1);

        switch (read_data->event)
        {
            case RT_TOUCH_EVENT_DOWN:
                rt_kprintf("down x: %03d y: %03d", read_data->x_coordinate, read_data->y_coordinate);
                rt_kprintf(" t: %d\n", read_data->timestamp);
                break;

            case RT_TOUCH_EVENT_MOVE:
                rt_kprintf("move x: %03d y: %03d", read_data->x_coordinate, read_data->y_coordinate);
                rt_kprintf(" t: %d\n", read_data->timestamp);
                break;

            case RT_TOUCH_EVENT_UP:
                rt_kprintf("up x: %03d y: %03d", read_data->x_coordinate, read_data->y_coordinate);
                rt_kprintf(" t: %d\n", read_data->timestamp);
                break;

            default:
                break;
        }
        rt_thread_delay(10);
    }

}

int test_gsl1680(void)
{
    gsl1680 = rt_device_find("gsl1680");

    rt_device_open(gsl1680, RT_DEVICE_FLAG_RDONLY);

    struct rt_touch_info info;
    rt_device_control(gsl1680, RT_TOUCH_CTRL_GET_INFO, &info);
    LOG_I("type       :%d", info.type);
    LOG_I("vendor     :%d", info.vendor);
    LOG_I("point_num  :%d", info.point_num);
    LOG_I("range_x    :%d", info.range_x);
    LOG_I("range_y    :%d", info.range_y);

    gsl1680_thread = rt_thread_create("gsl1680", gsl1680_thread_entry, RT_NULL, 1600, 25, 20);
    if (gsl1680_thread == RT_NULL)
    {
        LOG_D("create gsl1680 thread err");

        return -RT_ENOMEM;
    }
    rt_thread_startup(gsl1680_thread);

    return RT_EOK;
}
MSH_CMD_EXPORT(test_gsl1680, test gsl1680 sample);
