/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <stddef.h>
#include "mpp_mem.h"
#include "mpp_ge.h"
#include "mpp_decoder.h"
#include "mpp_list.h"
#include "mpp_fb.h"
#include "aic_dec.h"
#include "lv_fbdev.h"
#include "lv_port_disp.h"

static int g_buf_num = 1;
char *g_frame_buf[MAX_FRAME_NUM] = { NULL };
char *g_draw_buf[2] = { NULL };

static struct aicfb_screeninfo g_info;

#ifdef USE_DRAW_BUF
#define DRAW_BUF_STRIDE    ((DRAW_BUF_WIDTH * (LV_COLOR_DEPTH / 8) + 7) & (~7))
#define DRAW_BUF_SWAP_STRIDE    ((DRAW_BUF_HEIGHT * (LV_COLOR_DEPTH / 8) + 7) & (~7))
#define DRAW_BUF_SIZE      (DRAW_BUF_STRIDE * DRAW_BUF_HEIGHT)

#define ALIGN_1024B(x) ((x+1023) & (~1023))

static struct framebuf_info buf_info[2] = { 0 };

static int draw_buf_alloc(int id)
{
    buf_info[id].addr = (unsigned long)aicos_malloc(MEM_CMA, DRAW_BUF_SIZE + 1023);
    if (!buf_info[id].addr) {
        // goto out;
    } else {
        buf_info[id].align_addr = ALIGN_1024B(buf_info[id].addr);
        buf_info[id].size = DRAW_BUF_SIZE;
        g_draw_buf[id] = (char *)((unsigned long)buf_info[id].align_addr);
        aicos_dcache_clean_invalid_range((ulong *)((ulong)buf_info[id].align_addr),
                                        ALIGN_UP(DRAW_BUF_SIZE, CACHE_LINE_SIZE));
    }
    return 0;
}

static int draw_buf_free(int id)
{
    if (g_draw_buf[id] != 0) {
        aicos_free(MEM_CMA, (void*)(unsigned long)buf_info[id].align_addr);
        buf_info[id].align_addr = 0;
        g_draw_buf[id] = 0;
    }
    return 0;
}

#endif

void fbdev_get_info(void)
{
    struct mpp_fb *g_fb = mpp_fb_open();
    if (g_fb) {
        mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &g_info);
        g_frame_buf[0] = (char *)g_info.framebuffer;
#ifdef AIC_PAN_DISPLAY
        int fb_size = g_info.height * g_info.stride;
        g_frame_buf[1] = (char *)g_info.framebuffer + fb_size;
#endif
        mpp_fb_close(g_fb);
    }
    return;
}
int fbdev_open(void)
{
    // only use one draw buffer right now
    fbdev_get_info();
    g_buf_num = 1;
#ifdef USE_DRAW_BUF
    int i;
    for (i = 0; i < g_buf_num; i++)
        draw_buf_alloc(i);
#endif
    return 0;
}

int fbdev_get_size(int *width, int *height)
{
    *width = g_info.width;
    *height = g_info.height;
    return 0;
}

enum mpp_pixel_format fbdev_get_fmt(void)
{
    if (g_info.bits_per_pixel == 32)
        return MPP_FMT_ARGB_8888;
    else if (g_info.bits_per_pixel == 24)
        return MPP_FMT_RGB_888;
    else if (g_info.bits_per_pixel == 16)
        return MPP_FMT_RGB_565;

    return MPP_FMT_ARGB_8888;
}

int fbdev_get_bpp(void)
{
    return g_info.bits_per_pixel;
}

int fbdev_get_pitch(void)
{
    return g_info.stride;
}

int draw_buf_size(int *width, int *height)
{
#ifdef USE_DRAW_BUF
    *width = DRAW_BUF_WIDTH;
    *height = DRAW_BUF_HEIGHT;
#else
    *width = g_info.width;
    *height = g_info.height;
#endif
    return 0;
}

enum mpp_pixel_format draw_buf_fmt(void)
{
    if (g_info.bits_per_pixel == 32)
        return MPP_FMT_ARGB_8888;
    else if (g_info.bits_per_pixel == 24)
        return MPP_FMT_RGB_888;
    else if (g_info.bits_per_pixel == 16)
        return MPP_FMT_RGB_565;

    return MPP_FMT_ARGB_8888;
}

int draw_buf_bpp(void)
{
    return g_info.bits_per_pixel;
}

int draw_buf_pitch(void)
{
#ifdef USE_DRAW_BUF
    if(disp_is_swap())
        return DRAW_BUF_SWAP_STRIDE;
    else
        return DRAW_BUF_STRIDE;
#else
    return g_info.stride;
#endif
}

int draw_buf_len(void)
{
#ifdef USE_DRAW_BUF
    return DRAW_BUF_SIZE;
#else
    return g_info.smem_len;
#endif
}

void fbdev_close(void)
{
#ifdef USE_DRAW_BUF
    int i;
    for (i = 0; i < g_buf_num; i++)
        draw_buf_free(i);
#endif
}
