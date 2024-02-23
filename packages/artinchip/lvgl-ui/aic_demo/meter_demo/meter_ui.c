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
#include "lv_port_disp.h"
#include "mpp_fb.h"
#include "meter_ui.h"

struct rot_id_info
{
    int start_id;
    int end_id;
};

static lv_obj_t *bg_fps = NULL;
#ifdef KERNEL_RTTHREAD
static lv_obj_t *bg_cpu = NULL;
#endif
static lv_obj_t *img_bg = NULL;
static lv_obj_t *img_circle = NULL;
static lv_obj_t *img_point = NULL;
static lv_obj_t *img_engine = NULL;
static lv_obj_t *img_esp = NULL;
static lv_obj_t *img_oil_less = NULL;
static lv_obj_t *img_battery = NULL;
static lv_obj_t *img_battery_num = NULL;
static lv_obj_t *img_battery_hl = NULL;
static lv_obj_t *img_oil_fe = NULL;
static lv_obj_t *img_oil_num = NULL;
static lv_obj_t *img_oil = NULL;
static lv_obj_t *img_water_temp = NULL;
static lv_obj_t *img_water_num = NULL;
static lv_obj_t *img_gear = NULL;

static lv_obj_t *img_trip = NULL;
static lv_obj_t *img_trip_0 = NULL;
static lv_obj_t *img_trip_1 = NULL;
static lv_obj_t *img_trip_2 = NULL;
static lv_obj_t *img_trip_3 = NULL;
static lv_obj_t *img_trip_km = NULL;
static lv_obj_t *img_speed_num = NULL;
static lv_obj_t *img_speed_km = NULL;

static lv_obj_t *img_time = NULL;
static lv_obj_t *img_time_0 = NULL;
static lv_obj_t *img_time_1 = NULL;
static lv_obj_t *img_time_2 = NULL;
static lv_obj_t *img_time_3 = NULL;
static lv_obj_t *img_time_dot = NULL;
static lv_timer_t *speed_timer = NULL;

LV_FONT_DECLARE(ui_font_Title);

struct rot_id_info rot_mode_list[] = {
    {1, 74},
    {74, 1},
    {1, 149},
    {149, 75},
    {75, 149},
    {149, 1},
};

static inline  void meter_ui_set_pos(lv_obj_t * obj, lv_coord_t x, lv_coord_t y)
{
    lv_obj_set_pos(obj, x + 112, y + 60);
}

static void point_callback(lv_timer_t *tmr)
{
    char data_str[128];
    (void)tmr;

    static bool first = true;
    static int id = 1;
    static int direct = 0;
    static int mode_id = 0;
    static int mode_num = sizeof(rot_mode_list) / sizeof(rot_mode_list[0]);
    static int start_id = 0;
    static int end_id = 0;

    if (first) {
        first = false;
        start_id = rot_mode_list[mode_id].start_id;
        end_id = rot_mode_list[mode_id].end_id;
    }

    direct = start_id < end_id ? 0: 1;

    if (id < 75) {
       lv_img_set_src(img_circle, LVGL_PATH(bg/small_blue.png));
       lv_obj_clear_flag(img_circle, LV_OBJ_FLAG_HIDDEN);
    } else {
       lv_obj_add_flag(img_circle, LV_OBJ_FLAG_HIDDEN);
    }

    if (id < 75) {
#ifdef LV_METER_SIMPLE_POINT
       // id to angle
        float rot_angle = ((float)(74 - id) * 2 * 10) * 0.84;

        if (rot_angle > 0) {
            rot_angle = 3600.0 - rot_angle;
        }
        ui_snprintf(data_str, "%spoint/point_small_blue.png", LVGL_DIR);
        lv_img_set_src(img_point, data_str);
        lv_img_set_angle(img_point, (int16_t)rot_angle);
        lv_img_set_pivot(img_point, 15, 210);
#else
        ui_snprintf(data_str, "%spoint/point_%05d.png", LVGL_DIR, id);
        lv_img_set_src(img_point, data_str);
        lv_img_set_angle(img_point, 0);
        lv_img_set_pivot(img_point, 210, 210);
#endif

#if defined(AIC_CHIP_D13X) && !defined(LV_METER_SIMPLE_POINT)
        lv_timer_set_period(speed_timer, 150);
#endif

    } else {
        // id to angle
        float rot_angle = ((float)(id - 75) * 2 * 10) * 0.84;

#ifdef LV_METER_SIMPLE_POINT
        ui_snprintf(data_str, "%spoint/point_small_red.png", LVGL_DIR);
        lv_img_set_src(img_point, data_str);
        lv_img_set_pivot(img_point, 15, 210);
#else
        ui_snprintf(data_str, "%spoint/point_%05d.png", LVGL_DIR, 75);
        lv_img_set_src(img_point, data_str);
        lv_img_set_pivot(img_point, 210, 210);
#endif
        lv_img_set_angle(img_point, (int16_t)rot_angle);
#if defined(AIC_CHIP_D13X) && !defined(LV_METER_SIMPLE_POINT)
        lv_timer_set_period(speed_timer, 300);
#endif
    }

    if (direct == 0) {
        id++;
    } else {
        id--;
    }

    if ((!direct && (id > end_id)) ||
        (direct && (id < end_id))) {
        id = end_id;
        mode_id++;
        mode_id %= mode_num;
        start_id = rot_mode_list[mode_id].start_id;
        end_id = rot_mode_list[mode_id].end_id;
    }

    return;
}

