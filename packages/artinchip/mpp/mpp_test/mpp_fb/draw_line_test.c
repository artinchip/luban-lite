/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ZeQuan Liang <zequan.liang@artinchip.com>
 */

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <aic_core.h>
#include "mpp_fb.h"
#include "mpp_log.h"

struct frame_buffer_info
{
    int frame_buffer_format;
    int frame_buffer_width;
    int frame_buffer_height;
    unsigned char *frame_buffer;
};

struct drwa_line {
    int x1;
    int y1;
    int x2;
    int y2;
    int color;
    int width;
};

void draw_pixel_argb8888(int x, int y, int color, struct frame_buffer_info *info)
{
    int offset = (y * info->frame_buffer_width + x) * 4;
    unsigned char *pixel = &info->frame_buffer[offset];

    *(pixel + 0) = (color >> 0) & 0xFF;  // B
    *(pixel + 1) = (color >> 8) & 0xFF;  // G
    *(pixel + 2) = (color >> 16) & 0xFF; // R
    *(pixel + 3) = (color >> 24) & 0xFF; // A
}

void draw_pixel_rgb888(int x, int y, int color, struct frame_buffer_info *info)
{
    int offset = (y * info->frame_buffer_width + x) * 3;
    unsigned char *pixel = &info->frame_buffer[offset];

    *(pixel + 0) = (color >> 0) & 0xFF;  // B
    *(pixel + 1) = (color >> 8) & 0xFF;  // G
    *(pixel + 2) = (color >> 16) & 0xFF; // R
}


void draw_pixel_rgb565(int x, int y, int color, struct frame_buffer_info *info)
{
    int offset = (y * info->frame_buffer_width + x) * 2;
    unsigned short *pixel = (unsigned short *)&info->frame_buffer[offset];
    unsigned short blue =  (color >> 3) & 0x1F;
    unsigned short green = (color >> 10) & 0x3F;
    unsigned short red =   (color >> 19) & 0x1F;

    *pixel = (red << 11) | (green << 5) | blue;;
}

void draw_pixel_color(int x, int y, int color, struct frame_buffer_info *info)
{
    switch (info->frame_buffer_format)
    {
        case MPP_FMT_ARGB_8888:
            draw_pixel_argb8888(x, y, color, info);
            break;
        case MPP_FMT_RGB_888:
            draw_pixel_rgb888(x, y, color, info);
            break;
        case MPP_FMT_RGB_565:
            draw_pixel_rgb565(x, y, color, info);
            break;
        default:
            printf("The function to change the format and draw pixels has not been implemented yet.\n");
        break;
    }
}

void draw_wide_pixel(int x, int y, int width, int color,
                     struct frame_buffer_info *info)
{
    int halfWidth = (width - 1) / 2;

    for (int i = -halfWidth; i <= halfWidth; i++) {
        for (int j = -halfWidth; j <= halfWidth; j++) {
                int drawX = x + i;
                int drawY = y + j;
                /* Check if the pixel is within the frame_buffer boundaries */
                if (drawX >= 0 && drawX < info->frame_buffer_width && drawY >= 0 && drawY < info->frame_buffer_height) {
                    draw_pixel_color(drawX, drawY, color, info);
                }
        }
    }
}

/* Bresenham line drawing algorithm with line width and frame_buffer boundary checks */
void draw_line(struct drwa_line *line, struct frame_buffer_info *info)
{
    int dx = abs(line->x2 - line->x1), sx = line->x1 < line->x2 ? 1 : -1;
    int dy = -abs(line->y2 - line->y1), sy = line->y1 < line->y2 ? 1 : -1;
    int err = dx + dy, e2; /* error value e_xy */
    int x = line->x1, y = line->y1;

    while (1) {
        /* Draw the wide pixel with boundary checks */
        draw_wide_pixel(x, y, line->width, line->color, info);

        /* Check for end point */
        if (x == line->x2 && y == line->y2) {
            break;
        }
        e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y += sy;
        }
    }
}

static void draw_line_test(int argc, char **argv)
{
    int ret = -1;
    struct mpp_fb *fb;
    struct aicfb_screeninfo fb_info;

    fb = mpp_fb_open();
    if (!fb) {
        printf("mpp fb open failed\n");
    }

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO , &fb_info);
    if (ret) {
        printf("mpp_fb_ioctl ops failed\n");
    }

    memset(fb_info.framebuffer, 0, fb_info.height * fb_info.stride);

    struct frame_buffer_info info;
    info.frame_buffer_format = fb_info.format;
    info.frame_buffer_height = fb_info.height;
    info.frame_buffer_width = fb_info.width;
    info.frame_buffer = (uint8_t *)fb_info.framebuffer;

    struct drwa_line line;
    line.x1 = 0;
    line.y1 = 50;
    line.x2 = 100;
    line.y2 = 50;
    line.color = 0xff0000ff;
    line.width = 100;

    draw_line(&line, &info);

    aicos_dcache_clean_invalid_range((unsigned long *)fb_info.framebuffer, fb_info.smem_len);

    if (fb)
        mpp_fb_close(fb);
}
MSH_CMD_EXPORT_ALIAS(draw_line_test, draw_line, draw line test);
