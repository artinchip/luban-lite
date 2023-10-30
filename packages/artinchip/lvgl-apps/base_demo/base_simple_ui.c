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
#include "base_ui.h"
#include "aic_ui.h"
#include "lv_port_disp.h"
#include "mpp_fb.h"
#ifdef LPKG_USING_CPU_USAGE
#include "cpu_usage.h"
#endif

LV_FONT_DECLARE(ui_font_Big);
LV_FONT_DECLARE(ui_font_Title);
LV_FONT_DECLARE(ui_font_H1);

LV_IMG_DECLARE(circle_white);
LV_IMG_DECLARE(circle_gray);

static int rot_angle = 255;
static int rot_direct = 0;
static lv_obj_t *main_title = NULL;
static lv_obj_t *bg_fps = NULL;
static lv_obj_t *bg_logo = NULL;
static lv_obj_t *circle_0;
static lv_obj_t *circle_1;
static lv_obj_t *ui_speed = NULL;
static lv_obj_t *fps_info = NULL;
static lv_obj_t *cpu_info = NULL;
static lv_obj_t *mem_info = NULL;
static lv_obj_t *fps_title = NULL;
static lv_obj_t *cpu_title = NULL;
static lv_obj_t *mem_title = NULL;
static lv_obj_t *img_bg = NULL;
static lv_obj_t *tab_sub = NULL;

static int angle2speed(int angle)
{
    float speed;
    float ratio;
    float range;

    range = (360 - 255 + 105);

    if (angle >= 255 && angle < 360)
        speed = (float)(angle - 255);
    else
        speed = (float)(angle + 360 - 255);

    if (speed > 0) {
        ratio = 160.0 / range;
        speed = speed * ratio;
    }

    return (int)speed;
}

static void anim_set_angle(void *var, int32_t v)
{
    if (rot_direct == 0)
        rot_angle += 1;
    else
        rot_angle -= 1;

    if (rot_angle >= 360)
        rot_angle = 0;

    if (rot_angle < 0)
        rot_angle = 359;

    if (rot_direct == 0 && rot_angle == 106) {
        rot_angle = 104;
        rot_direct = 1;
    }

    if (rot_direct == 1 && rot_angle == 254) {
        rot_angle = 253;
        rot_direct = 0;
    }

    lv_img_set_angle(var, rot_angle * 10);
}

static void point_aimation(lv_obj_t *obj)
{
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, obj);
    lv_anim_set_values(&anim, 0, 3600);
    lv_anim_set_time(&anim, 1000);
    lv_anim_set_playback_delay(&anim, 0);
    lv_anim_set_playback_time(&anim, 0);
    lv_anim_set_repeat_delay(&anim, 0);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&anim, lv_anim_path_linear);
    lv_anim_set_exec_cb(&anim, anim_set_angle);
    lv_anim_start(&anim);
}

static void point_callback(lv_timer_t *tmr)
{
    char speed_str[128];

    (void)tmr;

    int speed = angle2speed(rot_angle);
    ui_snprintf(speed_str, "%d", speed);
    lv_label_set_text(ui_speed, speed_str);

    return;
}

static void timer_callback(lv_timer_t *tmr)
{
    char data_str[128];
    float value;

    (void)tmr;

    /* frame rate */
    ui_snprintf(data_str, "%2d fps", fbdev_draw_fps());
    lv_label_set_text(fps_info, data_str);
    ui_snprintf(data_str, "%2d FPS", fbdev_draw_fps());
    lv_label_set_text(bg_fps, data_str);

    /* cpu usage */
#ifdef LPKG_USING_CPU_USAGE
    value = cpu_load_average();
#else
    value = 0;
#endif

#ifdef AIC_PRINT_FLOAT_CUSTOM
    int cpu_i;
    int cpu_frac;

    cpu_i = (int)value;
    cpu_frac = (value - cpu_i) * 100;
    ui_snprintf(data_str, "%d.%02d\n", cpu_i, cpu_frac);
#else
    ui_snprintf(data_str, "%.2f\n", value);
#endif
    lv_label_set_text(cpu_info, data_str);

    /* mem usage */
#ifdef RT_USING_MEMHEAP
    extern long get_mem_used(void);
    value = ((float)(get_mem_used())) / (1024.0 * 1024.0);
#else
    value = 0;
#endif

#ifdef AIC_PRINT_FLOAT_CUSTOM
    int mem_i;
    int mem_frac;

    mem_i = (int)value;
    mem_frac = (value - mem_i) * 100;
    ui_snprintf(data_str, "%d.%02dMB\n", mem_i, mem_frac);
#else
    ui_snprintf(data_str, "%.2fMB\n", value);
#endif
    lv_label_set_text(mem_info, data_str);
    return;
}