static void speed_callback(lv_timer_t *tmr)
{
    char data_str[128];
    static int speed_num = 0;

    (void)tmr;
    ui_snprintf(data_str, "%sspeed_num/%d.png", LVGL_DIR, speed_num);
    speed_num++;
    speed_num = speed_num > 9 ? 0: speed_num;
    lv_img_set_src(img_speed_num, data_str);
}

static void time_callback(lv_timer_t *tmr)
{
    char data_str[128];
    static int hour = 2;
    static int min = 0;

    (void)tmr;
    min++;
    if (min >= 60) {
        hour++;
        min = 0;
    }

    if (hour >= 24)
        hour = 0;

    ui_snprintf(data_str, "%stime/%d.png", LVGL_DIR, hour / 10);
    lv_img_set_src(img_time_0, data_str);

    ui_snprintf(data_str, "%stime/%d.png", LVGL_DIR, hour % 10);
    lv_img_set_src(img_time_1, data_str);

    ui_snprintf(data_str, "%stime/%d.png", LVGL_DIR, min / 10);
    lv_img_set_src(img_time_2, data_str);

    ui_snprintf(data_str, "%stime/%d.png", LVGL_DIR, min % 10);
    lv_img_set_src(img_time_3, data_str);
}

static void trip_callback(lv_timer_t *tmr)
{
    char data_str[128];
    int num[4];
    int cur;
    static int trip = 98;

    (void)tmr;
    trip++;
    if (trip >= 9999)
        trip = 0;

    num[0] = trip / 1000;
    cur = trip % 1000;
    num[1] = cur / 100;
    cur = cur % 100;
    num[2] = cur / 10;
    num[3] = cur % 10;

    ui_snprintf(data_str, "%smileage/%d.png", LVGL_DIR, num[0]);
    lv_img_set_src(img_trip_0, data_str);

    ui_snprintf(data_str, "%smileage/%d.png", LVGL_DIR, num[1]);
    lv_img_set_src(img_trip_1, data_str);

    ui_snprintf(data_str, "%smileage/%d.png", LVGL_DIR, num[2]);
    lv_img_set_src(img_trip_2, data_str);

    ui_snprintf(data_str, "%smileage/%d.png", LVGL_DIR, num[3]);
    lv_img_set_src(img_trip_3, data_str);
}

