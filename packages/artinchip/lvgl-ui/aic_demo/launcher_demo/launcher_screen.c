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
#include "launcher_screen.h"
#include "launcher_ui.h"

static void launcher_tapview_event(lv_event_t * e)
{
    lv_obj_t * tapview = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        int tab_num;

        tab_num = lv_tabview_get_tab_act(tapview);
        (void)tab_num;
        //printf("tab_num:%d\n", tab_num);
    }
}

static void camera_event_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        // load camera screen
        lv_scr_load(scr_camera);
    }
}

lv_obj_t *launcher_screen_init(void)
{
    scr_launcher = lv_obj_create(NULL);
    lv_obj_clear_flag(scr_launcher, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *img_bg = lv_img_create(scr_launcher);
    lv_img_set_src(img_bg, LVGL_PATH(bg.jpg));
    lv_obj_set_pos(img_bg, 0, 0);

    lv_obj_t *tab_sub = lv_tabview_create(scr_launcher, LV_DIR_TOP, 0);
    lv_obj_set_pos(tab_sub, 0, 0);
    lv_obj_set_style_bg_opa(tab_sub, LV_OPA_0, 0);

    lv_obj_t *sub_tab0 = lv_tabview_add_tab(tab_sub, "lancher page 0");
    lv_obj_t *sub_tab1 = lv_tabview_add_tab(tab_sub, "lancher page 1");

    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_translate_x(&style_pr, 2);
    lv_style_set_translate_y(&style_pr, 2);

    lv_obj_t *sub_image00 = lv_imgbtn_create(sub_tab0);
    lv_imgbtn_set_src(sub_image00, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(Camera.png), NULL);
    lv_imgbtn_set_src(sub_image00, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(Camera.png), NULL);
    lv_obj_set_pos(sub_image00, 30, 25);
    lv_obj_set_size(sub_image00, 64, 64);
    lv_obj_add_style(sub_image00, &style_pr, LV_STATE_PRESSED);

    lv_obj_t *image00_name = lv_label_create(sub_tab0);
    lv_obj_set_width(image00_name, LV_SIZE_CONTENT);
    lv_obj_set_height(image00_name, LV_SIZE_CONTENT);
    lv_obj_set_pos(image00_name, 30 + 2, 25 + 64 + 2);
    lv_label_set_text(image00_name, "Camera");
    lv_obj_set_style_text_color(image00_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(image00_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *sub_image01 = lv_imgbtn_create(sub_tab0);
    lv_imgbtn_set_src(sub_image01, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(Activity_Monitor.png), NULL);
    lv_imgbtn_set_src(sub_image01, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(Activity_Monitor.png), NULL);
    lv_obj_set_pos(sub_image01, 30 + 64 + 44, 25);
    lv_obj_set_size(sub_image01, 64, 64);
    lv_obj_add_style(sub_image01, &style_pr, LV_STATE_PRESSED);

    lv_obj_t *image01_name = lv_label_create(sub_tab0);
    lv_obj_set_width(image01_name, LV_SIZE_CONTENT);
    lv_obj_set_height(image01_name, LV_SIZE_CONTENT);
    lv_obj_set_pos(image01_name, 30 + 3 + 64 + 44, 25 + 64 + 2);
    lv_label_set_text(image01_name, "Monitor");
    lv_obj_set_style_text_color(image01_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(image01_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *sub_image02 = lv_imgbtn_create(sub_tab0);
    lv_imgbtn_set_src(sub_image02, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(Downloads.png), NULL);
    lv_imgbtn_set_src(sub_image02, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(Downloads.png), NULL);
    lv_obj_set_pos(sub_image02, 30 + 64 + 44 + 64 + 44, 25);
    lv_obj_set_size(sub_image02, 64, 64);
    lv_obj_add_style(sub_image02, &style_pr, LV_STATE_PRESSED);

    lv_obj_t *image02_name = lv_label_create(sub_tab0);
    lv_obj_set_width(image02_name, LV_SIZE_CONTENT);
    lv_obj_set_height(image02_name, LV_SIZE_CONTENT);
    lv_obj_set_pos(image02_name, 30 + 64 + 44 + 64 + 44 - 8, 25 + 64 + 2);
    lv_label_set_text(image02_name, "Downloads");
    lv_obj_set_style_text_color(image02_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(image02_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *sub_image03 = lv_imgbtn_create(sub_tab0);
    lv_imgbtn_set_src(sub_image03, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(iChat.png), NULL);
    lv_imgbtn_set_src(sub_image03, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(iChat.png), NULL);
    lv_obj_set_pos(sub_image03, 30 + 64 + 44 + 64 + 44 + 64 + 44, 25);
    lv_obj_set_size(sub_image03, 64, 64);
    lv_obj_add_style(sub_image03, &style_pr, LV_STATE_PRESSED);

    lv_obj_t *image03_name = lv_label_create(sub_tab0);
    lv_obj_set_width(image03_name, LV_SIZE_CONTENT);
    lv_obj_set_height(image03_name, LV_SIZE_CONTENT);
    lv_obj_set_pos(image03_name, 30 + 15 + 64 + 44 + 64 + 44 + 64 + 44, 25 + 64 + 2);
    lv_label_set_text(image03_name, "iChat");
    lv_obj_set_style_text_color(image03_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(image03_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *sub_image04 = lv_imgbtn_create(sub_tab0);
    lv_imgbtn_set_src(sub_image04, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(Calculator.png), NULL);
    lv_imgbtn_set_src(sub_image04, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(Calculator.png), NULL);
    lv_obj_set_pos(sub_image04, 30, 40 + 100);
    lv_obj_set_size(sub_image04, 64, 64);
    lv_obj_add_style(sub_image04, &style_pr, LV_STATE_PRESSED);

    lv_obj_t *image04_name = lv_label_create(sub_tab0);
    lv_obj_set_width(image04_name, LV_SIZE_CONTENT);
    lv_obj_set_height(image04_name, LV_SIZE_CONTENT);
    lv_obj_set_pos(image04_name, 30 - 5, 40 + 100 + 64 + 2);
    lv_label_set_text(image04_name, "Calculator");
    lv_obj_set_style_text_color(image04_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(image04_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *sub_image05 = lv_imgbtn_create(sub_tab0);
    lv_imgbtn_set_src(sub_image05, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(Mail.png), NULL);
    lv_imgbtn_set_src(sub_image05, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(Mail.png), NULL);
    lv_obj_set_pos(sub_image05, 30 + 64 + 44, 40 + 100);
    lv_obj_set_size(sub_image05, 64, 64);
    lv_obj_add_style(sub_image05, &style_pr, LV_STATE_PRESSED);

    lv_obj_t *image05_name = lv_label_create(sub_tab0);
    lv_obj_set_width(image05_name, LV_SIZE_CONTENT);
    lv_obj_set_height(image05_name, LV_SIZE_CONTENT);
    lv_obj_set_pos(image05_name, 30 + 13 + 64 + 44, 40 + 100 + 64 + 2);
    lv_label_set_text(image05_name, "Mail");
    lv_obj_set_style_text_color(image05_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(image05_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *sub_image06 = lv_imgbtn_create(sub_tab0);
    lv_imgbtn_set_src(sub_image06, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(Maps.png), NULL);
    lv_imgbtn_set_src(sub_image06, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(Maps.png), NULL);
    lv_obj_set_pos(sub_image06, 30 + 64 + 44 + 64 + 44, 40 + 100);
    lv_obj_set_size(sub_image06, 64, 64);
    lv_obj_add_style(sub_image06, &style_pr, LV_STATE_PRESSED);

    lv_obj_t *image06_name = lv_label_create(sub_tab0);
    lv_obj_set_width(image06_name, LV_SIZE_CONTENT);
    lv_obj_set_height(image06_name, LV_SIZE_CONTENT);
    lv_obj_set_pos(image06_name, 30 + 64 + 44 + 64 + 44 + 10, 40 + 100 + 64 + 2);
    lv_label_set_text(image06_name, "Maps");
    lv_obj_set_style_text_color(image06_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(image06_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *sub_image07 = lv_imgbtn_create(sub_tab0);
    lv_imgbtn_set_src(sub_image07, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(Tools.png), NULL);
    lv_imgbtn_set_src(sub_image07, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(Tools.png), NULL);
    lv_obj_set_pos(sub_image07, 30 + 64 + 44 + 64 + 44 + 64 + 44, 40 + 100);
    lv_obj_set_size(sub_image07, 64, 64);
    lv_obj_add_style(sub_image07, &style_pr, LV_STATE_PRESSED);

    lv_obj_t *image07_name = lv_label_create(sub_tab0);
    lv_obj_set_width(image07_name, LV_SIZE_CONTENT);
    lv_obj_set_height(image07_name, LV_SIZE_CONTENT);
    lv_obj_set_pos(image07_name, 30 + 10 + 64 + 44 + 64 + 44 + 64 + 44, 40 + 100 + 64 + 2);
    lv_label_set_text(image07_name, "Tools");
    lv_obj_set_style_text_color(image07_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(image07_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *sub_image10 = lv_imgbtn_create(sub_tab1);
    lv_imgbtn_set_src(sub_image10, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(Weather.png), NULL);
    lv_imgbtn_set_src(sub_image10, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(Weather.png), NULL);
    lv_obj_set_pos(sub_image10, 30, 25);
    lv_obj_set_size(sub_image10, 64, 64);
    lv_obj_add_style(sub_image10, &style_pr, LV_STATE_PRESSED);

    lv_obj_t *image10_name = lv_label_create(sub_tab1);
    lv_obj_set_width(image10_name, LV_SIZE_CONTENT);
    lv_obj_set_height(image10_name, LV_SIZE_CONTENT);
    lv_obj_set_pos(image10_name, 30, 25 + 64 + 5);
    lv_label_set_text(image10_name, "Weather");
    lv_obj_set_style_text_color(image10_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(image10_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(tab_sub, launcher_tapview_event, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(sub_image00, camera_event_cb, LV_EVENT_ALL, NULL);

    return scr_launcher;
}
