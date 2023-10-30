/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_gpio.h"

/* GPIO register offset */

#define GROUP_FACTOR 0x100
#define PIN_FACTOR   0x4

#define GEN_IN_STAT_REG  0x0
#define GEN_OUT_CFG_REG  0x4
#define GEN_TRQ_EN_REG   0x8
#define GEN_IRQ_STAT_REG 0xC
#define GEN_OUT_CLR_REG  0x10
#define GEN_OUT_SET_REG  0x14
#define GEN_OUT_TOG_REG  0x18
#define PIN_CFG_REG      0x80

/* PIN_CFG register field */

#define PIN_FUN_SHIFT      0
#define PIN_FUN_MASK       0xF
#define PIN_DRV_SHIFT      4
#define PIN_DRV_MASK       0x7
#define PIN_PULL_SHIFT     8
#define PIN_PULL_MASK      0x3
#define PIN_IRQ_MODE_SHIFT 12
#define PIN_IRQ_MODE_MASK  0x7
#define PIN_DIR_SHIFT      16
#define PIN_DIR_MASK       0x3
#define PIN_DIR_INPUT      0x1
#define PIN_DIR_OUTPUT     0x2
#define PIN_SPE_IE_FORCE   (0x1 << 18)
#define PIN_DEBOUNCE_SHIFT 20
#define PIN_DEBOUNCE_MASK  0xFFF

#define gen_reg(_g, _offset) \
    (volatile void *)((_g * GROUP_FACTOR) + _offset + GPIO_BASE)
#define cfg_reg(_g, _p) \
    (volatile void *)((_g * GROUP_FACTOR) + (_p * PIN_FACTOR) + PIN_CFG_REG + \
                      GPIO_BASE)

unsigned int hal_gpio_name2pin(const char *name)
{
    unsigned int pin = 0;
    int hw_port_num, hw_pin_num = 0;
    int i, name_len;

    name_len = strlen(name);

    if ((name_len < 4) || (name_len > 6))
    {
        return -1;
    }
    if ((name[0] != 'P') || (name[1] < 'A') || (name[2] != '.'))
    {
        return -1;
    }

    hw_port_num = (int)(name[1] - 'A');

    for (i = 3; i < name_len; i++)
    {
        hw_pin_num *= 10;
        hw_pin_num += name[i] - '0';
    }

    pin = hw_port_num*GPIO_GROUP_SIZE + hw_pin_num;

    return pin;
}

int hal_gpio_get_value(unsigned int group, unsigned int pin,
                       unsigned int *pvalue)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = readl(gen_reg(group, GEN_IN_STAT_REG));

    *pvalue = !!(val & (1 << pin));

    return 0;
}

int hal_gpio_set_value(unsigned int group, unsigned int pin, unsigned int value)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = readl(gen_reg(group, GEN_OUT_CFG_REG));

    val &= ~(1 << pin);
    val |= (!!value) << pin;

    writel(val, gen_reg(group, GEN_OUT_CFG_REG));

    return 0;
}

int hal_gpio_clr_output(unsigned int group, unsigned int pin)
{
    unsigned int val;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = 1 << pin;

    writel(val, gen_reg(group, GEN_OUT_CLR_REG));

    return 0;
}

int hal_gpio_set_output(unsigned int group, unsigned int pin)
{
    unsigned int val;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = 1 << pin;

    writel(val, gen_reg(group, GEN_OUT_SET_REG));

    return 0;
}

int hal_gpio_toggle_output(unsigned int group, unsigned int pin)
{
    unsigned int val;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = 1 << pin;

    writel(val, gen_reg(group, GEN_OUT_TOG_REG));

    return 0;
}

int hal_gpio_enable_irq(unsigned int group, unsigned int pin)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = readl(gen_reg(group, GEN_TRQ_EN_REG));

    val |= (1 << pin);

    writel(val, gen_reg(group, GEN_TRQ_EN_REG));

    return 0;
}