static void obj_set_clear_hidden_flag(lv_obj_t *obj)
{
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void water_anim()
{
    char data_str[128];
    static int level = 1;
    static int direct = 0;

    ui_snprintf(data_str, "%swater_temp/%d.png", LVGL_DIR, level);
    lv_img_set_src(img_water_num, data_str);

    if (level < 19)
        lv_img_set_src(img_water_temp, LVGL_PATH(water_temp/water_temp2.png));
    else
        lv_img_set_src(img_water_temp, LVGL_PATH(water_temp/water_temp.png));

    if (direct == 0)
         level++;
    else
        level--;

    if (level > 20) {
        level = 20;
        direct = 1;
    }

    if (level < 1) {
        level = 1;
        direct = 0;
    }
}

static void baterry_anim()
{
    char data_str[128];
    static int level = 1;
    static int direct = 0;

    ui_snprintf(data_str, "%sbattery/%d.png", LVGL_DIR, level);
    lv_img_set_src(img_battery_num, data_str);

    if (level > 1)
        lv_img_set_src(img_battery, LVGL_PATH(battery/battery2.png));
    else
        lv_img_set_src(img_battery, LVGL_PATH(battery/battery.png));

    if (direct == 0)
         level++;
    else
        level--;

    if (level > 5) {
        level = 5;
        direct = 1;
    }

    if (level < 1) {
        level = 1;
        direct = 0;
    }
}

static void oil_anim()
{
    char data_str[128];
    static int level = 1;
    static int direct = 0;

    ui_snprintf(data_str, "%soil_level/%d.png", LVGL_DIR, level);
    lv_img_set_src(img_oil_num, data_str);

    if (level > 1)
        lv_img_set_src(img_oil, LVGL_PATH(oil_level/oil_level2.png));
    else
        lv_img_set_src(img_oil, LVGL_PATH(oil_level/oil_level.png));

    if (direct == 0)
         level++;
    else
        level--;

    if (level > 5) {
        level = 5;
        direct = 1;
    }

    if (level < 1) {
        level = 1;
        direct = 0;
    }
}

static void gear_callback()
{
    char data_str[128];
    static int level = 1;
    static int direct = 0;

    if (level <= 6)
        ui_snprintf(data_str, "%sgear/d%d.png", LVGL_DIR, level);
    else
        ui_snprintf(data_str, "%sgear/n.png", LVGL_DIR);

    lv_img_set_src(img_gear, data_str);

    if (direct == 0)
         level++;
    else
        level--;

    if (level > 7) {
        level = 7;
        direct = 1;
    }

    if (level < 1) {
        level = 1;
        direct = 0;
    }
}

static void other_callback(lv_timer_t *tmr)
{
    static int mode = 0;
    (void)tmr;

    if (mode <= 2) {
        water_anim();
        baterry_anim();
    } else if (mode > 2 && mode <= 5) {
        water_anim();
        oil_anim();
    }

    mode++;
    if (mode > 5)
        mode = 0;
}

static void signal_callback(lv_timer_t *tmr)
{
    static int mode = 0;
    (void)tmr;

    if (mode == 0) {
        obj_set_clear_hidden_flag(img_engine);
    } else if (mode == 1) {
        obj_set_clear_hidden_flag(img_engine);
    } else if (mode == 2) {
        obj_set_clear_hidden_flag(img_esp);
    } else if (mode == 3) {
        obj_set_clear_hidden_flag(img_esp);
    } else if (mode == 4) {
        obj_set_clear_hidden_flag(img_oil_less);
    } else if (mode == 5) {
        obj_set_clear_hidden_flag(img_oil_less);
    }

    mode++;
    if (mode > 5)
        mode = 0;
}

static void fps_callback(lv_timer_t *tmr)
{
    char data_str[128];
#ifdef KERNEL_RTTHREAD
    float value;
#endif

    (void)tmr;

    /* frame rate */
    ui_snprintf(data_str, "%2d FPS", fbdev_draw_fps());
    lv_label_set_text(bg_fps, data_str);

#ifdef KERNEL_RTTHREAD
    /* cpu usage */
#ifdef LPKG_USING_CPU_USAGE
#include "cpu_usage.h"
    value = cpu_load_average();
#else
    value = 0;
#endif

    ui_snprintf(data_str, "%d%% CPU", (int)value);
    lv_label_set_text(bg_cpu, data_str);
#endif /* KERNEL_RTTHREAD */
    return;
}

void meter_ui_init()
{
    img_bg = lv_img_create(lv_scr_act());
    lv_img_set_src(img_bg, LVGL_PATH(bg/bg_red.jpg));
    lv_obj_set_pos(img_bg, 0, 0);

    img_engine = lv_img_create(img_bg);
    lv_img_set_src(img_engine, LVGL_PATH(warning/engine_err.png));
    meter_ui_set_pos(img_engine, 21, 13);

    img_esp = lv_img_create(img_bg);
    lv_img_set_src(img_esp, LVGL_PATH(warning/esp.png));
    meter_ui_set_pos(img_esp, 109, 12);

    img_oil_less = lv_img_create(img_bg);
    lv_img_set_src(img_oil_less, LVGL_PATH(warning/oil_less.png));
    meter_ui_set_pos(img_oil_less, 740, 12);

    // water
    img_water_temp = lv_img_create(img_bg);
    lv_img_set_src(img_water_temp, LVGL_PATH(water_temp/water_temp2.png));
    meter_ui_set_pos(img_water_temp, 59, 178);

    img_water_num = lv_img_create(img_bg);
    lv_img_set_src(img_water_num, LVGL_PATH(water_temp/1.png));
    meter_ui_set_pos(img_water_num, 117, 121);

     // battery
    img_battery = lv_img_create(img_bg);
    lv_img_set_src(img_battery, LVGL_PATH(battery/battery.png));
    meter_ui_set_pos(img_battery, 12, 349);

    img_battery_hl = lv_img_create(img_bg);
    lv_img_set_src(img_battery_hl, LVGL_PATH(battery/h_l.png));
    meter_ui_set_pos(img_battery_hl, 39, 282);
    img_battery_num = lv_img_create(img_bg);
    lv_img_set_src(img_battery_num, LVGL_PATH(battery/5.png));
    meter_ui_set_pos(img_battery_num, 56, 281);

    // oil
    img_oil_num = lv_img_create(img_bg);
    lv_img_set_src(img_oil_num, LVGL_PATH(oil_level/5.png));
    meter_ui_set_pos(img_oil_num, 628, 281);

    img_oil_fe = lv_img_create(img_bg);
    lv_img_set_src(img_oil_fe, LVGL_PATH(oil_level/fe.png));
    meter_ui_set_pos(img_oil_fe, 731, 282);
    img_oil = lv_img_create(img_bg);
    lv_img_set_src(img_oil, LVGL_PATH(oil_level/oil_level.png));
    meter_ui_set_pos(img_oil, 754, 352);

    img_gear = lv_img_create(img_bg);
    lv_img_set_src(img_gear, LVGL_PATH(gear/d6.png));
    meter_ui_set_pos(img_gear, 357, 380);
    img_circle = lv_img_create(img_bg);
    lv_img_set_src(img_circle, LVGL_PATH(bg/small_blue.png));
    meter_ui_set_pos(img_circle, 306, 152);

    // trip
    img_trip = lv_img_create(img_bg);
    lv_img_set_src(img_trip, LVGL_PATH(mileage/trip.png));
    meter_ui_set_pos(img_trip, 9, 439);

    img_trip_0 = lv_img_create(img_bg);
    lv_img_set_src(img_trip_0, LVGL_PATH(mileage/0.png));
    meter_ui_set_pos(img_trip_0, 69 + 5, 439);

    img_trip_1 = lv_img_create(img_bg);
    lv_img_set_src(img_trip_1, LVGL_PATH(mileage/0.png));
    meter_ui_set_pos(img_trip_1, 89, 439);

    img_trip_2 = lv_img_create(img_bg);
    lv_img_set_src(img_trip_2, LVGL_PATH(mileage/9.png));
    meter_ui_set_pos(img_trip_2, 109 - 5, 439);

    img_trip_3 = lv_img_create(img_bg);
    lv_img_set_src(img_trip_3, LVGL_PATH(mileage/8.png));
    meter_ui_set_pos(img_trip_3, 129 - 10, 439);

    img_trip_km = lv_img_create(img_bg);
    lv_img_set_src(img_trip_km, LVGL_PATH(mileage/km.png));
    meter_ui_set_pos(img_trip_km, 149 - 10, 439);

    // speed
    img_speed_num = lv_img_create(img_bg);
    lv_img_set_src(img_speed_num, LVGL_PATH(speed_num/0.png));
    meter_ui_set_pos(img_speed_num, 372, 204);

    img_speed_km = lv_img_create(img_bg);
    lv_img_set_src(img_speed_km, LVGL_PATH(speed_num/mph.png));
    meter_ui_set_pos(img_speed_km, 361, 284);

    // time
    img_time = lv_img_create(img_bg);
    lv_img_set_src(img_time, LVGL_PATH(time/time.png));
    meter_ui_set_pos(img_time, 690, 154);

    img_time_0 = lv_img_create(img_bg);
    lv_img_set_src(img_time_0, LVGL_PATH(time/0.png));
    meter_ui_set_pos(img_time_0, 690 - 29, 154 - 28);
    img_time_1 = lv_img_create(img_bg);
    lv_img_set_src(img_time_1, LVGL_PATH(time/2.png));
    meter_ui_set_pos(img_time_1, 690, 154 - 28);

    img_time_dot = lv_img_create(img_bg);
    lv_img_set_src(img_time_dot, LVGL_PATH(time/dot.png));
    meter_ui_set_pos(img_time_dot, 690 + 29, 154 - 28);

    img_time_2 = lv_img_create(img_bg);
    lv_img_set_src(img_time_2, LVGL_PATH(time/0.png));
    meter_ui_set_pos(img_time_2, 690 + 29 + 17, 154 - 28);
    img_time_3 = lv_img_create(img_bg);
    lv_img_set_src(img_time_3, LVGL_PATH(time/0.png));
    meter_ui_set_pos(img_time_3, 690 + 29 * 2 + 17, 154 - 28);

    // point
    img_point = lv_img_create(img_bg);
#ifdef LV_METER_SIMPLE_POINT
    float rot_angle = 2373.0; // 237.3 degree
    char data_str[128];
    ui_snprintf(data_str, "%spoint/point_small_blue.png", LVGL_DIR);
    lv_img_set_src(img_point, data_str);
    lv_img_set_angle(img_point, (int16_t)rot_angle);
    lv_img_set_pivot(img_point, 15, 210);
    meter_ui_set_pos(img_point, 186 + 195, 21);
#else
    lv_img_set_src(img_point, LVGL_PATH(point/point_00001.png));
    lv_img_set_pivot(img_point, 210, 210);
    meter_ui_set_pos(img_point, 186, 21);
#endif

#ifdef KERNEL_RTTHREAD
    bg_cpu = lv_label_create(img_bg);
    lv_obj_set_width(bg_cpu, LV_SIZE_CONTENT);
    lv_obj_set_height(bg_cpu, LV_SIZE_CONTENT);
    lv_obj_align(bg_cpu, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_pos(bg_cpu, -20, 10);
    lv_label_set_text(bg_cpu, "");
    lv_obj_set_style_text_color(bg_cpu, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bg_cpu, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bg_cpu, &ui_font_Title, LV_PART_MAIN | LV_STATE_DEFAULT);
#endif

    bg_fps = lv_label_create(img_bg);
    lv_obj_set_width(bg_fps, LV_SIZE_CONTENT);
    lv_obj_set_height(bg_fps, LV_SIZE_CONTENT);
    lv_obj_align(bg_fps, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_pos(bg_fps, -20, 30);
    lv_label_set_text(bg_fps, "");
    lv_obj_set_style_text_color(bg_fps, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bg_fps, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

#if LV_USE_FREETYPE ==  1
    static lv_ft_info_t info;
    // FreeType uses C standard file system, so no driver letter is required
    info.name = "/rodata/lvgl_data/font/Lato-Regular.ttf";
    info.weight = 22;
    info.style = FT_FONT_STYLE_NORMAL;
    info.mem = NULL;
    if(!lv_ft_font_init(&info)) {
        LV_LOG_ERROR("create failed.");
    }

    // Create style
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_font(&style, info.font);
    lv_obj_add_style(bg_fps, &style, 0);
#else
    lv_obj_set_style_text_font(bg_fps, &ui_font_Title, LV_PART_MAIN | LV_STATE_DEFAULT);
#endif

    lv_timer_create(point_callback, 10, 0);
    lv_timer_create(fps_callback, 1000, 0);
    lv_timer_create(time_callback, 1000 * 60, 0);
    lv_timer_create(trip_callback, 1000 * 5, 0);
    lv_timer_create(other_callback, 600, 0);
    lv_timer_create(signal_callback, 500, 0);
    lv_timer_create(gear_callback, 1000 * 3, 0);
    speed_timer = lv_timer_create(speed_callback, 100, 0);

    return;
}
