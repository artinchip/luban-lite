/*
 * Copyright (C) 2022-2023 ArtinChip Technology Co., Ltd.
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include "lvgl.h"
#include "aic_ui.h"
#include "aic_osal.h"

void aic_ui_init()
{
#ifdef AIC_LVGL_BASE_DEMO
#include "base_ui.h"
    base_ui_init();
#endif

#ifdef AIC_LVGL_METER_DEMO
#include "meter_ui.h"
    meter_ui_init();
#endif

#ifdef AIC_LVGL_LAUNCHER_DEMO
    extern void launcher_ui_init();
    launcher_ui_init();
#endif

#ifdef AIC_LVGL_MUSIC_DEMO
    extern void lv_demo_music(void);
    lv_demo_music();
#endif

    return;
}
