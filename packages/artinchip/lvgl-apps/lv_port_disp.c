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

static int draw_fps = 0;
static struct mpp_ge *g_ge = NULL;
static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static struct mpp_fb *g_fb = 0;
static struct aicfb_screeninfo info;

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
        start.tv_usec = aic_get_time_us();
    }

    end.tv_sec = 0;
    end.tv_usec = aic_get_time_us();
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

    ret = mpp_ge_emit(g_ge);
    if (ret < 0) {
        LV_LOG_ERROR("emit fail\n");
        return;
    }

    ret = mpp_ge_sync(g_ge);
    if (ret < 0) {
        LV_LOG_ERROR("sync fail\n");
        return;
    }

    return;
}

static void fbdev_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t *color_p)
{
    int index = 0;
    lv_disp_t * disp = _lv_refr_get_disp_refreshing();
    lv_disp_draw_buf_t * draw_buf = lv_disp_get_draw_buf(disp);

    if (!disp->driver->direct_mode || draw_buf->flushing_last) {
        if (disp->driver->direct_mode)
            aicos_dcache_clean_invalid_range((unsigned long *)info.framebuffer, (unsigned long)info.smem_len * 2);
        else
            aicos_dcache_clean_invalid_range((unsigned long *)color_p, (unsigned long)info.smem_len);

        if ((void *)color_p == (void *)info.framebuffer)
            index = 0;
        else
            index = 1;

        mpp_fb_ioctl(g_fb, AICFB_PAN_DISPLAY , &index);

#ifndef AIC_DISP_COLOR_BLOCK
        /* enable display power after flush first frame */
        static bool first_frame = true;

        if (first_frame) {
            mpp_fb_ioctl(g_fb, AICFB_POWERON, 0);
            first_frame = false;
        }
#endif

        mpp_fb_ioctl(g_fb, AICFB_WAIT_FOR_VSYNC, 0);

        if (drv->direct_mode == 1) {
            for (int i = 0; i < disp->inv_p; i++) {
                if (disp->inv_area_joined[i] == 0) {
                    sync_disp_buf(drv, color_p, &disp->inv_areas[i]);
                }
            }
        }

        cal_frame_rate();
        lv_disp_flush_ready(drv);
    }
    else {
        lv_disp_flush_ready(drv);
    }
}

void lv_port_disp_init(void)
{
    void *buf1 = NULL;
    void *buf2 = NULL;
    uint32_t fb_size;
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

    g_ge = mpp_ge_open();
    if (!g_ge) {
        LV_LOG_ERROR("ge open fail\n");
        return;
    }

    fb_size = info.height * info.stride;
#ifdef AIC_FB_ROTATE_EN
    u32 fb_rotate = AIC_FB_ROTATE_DEGREE;
    if (fb_rotate == 90 || fb_rotate == 270) {
        info.stride = ALIGN_UP(info.width * info.bits_per_pixel / 8, 8);
        fb_size = info.height * info.stride;
    }
#endif

    LV_ASSERT(info.bits_per_pixel == 16 || info.bits_per_pixel == 24
              || info.bits_per_pixel == 32);

    buf1 = (void *)info.framebuffer;
    buf2 = (void *)((uint8_t *)info.framebuffer + fb_size);

    lv_disp_draw_buf_init(&disp_buf, buf2, buf1,
                          info.width * info.height);
    lv_disp_drv_init(&disp_drv);

    LV_LOG_INFO("info.bits_per_pixel: %d\n", info.bits_per_pixel);
    LV_LOG_INFO("info.width: %d, info.height: %d\n", info.width, info.height);

    /*Set a display buffer*/
    disp_drv.draw_buf = &disp_buf;

    /*Set the resolution of the display*/
    disp_drv.hor_res = info.width;
    disp_drv.ver_res = info.height;
    disp_drv.full_refresh = 0;
    disp_drv.direct_mode = 1;
    disp_drv.flush_cb = fbdev_flush;
    disp_drv.draw_ctx_init = lv_draw_aic_ctx_init;
    disp_drv.draw_ctx_deinit = lv_draw_aic_ctx_deinit;
    disp_drv.draw_ctx_size = sizeof(lv_draw_aic_ctx_t);

#ifdef AIC_FB_ROTATE_EN
    disp_drv.full_refresh = 1;
    disp_drv.direct_mode = 0;
    disp_drv.rotated = AIC_FB_ROTATE_DEGREE / 90;
#endif

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
}

int fbdev_draw_fps()
{
    return (int)draw_fps;
}
