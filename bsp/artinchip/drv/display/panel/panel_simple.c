/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"

static struct aic_panel_funcs simple_funcs = {
    .prepare = panel_default_prepare,
    .enable = panel_default_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing panel_timing = {
    .pixelclock   = PANEL_PIXELCLOCK * 1000000,

    .hactive      = PANEL_HACTIVE,
    .hback_porch  = PANEL_HBP,
    .hfront_porch = PANEL_HFP,
    .hsync_len    = PANEL_HSW,

    .vactive      = PANEL_VACTIVE,
    .vback_porch  = PANEL_VBP,
    .vfront_porch = PANEL_VFP,
    .vsync_len    = PANEL_VSW,
};

#ifdef AIC_DISP_RGB
static struct panel_rgb rgb = {
    .mode = AIC_RGB_MODE,
    .format = AIC_RGB_FORMAT,
    .clock_phase = AIC_RGB_CLK_CTL,
    .data_order = AIC_RGB_DATA_ORDER,
    .data_mirror = AIC_RGB_DATA_MIRROR,
};

struct aic_panel aic_panel_rgb = {
    .name = "panel-rgb",
    .timings = &panel_timing,
    .funcs = &simple_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};
#endif

#ifdef AIC_DISP_LVDS
static struct panel_lvds lvds = {
    .mode = AIC_LVDS_MODE,
    .link_mode = AIC_LVDS_LINK_MODE,
};

struct aic_panel aic_panel_lvds = {
    .name = "panel-lvds",
    .timings = &panel_timing,
    .funcs = &simple_funcs,
    .lvds = &lvds,
    .connector_type = AIC_LVDS_COM,
};
#endif

