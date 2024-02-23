/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <aic_core.h>
#include <aic_time.h>
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_refr.h"
#include "lv_ge2d.h"
#include "mpp_ge.h"
#include "mpp_log.h"
#include "lv_tpc_run.h"
#include "mpp_fb.h"
#include "lv_fbdev.h"

static int draw_fps = 0;
static struct mpp_ge *g_ge = NULL;
static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static struct mpp_fb *g_fb = 0;
static struct aicfb_screeninfo info;
#ifdef USE_DRAW_BUF
static int g_fb_num = 1;
static int g_fb_id = 0;
#endif

#define US_PER_SEC      1000000

static double get_time_gap(struct timeval *start, struct timeval *end)
{
    double diff;

    if (end->tv_usec < start->tv_usec) {
        diff = (double)(US_PER_SEC + end->tv_usec - start->tv_usec)/US_PER_SEC;
        diff += end->tv_sec - 1 - start->tv_sec;
    } else {
        diff = (double)(end->tv_usec - start->tv_usec)/US_PER_SEC;
        diff += end->tv_sec - start->tv_sec;
    }

    return diff;
}

static int cal_fps(double gap, int cnt)
{
    return (int)(cnt / gap);
}

static void cal_frame_rate()
{
    static int start_cal = 0;
    static int frame_cnt = 0;
    static struct timeval start, end;
    double interval = 0.5;
    double gap = 0;

    if (start_cal == 0) {
        start_cal = 1;
        start.tv_sec = 0;
        start.tv_usec = (long)aic_get_time_us();
    }

    end.tv_sec = 0;
    end.tv_usec = (long)aic_get_time_us();
    gap = get_time_gap(&start, &end);
    if (gap >= interval) {
        draw_fps = cal_fps(gap, frame_cnt);
        frame_cnt = 0;
        start_cal = 0;
    } else {
        frame_cnt++;
    }
    return;
}

void sync_disp_buf(lv_disp_drv_t * drv, lv_color_t * color_p, const lv_area_t * area_p)
{
    int32_t ret;
    struct ge_bitblt blt = { 0 };
    lv_coord_t w = lv_area_get_width(area_p);
    lv_coord_t h = lv_area_get_height(area_p);

    blt.src_buf.buf_type = MPP_PHY_ADDR;
    if (drv->draw_buf->buf_act == drv->draw_buf->buf1) {
        blt.src_buf.phy_addr[0] = (intptr_t)(info.framebuffer
            + info.stride * info.height
            + area_p->x1 * info.bits_per_pixel / 8
            + area_p->y1 * info.stride);
    } else {
        blt.src_buf.phy_addr[0] = (intptr_t)(info.framebuffer
            + area_p->x1 * info.bits_per_pixel / 8
            + area_p->y1 * info.stride);
    }

    blt.src_buf.stride[0] = info.stride;
    blt.src_buf.size.width = w;
    blt.src_buf.size.height = h;
    blt.src_buf.format = AICFB_FORMAT;
    blt.dst_buf.buf_type = MPP_PHY_ADDR;

    if (drv->draw_buf->buf_act == drv->draw_buf->buf1) {
        blt.dst_buf.phy_addr[0] = (intptr_t)(info.framebuffer
            + area_p->x1 * info.bits_per_pixel / 8
            + area_p->y1 * info.stride);
    } else {
        blt.dst_buf.phy_addr[0] = (intptr_t)(info.framebuffer
            + info.stride * info.height
            + area_p->x1 * info.bits_per_pixel / 8
            + area_p->y1 * info.stride);
    }
    blt.dst_buf.stride[0] = info.stride;
    blt.dst_buf.size.width = w;
    blt.dst_buf.size.height = h;
    blt.dst_buf.format = AICFB_FORMAT;

    ret = mpp_ge_bitblt(g_ge, &blt);
    if (ret < 0) {
        LV_LOG_ERROR("bitblt fail\n");
        return;
    }

    return;
}

