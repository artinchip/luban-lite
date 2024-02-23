/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ZeQuan Liang <zequan.liang@artinchip.com>
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <aic_core.h>

#include "ge_mem.h"
#include "aic_hal.h"
#include "aic_log.h"

/* input ge stride requires 8-bit alignment */
static void ge_cal_stride(enum mpp_pixel_format fmt, int input_width, int stride[])
{
    switch (fmt) {
    case MPP_FMT_ARGB_8888:
    case MPP_FMT_ABGR_8888:
    case MPP_FMT_RGBA_8888:
    case MPP_FMT_BGRA_8888:
    case MPP_FMT_XRGB_8888:
    case MPP_FMT_XBGR_8888:
    case MPP_FMT_RGBX_8888:
    case MPP_FMT_BGRX_8888:
        stride[0] = BYTE_ALIGN((input_width * 4), 8);
        stride[1] = 0;
        stride[2] = 0;
        break;
    case MPP_FMT_ARGB_4444:
    case MPP_FMT_ABGR_4444:
    case MPP_FMT_RGBA_4444:
    case MPP_FMT_BGRA_4444:
    case MPP_FMT_RGB_565:
    case MPP_FMT_BGR_565:
        stride[0] = BYTE_ALIGN((input_width * 2), 8);
        stride[1] = 0;
        stride[2] = 0;
        break;
    case MPP_FMT_ARGB_1555:
    case MPP_FMT_ABGR_1555:
    case MPP_FMT_RGBA_5551:
    case MPP_FMT_BGRA_5551:
    case MPP_FMT_RGB_888:
    case MPP_FMT_BGR_888:
        stride[0] = BYTE_ALIGN((input_width * 3), 8);
        stride[1] = 0;
        stride[2] = 0;
        break;
    case MPP_FMT_YUV420P:
        stride[0]  = BYTE_ALIGN((input_width), 8);
        stride[1] = BYTE_ALIGN((input_width / 2), 8);
        stride[2] = BYTE_ALIGN((input_width / 2), 8);
        break;
    case MPP_FMT_NV21:
    case MPP_FMT_NV12:
        stride[0]  = BYTE_ALIGN((input_width), 8);
        stride[1] = BYTE_ALIGN((input_width), 8);
        stride[2] = 0;
        break;
    case MPP_FMT_YUV422P:
        stride[0]  = BYTE_ALIGN((input_width), 8);
        stride[1] = BYTE_ALIGN((input_width / 2), 8);
        stride[2] = BYTE_ALIGN((input_width / 2), 8);
        break;
    case MPP_FMT_NV16:
    case MPP_FMT_NV61:
        stride[0]  = BYTE_ALIGN((input_width), 8);
        stride[1] = BYTE_ALIGN((input_width), 8);
        stride[2] = 0;
        break;
    case MPP_FMT_YUYV:
    case MPP_FMT_YVYU:
    case MPP_FMT_UYVY:
    case MPP_FMT_VYUY:
        stride[0]  = BYTE_ALIGN((input_width * 2), 8);
        stride[1] = 0;
        stride[2] = 0;
        break;
    case MPP_FMT_YUV400:
        stride[0]  = BYTE_ALIGN((input_width), 8);
        stride[1] = 0;
        stride[2] = 0;
        break;
    case MPP_FMT_YUV444P:
        stride[0]  = BYTE_ALIGN((input_width), 8);
        stride[1] = BYTE_ALIGN((input_width), 8);
        stride[2] = BYTE_ALIGN((input_width), 8);
        break;
    default:
        printf("input format error\n");
        break;
    }

}

