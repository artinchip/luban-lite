/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <string.h>
#include <getopt.h>

#include <drivers/touch.h>

#define LOG_TAG             "RTP"
#include "aic_core.h"

#include "hal_rtp.h"

#define AIC_RTP_DEFAULT_MODE    RTP_MODE_AUTO2

static struct rt_touch_device g_rt_rtp_dev = {0};
static struct aic_rtp_dev g_rtp_dev = {0};
static int g_last_count =1;

rt_err_t aic_rtp_init(void)
{
    if (g_rtp_dev.sem) {
        LOG_I("RTP is already inited!");
        return RT_EOK;
    }
    if (hal_rtp_clk_init())
        return -RT_ERROR;

    aicos_request_irq(RTP_IRQn, hal_rtp_isr, 0, NULL, NULL);

    memset(&g_rtp_dev, 0, sizeof(struct aic_rtp_dev));
    g_rtp_dev.x_plate = AIC_RTP_X_PLATE;
    if (AIC_RTP_Y_PLATE > 0)
        g_rtp_dev.y_plate = AIC_RTP_Y_PLATE;

    g_rtp_dev.mode = AIC_RTP_DEFAULT_MODE;
    g_rtp_dev.max_press = AIC_RTP_MAX_PRESSURE;
    g_rtp_dev.smp_period = AIC_RTP_PERIOD_MS;
    g_rtp_dev.pdeb = AIC_RTP_PDEB;
    g_rtp_dev.delay = AIC_RTP_DELAY;
    if (g_rtp_dev.mode != RTP_MODE_AUTO1)
        g_rtp_dev.pressure_det = 1;

    hal_rtp_enable(&g_rtp_dev, 1);
    hal_rtp_int_enable(&g_rtp_dev, 1);
    hal_rtp_auto_mode(&g_rtp_dev);

    return RT_EOK;
}

static rt_size_t drv_rtp_read_point(struct rt_touch_device *touch, void *buf,
                                   rt_size_t touch_num)
{
    s32 ret = 0;
    static s32 down = 0;
    struct aic_rtp_event e = {0};
    struct rt_touch_data *data = (struct rt_touch_data *)buf;

    RT_ASSERT(buf);
    RT_ASSERT(read_num == 1);

    if (g_last_count != touch_num) {
        hal_rtp_ebuf_sync(&g_rtp_dev.ebuf);
        g_last_count = touch_num;
    }

    ret = hal_rtp_ebuf_read(&g_rtp_dev.ebuf, &e);
    if (ret < 0) {
        LOG_I("Failed to get touch data");
        return 0;
    }

    data->x_coordinate = e.x;
    data->y_coordinate = e.y;
    data->width = e.pressure;
    data->timestamp = e.timestamp;
    if (!e.down) {
        data->event = RT_TOUCH_EVENT_UP;
        down = 0;
    } else {
        if (down)
            data->event = RT_TOUCH_EVENT_MOVE;
        else
            data->event = RT_TOUCH_EVENT_DOWN;

        down = 1;
    }

    return touch_num;
}

static rt_err_t drv_rtp_control(struct rt_touch_device *touch,
                                int cmd, void *arg)
{
    switch (cmd) {
    case RT_TOUCH_CTRL_ENABLE_INT:
        hal_rtp_int_enable(&g_rtp_dev, 1);
        break;

    case RT_TOUCH_CTRL_DISABLE_INT:
        hal_rtp_int_enable(&g_rtp_dev, 0);
        break;

    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }

    return RT_EOK;
}

static int rtp_touch_isr(void)
{
    struct rt_touch_device *dev = &g_rt_rtp_dev;
    rt_hw_touch_isr(dev);
    return 0;
}

static struct rt_touch_ops aic_rtp_ops = {
    .touch_readpoint = drv_rtp_read_point,
    .touch_control   = drv_rtp_control,
};

static int drv_rtp_init(void)
{
    rt_int8_t ret = RT_EOK;
    struct rt_touch_device *dev = &g_rt_rtp_dev;

    if (aic_rtp_init()) {
        LOG_E("Failed to init RTP controller");
        return -RT_ERROR;
    }

    dev->info.type = RT_TOUCH_TYPE_RESISTANCE;
    dev->info.point_num = 1;
    dev->info.range_x = AIC_RTP_MAX_VAL;
    dev->info.range_y = AIC_RTP_MAX_VAL;
    dev->config.dev_name = AIC_RTP_NAME;
    dev->ops = &aic_rtp_ops;
    ret = rt_hw_touch_register(dev, "aic-rtp", RT_DEVICE_FLAG_INT_RX, RT_NULL);
    if (ret != RT_EOK)
        LOG_E("Failed to register RTP, return %d", ret);

    hal_rtp_register_callback(rtp_touch_isr);
    LOG_I("ArtInChip RTP loaded");
    return ret;
}
INIT_DEVICE_EXPORT(drv_rtp_init);

#if defined(RT_USING_FINSH)
#include <finsh.h>

static void cmd_rtp_status(int argc, char **argv)
{
     hal_rtp_status_show(&g_rtp_dev);
}

MSH_CMD_EXPORT_ALIAS(cmd_rtp_status, rtp_status, Show the status of RTP);

#endif