#ifdef USE_DRAW_BUF
void disp_draw_buf(lv_disp_drv_t * drv, lv_color_t * color_p)
{
    int32_t ret;
    int disp_w;
    int disp_h;
    struct ge_bitblt blt = { 0 };
    enum mpp_pixel_format draw_fmt = draw_buf_fmt();
    int draw_pitch = draw_buf_pitch();
    enum mpp_pixel_format disp_fmt = fbdev_get_fmt();
    int disp_pitch = fbdev_get_pitch();
    int draw_w;
    int draw_h;

    if (drv->rotated == LV_DISP_ROT_90 || drv->rotated == LV_DISP_ROT_270) {
        draw_w = drv->ver_res;
        draw_h = drv->hor_res;
    } else  {
        draw_w = drv->hor_res;
        draw_h = drv->ver_res;
    }

    fbdev_get_size(&disp_w, &disp_h);

    blt.src_buf.buf_type = MPP_PHY_ADDR;
    if (color_p == (lv_color_t *)g_draw_buf[0])
        blt.src_buf.phy_addr[0] = (unsigned int)(long)g_draw_buf[0];
    else
        blt.src_buf.phy_addr[0] = (unsigned int)(long)g_draw_buf[1];

    blt.src_buf.stride[0] = draw_pitch;
    blt.src_buf.size.width = draw_w;
    blt.src_buf.size.height = draw_h;
    blt.src_buf.format =  draw_fmt;

    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = (unsigned int)(long)g_frame_buf[g_fb_id];
    blt.dst_buf.stride[0] = disp_pitch;
    blt.dst_buf.size.width = disp_w;
    blt.dst_buf.size.height = disp_h;
    blt.dst_buf.format = disp_fmt;

    /* rotation */
    switch (drv->rotated) {
    case LV_DISP_ROT_NONE:
        blt.ctrl.flags = MPP_ROTATION_0;
        break;
    case LV_DISP_ROT_90:
        blt.ctrl.flags = MPP_ROTATION_90;
        break;
    case LV_DISP_ROT_180:
        blt.ctrl.flags = MPP_ROTATION_180;
        break;
    case LV_DISP_ROT_270:
        blt.ctrl.flags = MPP_ROTATION_270;
        break;
    default:
        break;
    };

    ret = mpp_ge_bitblt(g_ge, &blt);
    if (ret < 0) {
        LV_LOG_ERROR("bitblt fail");
        return;
    }

    ret = mpp_ge_emit(g_ge);
    if (ret < 0) {
        LV_LOG_ERROR("emit fail");
        return;
    }

    ret = mpp_ge_sync(g_ge);
    if (ret < 0) {
        LV_LOG_ERROR("sync fail");
        return;
    }

    return;
}
#endif

static void fbdev_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t *color_p)
{
    lv_disp_t * disp = _lv_refr_get_disp_refreshing();
    lv_disp_draw_buf_t * draw_buf = lv_disp_get_draw_buf(disp);

    if (!disp->driver->direct_mode || draw_buf->flushing_last) {
#ifdef USE_DRAW_BUF
        aicos_dcache_clean_invalid_range((ulong *)color_p, (ulong)ALIGN_UP(draw_buf_len(), CACHE_LINE_SIZE));
        // copy draw buf to display buffer
        disp_draw_buf(drv, color_p);
#ifdef AIC_PAN_DISPLAY
        int index = 0;
        index = g_fb_id;
        g_fb_id++;
        if (g_fb_id >= g_fb_num)
            g_fb_id = 0;
        mpp_fb_ioctl(g_fb, AICFB_PAN_DISPLAY , &index);
        mpp_fb_ioctl(g_fb, AICFB_WAIT_FOR_VSYNC, 0);
#endif
#else
#ifdef AIC_PAN_DISPLAY
        int index = 0;
        if (disp->driver->direct_mode)
            aicos_dcache_clean_invalid_range((ulong *)color_p, (ulong)ALIGN_UP(info.smem_len * 2, CACHE_LINE_SIZE));
        else
            aicos_dcache_clean_invalid_range((ulong *)color_p, (ulong)ALIGN_UP(info.smem_len, CACHE_LINE_SIZE));

        if ((void *)color_p == (void *)info.framebuffer)
            index = 0;
        else
            index = 1;

        mpp_fb_ioctl(g_fb, AICFB_PAN_DISPLAY , &index);
        mpp_fb_ioctl(g_fb, AICFB_WAIT_FOR_VSYNC, 0);
#else
        aicos_dcache_clean_invalid_range((ulong *)color_p, (ulong)ALIGN_UP(info.smem_len, CACHE_LINE_SIZE));
#endif
#endif

#ifndef AIC_DISP_COLOR_BLOCK
        /* enable display power after flush first frame */
        static bool first_frame = true;

        if (first_frame) {
            mpp_fb_ioctl(g_fb, AICFB_POWERON, 0);
            first_frame = false;
        }
#endif

#ifndef USE_DRAW_BUF
#ifdef AIC_PAN_DISPLAY
        if (drv->direct_mode == 1) {
            for (int i = 0; i < disp->inv_p; i++) {
                if (disp->inv_area_joined[i] == 0) {
                    sync_disp_buf(drv, color_p, &disp->inv_areas[i]);
                }
            }
            if (disp->inv_p) {
                int ret = mpp_ge_emit(g_ge);
                if (ret < 0) {
                    LV_LOG_ERROR("emit fail\n");
                    return;
                }

                ret = mpp_ge_sync(g_ge);
                if (ret < 0) {
                    LV_LOG_ERROR("sync fail\n");
                    return;
                }
            }
        }
#endif
#endif
        cal_frame_rate();
        lv_disp_flush_ready(drv);
    } else {
        lv_disp_flush_ready(drv);
    }
}

