/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */

#include <stdio.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <string.h>
#include <aic_osal.h>
#include <getopt.h>

#include "drv_cir.h"
#include "ir_raw.h"

aic_cir_t aic_cir_dev;

void drv_cir_callback(aic_cir_ctrl_t *p_cir_ctrl, cir_event_t event, void *arg);

rt_err_t drv_cir_init(rt_device_t pdev)
{
    aic_cir_t *p_aic_cir = (aic_cir_t *)pdev;
    aic_cir_ctrl_t *p_cir_ctrl = &p_aic_cir->aic_cir_ctrl;

    hal_cir_init(p_cir_ctrl);

    hal_cir_attach_callback(p_cir_ctrl, drv_cir_callback, NULL);
    aicos_request_irq(p_cir_ctrl->irq_num, hal_cir_irq, 0, NULL,
                      (void *)p_cir_ctrl);

    return RT_EOK;
}

rt_size_t drv_cir_read(rt_device_t pdev, rt_off_t pos, void *buffer,
                       rt_size_t size)
{
    int ret;
    aic_cir_t *p_aic_cir = (aic_cir_t *)pdev;
    aic_cir_ctrl_t *p_cir_ctrl = &p_aic_cir->aic_cir_ctrl;
    cir_config_t *config = &p_aic_cir->config;

    ret = ir_raw_decode_scancode(config->protocol,
                                 (uint8_t *)&p_cir_ctrl->rx_data,
                                 p_cir_ctrl->rx_idx, (uint32_t *)buffer);
    if (ret)
    {
        LOG_E("ir_raw_decode_scancode error\n");
        size = 0;
    }

    hal_cir_rx_reset_status(p_cir_ctrl);

    return size;
}

rt_size_t drv_cir_write(rt_device_t pdev, rt_off_t pos, const void *buffer,
                        rt_size_t size)
{
    int encode_size;
    aic_cir_t *p_aic_cir = (aic_cir_t *)pdev;
    aic_cir_ctrl_t *p_cir_ctrl = &p_aic_cir->aic_cir_ctrl;
    uint32_t scancode = *(uint32_t *)buffer;
    cir_config_t *config = &p_aic_cir->config;
    void *tx_data = (void *)p_cir_ctrl->tx_data;

    encode_size = ir_raw_encode_scancode(config->protocol, scancode, tx_data,
                                         sizeof(p_cir_ctrl->tx_data));
    if (encode_size < 0)
    {
        LOG_E("ir_raw_encode_scancode error\n");
        return 0;
    }

    hal_cir_enable_transmitter(p_cir_ctrl);
    hal_cir_send_data(p_cir_ctrl, tx_data, encode_size);

    return size;
}

rt_err_t drv_cir_control(rt_device_t pdev, int cmd, void *args)
{
    int ret;
    aic_cir_t *p_aic_cir = (aic_cir_t *)pdev;
    aic_cir_ctrl_t *p_cir_ctrl = &p_aic_cir->aic_cir_ctrl;
    cir_config_t *config;

    switch (cmd)
    {
    case IOC_CIR_CONFIGURE:
        config = (cir_config_t *)args;
        p_aic_cir->config.protocol = config->protocol;
        p_aic_cir->config.tx_duty = config->tx_duty;
        p_aic_cir->config.rx_level = config->rx_level;

        ret = hal_cir_set_tx_carrier(p_cir_ctrl, (uint8_t)config->protocol,
                                     config->tx_duty);
        if (ret)
        {
            LOG_E("hal_cir_set_tx_carrier error\n");
            return -RT_ERROR;
        }

        hal_cir_set_rx_sample_clock(p_cir_ctrl, (uint8_t)config->protocol);
        hal_cir_set_rx_level(p_cir_ctrl, config->rx_level);
        hal_cir_enable_receiver(p_cir_ctrl);
        break;
    default:
        break;
    }

    return RT_EOK;
}

void drv_cir_callback(aic_cir_ctrl_t *p_cir_ctrl, cir_event_t event, void *arg)
{
    aic_cir_t *p_aic_cir = rt_container_of(p_cir_ctrl, aic_cir_t, aic_cir_ctrl);

    if (p_aic_cir->dev.rx_indicate)
    {
        switch (event)
        {
        case CIR_EVENT_RECEIVE_COMPLETE:
            p_aic_cir->dev.rx_indicate(&p_aic_cir->dev, CIR_RX_DONE);
            break;
        case CIR_EVENT_ERROR:
            p_aic_cir->dev.rx_indicate(&p_aic_cir->dev, CIR_RX_ERROR);
            break;
        default:
            break;
        }
    }
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops aic_cir_ops =
{
    drv_cir_init,
    NULL,
    NULL,
    drv_cir_read,
    drv_cir_write,
    drv_cir_control,
};
#endif

int rt_hw_aic_cir_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    aic_cir_dev.dev.ops = &aic_cir_ops;
#else
    aic_cir_dev.dev.init = drv_cir_init;
    aic_cir_dev.dev.open = NULL;
    aic_cir_dev.dev.close = NULL;
    aic_cir_dev.dev.read = drv_cir_read;
    aic_cir_dev.dev.write = drv_cir_write;
    aic_cir_dev.dev.control = drv_cir_control;
    aic_cir_dev.dev.type = RT_Device_Class_Char;
#endif

    rt_device_register(&aic_cir_dev.dev, "cir", 0);

    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_aic_cir_init);
