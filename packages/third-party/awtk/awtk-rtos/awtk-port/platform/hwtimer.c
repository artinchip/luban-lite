/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

/**
 * History:
 * ================================================================
 * 2023-10-16 Zequan Liang <zequan.liang@artinchip.com> created
 *
 */

#include <stdlib.h>
#include <posix/string.h>
#include <getopt.h>
#include <rtthread.h>
#include <rtdevice.h>

#include "aic_core.h"
#include "drv_hrtimer.h"
#include "boot_param.h"
#include "hwtimer.h"

#include <rtthread.h>
#include <rtdevice.h>

#define HWTIMER_DEV_NAME   "timer0"

static uint64_t count_us;
static uint64_t count_ms;

static rt_err_t timeout_cb(rt_device_t dev, rt_size_t size)
{
  rt_kprintf("this is hwtimer timeout callback fucntion!\n");
  rt_kprintf("tick is :%d !\n", rt_tick_get());

  return 0;
}

uint64_t hw_get_time_us64(void)
{
  return count_us;
}

uint64_t hw_get_time_ms64(void)
{
  return count_ms;
}

int hwtimer_init(void)
{
  rt_err_t ret = RT_EOK;
  rt_hwtimerval_t timeout_s;
  rt_device_t hw_dev = RT_NULL;
  rt_hwtimer_mode_t mode;
  rt_uint32_t freq = 10000;

  return -1;

  hw_dev = rt_device_find(HWTIMER_DEV_NAME);
  if (hw_dev == RT_NULL) {
    rt_kprintf("hwtimer sample run failed! can't find %s device!\n", HWTIMER_DEV_NAME);
    return RT_ERROR;
  }

  ret = rt_device_open(hw_dev, RT_DEVICE_OFLAG_RDWR);
  if (ret != RT_EOK) {
    rt_kprintf("open %s device failed!\n", HWTIMER_DEV_NAME);
    return ret;
  }

  rt_device_set_rx_indicate(hw_dev, timeout_cb);

  rt_device_control(hw_dev, HWTIMER_CTRL_FREQ_SET, &freq);
  mode = HWTIMER_MODE_PERIOD;
  ret = rt_device_control(hw_dev, HWTIMER_CTRL_MODE_SET, &mode);
  if (ret != RT_EOK) {
    rt_kprintf("set mode failed! ret is :%d\n", ret);
    return ret;
  }

  timeout_s.sec = 5;
  timeout_s.usec = 0;
  if (rt_device_write(hw_dev, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s)) {
    rt_kprintf("set timeout value failed\n");
    return RT_ERROR;
  }

  rt_device_read(hw_dev, 0, &timeout_s, sizeof(timeout_s));
  rt_kprintf("Read: Sec = %d, Usec = %d\n", timeout_s.sec, timeout_s.usec);

  return ret;
}