void lv_port_disp_init(void)
{
    int width, height;
    void *buf1 = NULL;
    void *buf2 = NULL;

    int result;
    g_fb = mpp_fb_open();
    if (g_fb == 0) {
        LV_LOG_ERROR("can't find aic framebuffer device!");
        return;
    }

    result = mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &info);
    if (result) {
        LV_LOG_ERROR("get device fb info failed!");
        return;
    }

    fbdev_open();
    draw_buf_size(&width, &height);

    g_ge = mpp_ge_open();
    if (!g_ge) {
        LV_LOG_ERROR("ge open fail\n");
        return;
    }

#ifdef USE_DRAW_BUF
    buf1 = (void *)g_draw_buf[0];
    if (g_frame_buf[1]) {
        g_fb_id = 1;
        g_fb_num = 2;
    }
#else
    if (g_frame_buf[1]) {
        buf1 = (void *)g_frame_buf[1];
        buf2 = (void *)g_frame_buf[0];
    } else {
        buf1 = (void *)g_frame_buf[0];
    }
#endif

    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, width * height);
    lv_disp_drv_init(&disp_drv);

    LV_LOG_INFO("info.bits_per_pixel: %d\n", info.bits_per_pixel);
    LV_LOG_INFO("info.width: %d, info.height: %d\n", info.width, info.height);

    /*Set a display buffer*/
    disp_drv.draw_buf = &disp_buf;

    /*Set the resolution of the display*/
    disp_drv.hor_res = width;
    disp_drv.ver_res = height;
    disp_drv.full_refresh = 0;
    disp_drv.direct_mode = 1;
    disp_drv.flush_cb = fbdev_flush;
    disp_drv.draw_ctx_init = lv_draw_aic_ctx_init;
    disp_drv.draw_ctx_deinit = lv_draw_aic_ctx_deinit;
    disp_drv.draw_ctx_size = sizeof(lv_draw_aic_ctx_t);

    /* when define USE_DRAW_BUF, disp_drv.rotated can be
      LV_DISP_ROT_90/LV_DISP_ROT_180/LV_DISP_ROT_270
    */
    //disp_drv.rotated = LV_DISP_ROT_90;

    /*Finally register the driver*/
#ifdef AIC_LVGL_METER_DEMO
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    disp->bg_opa = LV_OPA_TRANSP;
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_0, 0);
#else
    lv_disp_drv_register(&disp_drv);
#endif

#ifdef KERNEL_RTTHREAD
    /* run touch panel */
    tpc_run("gt911", info.width, info.height);
#endif
}

void lv_port_disp_exit(void)
{
    if (g_ge) {
        mpp_ge_close(g_ge);
        g_ge = NULL;
    }

    if (g_fb) {
        mpp_fb_close(g_fb);
        g_fb = NULL;
    }
    fbdev_close();
}

int fbdev_draw_fps()
{
    return (int)draw_fps;
}

int disp_is_swap(void)
{
    if (disp_drv.rotated == LV_DISP_ROT_90 || disp_drv.rotated == LV_DISP_ROT_270)
        return 1;
    else
        return 0;
}
