/*
 * Copyright (C) 2022-2023 ArtinChip Technology Co., Ltd.
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "lvgl.h"
#include "gif_ui.h"

static void gif_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        int *end_flag = (int *)lv_event_get_user_data(e);
        *end_flag = 1;
    }
}

void gif_ui_init(int *end_flag)
{
    lv_obj_t * img;
    img = lv_gif_create(lv_scr_act());

    lv_gif_set_src(img, LVGL_PATH(gif/cherub.gif));

    printf("path:%s\n", LVGL_PATH(gif/cherub.gif));
    // must call after lv_gif_set_src
    lv_gif_set_loop_count(img, 1);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    lv_obj_add_event_cb(img, gif_event, LV_EVENT_ALL, end_flag);
    printf("gif_ini_ok\n");
}
