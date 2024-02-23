/*
 * Copyright (C) 2022-2023 ArtinChip Technology Co., Ltd.
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lvgl.h"
#include "launcher_ui.h"
#include "launcher_screen.h"
#include "camera_screen.h"
#include "aic_ui.h"

lv_obj_t *scr_launcher;
lv_obj_t *scr_camera;

void launcher_ui_init()
{
    scr_launcher = launcher_screen_init();
    scr_camera = camera_screen_init();

    lv_scr_load(scr_launcher);
}
