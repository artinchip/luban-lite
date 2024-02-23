/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#include <string.h>
#include <drivers/rt_inputcapture.h>
#include <drivers/pm.h>

#define LOG_TAG         "CAP"
#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_cap.h"

struct aic_cap {
    struct rt_inputcapture_device rtdev;
    struct aic_cap_data *data;
};

static struct aic_cap *g_cap[AIC_CAP_CH_NUM];
static struct aic_cap_data g_cap_info[] = {
#ifdef AIC_USING_CAP0
    {.id = 0,},
#endif
#ifdef AIC_USING_CAP1
    {.id = 1,},
#endif
#ifdef AIC_USING_CAP2
    {.id = 2,},
#endif
#ifdef AIC_USING_CAP3
    {.id = 3,},
#endif
#ifdef AIC_USING_CAP4
    {.id = 4,},
#endif
#ifdef AIC_USING_CAP5
    {.id = 5,},
#endif
};

static rt_err_t aic_cap_init(struct rt_inputcapture_device *capture)
{
    struct aic_cap *aic_capture;

    RT_ASSERT(capture != RT_NULL);

    aic_capture = (struct aic_cap *)capture;

    hal_cap_ch_init(aic_capture->data->id);

    return RT_EOK;
}

static rt_err_t aic_cap_open(struct rt_inputcapture_device *capture)
{
    struct aic_cap *aic_capture;

    RT_ASSERT(capture != RT_NULL);

    aic_capture = (struct aic_cap *)capture;

    hal_cap_in_config(aic_capture->data->id);
    hal_cap_cnt_start(aic_capture->data->id);

    return RT_EOK;
}

static rt_err_t aic_cap_close(struct rt_inputcapture_device *capture)
{
    struct aic_cap *aic_capture;

    RT_ASSERT(capture != RT_NULL);

    aic_capture = (struct aic_cap *)capture;

    hal_cap_int_enable(aic_capture->data->id, 0);
    hal_cap_cnt_stop(aic_capture->data->id);
    hal_cap_disable(aic_capture->data->id);

    return RT_EOK;
}

static rt_err_t aic_cap_get_pulsewidth(struct rt_inputcapture_device *capture, rt_uint32_t *pulsewidth_us)
{
    struct aic_cap *aic_capture;

    RT_ASSERT(capture != RT_NULL);

    aic_capture = (struct aic_cap *)capture;

    aic_capture->data->freq = PWMCS_CLK_RATE / hal_cap_reg2(aic_capture->data->id);
    aic_capture->data->duty = (float)hal_cap_reg1(aic_capture->data->id) * 100 / (float)hal_cap_reg2(aic_capture->data->id);

    aic_capture->rtdev.parent.user_data = (void *)aic_capture->data;

    *pulsewidth_us = hal_cap_reg2(aic_capture->data->id) / (PWMCS_CLK_RATE / 1000000);

    return RT_EOK;
}

static struct rt_inputcapture_ops aic_input_ops =
{
    .init   =   aic_cap_init,
    .open   =   aic_cap_open,
    .close  =   aic_cap_close,
    .get_pulsewidth =   aic_cap_get_pulsewidth,
};

irqreturn_t aic_cap_irq(int irq, void *arg)
{
    u32 stat;

    for (int i = 0; i < AIC_CAP_CH_NUM; i++) {
        stat = hal_cap_int_flg(i);
        if (stat & CAP_EVENT3_FLG) {
            rt_hw_inputcapture_isr(&g_cap[i]->rtdev, 0);
            hal_cap_clr_flg(i, CAP_EVENT3_FLG);
        }
    }

    return IRQ_HANDLED;
}

static rt_err_t aic_cap_probe(struct aic_cap_data *pdata)
{
    struct aic_cap *cap;
    rt_err_t ret = RT_EOK;
    char aic_cap_device_name[10] = "";

    cap = (struct aic_cap *)malloc(sizeof(struct aic_cap));
    if (!cap) {
        LOG_E("Failed to malloc(%d)\n", (u32)sizeof(struct aic_cap));
        return -RT_ENOMEM;
    }

    cap->data = pdata;
    cap->rtdev.ops = &aic_input_ops;

    /* store the pointer */
    g_cap[cap->data->id] = cap;

    snprintf(aic_cap_device_name, 10, "cap%d", cap->data->id);

    ret = rt_device_inputcapture_register(&cap->rtdev, aic_cap_device_name, NULL);
    if (ret == RT_EOK) {
        LOG_I("ArtInChip %s loaded", aic_cap_device_name);
    } else {
        LOG_E("%s register failed", aic_cap_device_name);
        goto err;
    }

    return ret;

err:
    if (cap)
        free(cap);

    return ret;
}

static int drv_cap_init(void)
{
    rt_err_t ret = RT_EOK;
    u32 i;

    for (i = 0; i < ARRAY_SIZE(g_cap_info); i++) {
        ret = aic_cap_probe(&g_cap_info[i]);
        if (ret)
            return ret;
    }

    hal_cap_init();
    aicos_request_irq(PWMCS_CAP_IRQn, aic_cap_irq, 0, NULL, NULL);

    return ret;
}
INIT_DEVICE_EXPORT(drv_cap_init);
