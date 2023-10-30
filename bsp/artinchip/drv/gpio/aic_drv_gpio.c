/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_gpio.h"

#ifdef RT_USING_PIN

void drv_pin_bias_set(unsigned int pin, unsigned int pull)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_bias_pull(g, p, pull);
}

void drv_pin_drive_set(unsigned int pin, unsigned int strength)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_drive_strength(g, p, strength);
}

void drv_pin_mux_set(unsigned int pin, unsigned int func)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, func);
}

unsigned int drv_pin_mux_get(unsigned int pin)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);
    unsigned int func;

    hal_gpio_get_func(g, p, &func);

    return func;
}

void drv_pin_mode(struct rt_device *device, rt_base_t pin, rt_base_t mode)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, 1);
    switch (mode)
    {
    case PIN_MODE_INPUT:
        hal_gpio_set_bias_pull(g, p, PIN_PULL_DIS);
        hal_gpio_direction_input(g, p);
        break;
    case PIN_MODE_INPUT_PULLUP:
        hal_gpio_set_bias_pull(g, p, PIN_PULL_UP);
        hal_gpio_direction_input(g, p);
        break;
    case PIN_MODE_INPUT_PULLDOWN:
        hal_gpio_set_bias_pull(g, p, PIN_PULL_DOWN);
        hal_gpio_direction_input(g, p);
        break;
    case PIN_MODE_OUTPUT:
    case PIN_MODE_OUTPUT_OD:
    default:
        hal_gpio_set_bias_pull(g, p, PIN_PULL_DIS);
        hal_gpio_direction_output(g, p);
        break;
    }
}

void drv_pin_write(struct rt_device *device, rt_base_t pin, rt_base_t value)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    if (PIN_LOW == value)
    {
        hal_gpio_clr_output(g, p);
    }
    else
    {
        hal_gpio_set_output(g, p);
    }
}

int drv_pin_read(struct rt_device *device, rt_base_t pin)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);
    unsigned int value = PIN_LOW;

    hal_gpio_get_value(g, p, &value);

    return value;
}

#ifdef AIC_GPIO_IRQ_DRV_EN
rt_err_t drv_pin_attach_irq(struct rt_device *device, rt_int32_t pin,
                             rt_uint32_t mode, void (*hdr)(void *args), void *args)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);
    unsigned int irq_mode = 0;

    switch (mode)
    {
      case PIN_IRQ_MODE_RISING:
      irq_mode=PIN_IRQ_MODE_EDGE_RISING;
      break;
      case PIN_IRQ_MODE_FALLING:
      irq_mode=PIN_IRQ_MODE_EDGE_FALLING;
      break;
      case PIN_IRQ_MODE_RISING_FALLING:
      irq_mode=PIN_IRQ_MODE_EDGE_BOTH;
      break;
      case PIN_IRQ_MODE_HIGH_LEVEL:
      irq_mode=PIN_IRQ_MODE_LEVEL_HIGH;
      break;
      case PIN_IRQ_MODE_LOW_LEVEL:
      irq_mode=PIN_IRQ_MODE_LEVEL_LOW;
      break;
    }
    hal_gpio_set_irq_mode(g, p, irq_mode);

    aicos_request_irq(AIC_GPIO_TO_IRQ(pin), (irq_handler_t)hdr, 0, "pin", args);

    return RT_EOK;
}

rt_err_t drv_pin_detach_irq(struct rt_device *device, rt_int32_t pin)
{
    return RT_EOK;
}

irqreturn_t drv_gpio_group_irqhandler(int irq, void * data)
{
    unsigned int g = irq - GPIO_IRQn;
    unsigned int stat = 0;
    unsigned int mask = 0;
    unsigned int i = 0;
    unsigned int gpio_irq = 0;

    hal_gpio_group_get_irq_stat(g, &stat);
    hal_gpio_group_get_irq_en(g, &mask);
    stat &= mask;

    for (i=0; i<32; i++){
        if (!(stat & (1U<<i)))
            continue;

        gpio_irq = AIC_GPIO_TO_IRQ(g*GPIO_GROUP_SIZE + i);
        drv_irq_call_isr(gpio_irq);
    }

    hal_gpio_group_set_irq_stat(g, 0xFFFFFFFF);

    return IRQ_HANDLED;
}

unsigned int pin_group_irq_en = 0;
rt_err_t drv_pin_irq_enable(struct rt_device *device, rt_base_t pin, rt_uint32_t enabled)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    if (enabled) {
        if (!(pin_group_irq_en & (1<<g))){
            aicos_request_irq(GPIO_IRQn + g, drv_gpio_group_irqhandler, 0, "pin_group", NULL);
            aicos_irq_enable(GPIO_IRQn + g);
            pin_group_irq_en |= (1<<g);
        }
        hal_gpio_enable_irq(g, p);
    } else {
        hal_gpio_disable_irq(g, p);
    }
    return RT_EOK;
}
#endif

rt_base_t drv_pin_get(const char *name)
{
    return hal_gpio_name2pin(name);
}

const static struct rt_pin_ops _drv_pin_ops =
{
    drv_pin_mode,
    drv_pin_write,
    drv_pin_read,
#ifdef AIC_GPIO_IRQ_DRV_EN
    drv_pin_attach_irq,
    drv_pin_detach_irq,
    drv_pin_irq_enable,
#else
    RT_NULL,
    RT_NULL,
    RT_NULL,
#endif
    drv_pin_get,
};

int drv_pin_init(void)
{
    int ret = RT_EOK;

    ret = rt_device_pin_register("pin", &_drv_pin_ops, RT_NULL);

    return ret;
}
INIT_BOARD_EXPORT(drv_pin_init);

#endif /*RT_USING_PIN */