int hal_gpio_disable_irq(unsigned int group, unsigned int pin)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = readl(gen_reg(group, GEN_TRQ_EN_REG));

    val &= ~(1 << pin);

    writel(val, gen_reg(group, GEN_TRQ_EN_REG));

    return 0;
}

int hal_gpio_group_get_irq_en(unsigned int group, unsigned int *pen)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0, -EINVAL);

    val = readl(gen_reg(group, GEN_TRQ_EN_REG));

    *pen = val;

    return 0;
}


int hal_gpio_group_get_irq_stat(unsigned int group, unsigned int *pstat)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0, -EINVAL);

    val = readl(gen_reg(group, GEN_IRQ_STAT_REG));

    *pstat = val;

    return 0;
}

int hal_gpio_group_set_irq_stat(unsigned int group, unsigned int stat)
{
    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0, -EINVAL);

    writel(stat, gen_reg(group, GEN_IRQ_STAT_REG));

    return 0;
}


int hal_gpio_get_irq_stat(unsigned int group, unsigned int pin,
                          unsigned int *pstat)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = readl(gen_reg(group, GEN_IRQ_STAT_REG));

    *pstat = !!(val & (1 << pin));

    return 0;
}

int hal_gpio_clr_irq_stat(unsigned int group, unsigned int pin)
{
    unsigned int val;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = 1 << pin;

    writel(val, gen_reg(group, GEN_IRQ_STAT_REG));

    return 0;
}

int hal_gpio_set_func(unsigned int group, unsigned int pin, unsigned int func)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~(PIN_FUN_MASK << PIN_FUN_SHIFT);
    val |= (func & PIN_FUN_MASK) << PIN_FUN_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_get_func(unsigned int group, unsigned int pin, unsigned int *pfunc)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = readl(cfg_reg(group, pin));

    *pfunc = (val & PIN_FUN_MASK) >> PIN_FUN_SHIFT;

    return 0;
}

int hal_gpio_set_drive_strength(unsigned int group, unsigned int pin,
                                unsigned int strength)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~(PIN_DRV_MASK << PIN_DRV_SHIFT);
    val |= (strength & PIN_DRV_MASK) << PIN_DRV_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_set_bias_pull(unsigned int group, unsigned int pin,
                           unsigned int pull)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~(PIN_PULL_MASK << PIN_PULL_SHIFT);
    val |= (pull & PIN_PULL_MASK) << PIN_PULL_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_set_irq_mode(unsigned int group, unsigned int pin,
                          unsigned int irq_mode)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~(PIN_IRQ_MODE_MASK << PIN_IRQ_MODE_SHIFT);
    val |= (irq_mode & PIN_IRQ_MODE_MASK) << PIN_IRQ_MODE_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_direction_input(unsigned int group, unsigned int pin)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~(PIN_DIR_MASK << PIN_DIR_SHIFT);
    val |= PIN_DIR_INPUT << PIN_DIR_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_direction_output(unsigned int group, unsigned int pin)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~(PIN_DIR_MASK << PIN_DIR_SHIFT);
    val |= PIN_DIR_OUTPUT << PIN_DIR_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_set_debounce(unsigned int group, unsigned int pin,
                          unsigned int debounce)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && group >= 0 && pin < 32 && pin >= 0,
                -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~((u32)PIN_DEBOUNCE_MASK << PIN_DEBOUNCE_SHIFT);
    val |= (debounce & PIN_DEBOUNCE_MASK) << PIN_DEBOUNCE_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_cfg(struct gpio_cfg *cfg, u32 cnt)
{
    u32 i = 0;

    for (i = 0; i < cnt; i++, cfg++) {
        if (!cfg) {
            //hal_log_err("%d gpio_cfg is NULL", i);
            return -1;
        }

        hal_gpio_set_func(cfg->port, cfg->pin, cfg->func);
        hal_gpio_set_drive_strength(cfg->port, cfg->pin, cfg->driver);
        hal_gpio_set_bias_pull(cfg->port, cfg->pin, cfg->pull);
    }
    return 0;
}
