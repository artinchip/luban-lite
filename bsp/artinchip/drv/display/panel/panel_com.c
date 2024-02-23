
/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <aic_hal.h>

#include "panel_com.h"

static struct aic_panel *panels[] = {
#if defined(AIC_DISP_RGB) && defined(AIC_SIMPLE_PANEL)
    &aic_panel_rgb,
#endif
#if defined(AIC_DISP_LVDS) && defined(AIC_SIMPLE_PANEL)
    &aic_panel_lvds,
#endif
#ifdef AIC_PANEL_DSI_XM91080
    &dsi_xm91080,
#endif
#ifdef AIC_PANEL_DSI_ST7797
    &dsi_st7797,
#endif
#ifdef AIC_PANEL_DBI_ILI9488
    &dbi_ili9488,
#endif
#ifdef AIC_PANEL_DBI_ILI9341
    &dbi_ili9341,
#endif
#ifdef AIC_PANEL_DBI_ST77903
    &dbi_st77903,
#endif
#ifdef AIC_PANEL_DBI_ILI9486L
    &dbi_ili9486l,
#endif
#ifdef AIC_PANEL_RGB_ST7701S
    &rgb_st7701s,
#endif
#ifdef AIC_PANEL_SRGB_HX8238
    &srgb_hx8238
#endif
};

struct aic_panel *aic_find_panel(u32 connector_type)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(panels); i++) {
        if (panels[i]->connector_type == connector_type) {
            break;
        }
    }

    if (i >= ARRAY_SIZE(panels))
        return NULL;

    pr_info("find panel driver : %s\n", panels[i]->name);

    return panels[i];
}

/* The follow functions are defined for panel driver */

/* Enable the display interface */
void panel_di_enable(struct aic_panel *panel, u32 ms)
{
    if (panel && panel->callbacks.di_enable)
        panel->callbacks.di_enable();

    if (ms)
        aic_delay_ms(ms);
}

/* Disable the display interface */
void panel_di_disable(struct aic_panel *panel, u32 ms)
{
    if (panel && panel->callbacks.di_disable)
        panel->callbacks.di_disable();

    if (ms)
        aic_delay_ms(ms);
}

void panel_de_timing_enable(struct aic_panel *panel, u32 ms)
{
    if (panel && panel->callbacks.timing_enable)
        panel->callbacks.timing_enable();

    if (ms)
        aic_delay_ms(ms);
}

void panel_de_timing_disable(struct aic_panel *panel, u32 ms)
{
    if (panel && panel->callbacks.timing_disable)
        panel->callbacks.timing_disable();

    if (ms)
        aic_delay_ms(ms);
}

void panel_backlight_enable(struct aic_panel *panel, u32 ms)
{
#ifdef AIC_PANEL_ENABLE_GPIO
    unsigned int g, p;
    long pin;

    pin = hal_gpio_name2pin(AIC_PANEL_ENABLE_GPIO);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_direction_output(g, p);

#ifndef AIC_PANEL_ENABLE_GPIO_LOW
    hal_gpio_set_output(g, p);
#else
    hal_gpio_clr_output(g, p);
#endif
#endif /* AIC_PANEL_ENABLE_GPIO */

#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)
    struct rt_device_pwm *pwm_dev;

    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    /* pwm frequency: 1KHz = 1000000ns */
    rt_pwm_set(pwm_dev, AIC_PWM_BACKLIGHT_CHANNEL,
            1000000, 10000 * AIC_PWM_BRIGHTNESS_LEVEL);
    rt_pwm_enable(pwm_dev, AIC_PWM_BACKLIGHT_CHANNEL);
#endif
}

void panel_backlight_disable(struct aic_panel *panel, u32 ms)
{
#ifdef AIC_PANEL_ENABLE_GPIO
    unsigned int g, p;
    long pin;

    pin = hal_gpio_name2pin(AIC_PANEL_ENABLE_GPIO);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

#ifndef AIC_PANEL_ENABLE_GPIO_LOW
    hal_gpio_clr_output(g, p);
#else
    hal_gpio_set_output(g, p);
#endif
#endif /* AIC_PANEL_ENABLE_GPIO */

#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)
    struct rt_device_pwm *pwm_dev;

    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    rt_pwm_disable(pwm_dev, AIC_PWM_BACKLIGHT_CHANNEL);
#endif
}

int panel_default_prepare(void)
{
    return 0;
}

int panel_default_unprepare(void)
{
    return 0;
}

