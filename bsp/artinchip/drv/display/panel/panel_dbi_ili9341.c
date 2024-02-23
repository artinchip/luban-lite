/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dbi.h"

/* Init sequence, each line consists of command, count of data, data... */
static const u8 ili9341_commands[] = {
    0xcf,   3,  0x00,   0xc1,   0x30,
    0xed,   4,  0x64,   0x03,   0x12,   0x81,
    0xe8,   3,  0x85,   0x01,   0x78,
    0xcb,   5,  0x39,   0x2c,   0x00,   0x34,   0x02,
    0xf7,   1,  0x20,
    0xea,   2,  0x00,   0x00,
    0xc0,   1,  0x21,
    0xc1,   1,  0x12,
    0xc5,   2,  0x5a,   0x5d,
    0xc7,   1,  0x82,
    0x36,   1,  0x08,
    0x3a,   1,  0x66,
    0xb1,   2,  0x00,   0x16,
    0xb6,   2,  0x0a,   0xa2,
    0xf2,   1,  0x00,
    0xf2,   1,  0x00,
    0xf6,   2,  0x01,   0x30,
    0x26,   1,  0x01,
    0xe0,   15, 0x0f,   0x09,   0x1e,   0x07,   0x0b,   0x01,   0x45,   0x6d,
                0x37,   0x08,   0x13,   0x01,   0x06,   0x06,   0x00,
    0xe1,   15, 0x00,   0x01,   0x18,   0x00,   0x0d,   0x00,   0x2a,   0x44,
                0x44,   0x04,   0x11,   0x0c,   0x30,   0x34,   0x0f,
    0x2a,   2,  0x00,   0x00,
    0x2a,   4,  0x00,   0x00,   0x00,   0xef,
    0x2b,   4,  0x00,   0x00,   0x01,   0x3f,
    0x11,   0,
    0x00,   1,  120,
    0x29,   0,
};

#define RESET_PIN "PD.23"

static struct gpio_desc reset;

static int panel_prepare(void)
{
    panel_get_gpio(&reset, RESET_PIN);

    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(5);
    panel_gpio_set_value(&reset, 0);
    aic_delay_ms(5);
    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(5);

    return 0;
}

static struct aic_panel_funcs ili9341_funcs = {
    .prepare = panel_prepare,
    .enable = panel_dbi_default_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing ili9341_timing = {
    .pixelclock   = 3000000,

    .hactive      = 240,
    .hback_porch  = 2,
    .hfront_porch = 3,
    .hsync_len    = 1,

    .vactive      = 320,
    .vback_porch  = 3,
    .vfront_porch = 2,
    .vsync_len    = 1,
};

static struct panel_dbi dbi = {
    .type = SPI,
    .format = SPI_4LINE_RGB888,
    .commands = {
        .buf = ili9341_commands,
        .len = ARRAY_SIZE(ili9341_commands),
    }
};

struct aic_panel dbi_ili9341 = {
    .name = "panel-ili9341",
    .timings = &ili9341_timing,
    .funcs = &ili9341_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};