static void main_tapview_event(lv_event_t * e)
{
    lv_obj_t * tapview = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        int tab_num = lv_tabview_get_tab_act(tapview);

        if (tab_num == 0) {
            lv_label_set_text(main_title, "Meter");
            lv_img_set_src(circle_0, &circle_white);
            lv_obj_align(circle_0, LV_ALIGN_BOTTOM_MID, -16, -28);
            lv_img_set_src(circle_1, &circle_gray);
            lv_obj_align(circle_1, LV_ALIGN_BOTTOM_MID, 16, -30);
        } else if (tab_num == 1) {
            lv_label_set_text(main_title, "Cookbook (1/2)");
            lv_img_set_src(circle_0, &circle_gray);
            lv_obj_align(circle_0, LV_ALIGN_BOTTOM_MID, -16, -30);
            lv_img_set_src(circle_1, &circle_white);
            lv_obj_align(circle_1, LV_ALIGN_BOTTOM_MID, 16, -28);
        }
    }
}

static void sub_tapview_event(lv_event_t * e)
{
    lv_obj_t * tapview = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        char data_str[128];
        int tab_num;

        tab_num = lv_tabview_get_tab_act(tapview);
        ui_snprintf(data_str, "Cookbook (%d/2)", tab_num + 1);
        lv_label_set_text(main_title, data_str);

        printf("sub_num:%d\n", tab_num);
    }
}

