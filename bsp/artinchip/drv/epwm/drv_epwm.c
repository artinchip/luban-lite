/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#include <drivers/rt_drv_pwm.h>
#include <drivers/pm.h>

#define LOG_TAG         "EPWM"
#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_epwm.h"

static struct rt_device_pwm g_aic_epwm = {0};
static struct aic_epwm_pulse_para g_pulse_para[AIC_EPWM_CH_NUM] = {0};

static void aic_epwm_default_action(void)
{
    struct aic_epwm_action action0 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_LOW, EPWM_ACT_HIGH, EPWM_ACT_NONE};
    struct aic_epwm_action action1 = {
        /*       CBD,          CBU,          CAD, */
        EPWM_ACT_NONE, EPWM_ACT_NONE, EPWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        EPWM_ACT_LOW, EPWM_ACT_HIGH,  EPWM_ACT_NONE};

#ifdef AIC_USING_EPWM0
    hal_epwm_ch_init(0, EPWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_EPWM1
    hal_epwm_ch_init(1, EPWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_EPWM2
    hal_epwm_ch_init(2, EPWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_EPWM3
    hal_epwm_ch_init(3, EPWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_EPWM4
    hal_epwm_ch_init(4, EPWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_EPWM5
    hal_epwm_ch_init(5, EPWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_EPWM6
    hal_epwm_ch_init(6, EPWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_EPWM7
    hal_epwm_ch_init(7, EPWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_EPWM8
    hal_epwm_ch_init(8, EPWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_EPWM9
    hal_epwm_ch_init(9, EPWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_EPWM10
    hal_epwm_ch_init(10, EPWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_EPWM11
    hal_epwm_ch_init(11, EPWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
}

static rt_bool_t drv_epwm_ch_valid(struct rt_pwm_configuration *cfg)
{
    if (cfg->channel > (AIC_EPWM_CH_NUM - 1)) {
        LOG_E("Invalid channel No.%d", cfg->channel);
        return RT_TRUE;
    }

    return RT_FALSE;
}

static rt_err_t drv_epwm_enable(struct rt_device_pwm *device,
                                struct rt_pwm_configuration *cfg,
                                rt_bool_t enable)
{
    if (drv_epwm_ch_valid(cfg))
        return -RT_EINVAL;

    if (enable)
        return !hal_epwm_enable(cfg->channel) ? RT_EOK : -RT_ERROR;
    else
        return !hal_epwm_disable(cfg->channel) ? RT_EOK : -RT_ERROR;
}

static rt_err_t drv_epwm_set(struct rt_device_pwm *device,
                             struct rt_pwm_configuration *cfg)
{
    if (drv_epwm_ch_valid(cfg))
        return -RT_EINVAL;

    if (hal_epwm_set(cfg->channel, cfg->pulse, cfg->period))
        return -RT_ERROR;

    return RT_EOK;
}

static rt_err_t drv_epwm_set_pul(struct rt_device_pwm *device,
                             struct rt_pwm_configuration *cfg)
{
    if (drv_epwm_ch_valid(cfg))
        return -RT_EINVAL;

    g_pulse_para[cfg->channel].pulse_cnt = cfg->pul_cnt;
    g_pulse_para[cfg->channel].duty_ns = cfg->pulse;
    g_pulse_para[cfg->channel].prd_ns = cfg->period;

    if (hal_epwm_set(cfg->channel, cfg->pulse, cfg->period))
        return -RT_ERROR;

    hal_epwm_int_config(cfg->channel, cfg->irq_mode, 1);

    hal_epwm_enable(cfg->channel);
    return RT_EOK;
}

static rt_err_t drv_epwm_get(struct rt_device_pwm *device,
                             struct rt_pwm_configuration *cfg)
{
    if (drv_epwm_ch_valid(cfg))
        return -RT_EINVAL;

    if (hal_epwm_get(cfg->channel, (u32 *)&cfg->pulse, (u32 *)&cfg->period))
        return -RT_ERROR;

    return RT_EOK;
}

static rt_err_t drv_epwm_control(struct rt_device_pwm *device,
                                int cmd, void *arg)
{
    struct rt_pwm_configuration *cfg = (struct rt_pwm_configuration *)arg;

    switch (cmd) {
    case PWM_CMD_ENABLE:
        return drv_epwm_enable(device, cfg, RT_TRUE);
    case PWM_CMD_DISABLE:
        return drv_epwm_enable(device, cfg, RT_FALSE);
    case PWM_CMD_SET:
        return drv_epwm_set(device, cfg);
    case PWM_CMD_GET:
        return drv_epwm_get(device, cfg);
    case PWM_CMD_SET_PUL:
        return drv_epwm_set_pul(device, cfg);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }

    return RT_EOK;
}

static struct rt_pwm_ops aic_epwm_ops = {
    .control = drv_epwm_control
};

#ifdef RT_USING_PM
static int drv_epwm_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        hal_clk_disable(CLK_PWMCS);
        break;
    default:
        break;
    }

    return 0;
}

static void drv_epwm_resume(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        hal_clk_set_freq(CLK_PWMCS, EPWM_CLK_RATE);
        hal_clk_enable(CLK_PWMCS);
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops drv_epwm_pm_ops =
{
    SET_LATE_DEVICE_PM_OPS(drv_epwm_suspend, drv_epwm_resume)
    NULL,
};
#endif

irqreturn_t aic_epwm_irq(int irq, void *arg)
{
    static u32 isr_cnt[AIC_EPWM_CH_NUM] = {0};
    u32 stat;
    int i;

    for (i = 0; i < AIC_EPWM_CH_NUM; i++) {
        stat = hal_epwm_int_sts(i);
        if (stat & EPWM_INT_FLG) {
            isr_cnt[i]++;
            if (isr_cnt[i] == g_pulse_para[i].pulse_cnt) {
                hal_epwm_set(i, g_pulse_para[i].prd_ns, g_pulse_para[i].prd_ns);
                hal_epwm_int_config(i, 0, 0);
                pr_info("\nisr cnt:%d,disabled the epwm%d interrupt now.\n", isr_cnt[i], i);
                isr_cnt[i] = 0;
            }
            hal_epwm_clr_int(stat, i);
        }
    }

    return IRQ_HANDLED;
}

int drv_epwm_init(void)
{
    hal_epwm_init();
    aic_epwm_default_action();

    aicos_request_irq(PWMCS_PWM_IRQn, aic_epwm_irq, 0, NULL, NULL);

    if (rt_device_pwm_register(&g_aic_epwm, "epwm", &aic_epwm_ops, NULL))
        return -RT_ERROR;

#ifdef RT_USING_PM
    rt_pm_device_register(&g_aic_epwm.parent, &drv_epwm_pm_ops);
#endif

    LOG_I("ArtInChip EPWM loaded");
    return RT_EOK;
}
INIT_PREV_EXPORT(drv_epwm_init);


#if defined(RT_USING_FINSH)
#include <finsh.h>

static void cmd_epwm_status(int argc, char **argv)
{
     hal_epwm_status_show();
}

MSH_CMD_EXPORT_ALIAS(cmd_epwm_status, epwm_status, Show the status of EPWM);

#endif