static void ge_cal_height(enum mpp_pixel_format fmt, int input_height, int height[])
{
    if ((fmt >= MPP_FMT_ARGB_8888) && (fmt <= MPP_FMT_BGRA_4444)) {
        height[0] = input_height;
        height[1] = 0;
        height[2] = 0;
        return;
    }

    switch (fmt) {
    case MPP_FMT_YUV420P:
        height[0]  = BYTE_ALIGN((input_height), 2);
        height[1] = BYTE_ALIGN((input_height / 2), 2);
        height[2] = BYTE_ALIGN((input_height / 2), 2);
        break;
    case MPP_FMT_NV21:
    case MPP_FMT_NV12:
        height[0] = BYTE_ALIGN((input_height), 2);
        height[1] = BYTE_ALIGN((input_height / 2), 2);
        height[2] = 0;
        break;
    case MPP_FMT_YUV422P:
        height[0] = BYTE_ALIGN(input_height, 2);
        height[1] = BYTE_ALIGN(input_height, 2);
        height[2] = BYTE_ALIGN(input_height, 2);
        break;
    case MPP_FMT_NV16:
    case MPP_FMT_NV61:
        height[0] = BYTE_ALIGN(input_height, 2);
        height[1] = BYTE_ALIGN(input_height, 2);
        height[2] = 0;
        break;
    case MPP_FMT_YUYV:
    case MPP_FMT_YVYU:
    case MPP_FMT_UYVY:
    case MPP_FMT_VYUY:
        height[0] = BYTE_ALIGN(input_height, 2);
        height[1] = 0;
        height[2] = 0;
        break;
    case MPP_FMT_YUV400:
        height[0] = BYTE_ALIGN((input_height), 2);
        height[1] = 0;
        height[2] = 0;
        break;
    case MPP_FMT_YUV444P:
        height[0] = BYTE_ALIGN(input_height, 2);
        height[1] = BYTE_ALIGN(input_height, 2);
        height[2] = BYTE_ALIGN(input_height, 2);
        break;
    default:
        printf("input format error\n");
        break;
    }

}

static void ge_align_phy_addr(struct ge_buf * buffer)
{
    int i = 0;

    for (i = 0; i < 3 && buffer->ori_buf[i] != NULL; i++) {
        buffer->buf.phy_addr[i] = (unsigned int)BYTE_ALIGN(((uintptr_t)buffer->ori_buf[i]), CACHE_LINE_SIZE);
    }
}

struct ge_buf * ge_buf_malloc(int width, int height, enum mpp_pixel_format fmt)
{
    int i = 0;
    int cal_height[3] = {0};
    int buf_size[3] = {0};
    int raw_buf_size = 0;

    struct ge_buf * buffer;
    buffer = (struct ge_buf *)aicos_malloc(MEM_CMA, sizeof(struct ge_buf));
    memset(buffer, 0, sizeof(struct ge_buf));

    ge_cal_stride(fmt, width, (int *)buffer->buf.stride);
    ge_cal_height(fmt, height, cal_height);

    buffer->buf.buf_type = MPP_PHY_ADDR;
    buffer->buf.size.width = width;
    buffer->buf.size.height = height;
    buffer->buf.format = fmt;

    /* alignment of the physical address of malloc */
    for (i = 0; i < 3 && buffer->buf.stride[i] != 0; i++) {
        raw_buf_size = buffer->buf.stride[i] * cal_height[i];
        buf_size[i] = raw_buf_size + CACHE_LINE_SIZE * 2;
        buffer->use_buf_size[i] = BYTE_ALIGN(raw_buf_size, CACHE_LINE_SIZE); /* actual memory size used */
    }

    for (i = 0; i < 3 && buf_size[i] != 0; i++) {
        buffer->ori_buf[i] = aicos_malloc(MEM_CMA, buf_size[i]);
        if (!buffer->ori_buf[i]) {
            printf("ge_buf_malloc fail, buf_size[%d] = %d\n", i, buf_size[i]);
            return NULL;
        }
        memset(buffer->ori_buf[i], 0, buf_size[i]);
    }

    ge_align_phy_addr(buffer);

    return buffer;
}

void ge_buf_free(struct ge_buf * buffer)
{
    int i = 0;

    for (i = 0; i < 3; i++) {
        if (!buffer->ori_buf[i])
            aicos_free(MEM_CMA, buffer->ori_buf[i]);
    }

    if (!buffer)
        aicos_free(MEM_CMA, buffer);
}

void ge_buf_clean_dcache(struct ge_buf * buffer)
{
    int i = 0;

    for (i = 0; i < 3 && buffer->buf.phy_addr[i] != 0; i++) {
        aicos_dcache_clean_invalid_range((void *)((uintptr_t)buffer->buf.phy_addr[i]), buffer->use_buf_size[i]);
    }
}
