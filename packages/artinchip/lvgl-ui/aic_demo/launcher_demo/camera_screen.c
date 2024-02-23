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
#include "aic_ui.h"
#include "launcher_ui.h"
#include "camera_screen.h"

FAKE_IMAGE_DECLARE(bg_dark);

static void back_event_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        // back to launcher screen
        lv_scr_load(scr_launcher);
    }
}

lv_obj_t *camera_screen_init(void)
{
    scr_camera = lv_obj_create(NULL);
    lv_obj_clear_flag(scr_camera, LV_OBJ_FLAG_SCROLLABLE);

    FAKE_IMAGE_INIT(bg_dark, 480, 272, 0, 0x00000000);

    lv_obj_t *img_bg = lv_img_create(scr_camera);
    lv_img_set_src(img_bg, FAKE_IMAGE_NAME(bg_dark));
    lv_obj_set_pos(img_bg, 0, 0);

    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_translate_x(&style_pr, 2);
    lv_style_set_translate_y(&style_pr, 2);

    lv_obj_t *back_btn = lv_imgbtn_create(scr_camera);
    lv_imgbtn_set_src(back_btn, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(Back.png), NULL);
    lv_imgbtn_set_src(back_btn, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(Back.png), NULL);
    lv_obj_set_pos(back_btn, 10, 10);
    lv_obj_set_size(back_btn, 32, 32);
    lv_obj_add_style(back_btn, &style_pr, LV_STATE_PRESSED);

    lv_obj_add_event_cb(back_btn, back_event_cb, LV_EVENT_ALL, NULL);

    return scr_camera;
}
