/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include <aic_hal.h>

#define SLEEP_PIN  "PE.1"
#define RESET_PIN  "PE.2"

#define CS         "PE.3"
#define SCL        "PA.8"
#define SDI        "PA.9"

static struct gpio_desc reset_gpio;
static struct gpio_desc sleep_gpio;

static void panel_gpio_init(void)
{
    panel_get_gpio(&reset_gpio, RESET_PIN);
    panel_get_gpio(&sleep_gpio, SLEEP_PIN);

    panel_gpio_set_value(&sleep_gpio, 1);
    aic_delay_ms(2);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(20);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}

static int panel_enable(struct aic_panel *panel)
{
    panel_gpio_init();

    panel_spi_device_emulation(CS, SDI, SCL);

    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x13);
    panel_spi_cmd_wr(0xEF);
    panel_spi_data_wr(0x08);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x10);
    panel_spi_cmd_wr(0xC0);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x0C);
    panel_spi_cmd_wr(0xC2);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x02);

    panel_spi_cmd_wr(0xC3);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xCC);
    panel_spi_data_wr(0x30);

    panel_spi_cmd_wr(0xCD);
    panel_spi_data_wr(0x08);

    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x17);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x26);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x2A);
    panel_spi_data_wr(0x31);
    panel_spi_data_wr(0x1C);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x17);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x26);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x2A);
    panel_spi_data_wr(0x31);
    panel_spi_data_wr(0x1C);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x5C);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x60);
    panel_spi_cmd_wr(0xB2);
    panel_spi_data_wr(0x85);
    panel_spi_cmd_wr(0xB3);
    panel_spi_data_wr(0x80);
    panel_spi_cmd_wr(0xB5);
    panel_spi_data_wr(0x49);
    panel_spi_cmd_wr(0xB7);
    panel_spi_data_wr(0x87);
    panel_spi_cmd_wr(0xB8);
    panel_spi_data_wr(0x22);

    panel_spi_cmd_wr(0xC0);
    panel_spi_data_wr(0x09);
    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x88);
    panel_spi_cmd_wr(0xC2);
    panel_spi_data_wr(0x88);
    panel_spi_cmd_wr(0xD0);
    panel_spi_data_wr(0x88);
    panel_spi_cmd_wr(0xE0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0C);
    panel_spi_cmd_wr(0xE1);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x96);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x96);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x96);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x96);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE2);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xE3);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x33);
    panel_spi_cmd_wr(0xE4);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE5);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0xD4);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x8C);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0xD6);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x8C);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0xD0);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x8C);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0xD2);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x8C);
    panel_spi_cmd_wr(0xE6);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x33);
    panel_spi_cmd_wr(0xE7);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0xD5);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x8C);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0xD7);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x8C);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0xD1);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x8C);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0xD3);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x8C);
    panel_spi_cmd_wr(0xEB);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xED);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x67);
    panel_spi_data_wr(0xFC);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0xAB);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xBA);
    panel_spi_data_wr(0xF3);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0xCF);
    panel_spi_data_wr(0x76);
    panel_spi_data_wr(0x54);
    panel_spi_data_wr(0xFF);
    panel_spi_cmd_wr(0xEF);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x54);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x13);
    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0E);

    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0C);

    aic_delay_ms(20);

    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x40);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xE6);
    panel_spi_data_wr(0x16);
    panel_spi_data_wr(0x7C);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x36);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x35);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x3a);
    panel_spi_data_wr(0x66);

#ifdef BIST_MODE
    panel_spi_cmd_wr (0xFF);
    panel_spi_data_wr (0x77);
    panel_spi_data_wr (0x01);
    panel_spi_data_wr (0x00);
    panel_spi_data_wr (0x00);
    panel_spi_data_wr (0x12);
    panel_spi_cmd_wr (0xD1);
    panel_spi_data_wr (0x81);
    panel_spi_data_wr (0x08);
    panel_spi_data_wr (0x03);
    panel_spi_data_wr (0x20);
    panel_spi_data_wr (0x08);
    panel_spi_data_wr (0x01);
    panel_spi_data_wr (0xA0);
    panel_spi_data_wr (0x01);
    panel_spi_data_wr (0xE0);
    panel_spi_data_wr (0xA0);
    panel_spi_data_wr (0x01);
    panel_spi_data_wr (0xE0);
    panel_spi_data_wr (0x03);
    panel_spi_data_wr (0x20);
    panel_spi_cmd_wr (0xD2);
    /* 0x08: colorbar, 0X02: red  etc... */
    panel_spi_data_wr (0x08);
#endif

    panel_spi_cmd_wr(0x11);
    aic_delay_ms(120);
    panel_spi_cmd_wr(0x29);
    aic_delay_ms(120);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs st7701s_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing st7701s_timing = {
    .pixelclock = 42000000,
    .hactive = 400,
    .hfront_porch = 100,
    .hback_porch = 100,
    .hsync_len = 10,
    .vactive = 960,
    .vfront_porch = 80,
    .vback_porch = 100,
    .vsync_len = 20,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_0,
    .data_order = RGB,
    .data_mirror = 0,
};

struct aic_panel rgb_st7701s = {
    .name = "panel-st7701s",
    .timings = &st7701s_timing,
    .funcs = &st7701s_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

