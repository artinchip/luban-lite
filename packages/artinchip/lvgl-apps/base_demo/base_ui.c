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

#ifdef  AIC_USING_FS_IMAGE_1
#define VIDEO_PLAYER
#endif

//#define FILE_LIST
FAKE_IMAGE_DECLARE(bg_dark);

LV_FONT_DECLARE(ui_font_Big);
LV_FONT_DECLARE(ui_font_Title);
LV_FONT_DECLARE(ui_font_H1);

LV_IMG_DECLARE(circle_white);
LV_IMG_DECLARE(circle_gray);
LV_IMG_DECLARE(next_normal);
LV_IMG_DECLARE(next_press);
LV_IMG_DECLARE(pause_norma_1);
LV_IMG_DECLARE(pause_press_1);
LV_IMG_DECLARE(play_button);
LV_IMG_DECLARE(play_down);
LV_IMG_DECLARE(play_home);
LV_IMG_DECLARE(play_left);
LV_IMG_DECLARE(play_love);
LV_IMG_DECLARE(play_repeat);
LV_IMG_DECLARE(play_right);
LV_IMG_DECLARE(play_say);
LV_IMG_DECLARE(play_share);
LV_IMG_DECLARE(play_shuffle);
LV_IMG_DECLARE(play_pause);

#ifdef VIDEO_PLAYER
LV_IMG_DECLARE(play_normal);
LV_IMG_DECLARE(play_press);
LV_IMG_DECLARE(pre_normal);
LV_IMG_DECLARE(pre_press);
#endif

#ifdef VIDEO_PLAYER
#include "aic_player.h"

#define LVGL_PLAYER_STATE_PLAY  1
#define LVGL_PLAYER_STATE_PAUSE  2
#define LVGL_PLAYER_STATE_STOP   3

struct lvgl_player_context{
    int file_cnt;
    int file_index;
    int player_state;
    struct aic_player *player;
    int sync_flag;
    struct av_media_info media_info;
    int demuxer_detected_flag;
    int player_end;
    struct mpp_size screen_size;
    struct mpp_rect disp_rect;
};

static char g_filename[][256] = {
    LVGL_FILE_LIST_PATH(cartoon.mp4),
};

static struct lvgl_player_context g_lvgl_player_ctx;
static lv_obj_t *g_btn_pre;
static lv_obj_t *g_btn_next;
static lv_obj_t *g_btn_pause_play;
static lv_obj_t *g_btn_hide_show;
static struct mpp_fb *fd_dev;

#ifdef FILE_LIST
static lv_obj_t * g_file_list;
static lv_style_t g_style_scrollbar;
static lv_style_t g_file_list_btn_style_def;
static lv_style_t g_file_list_btn_style_pre;
static lv_style_t g_file_list_btn_style_chk;
#endif

static s32 event_handle(void* app_data,s32 event,s32 data1,s32 data2)
{
    int ret = 0;
    struct lvgl_player_context *ctx = (struct lvgl_player_context *)app_data;

    switch (event) {
        case AIC_PLAYER_EVENT_PLAY_END:
            ctx->player_end = 1;
        case AIC_PLAYER_EVENT_PLAY_TIME:
            break;
        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_DETECTED:
            if (AIC_PLAYER_PREPARE_ASYNC == ctx->sync_flag) {
                ctx->demuxer_detected_flag = 1;
            }
            break;
        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_NOT_DETECTED:
            if (AIC_PLAYER_PREPARE_ASYNC == ctx->sync_flag) {
                ctx->player_end = 1;
            }
            break;
        default:
            break;
    }
    return ret;
}

