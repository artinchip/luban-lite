/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <drivers/hwtimer.h>

#include "drv_hrtimer.h"

#define LOG_TAG                 "HRTimer"
#include "aic_core.h"
#include "hal_cap.h"

#define HRTIMER_DEV_NAME        "hrtimer"
#define HRTIMER_DEFAULT_CYCLE   1000000

static const struct rt_hwtimer_info drv_hrtimer_info =
{
    .maxfreq = HRTIMER_DEFAULT_CYCLE,
    .minfreq = 1,
    .maxcnt  = 0x7FFFFF,
    .cntmode = HWTIMER_CNTMODE_UP,
};

static struct hrtimer_info g_hrtimer_info[] = {
#ifdef AIC_USING_HRTIMER0
    {HRTIMER_DEV_NAME"0", 0},
#endif
#ifdef AIC_USING_HRTIMER1
    {HRTIMER_DEV_NAME"1", 1},
#endif
#ifdef AIC_USING_HRTIMER2
    {HRTIMER_DEV_NAME"2", 2},
#endif
#ifdef AIC_USING_HRTIMER3
    {HRTIMER_DEV_NAME"3", 3},
#endif
#ifdef AIC_USING_HRTIMER4
    {HRTIMER_DEV_NAME"4", 4},
#endif
#ifdef AIC_USING_HRTIMER5
    {HRTIMER_DEV_NAME"5", 5},
#endif
};

struct hrtimer_info *_get_hrtimer_priv(rt_hwtimer_t *timer)
{
    return (struct hrtimer_info *)timer->parent.user_data;
}

irqreturn_t drv_hrtimer_irq(int irq, void *arg)
{
    u32 i;
    struct hrtimer_info *info = g_hrtimer_info;

    for (i = 0; i < ARRAY_SIZE(g_hrtimer_info); i++, info++) {
        if (hal_cap_is_pending(info->id))
            rt_device_hwtimer_isr(&info->hrtimer);
    }

    return IRQ_HANDLED;
}

static void drv_hrtimer_init(rt_hwtimer_t *timer, rt_uint32_t state)
{
    struct hrtimer_info *info = _get_hrtimer_priv(timer);

    if (state)
        hal_cap_ch_init(info->id);
    else
        hal_cap_ch_deinit(info->id);
}

static rt_err_t drv_hrtimer_start(rt_hwtimer_t *timer, rt_uint32_t cnt,
                                  rt_hwtimer_mode_t mode)
{
    struct hrtimer_info *info = _get_hrtimer_priv(timer);

    hal_cap_enable(info->id);
    hal_cap_set_cnt(info->id, cnt);
    hal_cap_int_enable(info->id, 1);
    hal_cap_cnt_start(info->id);
    return RT_EOK;
}

static void drv_hrtimer_stop(rt_hwtimer_t *timer)
{
    struct hrtimer_info *info = _get_hrtimer_priv(timer);

    hal_cap_cnt_stop(info->id);
    hal_cap_int_enable(info->id, 0);
    hal_cap_disable(info->id);
}

static rt_err_t drv_hrtimer_ctrl(rt_hwtimer_t *timer,
                                 rt_uint32_t cmd, void *args)
{
    struct hrtimer_info *info = _get_hrtimer_priv(timer);

    switch (cmd) {
    case HWTIMER_CTRL_FREQ_SET:
        /* set timer frequence */
        return hal_cap_set_freq(info->id, *((rt_uint32_t *)args));

    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

static const struct rt_hwtimer_ops drv_hrtimer_ops =
{
    .init  = drv_hrtimer_init,
    .start = drv_hrtimer_start,
    .stop  = drv_hrtimer_stop,
    .control = drv_hrtimer_ctrl,
};

static int drv_hwtimer_init(void)
{
    u32 i;
    rt_err_t ret = RT_EOK;
    struct hrtimer_info *info = g_hrtimer_info;

    for (i = 0; i < ARRAY_SIZE(g_hrtimer_info); i++, info++) {
        info->hrtimer.info = &drv_hrtimer_info;
        info->hrtimer.ops  = &drv_hrtimer_ops;
        ret = rt_device_hwtimer_register(&info->hrtimer, info->name, info);
        if (ret == RT_EOK)
            LOG_D("%s register success", info->name);
        else
            LOG_E("%s register failed", info->name);
    }

    hal_cap_init();
    aicos_request_irq(PWMCS_CAP_IRQn, drv_hrtimer_irq, 0, NULL, NULL);

    return ret;
}
INIT_DEVICE_EXPORT(drv_hwtimer_init);

#if defined(RT_USING_FINSH)
#include <finsh.h>

static void cmd_hrtimer_status(int argc, char **argv)
{
     hal_cap_status_show();
}

MSH_CMD_EXPORT_ALIAS(cmd_hrtimer_status, hrtimer_status,
                     Show the status of HRTimer);

#endif
