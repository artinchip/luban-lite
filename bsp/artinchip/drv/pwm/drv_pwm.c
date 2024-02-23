/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <drivers/rt_drv_pwm.h>
#include <drivers/pm.h>

#define LOG_TAG         "PWM"
#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_pwm.h"

static struct rt_device_pwm g_aic_pwm = {0};
static struct aic_pwm_pulse_para g_pulse_para[AIC_PWM_CH_NUM] = {0};

static void aic_pwm_default_action(void)
{
    struct aic_pwm_action action0 = {
        /*       CBD,          CBU,          CAD, */
        PWM_ACT_NONE, PWM_ACT_NONE, PWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        PWM_ACT_LOW,  PWM_ACT_NONE, PWM_ACT_HIGH};
    struct aic_pwm_action action1 = {
        /*       CBD,          CBU,          CAD, */
        PWM_ACT_NONE, PWM_ACT_NONE, PWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        PWM_ACT_LOW, PWM_ACT_NONE,  PWM_ACT_HIGH};

#ifdef AIC_USING_PWM0
    hal_pwm_ch_init(0, PWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_PWM1
    hal_pwm_ch_init(1, PWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_PWM2
    hal_pwm_ch_init(2, PWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
#ifdef AIC_USING_PWM3
    hal_pwm_ch_init(3, PWM_MODE_UP_COUNT, 0, &action0, &action1);
#endif
}

static rt_bool_t drv_pwm_ch_valid(struct rt_pwm_configuration *cfg)
{
    if (cfg->channel > (AIC_PWM_CH_NUM - 1)) {
        LOG_E("Invalid channel No.%d", cfg->channel);
        return RT_TRUE;
    }
    return RT_FALSE;
}

static rt_err_t drv_pwm_enable(struct rt_device_pwm *device,
                                struct rt_pwm_configuration *cfg,
                                rt_bool_t enable)
{
    if (drv_pwm_ch_valid(cfg))
        return -RT_EINVAL;

    if (enable)
        return !hal_pwm_enable(cfg->channel) ? RT_EOK : -RT_ERROR;
    else
        return !hal_pwm_disable(cfg->channel) ? RT_EOK : -RT_ERROR;
}

static rt_err_t drv_pwm_set(struct rt_device_pwm *device,
                             struct rt_pwm_configuration *cfg)
{
    if (drv_pwm_ch_valid(cfg))
        return -RT_EINVAL;

    if (hal_pwm_set(cfg->channel, cfg->pulse, cfg->period))
        return -RT_ERROR;

    return RT_EOK;
}

static rt_err_t drv_pwm_set_pul(struct rt_device_pwm *device,
                             struct rt_pwm_configuration *cfg)
{
    if (drv_pwm_ch_valid(cfg))
        return -RT_EINVAL;

    g_pulse_para[cfg->channel].pulse_cnt = cfg->pul_cnt;
    g_pulse_para[cfg->channel].duty_ns = cfg->pulse;
    g_pulse_para[cfg->channel].prd_ns = cfg->period;

    hal_pwm_int_config(cfg->channel, cfg->irq_mode, 1);

    if (hal_pwm_set(cfg->channel, cfg->pulse, cfg->period))
        return -RT_ERROR;

    hal_pwm_enable(cfg->channel);

    return RT_EOK;
}

static rt_err_t drv_pwm_get(struct rt_device_pwm *device,
                             struct rt_pwm_configuration *cfg)
{
    if (drv_pwm_ch_valid(cfg))
        return -RT_EINVAL;

    if (hal_pwm_get(cfg->channel, (u32 *)&cfg->pulse, (u32 *)&cfg->period))
        return -RT_ERROR;

    return RT_EOK;
}

static rt_err_t drv_pwm_control(struct rt_device_pwm *device,
                                int cmd, void *arg)
{
    struct rt_pwm_configuration *cfg = (struct rt_pwm_configuration *)arg;

    switch (cmd) {
    case PWM_CMD_ENABLE:
        return drv_pwm_enable(device, cfg, RT_TRUE);
    case PWM_CMD_DISABLE:
        return drv_pwm_enable(device, cfg, RT_FALSE);
    case PWM_CMD_SET:
        return drv_pwm_set(device, cfg);
    case PWM_CMD_GET:
        return drv_pwm_get(device, cfg);
    case PWM_CMD_SET_PUL:
        return drv_pwm_set_pul(device, cfg);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

static struct rt_pwm_ops aic_pwm_ops = {
    .control = drv_pwm_control
};

#ifdef RT_USING_PM
static int drv_pwm_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        hal_clk_disable(CLK_PWM);
        break;
    default:
        break;
    }

    return 0;
}

static void drv_pwm_resume(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        hal_clk_set_freq(CLK_PWM, PWM_CLK_RATE);
        hal_clk_enable(CLK_PWM);
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops drv_pwm_pm_ops =
{
    SET_LATE_DEVICE_PM_OPS(drv_pwm_suspend, drv_pwm_resume)
    NULL,
};
#endif

irqreturn_t aic_pwm_irq(int irq, void *arg)
{
    static u32 isr_cnt[AIC_PWM_CH_NUM] = {0};
    u32 stat;
    int i;

    stat = hal_pwm_int_sts();

    for (i = 0; i < AIC_PWM_CH_NUM; i++) {
        if (stat & (1 << i)) {
            isr_cnt[i]++;
            if (isr_cnt[i] == g_pulse_para[i].pulse_cnt) {
                hal_pwm_set(i, g_pulse_para[i].prd_ns, g_pulse_para[i].prd_ns);
                hal_pwm_int_config(i, 0, 0);
                pr_info("\nisr cnt:%d,disabled the pwm%d interrupt now.\n", isr_cnt[i], i);
                isr_cnt[i] = 0;
            }
        }
    }

    hal_pwm_clr_int(stat);

    return IRQ_HANDLED;
}

int drv_pwm_init(void)
{
    hal_pwm_init();
    aic_pwm_default_action();

    aicos_request_irq(PWM_IRQn, aic_pwm_irq, 0, NULL, NULL);

    if (rt_device_pwm_register(&g_aic_pwm, "pwm", &aic_pwm_ops, NULL))
        return -RT_ERROR;

#ifdef RT_USING_PM
    rt_pm_device_register(&g_aic_pwm.parent, &drv_pwm_pm_ops);
#endif

    LOG_I("ArtInChip PWM loaded");
    return RT_EOK;
}
INIT_PREV_EXPORT(drv_pwm_init);

#if defined(RT_USING_FINSH)
#include <finsh.h>

static void cmd_pwm_status(int argc, char **argv)
{
     hal_pwm_status_show();
}

MSH_CMD_EXPORT_ALIAS(cmd_pwm_status, pwm_status, Show the status of PWM);

#endif