static int lvgl_play(struct lvgl_player_context *ctx)
{
    int k = 0;
    int ret = 0;

    k  = ctx->file_index%ctx->file_cnt;
    aic_player_set_uri(ctx->player,g_filename[k]);
    ctx->sync_flag = AIC_PLAYER_PREPARE_SYNC;
    if (ctx->sync_flag == AIC_PLAYER_PREPARE_ASYNC) {
        ret = aic_player_prepare_async(ctx->player);
    } else {
        ret = aic_player_prepare_sync(ctx->player);
    }
    if (ret) {
        return -1;
    }
    if (ctx->sync_flag == AIC_PLAYER_PREPARE_SYNC) {
        ret = aic_player_start(ctx->player);
        if(ret != 0) {
            return -1;
        }
        aic_player_get_screen_size(ctx->player, &ctx->screen_size);
        printf("screen_width:%d,screen_height:%d\n",ctx->screen_size.width,ctx->screen_size.height);
#if 0
        ctx->disp_rect.x = 324;
        ctx->disp_rect.y = 20;
        ctx->disp_rect.width = 600;
        ctx->disp_rect.height = 450;

        ret = aic_player_set_disp_rect(ctx->player, &ctx->disp_rect);
        if(ret != 0) {
            printf("aic_player_set_disp_rect error\n");
            return -1;
        }
#endif
        ret =  aic_player_get_media_info(ctx->player,&ctx->media_info);
        if (ret != 0) {
            return -1;
        }
        ret = aic_player_play(ctx->player);
        if (ret != 0) {
            return -1;
        }
    }
    return 0;
}

static int lvgl_stop(struct lvgl_player_context *ctx)
{
    return aic_player_stop(ctx->player);
}

static int lvgl_pause(struct lvgl_player_context *ctx)
{
    return aic_player_pause(ctx->player);
}

static int lvgl_play_pre(struct lvgl_player_context *ctx)
{
    ctx->file_index--;
    ctx->file_index = (ctx->file_index < 0) ? (ctx->file_cnt - 1) : ctx->file_index;
    lvgl_stop(ctx);
    if (lvgl_play(ctx) != 0) {
        return -1;
    }
    return 0;
}

static int lvgl_play_next(struct lvgl_player_context *ctx)
{
    ctx->file_index++;
    ctx->file_index = (ctx->file_index > ctx->file_cnt - 1) ? 0 : ctx->file_index;
    lvgl_stop(ctx);
    if (lvgl_play(ctx) != 0) {
        return -1;
    }
    return 0;
}

static void btn_pause_play_event_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    struct lvgl_player_context * ctx = (struct lvgl_player_context *)btn->user_data;
    if (code == LV_EVENT_CLICKED) {
        if (ctx->player_state == LVGL_PLAYER_STATE_STOP) {
            if (lvgl_play(ctx) != 0) {//if play fail ,it is considered play finsh.play the next one
                ctx->player_state = LVGL_PLAYER_STATE_STOP;
                ctx->player_end = 1;
                return;
            }
            ctx->player_end = 0;
            ctx->player_state = LVGL_PLAYER_STATE_PLAY;
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &pause_norma_1, NULL);
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &pause_press_1, NULL);
        } else if (ctx->player_state == LVGL_PLAYER_STATE_PLAY) {
            lvgl_pause(ctx);
            ctx->player_state = LVGL_PLAYER_STATE_PAUSE;
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &play_normal, NULL);
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &play_press, NULL);
        } else if (ctx->player_state == LVGL_PLAYER_STATE_PAUSE) {
            lvgl_pause(ctx);
            ctx->player_state = LVGL_PLAYER_STATE_PLAY;
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &pause_norma_1, NULL);
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &pause_press_1, NULL);
        }
    }
}

static void btn_next_event_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    struct lvgl_player_context * ctx = (struct lvgl_player_context *)btn->user_data;

    if (code == LV_EVENT_CLICKED) {
        if (lvgl_play_next(ctx) != 0) {//if play fail ,it is considered play finsh.play the next one
            ctx->player_state =LVGL_PLAYER_STATE_STOP;
            ctx->player_end = 1;
        } else {
            ctx->player_state =LVGL_PLAYER_STATE_PLAY;
            ctx->player_end = 0;
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &pause_norma_1, NULL);
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &pause_press_1, NULL);

        }
    }
}

static void btn_pre_event_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    struct lvgl_player_context * ctx = (struct lvgl_player_context *)btn->user_data;

    if (code == LV_EVENT_CLICKED) {
        if (lvgl_play_pre(ctx) != 0) {//if play fail ,it is considered play finsh.play the next one
            ctx->player_state =LVGL_PLAYER_STATE_STOP;
            ctx->player_end = 1;
        } else {
            ctx->player_state =LVGL_PLAYER_STATE_PLAY;
            ctx->player_end = 0;
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &pause_norma_1, NULL);
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &pause_press_1, NULL);
        }
    }
}

