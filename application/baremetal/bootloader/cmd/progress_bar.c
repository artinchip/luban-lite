/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <aic_core.h>
#include <aic_hal.h>
#include <aic_common.h>
#include <aic_errno.h>

#include <string.h>
#include <console.h>
#include <artinchip_fb.h>

#undef ALIGN_DOWM
#define ALIGN_DOWM(x, align)        ((x) & ~(align - 1))

#define BAR_BACKGROUND_COLOR        0x00, 0xA2, 0xE9
#define BAR_FILL_COLOR              0x18, 0xD4, 0x0A

#define WIDTH_SPLIT_NUMERATOR       5
#define WIDTH_SPLIT_DENOMINATOR     6

#define BAR_HEIGHT                  35

#define SPLIT_WIDTH(w)              \
    ((w) * WIDTH_SPLIT_NUMERATOR / WIDTH_SPLIT_DENOMINATOR)

#define PROGRESS_BAR_HELP                                       \
    "display progress bar:\n"                                   \
    "  progress_bar  <value>\n"                                 \
    "    value: precentage of progress bar\n"                   \
    "  e.g.: \n"                                                \
    "    progress_bar 0\n"                                      \
    "    progress_bar 99\n"                                     \
    "    progress_bar 100\n"

static void progress_bar_help(void)
{
    puts(PROGRESS_BAR_HELP);
}

void aicfb_draw_rect(struct aicfb_screeninfo *info,
                    unsigned int x, unsigned y,
                    unsigned int width, unsigned int height,
                    u8 red, u8 green, u8 blue)
{
    unsigned long dcache_size, fb_dcache_start;
    int pbytes = info->bits_per_pixel / 8;
    unsigned char *fb;
    int i, j;

    fb = (unsigned char *)(info->framebuffer + y * info->stride + x * pbytes);
    fb_dcache_start = ALIGN_DOWM((unsigned long)fb, ARCH_DMA_MINALIGN);

    switch (info->format) {
    case MPP_FMT_ARGB_8888:
        for (i = 0; i < height; ++i) {
            for (j = 0; j < width; j++) {
                *(fb++) = blue;
                *(fb++) = green;
                *(fb++) = red;
                *(fb++) = 0xFF;
            }
            fb += info->stride - width * pbytes;
        }
        break;
    case MPP_FMT_RGB_888:
        for (i = 0; i < height; ++i) {
            for (j = 0; j < width; j++) {
                *(fb++) = blue;
                *(fb++) = green;
                *(fb++) = red;
            }
            fb += info->stride - width * pbytes;
        }
        break;
    case MPP_FMT_RGB_565:
        for (i = 0; i < height; ++i) {
            for (j = 0; j < width; j++) {
                *(uint16_t *)fb = ((red >> 3) << 11)
                        | ((green >> 2) << 5)
                        | (blue >> 3);
                fb += sizeof(uint16_t) / sizeof(*fb);
            }
            fb += info->stride - width * pbytes;
        }
        break;
    default:
        printf("%s: unsupported fb format %d", __func__, info->format);
        return;
    };

    dcache_size = ALIGN_UP((unsigned long)fb - fb_dcache_start,
            ARCH_DMA_MINALIGN);
    aicos_dcache_clean_range((unsigned long *)fb_dcache_start, dcache_size);
}

void aicfb_draw_bar(unsigned int value)
{
    struct aicfb_screeninfo info;
    unsigned int x, y, width, height;
    static bool power_on = false;

    aicfb_probe();
    aicfb_ioctl(AICFB_GET_SCREENINFO, &info);

    if (!power_on) {
        memset(info.framebuffer, 0x00, info.smem_len);
        aicos_dcache_clean_range((unsigned long *)info.framebuffer, info.smem_len);

        aicfb_ioctl(AICFB_POWERON, 0);
        power_on = true;
    }

    width  = SPLIT_WIDTH(info.width);
    height = BAR_HEIGHT;
    x    = (info.width - width) / 2;
    y    = (info.height - height) / 2;

    if (value == 0) {
        aicfb_draw_rect(&info, x, y, width, height, BAR_BACKGROUND_COLOR);
        return;
    }

    if (value < 100)
        width = width * value / 100;

    aicfb_draw_rect(&info, x, y, width, height, BAR_FILL_COLOR);
}

static int do_progress_bar(int argc, char *argv[])
{
    unsigned int intf;

    if (argc != 2)
        goto help;

    intf = strtol(argv[1], NULL, 0);
    aicfb_draw_bar(intf);
    return 0;

help:
    progress_bar_help();
    return 0;
}

CONSOLE_CMD(progress_bar, do_progress_bar, "Display progress bar.");
