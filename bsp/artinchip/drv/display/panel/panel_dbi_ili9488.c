/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_dbi.h"

/* Init sequence, each line consists of command, count of data, data... */
static const u8 ili9488_commands[] = {
    0xf7,   4,  0xa9,   0x51,   0x2c,   0x82,
    0xec,   4,  0x00,   0x02,   0x03,   0x7a,
    0xc0,   2,  0x13,   0x13,
    0xc1,   1,  0x41,
    0xc5,   3,  0x00,   0x28,   0x80,
    0xb1,   2,  0xb0,   0x11,
    0xb4,   1,  0x02,
    0xb6,   2,  0x02,   0x22,
    0xb7,   1,  0xc6,
    0xbe,   2,  0x00,   0x04,
    0xe9,   1,  0x00,
    0xb7,   1,  0x07,
    0xf4,   3,  0x00,   0x00,   0x0f,
    0xe0,   15, 0x00,   0x04,   0x0e,   0x08,   0x17,   0x0a,   0x40,   0x79,
                0x4d,   0x07,   0x0e,   0x0a,   0x1a,   0x1d,   0x0f,
    0xe1,   15, 0x00,   0x1b,   0x1f,   0x02,   0x10,   0x05,   0x32,   0x34,
                0x43,   0x02,   0x0a,   0x09,   0x33,   0x37,   0x0f,
    0xf4,   3,  0x00,   0x00,   0x0f,
    0x35,   1,  0x00,
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
    0x50,   0,
};

static struct aic_panel_funcs ili9488_funcs = {
    .prepare = panel_default_prepare,
    .enable = panel_dbi_default_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing ili9488_timing = {
    .pixelclock   = 6000000,

    .hactive      = 320,
    .hback_porch  = 2,
    .hfront_porch = 3,
    .hsync_len    = 1,

    .vactive      = 480,
    .vback_porch  = 3,
    .vfront_porch = 2,
    .vsync_len    = 1,
};

static struct panel_dbi dbi = {
    .type = I8080,
    .format = I8080_RGB666_16BIT_3CYCLE,
    .commands = {
        .buf = ili9488_commands,
        .len = ARRAY_SIZE(ili9488_commands),
    }
};

struct aic_panel dbi_ili9488 = {
    .name = "panel-ili9488",
    .timings = &ili9488_timing,
    .funcs = &ili9488_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};
