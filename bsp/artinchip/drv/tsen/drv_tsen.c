/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <sys/time.h>

#include "drivers/sensor.h"

#define LOG_TAG             "TSEN"
#include "aic_core.h"

#include "hal_tsen.h"

struct aic_tsen_dev {
    struct rt_sensor_device dev;
    u32 pclk_rate;
    struct aic_tsen_ch *ch;
};

// TODO: slope & shift should be read from eFuse, which was writen by CP tester
struct aic_tsen_ch aic_tsen_chs = {
        .id = 0,
        .available = 1,
        .name = "tsen-cpu",
        .mode = AIC_TSEN_MODE_SINGLE,
        .hta_enable = 0,
        .lta_enable = 0,
        .otp_enable = 0,
#ifndef CONFIG_FPGA_BOARD_ARTINCHIP
        .slope  = -1134,
        .offset = 2439001,
#endif
};

static struct aic_tsen_dev g_aic_tsen = {0};

static rt_size_t aic_tsen_fetch(struct rt_sensor_device *sensor, void *buf, rt_size_t len)
{
    struct aic_tsen_ch *chan = &aic_tsen_chs;
    struct rt_sensor_data *data = (struct rt_sensor_data *)buf;

    if (!chan->available) {
        pr_warn("%s is unavailable!\n", chan->name);
        return -ENODEV;
    }
#ifdef AIC_SID_DRV
    hal_tsen_curve_fitting(chan);
#endif
    data->type = RT_SENSOR_CLASS_TEMP;
    data->timestamp = rt_sensor_get_ts();
    if (chan->mode == AIC_TSEN_MODE_SINGLE)
        hal_tsen_get_temp(chan, (s32 *)&data->data.temp);
    else
        data->data.temp = hal_tsen_data2temp(chan);

    return 1;
}

static rt_err_t aic_tsen_control(struct rt_sensor_device *sensor,
                                 int cmd, void *args)
{
    LOG_D("Unsupported cmd: 0x%x", cmd);
    return -RT_ERROR;
}

static struct rt_sensor_ops aic_sensor_ops = {
    .fetch_data = aic_tsen_fetch,
    .control = aic_tsen_control
};

static int drv_tsen_init(void)
{
    s32 ret = 0;
    struct rt_sensor_info *info = &g_aic_tsen.dev.info;

    hal_tsen_clk_init();
    hal_tsen_pclk_get(&aic_tsen_chs);
    aicos_request_irq(TSEN_IRQn, hal_tsen_irq_handle, 0, NULL, NULL);
    hal_tsen_enable(1);
    hal_tsen_ch_init(&aic_tsen_chs, g_aic_tsen.pclk_rate);
    if (aic_tsen_chs.mode == AIC_TSEN_MODE_SINGLE)
        aic_tsen_chs.complete = aicos_sem_create(0);

    g_aic_tsen.ch = &aic_tsen_chs;

    info->type       = RT_SENSOR_CLASS_TEMP;
    info->model      = "aic-tsen";
    info->unit       = RT_SENSOR_UNIT_DCELSIUS;
    info->range_max  = 125;
    info->range_min  = 0;
    info->period_min = 1000; // 1000ms
    g_aic_tsen.dev.ops = &aic_sensor_ops;
    ret = rt_hw_sensor_register(&g_aic_tsen.dev, "aic",
                                RT_DEVICE_FLAG_RDWR, RT_NULL);
    if (ret != RT_EOK) {
        LOG_E("Failed to register %s. ret %d", aic_tsen_chs.name, ret);
        return -RT_ERROR;
    }

    return 0;
}
INIT_DEVICE_EXPORT(drv_tsen_init);

#if defined(RT_USING_FINSH)
#include <finsh.h>

static void cmd_tsen_status(int argc, char **argv)
{
     hal_tsen_status_show(&aic_tsen_chs);
}

MSH_CMD_EXPORT_ALIAS(cmd_tsen_status, tsen_status, Show the status of TSensor);

#endif