static void btn_show_hide_event_cb(lv_event_t *e)
{
    static int show = 1;
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_RELEASED) {
        printf("!!!!!btn_show_hide_event_cb!!!!!!!!!!!\n");
        if (show) {
            lv_obj_add_flag(g_btn_pre, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(g_btn_next, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(g_btn_pause_play, LV_OBJ_FLAG_HIDDEN);
            #ifdef FILE_LIST
            lv_obj_add_flag(g_file_list, LV_OBJ_FLAG_HIDDEN);
            #endif

            show = 0;
        } else {
            lv_obj_clear_flag(g_btn_pre, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(g_btn_next, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(g_btn_pause_play, LV_OBJ_FLAG_HIDDEN);
            #ifdef FILE_LIST
            lv_obj_clear_flag(g_file_list, LV_OBJ_FLAG_HIDDEN);
            #endif
            show = 1;
        }
    }
}

#ifdef FILE_LIST
static void file_list_btn_click_event_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    uint32_t idx = lv_obj_get_child_id(btn);
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    struct lvgl_player_context * ctx = (struct lvgl_player_context *)btn->user_data;

    if (code == LV_EVENT_CLICKED) {
        printf("idx=%u\n",idx);
        ctx->file_index = idx;
        lvgl_stop(ctx);
        if (lvgl_play(ctx) != 0) {
            ctx->player_end = 1;
            ctx->player_state = LVGL_PLAYER_STATE_STOP;
        } else {
            ctx->player_end = 0;
            ctx->player_state = LVGL_PLAYER_STATE_PLAY;
        }
    }
}

static lv_obj_t * add_file_list_btn(lv_obj_t * parent, uint32_t video_id)
{
    lv_obj_t * btn = lv_obj_create(parent);
    lv_obj_t * title_label = lv_label_create(btn);
    lv_obj_set_size(btn, lv_pct(95), 60);
    btn->user_data = &g_lvgl_player_ctx;
     lv_obj_add_style(btn, &g_file_list_btn_style_pre, LV_STATE_DEFAULT);
    lv_obj_add_style(btn, &g_file_list_btn_style_def, LV_STATE_PRESSED);
    lv_obj_add_style(btn, &g_file_list_btn_style_chk, LV_STATE_CHECKED);
    lv_obj_add_event_cb(btn, file_list_btn_click_event_cb, LV_EVENT_CLICKED, NULL);
    const char * title = (video_id >= sizeof(g_filename) / sizeof(g_filename[0]))?NULL:g_filename[video_id];
    lv_label_set_text(title_label, title);
    return btn;
}
#endif

static void create_player(lv_obj_t * parent)
{
#ifdef FILE_LIST
    int i = 0;
#endif
    static lv_style_t btn_style;
    lv_style_init(&btn_style);
    lv_style_set_radius(&btn_style, 0);
    lv_style_set_border_width(&btn_style, 2);
    //lv_style_set_border_color(&btn_style, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_bg_opa(&btn_style, LV_OPA_0);

    lv_memset_00(&g_lvgl_player_ctx, sizeof(struct lvgl_player_context));
    g_lvgl_player_ctx.player = aic_player_create(NULL);
    if (g_lvgl_player_ctx.player == NULL) {
        printf("aic_player_create fail!!!!\n");
        return;
    }

    g_lvgl_player_ctx.file_cnt =  sizeof(g_filename) / sizeof(g_filename[0]);
    g_lvgl_player_ctx.file_index = 0;
    g_lvgl_player_ctx.player_state = LVGL_PLAYER_STATE_STOP;
    aic_player_set_event_callback(g_lvgl_player_ctx.player, &g_lvgl_player_ctx, event_handle);

    g_btn_hide_show = lv_btn_create(parent);
    g_btn_hide_show->user_data = &g_lvgl_player_ctx;
    lv_obj_set_pos(g_btn_hide_show, 800, 40);
    lv_obj_set_size(g_btn_hide_show, 180, 520);
    lv_obj_add_event_cb(g_btn_hide_show,btn_show_hide_event_cb,LV_EVENT_ALL, NULL);
    lv_obj_add_style(g_btn_hide_show,&btn_style, 0);
    lv_obj_set_style_border_opa(g_btn_hide_show,LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_opa(g_btn_hide_show, LV_OPA_TRANSP, 0);

    g_btn_pre = lv_imgbtn_create(parent);
    lv_imgbtn_set_src(g_btn_pre, LV_IMGBTN_STATE_RELEASED , NULL, &pre_normal, NULL);
    lv_imgbtn_set_src(g_btn_pre, LV_IMGBTN_STATE_PRESSED , NULL, &pre_press, NULL);
    g_btn_pre->user_data = &g_lvgl_player_ctx;
    lv_obj_set_pos(g_btn_pre, 200, 450);
    lv_obj_set_size(g_btn_pre, 64, 64);
    lv_obj_add_event_cb(g_btn_pre,btn_pre_event_cb,LV_EVENT_ALL, NULL);
    lv_obj_add_style(g_btn_pre,&btn_style, 0);
    lv_obj_set_style_border_opa(g_btn_pre,LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_opa(g_btn_pre, LV_OPA_TRANSP, 0);

    g_btn_pause_play = lv_imgbtn_create(parent);
    lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &play_normal, NULL);
    lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &play_press, NULL);
    g_btn_pause_play->user_data = &g_lvgl_player_ctx;
    lv_obj_set_pos(g_btn_pause_play, 450, 450);
    lv_obj_set_size(g_btn_pause_play, 64, 64);
    lv_obj_add_event_cb(g_btn_pause_play,btn_pause_play_event_cb,LV_EVENT_ALL, NULL);
    lv_obj_add_style(g_btn_pause_play,&btn_style, 0);
    lv_obj_set_style_border_opa(g_btn_pause_play,LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_opa(g_btn_pause_play, LV_OPA_TRANSP, 0);

    g_btn_next = lv_imgbtn_create(parent);
    lv_imgbtn_set_src(g_btn_next, LV_IMGBTN_STATE_RELEASED , NULL, &next_normal, NULL);
    lv_imgbtn_set_src(g_btn_next, LV_IMGBTN_STATE_PRESSED , NULL, &next_press, NULL);
    g_btn_next->user_data = &g_lvgl_player_ctx;
    lv_obj_set_pos(g_btn_next, 700, 450);
    lv_obj_set_size(g_btn_next, 64, 64);
    lv_obj_add_event_cb(g_btn_next,btn_next_event_cb,LV_EVENT_ALL, NULL);
    lv_obj_add_style(g_btn_next,&btn_style, 0);
    lv_obj_set_style_border_opa(g_btn_next,LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_opa(g_btn_next, LV_OPA_TRANSP, 0);

#ifdef FILE_LIST
    lv_style_init(&g_style_scrollbar);
    lv_style_set_width(&g_style_scrollbar,  4);
    lv_style_set_bg_opa(&g_style_scrollbar, LV_OPA_COVER);
    lv_style_set_bg_color(&g_style_scrollbar, lv_color_hex3(0xeee));
    lv_style_set_radius(&g_style_scrollbar, LV_RADIUS_CIRCLE);
    lv_style_set_pad_right(&g_style_scrollbar, 4);

    g_file_list = lv_obj_create(parent);
    lv_obj_remove_style_all(g_file_list);
    lv_obj_set_size(g_file_list, 300, 450);
    lv_obj_set_y(g_file_list, 40);
    lv_obj_add_style(g_file_list, &g_style_scrollbar, LV_PART_SCROLLBAR);
    lv_obj_set_flex_flow(g_file_list, LV_FLEX_FLOW_COLUMN);

    lv_style_init(&g_file_list_btn_style_def);
    lv_style_set_bg_opa(&g_file_list_btn_style_def, LV_OPA_TRANSP);
    lv_style_set_radius(&g_file_list_btn_style_def, 0);

    lv_style_init(&g_file_list_btn_style_pre);
    lv_style_set_bg_opa(&g_file_list_btn_style_pre, LV_OPA_COVER);
    lv_style_set_bg_color(&g_file_list_btn_style_pre,  lv_color_hex(0x4c4965));
    lv_style_set_radius(&g_file_list_btn_style_pre, 0);

    lv_style_init(&g_file_list_btn_style_chk);
    lv_style_set_bg_opa(&g_file_list_btn_style_chk, LV_OPA_COVER);
    lv_style_set_bg_color(&g_file_list_btn_style_chk, lv_color_hex(0x4c4965));
    lv_style_set_radius(&g_file_list_btn_style_chk, 0);

    for (i = 0;i < sizeof(g_filename) / sizeof(g_filename[0]); i++) {
        add_file_list_btn(g_file_list,i);
    }
#endif
}

static void player_callback(lv_timer_t *tmr)
{
    struct lvgl_player_context *ctx;

    (void)tmr;

    ctx = &g_lvgl_player_ctx;
    if (ctx->player_end && ctx->player_state != LVGL_PLAYER_STATE_STOP) {
        ctx->player_end = 0;
        lvgl_stop(ctx);
        ctx->player_state = LVGL_PLAYER_STATE_STOP;
        lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &play_normal, NULL);
        lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &play_press, NULL);
    }

    return;
}

#endif

static int rot_angle = 255;
static int rot_direct = 0;
static int img_angle = 0;
static lv_obj_t *main_title = NULL;
static lv_obj_t *bg_fps = NULL;
static lv_obj_t *bg_logo = NULL;
static lv_obj_t *circle_0;
static lv_obj_t *circle_1;
static lv_obj_t *circle_2;
static lv_obj_t *circle_3;
static lv_obj_t *ui_speed = NULL;
static lv_obj_t *fps_info = NULL;
static lv_obj_t *cpu_info = NULL;
static lv_obj_t *mem_info = NULL;
static lv_obj_t *fps_title = NULL;
static lv_obj_t *cpu_title = NULL;
static lv_obj_t *mem_title = NULL;
static lv_obj_t *tab_sub = NULL;
static lv_obj_t *img_bg = NULL;

static bool only_two_tap = false;

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

static void image_aimation(lv_obj_t *obj)
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
    char speed_str[8];

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

static void main_tapview_event(lv_event_t * e)
{
    lv_obj_t * tapview = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

#ifdef VIDEO_PLAYER
    struct aicfb_alpha_config alpha = {0};
    alpha.layer_id = 1;
    alpha.enable = 1;
    alpha.mode = 1;
    alpha.value = 255;
#endif
    if(code == LV_EVENT_VALUE_CHANGED) {
        int tab_num = lv_tabview_get_tab_act(tapview);

        if (tab_num == 0) {
            lv_label_set_text(main_title, "Meter");

            if (only_two_tap) {
                lv_img_set_src(circle_0, &circle_white);
                lv_obj_align(circle_0, LV_ALIGN_BOTTOM_MID, -16, -28);
                lv_img_set_src(circle_1, &circle_gray);
                lv_obj_align(circle_1, LV_ALIGN_BOTTOM_MID, 16, -30);
            } else {
                lv_img_set_src(circle_0, &circle_white);
                lv_obj_align(circle_0, LV_ALIGN_BOTTOM_MID, -48, -28);
                lv_img_set_src(circle_1, &circle_gray);
                lv_obj_align(circle_1, LV_ALIGN_BOTTOM_MID, -16, -30);
                lv_img_set_src(circle_2, &circle_gray);
                lv_obj_align(circle_2, LV_ALIGN_BOTTOM_MID, 16, -30);
                lv_img_set_src(circle_3, &circle_gray);
                lv_obj_align(circle_3, LV_ALIGN_BOTTOM_MID, 48, -30);
            }
#ifdef VIDEO_PLAYER
            alpha.mode = 1;
            mpp_fb_ioctl(fd_dev, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
#endif
        } else if (tab_num == 1) {
            lv_label_set_text(main_title, "Media Player");
            if (only_two_tap) {
                lv_img_set_src(circle_0, &circle_gray);
                lv_obj_align(circle_0, LV_ALIGN_BOTTOM_MID, -16, -30);
                lv_img_set_src(circle_1, &circle_white);
                lv_obj_align(circle_1, LV_ALIGN_BOTTOM_MID, 16, -28);
            } else {
                lv_img_set_src(circle_0, &circle_gray);
                lv_obj_align(circle_0, LV_ALIGN_BOTTOM_MID, -48, -30);
                lv_img_set_src(circle_1, &circle_white);
                lv_obj_align(circle_1, LV_ALIGN_BOTTOM_MID, -16, -28);
                lv_img_set_src(circle_2, &circle_gray);
                lv_obj_align(circle_2, LV_ALIGN_BOTTOM_MID, 16, -30);
                lv_img_set_src(circle_3, &circle_gray);
                lv_obj_align(circle_3, LV_ALIGN_BOTTOM_MID, 48, -30);
            }

#ifdef VIDEO_PLAYER
            alpha.mode = 1;
            mpp_fb_ioctl(fd_dev, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
#endif
        } else if (tab_num == 2) {
            char data_str[128];
            int tab_sub_num;

            lv_obj_clear_flag(main_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(circle_0, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(circle_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(circle_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(circle_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(bg_fps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(bg_logo, LV_OBJ_FLAG_HIDDEN);
            lv_img_set_src(img_bg, LVGL_PATH(global_bg.png));

            tab_sub_num = lv_tabview_get_tab_act(tab_sub);
            ui_snprintf(data_str, "Cookbook (%d/2)", tab_sub_num + 1);
            lv_label_set_text(main_title, data_str);

            lv_img_set_src(circle_0, &circle_gray);
            lv_obj_align(circle_0, LV_ALIGN_BOTTOM_MID, -48, -30);
            lv_img_set_src(circle_1, &circle_gray);
            lv_obj_align(circle_1, LV_ALIGN_BOTTOM_MID, -16, -30);
            lv_img_set_src(circle_2, &circle_white);
            lv_obj_align(circle_2, LV_ALIGN_BOTTOM_MID, 16, -28);
            lv_img_set_src(circle_3, &circle_gray);
            lv_obj_align(circle_3, LV_ALIGN_BOTTOM_MID, 48, -30);
#ifdef VIDEO_PLAYER
            alpha.mode = 1;
            mpp_fb_ioctl(fd_dev, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
#endif
        } else if (tab_num == 3) {
            lv_img_set_src(img_bg, FAKE_IMAGE_NAME(bg_dark));

            lv_obj_add_flag(main_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(circle_0, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(circle_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(circle_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(circle_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(bg_fps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(bg_logo, LV_OBJ_FLAG_HIDDEN);
            lv_img_cache_invalidate_src(NULL);
#ifdef VIDEO_PLAYER
            alpha.mode = 0;
            mpp_fb_ioctl(fd_dev, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
#endif
        }
    }
}

void base_ui_init()
{
    FAKE_IMAGE_INIT(bg_dark, 1024, 600, 0, 0x00000000);

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

    if (only_two_tap) {
        circle_0 = lv_img_create(img_bg);
        lv_img_set_src(circle_0, &circle_white);
        lv_obj_align(circle_0, LV_ALIGN_BOTTOM_MID, -16, -28);

        circle_1 = lv_img_create(img_bg);
        lv_img_set_src(circle_1, &circle_gray);
        lv_obj_align(circle_1, LV_ALIGN_BOTTOM_MID, 16, -30);
    } else {
        circle_0 = lv_img_create(img_bg);
        lv_img_set_src(circle_0, &circle_white);
        lv_obj_align(circle_0, LV_ALIGN_BOTTOM_MID, -48, -28);

        circle_1 = lv_img_create(img_bg);
        lv_img_set_src(circle_1, &circle_gray);
        lv_obj_align(circle_1, LV_ALIGN_BOTTOM_MID, -16, -30);

        circle_2 = lv_img_create(img_bg);
        lv_img_set_src(circle_2, &circle_gray);
        lv_obj_align(circle_2, LV_ALIGN_BOTTOM_MID, 16, -30);

        circle_3 = lv_img_create(img_bg);
        lv_img_set_src(circle_3, &circle_gray);
        lv_obj_align(circle_3, LV_ALIGN_BOTTOM_MID, 48, -30);
    }

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

    lv_obj_t *play_home_img = lv_img_create(main_tab1);
    lv_img_set_src(play_home_img, &play_home);
    lv_obj_set_pos(play_home_img, 172, 272);

    lv_obj_t *play_down_img = lv_img_create(main_tab1);
    lv_img_set_src(play_down_img, &play_down);
    lv_obj_set_pos(play_down_img, 247, 272);

    lv_obj_t *play_love_img = lv_img_create(main_tab1);
    lv_img_set_src(play_love_img, &play_love);
    lv_obj_set_pos(play_love_img, 316, 272);

    lv_obj_t *play_say_img = lv_img_create(main_tab1);
    lv_img_set_src(play_say_img, &play_say);
    lv_obj_set_pos(play_say_img, 392, 272);

    lv_obj_t *play_share_img = lv_img_create(main_tab1);
    lv_img_set_src(play_share_img, &play_share);
    lv_obj_set_pos(play_share_img, 460, 272);

    lv_obj_t *play_image_img = lv_img_create(main_tab1);
    lv_img_set_src(play_image_img, LVGL_PATH(play_image.png));
    lv_obj_set_pos(play_image_img, 660, 178);
    lv_img_set_angle(play_image_img, img_angle * 10);
    image_aimation(play_image_img);

    lv_obj_t *play_shuffle_img = lv_img_create(main_tab1);
    lv_img_set_src(play_shuffle_img, &play_shuffle);
    lv_obj_set_pos(play_shuffle_img, 120, 382);

    lv_obj_t *play_left_img = lv_img_create(main_tab1);
    lv_img_set_src(play_left_img, &play_left);
    lv_obj_set_pos(play_left_img, 208, 382);

    lv_obj_t *play_button_img = lv_img_create(main_tab1);
    lv_img_set_src(play_button_img, &play_button);
    lv_obj_set_pos(play_button_img, 208, 305);

    lv_obj_t *play_pause_img = lv_img_create(main_tab1);
    lv_img_set_src(play_pause_img, &play_pause);
    lv_obj_set_pos(play_pause_img, 302, 382);

    lv_obj_t *play_right_img = lv_img_create(main_tab1);
    lv_img_set_src(play_right_img, &play_right);
    lv_obj_set_pos(play_right_img, 405, 382);

    lv_obj_t *play_repeat_img = lv_img_create(main_tab1);
    lv_img_set_src(play_repeat_img, &play_repeat);
    lv_obj_set_pos(play_repeat_img, 490, 382);

    if (!only_two_tap) {
        lv_obj_t * main_tab2 = lv_tabview_add_tab(main_tabview, "main page 2");
#ifdef VIDEO_PLAYER
        lv_obj_t * main_tab3 = lv_tabview_add_tab(main_tabview, "main page 3");
#endif
        lv_obj_set_style_bg_opa(main_tab2, LV_OPA_0, 0);
        lv_obj_set_size(main_tab2, 1024, 600);
        lv_obj_set_pos(main_tab2, 0, 0);

        lv_obj_t *cook_top_img = lv_img_create(main_tab2);
        lv_img_set_src(cook_top_img, LVGL_PATH(cook_top.png));
        lv_obj_set_pos(cook_top_img, 280, 80);
        lv_obj_t *cook_buttom_img = lv_img_create(main_tab2);
        lv_img_set_src(cook_buttom_img, LVGL_PATH(cook_buttom.png));
        lv_obj_set_pos(cook_buttom_img, 0, 430);

        tab_sub = lv_tabview_create(main_tab2, LV_DIR_TOP, 0);
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

        lv_obj_t *sub_image10 = lv_img_create(sub_tab1);
        lv_img_set_src(sub_image10, LVGL_PATH(cook_3.jpg));
        lv_obj_set_pos(sub_image10, 36, 100);

        lv_obj_t *sub_image11 = lv_img_create(sub_tab1);
        lv_img_set_src(sub_image11, LVGL_PATH(cook_4.jpg));
        lv_obj_set_pos(sub_image11, 366, 100);

        lv_obj_t *sub_image12 = lv_img_create(sub_tab1);
        lv_img_set_src(sub_image12, LVGL_PATH(cook_5.jpg));
        lv_obj_set_pos(sub_image12, 696, 100);
        lv_obj_add_event_cb(tab_sub, sub_tapview_event, LV_EVENT_ALL, NULL);
#ifdef VIDEO_PLAYER
        fd_dev = mpp_fb_open();
        create_player(main_tab3);
        lv_timer_create(player_callback, 20, 0);
#endif
    }

    point_aimation(img_point);
    lv_timer_create(timer_callback, 1000, 0);
    lv_timer_create(point_callback, 100, 0);

    lv_obj_add_event_cb(main_tabview, main_tapview_event, LV_EVENT_ALL, NULL);
}