void base_ui_init()
{
    img_bg = lv_img_create(lv_scr_act());
    lv_img_set_src(img_bg, LVGL_PATH(global_bg.png));
    lv_obj_set_pos(img_bg, 0, 0);

    main_title = lv_label_create(img_bg);
    lv_obj_set_width(main_title, LV_SIZE_CONTENT);
    lv_obj_set_height(main_title, LV_SIZE_CONTENT);
    lv_obj_align(main_title, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_pos(main_title, 0, 10);
    lv_label_set_text(main_title, "Meter");
    lv_obj_set_style_text_color(main_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(main_title, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(main_title, &ui_font_Title, LV_PART_MAIN | LV_STATE_DEFAULT);

    bg_fps = lv_label_create(img_bg);
    lv_obj_set_width(bg_fps, LV_SIZE_CONTENT);
    lv_obj_set_height(bg_fps, LV_SIZE_CONTENT);
    lv_obj_align(bg_fps, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_pos(bg_fps, -30, 10);
    lv_label_set_text(bg_fps, "");
    lv_obj_set_style_text_color(bg_fps, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bg_fps, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bg_fps, &ui_font_Title, LV_PART_MAIN | LV_STATE_DEFAULT);

    bg_logo = lv_label_create(img_bg);
    lv_obj_set_width(bg_logo, LV_SIZE_CONTENT);
    lv_obj_set_height(bg_logo, LV_SIZE_CONTENT);
    lv_obj_align(bg_logo, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_pos(bg_logo, -30, -30);
    lv_label_set_text(bg_logo, "ArtInChip");
    lv_obj_set_style_text_color(bg_logo, lv_color_hex(0x00FFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bg_logo, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bg_logo, &ui_font_Big, LV_PART_MAIN | LV_STATE_DEFAULT);

    circle_0 = lv_img_create(img_bg);
    lv_img_set_src(circle_0, &circle_white);
    lv_obj_align(circle_0, LV_ALIGN_BOTTOM_MID, -16, -28);

    circle_1 = lv_img_create(img_bg);
    lv_img_set_src(circle_1, &circle_gray);
    lv_obj_align(circle_1, LV_ALIGN_BOTTOM_MID, 16, -30);

    lv_obj_t *main_tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 0);

    lv_obj_set_size(main_tabview, 1024, 600);
    lv_obj_set_pos(main_tabview, 0, 0);
    lv_obj_set_style_bg_opa(main_tabview, LV_OPA_0, 0);

    lv_obj_t *main_tab0 = lv_tabview_add_tab(main_tabview, "main page 0");
    lv_obj_t *main_tab1 = lv_tabview_add_tab(main_tabview, "main page 1");

    lv_obj_set_style_bg_opa(main_tab0, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(main_tab1, LV_OPA_0, 0);
    lv_obj_set_size(main_tab0, 1024, 600);
    lv_obj_set_size(main_tab1, 1024, 600);

    lv_obj_set_pos(main_tab0, 0, 0);
    lv_obj_set_pos(main_tab1, 0, 0);

    lv_obj_t *img_ck = lv_img_create(main_tab0);
    lv_img_set_src(img_ck, LVGL_PATH(meter_clk.png));
    lv_obj_set_pos(img_ck, 50, 152);

    lv_obj_t *img_point = lv_img_create(main_tab0);
    lv_img_set_src(img_point, LVGL_PATH(meter_point.png));
    lv_obj_set_pos(img_point, 192, 200);
    lv_img_set_pivot(img_point, 12, 108);
    lv_img_set_angle(img_point, rot_angle * 10);

    ui_speed = lv_label_create(main_tab0);
    lv_obj_set_width(ui_speed, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_speed, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_speed, 192);
    lv_obj_set_y(ui_speed, 380);
    lv_label_set_text(ui_speed, "0");
    lv_obj_set_style_text_color(ui_speed, lv_color_hex(0xF9E09D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_speed, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_speed, &ui_font_Big, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *img_info = lv_img_create(main_tab0);
    lv_img_set_src(img_info, LVGL_PATH(meter_info.png));
    lv_obj_set_pos(img_info, 340, 125);

    fps_title = lv_label_create(main_tab0);
    lv_obj_set_width(fps_title, LV_SIZE_CONTENT);
    lv_obj_set_height(fps_title, LV_SIZE_CONTENT);
    lv_obj_set_pos(fps_title, 425, 286);
    lv_label_set_text(fps_title, "Frame rate");
    lv_obj_set_style_text_color(fps_title, lv_color_hex(0x00A0EF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(fps_title, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(fps_title, &ui_font_Title, LV_PART_MAIN | LV_STATE_DEFAULT);

    cpu_title = lv_label_create(main_tab0);
    lv_obj_set_width(cpu_title, LV_SIZE_CONTENT);
    lv_obj_set_height(cpu_title, LV_SIZE_CONTENT);
    lv_obj_set_pos(cpu_title, 615, 286);
    lv_label_set_text(cpu_title, "CPU usage");
    lv_obj_set_style_text_color(cpu_title, lv_color_hex(0x00A0EF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cpu_title, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cpu_title, &ui_font_Title, LV_PART_MAIN | LV_STATE_DEFAULT);

    mem_title = lv_label_create(main_tab0);
    lv_obj_set_width(mem_title, LV_SIZE_CONTENT);
    lv_obj_set_height(mem_title, LV_SIZE_CONTENT);
    lv_obj_set_pos(mem_title, 795, 286);
    lv_label_set_text(mem_title, "Mem usage");
    lv_obj_set_style_text_color(mem_title, lv_color_hex(0x00A0EF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(mem_title, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(mem_title, &ui_font_Title, LV_PART_MAIN | LV_STATE_DEFAULT);

    fps_info = lv_label_create(main_tab0);
    lv_obj_set_width(fps_info, LV_SIZE_CONTENT);
    lv_obj_set_height(fps_info, LV_SIZE_CONTENT);
    lv_obj_set_pos(fps_info, 464, 345);
    lv_label_set_text(fps_info, "0");
    lv_obj_set_style_text_color(fps_info, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(fps_info, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(fps_info, &ui_font_Big, LV_PART_MAIN | LV_STATE_DEFAULT);

    cpu_info = lv_label_create(main_tab0);
    lv_obj_set_width(cpu_info, LV_SIZE_CONTENT);
    lv_obj_set_height(cpu_info, LV_SIZE_CONTENT);
    lv_obj_set_pos(cpu_info, 644, 345);
    lv_label_set_text(cpu_info, "0");
    lv_obj_set_style_text_color(cpu_info, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(cpu_info, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cpu_info, &ui_font_Big, LV_PART_MAIN | LV_STATE_DEFAULT);

    mem_info = lv_label_create(main_tab0);
    lv_obj_set_width(mem_info, LV_SIZE_CONTENT);
    lv_obj_set_height(mem_info, LV_SIZE_CONTENT);
    lv_obj_set_pos(mem_info, 824, 345);
    lv_label_set_text(mem_info, "0");
    lv_obj_set_style_text_color(mem_info, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(mem_info, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(mem_info, &ui_font_Big, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cook_top_img = lv_img_create(main_tab1);
    lv_img_set_src(cook_top_img, LVGL_PATH(cook_top.png));
    lv_obj_set_pos(cook_top_img, 280, 80);
    lv_obj_t *cook_buttom_img = lv_img_create(main_tab1);
    lv_img_set_src(cook_buttom_img, LVGL_PATH(cook_buttom.png));
    lv_obj_set_pos(cook_buttom_img, 0, 430);

    tab_sub = lv_tabview_create(main_tab1, LV_DIR_TOP, 0);
    lv_obj_set_pos(tab_sub, 0, 00);
    lv_obj_set_style_bg_opa(tab_sub, LV_OPA_0, 0);

    lv_obj_t *sub_tab0 = lv_tabview_add_tab(tab_sub, "sub page 0");
    lv_obj_t *sub_tab1 = lv_tabview_add_tab(tab_sub, "sub page 1");

    lv_obj_t *sub_image00 = lv_img_create(sub_tab0);
    lv_img_set_src(sub_image00, LVGL_PATH(cook_0.jpg));
    lv_obj_set_pos(sub_image00, 36, 100);

    lv_obj_t *sub_image01 = lv_img_create(sub_tab0);
    lv_img_set_src(sub_image01, LVGL_PATH(cook_1.jpg));
    lv_obj_set_pos(sub_image01, 366, 100);

    lv_obj_t *sub_image02 = lv_img_create(sub_tab0);
    lv_img_set_src(sub_image02, LVGL_PATH(cook_2.jpg));
    lv_obj_set_pos(sub_image02, 696, 100);

    lv_obj_t *sub_image03 = lv_img_create(sub_tab1);
    lv_img_set_src(sub_image03, LVGL_PATH(cook_3.jpg));
    lv_obj_set_pos(sub_image03, 36, 100);

    lv_obj_t *sub_image04 = lv_img_create(sub_tab1);
    lv_img_set_src(sub_image04, LVGL_PATH(cook_4.jpg));
    lv_obj_set_pos(sub_image04, 366, 100);

    lv_obj_t *sub_image05 = lv_img_create(sub_tab1);
    lv_img_set_src(sub_image05, LVGL_PATH(cook_5.jpg));
    lv_obj_set_pos(sub_image05, 696, 100);
    lv_obj_add_event_cb(tab_sub, sub_tapview_event, LV_EVENT_ALL, NULL);

    point_aimation(img_point);
    lv_timer_create(timer_callback, 1000, 0);
    lv_timer_create(point_callback, 100, 0);

    lv_obj_add_event_cb(main_tabview, main_tapview_event, LV_EVENT_ALL, NULL);
}