int panel_default_enable(struct aic_panel *panel)
{
    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

int panel_default_disable(struct aic_panel *panel)
{
    panel_backlight_disable(panel, 0);
    panel_di_disable(panel, 0);
    panel_de_timing_disable(panel, 0);
    return 0;
}

int panel_register_callback(struct aic_panel *panel,
                struct aic_panel_callbacks *pcallback)
{
    panel->callbacks.di_enable = pcallback->di_enable;
    panel->callbacks.di_disable = pcallback->di_disable;
    panel->callbacks.di_send_cmd = pcallback->di_send_cmd;
    panel->callbacks.di_set_videomode = pcallback->di_set_videomode;
    panel->callbacks.timing_enable = pcallback->timing_enable;
    panel->callbacks.timing_disable = pcallback->timing_disable;
    return 0;
}

void panel_send_command(u8 *para_cmd, u32 size, struct aic_panel *panel)
{
    u8 *p;
    u8 num, code;

    for (p = para_cmd; p < (para_cmd + size);) {
        num  = *p++ - 1;
        code = *p++;

        if (num == 0)
            aic_delay_ms((u32)code);
        else
            panel->callbacks.di_send_cmd((u32)code, 0, p, num);

        p += num;
    }
}

void panel_get_gpio(struct gpio_desc *desc, char *name)
{
    long pin;

    pin = hal_gpio_name2pin(name);

    desc->g = GPIO_GROUP(pin);
    desc->p = GPIO_GROUP_PIN(pin);

    hal_gpio_direction_output(desc->g, desc->p);
}

void panel_gpio_set_value(struct gpio_desc *desc, u32 value)
{
    if (value)
        hal_gpio_set_output(desc->g, desc->p);
    else
        hal_gpio_clr_output(desc->g, desc->p);
}

#ifdef AIC_PANEL_SPI_EMULATION
static struct panel_spi_device spi = { 0 };
static bool panel_spi_emulation = false;

static inline void panel_spi_set_scl(u32 value)
{
    panel_gpio_set_value(&spi.scl, value);
}

static inline void panel_spi_set_sdi(u32 value)
{
    panel_gpio_set_value(&spi.sdi, value);
}

static inline void panel_spi_set_cs(u32 value)
{
    panel_gpio_set_value(&spi.cs, value);
}

void panel_spi_cmd_wr(u8 cmd)
{
    u32 i;

    if (!panel_spi_emulation)
        return;

    panel_spi_set_cs(0);

    panel_spi_set_sdi(0);
    panel_spi_set_scl(0);

    aic_delay_us(1);
    panel_spi_set_scl(1);

    aic_delay_us(1);
    panel_spi_set_scl(0);

    for (i = 0; i < 8; i++) {
        if ((cmd & 0x80) == 0x80)
            panel_spi_set_sdi(1);
        else
            panel_spi_set_sdi(0);

        aic_delay_us(1);
        panel_spi_set_scl(1);
        aic_delay_us(1);
        panel_spi_set_scl(0);
        aic_delay_us(1);
        cmd = cmd << 1;
    }

    panel_spi_set_cs(1);
    panel_spi_set_sdi(0);
    panel_spi_set_scl(0);
    aic_delay_us(1);
}

void panel_spi_data_wr(u8 data)
{
    u32 i;

    if (!panel_spi_emulation)
        return;

    panel_spi_set_cs(0);
    panel_spi_set_scl(0);
    panel_spi_set_sdi(1);

    aic_delay_us(1);
    panel_spi_set_scl(1);

    aic_delay_us(1);
    panel_spi_set_scl(0);

    for (i = 0; i < 8; i++) {
        if ((data & 0x80) == 0x80)
            panel_spi_set_sdi(1);
        else
            panel_spi_set_sdi(0);

        aic_delay_us(1);
        panel_spi_set_scl(1);
        aic_delay_us(1);
        panel_spi_set_scl(0);
        aic_delay_us(1);
        data = data << 1;
    }
    panel_spi_set_cs(1);
    panel_spi_set_scl(0);
    panel_spi_set_sdi(0);
    aic_delay_us(1);
}

void panel_spi_device_emulation(char *cs, char *sdi, char *scl)
{
    panel_get_gpio(&spi.cs, cs);
    panel_get_gpio(&spi.scl, scl);
    panel_get_gpio(&spi.sdi, sdi);

    panel_spi_set_cs(1);
    panel_spi_set_scl(0);
    panel_spi_set_sdi(0);
    aic_delay_us(1);

    panel_spi_emulation = true;
}
#endif
