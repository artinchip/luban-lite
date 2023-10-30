/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_dbi.h"

/* Init sequence, each line consists of command, count of data, data... */
static const u8 ili9486l_commands[] = {
    0xf2,   9,  0x18,   0xa3,   0x12,   0x02,   0xb2,   0x12,   0xff,   0x10,
                0x00,
    0xf8,   2,  0x21,   0x04,
    0xf9,   2,  0x00,   0x08,
    0xf1,   6,  0x36,   0x04,   0x00,   0x3c,   0x0f,   0x8f,
    0xc0,   2,  0x19,   0x1a,
    0xc1,   1,  0x44,
    0xc2,   1,  0x11,
    0xc5,   2,  0x00,   0x06,
    0xb1,   2,  0x80,   0x11,
    0xb4,   1,  0x02,
    0xb0,   1,  0x00,
    0xb6,   3,  0x00,   0x22,   0x3b,
    0xb7,   1,  0x07,
    0xe0,   15, 0x1f,   0x25,   0x22,   0x0a,   0x05,   0x07,   0x50,   0xa8,
                0x40,   0x0d,   0x19,   0x07,   0x00,   0x00,   0x00,
    0xe1,   15, 0x1f,   0x3f,   0x3f,   0x08,   0x06,   0x02,   0x3f,   0x75,
                0x2e,   0x08,   0x0b,   0x05,   0x1d,   0x1a,   0x00,
    0x35,   1,  0x01,
    0x44,   2,  0x00,   0x10,
    0x33,   6,  0x00,   0x00,   0x01,   0xe0,   0x00,   0x00,
    0x37,   2,  0x00,   0x00,
    0x2a,   4,  0x00,   0x00,   0x01,   0x3f,
    0x2b,   4,  0x00,   0x00,   0x01,   0xdf,
    0x36,   1,  0x08,
    0x3a,   1,  0x66,
    0x11,   0,
    0x00,   1,  120,
    0x29,   0,
};

static struct aic_panel_funcs ili9486l_funcs = {
    .prepare = panel_default_prepare,
    .enable = panel_dbi_default_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing ili9486l_timing = {
    .pixelclock   = 9216000,

    .hactive      = 320,
    .hback_porch  = 3,
    .hfront_porch = 2,
    .hsync_len    = 1,

    .vactive      = 480,
    .vback_porch  = 1,
    .vfront_porch = 3,
    .vsync_len    = 2,
};

static struct panel_dbi dbi = {
    .type = I8080,
    .format = I8080_RGB666_8BIT,
    .commands = {
        .buf = ili9486l_commands,
        .len = ARRAY_SIZE(ili9486l_commands),
    }
};

struct aic_panel dbi_ili9486l = {
    .name = "panel-ili9486l",
    .timings = &ili9486l_timing,
    .funcs = &ili9486l_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};

