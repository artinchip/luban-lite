/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"

#define SDA_GPIO    "PD.5"
#define SCL_GPIO    "PD.4"
#define CS_GPIO     "PD.0"
#define RESET       "PE.18"

static struct gpio_desc sda;
static struct gpio_desc scl;
static struct gpio_desc cs;
static struct gpio_desc reset;

static void hx8238_send_cmd(unsigned char cmd)
{
    int i;

    for (i = 0; i < 8; i++) {
        panel_gpio_set_value(&scl, 0);

        if (cmd & 0x80)
            panel_gpio_set_value(&sda, 1);
        else
            panel_gpio_set_value(&sda, 0);

        aic_delay_us(5);

        panel_gpio_set_value(&scl, 1);
        aic_delay_us(5);
        cmd <<= 1;
    }
}

static void hx8238_send_data(unsigned int data)
{
    int i;

    for (i = 0; i < 16; i++) {
        panel_gpio_set_value(&scl, 0);

        if (data & 0x8000)
            panel_gpio_set_value(&sda, 1);
        else
            panel_gpio_set_value(&sda, 0);

        aic_delay_us(5);

        panel_gpio_set_value(&scl, 1);
        aic_delay_us(5);
        data <<= 1;
    }
}

static void hx8238_write(unsigned char code,unsigned int data)
{
    panel_gpio_set_value(&sda, 1);
    panel_gpio_set_value(&scl, 1);
    panel_gpio_set_value(&cs, 1);

    aic_delay_us(50);
    panel_gpio_set_value(&cs, 0);
    aic_delay_us(50);

    hx8238_send_cmd(code);
    hx8238_send_data(data);

    panel_gpio_set_value(&scl, 0);
    aic_delay_us(5);

    panel_gpio_set_value(&sda, 1);
    panel_gpio_set_value(&scl, 1);
    panel_gpio_set_value(&cs, 1);
}

void hx8238_init(void)
{
    panel_get_gpio(&sda, SDA_GPIO);
    panel_get_gpio(&scl, SCL_GPIO);
    panel_get_gpio(&cs, CS_GPIO);
    panel_get_gpio(&reset, RESET);

    panel_gpio_set_value(&reset, 0);
    aic_delay_us(20);
    panel_gpio_set_value(&reset, 1);

    panel_gpio_set_value(&sda, 1);
    panel_gpio_set_value(&scl, 1);
    panel_gpio_set_value(&cs, 1);

    aic_delay_us(10);

    hx8238_write(0x70, 0x0001);
    hx8238_write(0x72, 0x6300);

    hx8238_write(0x70, 0x0002);
    hx8238_write(0x72, 0x0200);

    hx8238_write(0x70, 0x0003);
    hx8238_write(0x72, 0x6364);

    hx8238_write(0x70, 0x0004);
    hx8238_write(0x72, 0x0489);

    hx8238_write(0x70, 0x0005);
    hx8238_write(0x72, 0xBCC4);

    hx8238_write(0x70, 0x000A);
    hx8238_write(0x72, 0x4008);

    hx8238_write(0x70, 0x000B);
    hx8238_write(0x72, 0xD400);

    hx8238_write(0x70, 0x000D);
    hx8238_write(0x72, 0x3229);

    hx8238_write(0x70, 0x000E);
    hx8238_write(0x72, 0x3200);

    hx8238_write(0x70, 0x000F);
    hx8238_write(0x72, 0x0000);

    hx8238_write(0x70, 0x0016);
    hx8238_write(0x72, 0x9F80);

    hx8238_write(0x70, 0x0017);
    hx8238_write(0x72, 0x2212);

    hx8238_write(0x70, 0x001E);
    hx8238_write(0x72, 0x00D2);

    hx8238_write(0x70, 0x0030);
    hx8238_write(0x72, 0x0000);

    hx8238_write(0x70, 0x0031);
    hx8238_write(0x72, 0x0407);

    hx8238_write(0x70, 0x0032);
    hx8238_write(0x72, 0x0202);

    hx8238_write(0x70, 0x0033);
    hx8238_write(0x72, 0x0000);

    hx8238_write(0x70, 0x0034);
    hx8238_write(0x72, 0x0505);

    hx8238_write(0x70, 0x0035);
    hx8238_write(0x72, 0x0003);

    hx8238_write(0x70, 0x0036);
    hx8238_write(0x72, 0x0707);

    hx8238_write(0x70, 0x0037);
    hx8238_write(0x72, 0x0000);

    hx8238_write(0x70, 0x003A);
    hx8238_write(0x72, 0x0904);

    hx8238_write(0x70, 0x003B);
    hx8238_write(0x72, 0x0904);
}

static int panel_prepare(void)
{
    hx8238_init();

    return panel_default_prepare();
}

static struct aic_panel_funcs panel_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_prepare,
    .enable = panel_default_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing hx8238_timing = {
    .pixelclock = 8000000,
    .hactive = 320,
    .hfront_porch = 20,
    .hback_porch = 12,
    .hsync_len = 11,
    .vactive = 240,
    .vfront_porch = 8,
    .vback_porch = 6,
    .vsync_len = 16,
};

static struct panel_rgb rgb = {
    .mode = SRGB,
    .format = SRGB_8BIT,
};

struct aic_panel srgb_hx8238 = {
    .name = "panel-hx8238",
    .timings = &hx8238_timing,
    .funcs = &panel_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};
