/*
 * Copyright (C) 2022-2023 ArtinChip Technology Co., Ltd.
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef LAUNCHER_UI_H
#define LAUNCHER_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "aic_ui.h"

// for screen size 480 x 272
void launcher_ui_init();

extern lv_obj_t *scr_launcher;
extern lv_obj_t *scr_camera;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //LAUNCHER_UI_H
