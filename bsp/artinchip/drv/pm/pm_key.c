/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */

#include <stdio.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <string.h>
#include <aic_osal.h>

volatile uint8_t wakeup_triggered;

void pm_key_irq_callback(void *args)
{
    rt_uint8_t sleep_mode;

    sleep_mode = rt_pm_get_sleep_mode();

    if (sleep_mode == PM_SLEEP_MODE_NONE)
    {   /* enter sleep */
        rt_pm_module_release(PM_POWER_ID, PM_SLEEP_MODE_NONE);
        wakeup_triggered = 0;
    }
    else
    {
        rt_pm_module_request(PM_POWER_ID, PM_SLEEP_MODE_NONE);
        wakeup_triggered = 1;
#ifdef AIC_PM_POWER_TOUCH_WAKEUP
        /* touch timer restart */
        rt_timer_start(touch_timer);
#endif
    }
}

int pm_key_init(void)
{
    rt_base_t pin;
    unsigned int g, p;

    pin = rt_pin_get(AIC_PM_POWER_KEY_GPIO);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_set_drive_strength(g, p, 3);
    hal_gpio_set_debounce(g, p, 0xFFF);

    rt_pin_mode(pin, PIN_MODE_INPUT_PULLUP);

    rt_pin_attach_irq(pin, PIN_IRQ_MODE_FALLING, pm_key_irq_callback, RT_NULL);
    rt_pin_irq_enable(pin, PIN_IRQ_ENABLE);

    return 0;
}
INIT_DEVICE_EXPORT(pm_key_init);
