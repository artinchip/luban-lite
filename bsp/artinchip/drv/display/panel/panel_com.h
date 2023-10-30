/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PANEL_COM_H_
#define _PANEL_COM_H_

#include "../disp_com.h"

struct gpio_desc {
    unsigned int g;
    unsigned int p;
};

struct aic_panel_callbacks;

struct aic_panel *aic_find_panel(u32 connector_type);

extern struct aic_panel aic_panel_rgb;
extern struct aic_panel aic_panel_lvds;
extern struct aic_panel dsi_xm91080;
extern struct aic_panel dsi_st7797;
extern struct aic_panel dbi_ili9488;
extern struct aic_panel dbi_ili9341;
extern struct aic_panel dbi_st77903;
extern struct aic_panel dbi_ili9486l;
extern struct aic_panel srgb_hx8238;

void panel_di_enable(struct aic_panel *panel, u32 ms);
void panel_di_disable(struct aic_panel *panel, u32 ms);
void panel_de_timing_enable(struct aic_panel *panel, u32 ms);
void panel_de_timing_disable(struct aic_panel *panel, u32 ms);

int panel_default_prepare(void);
int panel_default_unprepare(void);
int panel_default_enable(struct aic_panel *panel);
int panel_default_disable(struct aic_panel *panel);
int panel_register_callback(struct aic_panel *panel,
                struct aic_panel_callbacks *pcallback);

void panel_backlight_enable(struct aic_panel *panel, u32 ms);

void panel_backlight_disable(struct aic_panel *panel, u32 ms);

void panel_send_command(u8 *para_cmd, u32 size, struct aic_panel *panel);

void panel_get_gpio(struct gpio_desc *desc, char *name);

void panel_gpio_set_value(struct gpio_desc *desc, u32 value);

#endif /* _PANEL_COM_H_ */

