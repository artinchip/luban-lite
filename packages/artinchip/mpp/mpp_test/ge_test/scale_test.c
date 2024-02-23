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

#include <aic_core.h>

#include "./public/bmp.h"
#include "./public/ge_fb.h"
#include "./public/ge_mem.h"

#define SCALE_IMAGE     "/sdcard/ge_test/image/scale.bmp"

#define LOGE(fmt, ...) aic_log(AIC_LOG_ERR, "E", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) aic_log(AIC_LOG_WARN, "W", fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) aic_log(AIC_LOG_INFO, "I", fmt, ##__VA_ARGS__)

/* ge scale test */
#define SCALE_WIDTH  0
#define SCALE_HEIGHT 1
#define SCALE_W_H    2

#define ONE_PIXEL 1.0

static void usage(char *app)
{
    printf("Usage: %s [Options]: \n", app);

    printf("\t    --Framebuffer uses double buffer in this test, ensure that fb0 is properly configure\n\n");
    printf("\t-t, --type, Select scale type (default 2)\n");
    printf("\t    --type, 0: wide stretch, 1: height stretch, 2: width and height stretch\n\n");
    printf("\t-u, --usage\n");
}

static long long int str2int(char *_str)
{
    if (_str == NULL) {
        pr_err("The string is empty!\n");
        return -1;
    }

    if (strncmp(_str, "0x", 2)) {
        return (long long)atoi(_str);
    } else {
        return (long long)strtoll(_str, NULL, 16);
    }
}

static int bitblt_run(struct mpp_ge *ge, struct ge_bitblt *blt)
{
    int ret = -1;

    ret = mpp_ge_bitblt(ge, blt);
    if (ret < 0) {
        LOGE("bitblt task failed\n");
        return ret;
    }

    ret = mpp_ge_emit(ge);
    if (ret < 0) {
        LOGE("emit task failed\n");
        return ret;
    }

    ret = mpp_ge_sync(ge);
    if (ret < 0) {
        LOGE("ge sync fail\n");
        return ret;
    }

    return 0;
}

/* use ge fill rectangle to clear screen data */
static int clear_screen(struct mpp_ge *ge, struct ge_fb_info *fb_info)
{
    int ret = -1;
    unsigned int fb_phy = 0;
    struct ge_fillrect fill = {0};

    fill.ctrl.flags = 0;
    fill.type = GE_NO_GRADIENT;
    fill.start_color = 0;
    fill.end_color = 0;

    fb_phy = fb_get_cur_frame(fb_info);
    fill.dst_buf.buf_type = MPP_PHY_ADDR;
    fill.dst_buf.phy_addr[0] = fb_phy;
    fill.dst_buf.stride[0] = fb_info->fb_data.stride;
    fill.dst_buf.size.width = fb_info->fb_data.width;
    fill.dst_buf.size.height = fb_info->fb_data.height;
    fill.dst_buf.format = fb_info->fb_data.format;
    fill.dst_buf.crop_en = 0;

    ret =  mpp_ge_fillrect(ge, &fill);
    if (ret < 0) {
        LOGE("ge fillrect fail\n");
        return -1;
    }
    ret = mpp_ge_emit(ge);
    if (ret < 0) {
        LOGE("ge emit fail\n");
        return -1;
    }
    ret = mpp_ge_sync(ge);
    if (ret < 0) {
        LOGE("ge sync fail\n");
        return -1;
    }

    return 0;
}

static int set_display_pos(struct ge_fb_info *fb_info, int width, int height, int *pos_x, int *pos_y)
{
    if (width < 4 || height < 4)
        return -1;

    if (width > fb_info->fb_data.width)
        width = fb_info->fb_data.width;

    if (height > fb_info->fb_data.height)
        height = fb_info->fb_data.height;

    *pos_x = (fb_info->fb_data.width / 2 - width / 2);
    *pos_y = (fb_info->fb_data.height / 2 - height / 2);

    return 0;
}

static void display_set(struct ge_bitblt *blt, struct ge_fb_info *fb_info, struct ge_buf *buffer,
                        int width, int height)
{
    int ret = -1;
    int x = 0;
    int y = 0;
    unsigned int fb_phy = 0;

    memset(blt, 0, sizeof(struct ge_bitblt));

    /* Src layer set */
    memcpy(&blt->src_buf, &buffer->buf, sizeof(struct mpp_buf));
    blt->src_buf.crop_en = 1;
    blt->src_buf.crop.x = 0;
    blt->src_buf.crop.y = 0;
    blt->src_buf.crop.width = width;
    blt->src_buf.crop.height = height;

    fb_phy = fb_get_cur_frame(fb_info);
    ret = set_display_pos(fb_info, width, height, &x, &y);
    if (ret < 0)
        LOGE("set_display_pos failed.\n");

    /* Dst layer set */
    blt->dst_buf.buf_type = MPP_PHY_ADDR;
    blt->dst_buf.phy_addr[0] = fb_phy;
    blt->dst_buf.stride[0] = fb_info->fb_data.stride;
    blt->dst_buf.size.width = fb_info->fb_data.width;
    blt->dst_buf.size.height = fb_info->fb_data.height;
    blt->dst_buf.format = fb_info->fb_data.format;
    blt->dst_buf.crop_en = 1;
    blt->dst_buf.crop.x = x;
    blt->dst_buf.crop.y = y;
    blt->dst_buf.crop.width = width;
    blt->dst_buf.crop.height = height;
}

static int display(struct mpp_ge *ge, struct ge_bitblt *blt, struct ge_fb_info *fb_info, struct ge_buf *buffer,
                   int width, int height)
{
    int ret = 0;

    display_set(blt, fb_info, buffer, width, height);

    ret = bitblt_run(ge, blt);
    if (ret < 0) {
        LOGE("bitblt_run failed\n");
        return ret;
    }

    return ret;
}

/* Calculate the size that should be displayed and processed */
static int resize_image(double *width, double *height, int scale_type, double num,
                        struct bmp_header *bmp_head, struct ge_fb_info *fb_info)
{
    double aspect_ration = 0;
    double new_height = 0;
    double new_width = 0;
    float width_scale = 0;
    float height_scale = 0;

    aspect_ration = (*width) / (*height);
    new_height = *height;
    new_width = *width;
    if (scale_type == SCALE_W_H) {
        if (*width > *height) {
            new_width = *width + num;
            new_height = new_width / aspect_ration;
        } else {
            new_height = *height + num;
            new_width = new_height * aspect_ration;
        }
    } else if (scale_type == SCALE_WIDTH) {
        new_width += num;
    } else {
        new_height += num;
    }

    width_scale = (int)new_width / (float)bmp_head->width;
    height_scale = (int)new_height / (float)abs(bmp_head->height);
    /* limit the width and height to the minimum size supported by the scale function */
    if (width_scale < (float)1.0 / 16 || height_scale < (float)1.0 / 16)
        return 1;
    if (width_scale > (float)16 || height_scale > (float)16)
        return 1;

    new_width = new_width >= fb_info->fb_data.width ? fb_info->fb_data.width : new_width;
    new_height = new_height >= fb_info->fb_data.height ? fb_info->fb_data.height : new_height;
    new_width = new_width < 4 ? 4 : new_width;
    new_height = new_height < 4 ? 4 : new_height;

    *width = new_width;
    *height = new_height;

    if ((int)*width <= 4 || (int)*height <= 4 ||
        (int)*width == fb_info->fb_data.width || (int)*height == fb_info->fb_data.height)
        return 1;

    return 0;
}

static void ge_scale_set(struct ge_bitblt *blt, struct ge_buf *src_buffer, struct ge_buf *dst_buffer,
                         int width, int height)
{
    memset(blt, 0, sizeof(struct ge_bitblt));
    /* src layer settings  */
    memcpy(&blt->src_buf, &src_buffer->buf, sizeof(struct mpp_buf));
    blt->src_buf.crop_en = 0;
    /* dst layer settings */
    memcpy(&blt->dst_buf, &dst_buffer->buf, sizeof(struct mpp_buf));
    blt->dst_buf.crop_en = 1;
    blt->dst_buf.crop.x = 0;
    blt->dst_buf.crop.y = 0;
    blt->dst_buf.crop.height = height;
    blt->dst_buf.crop.width = width;
}

static int scale_run(struct mpp_ge *ge, struct ge_bitblt *blt,
                     struct bmp_header *bmp_head, struct ge_fb_info *fb_info,
                     struct ge_buf *src_buffer, struct ge_buf *dst_buffer, int scale_type)
{
    int ret = 0;
    int shrink_en = 1;
    double width = 0;
    double height = 0;

    /* Obtain the width and height of the original image */
    width = bmp_head->width;
    height = abs(bmp_head->height);

    while(1)
    {
        ge_scale_set(blt, src_buffer, dst_buffer, width, height);
        ret = bitblt_run(ge, blt);
        if (ret < 0) {
            LOGE("bitblt_run error\n");
            return -1;
        }

        ret = display(ge, blt, fb_info, dst_buffer, width, height);
        if (ret < 0) {
            LOGE("display error\n");
            return -1;
        }
        fb_start_and_wait(fb_info);
        fb_swap_frame(fb_info);

        /* Determine whether to perform a reduction or enlargement operation */
        if (shrink_en == 1)
        {
            ret = resize_image(&width, &height, scale_type, ONE_PIXEL,
                               bmp_head, fb_info);
            if (ret < 0) {
                LOGE("resize_image task failed\n");
                return -1;
            }
            if (ret == 1)
                shrink_en = 0;
        } else {
            ret = resize_image(&width, &height, scale_type, - ONE_PIXEL,
                               bmp_head, fb_info);
            if (ret < 0) {
                LOGE("resize_image task failed\n");
                return -1;
            }
            if (ret == 1)
                return 0;
        }

        aicos_msleep(30);
        ret = clear_screen(ge, fb_info);
        if (ret < 0) {
            LOGE("clear_screen error\n");
            return -1;
        }
    }
    return 0;
}

static void scale_test(int argc, char **argv)
{
    int ret = -1;
    int scale_type = 0;

    int bmp_fd = -1;
    enum mpp_pixel_format bmp_fmt = 0;
    struct mpp_ge *ge = NULL;
    struct ge_bitblt blt = {0};
    struct ge_fb_info *fb_info = NULL;
    struct ge_buf *bmp_buffer = NULL;
    struct ge_buf *dst_buffer = NULL;
    struct bmp_header bmp_head = {0};

    const char sopts[] = "ut:";
    const struct option lopts[] = {
        {"usage",   no_argument,       NULL, 'u'},
        {"type",    required_argument, NULL, 't'},
        {0, 0, 0, 0}
    };
    optind = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 't':
            scale_type = str2int(optarg);
            if ((scale_type > 2) || (scale_type < 0)) {
                printf("scale_type invalid, please set against\n");
                return;
            }
            break;
        case 'u':
            usage(argv[0]);
            return;
        default:
            LOGE("Invalid parameter: %#x\n", ret);
            return;
        }
    }

    ge = mpp_ge_open();
    if (!ge) {
        LOGE("open ge device error\n");
        goto EXIT;
    }

    fb_info = fb_open();

    bmp_fd = bmp_open(SCALE_IMAGE, &bmp_head);
    if (bmp_fd < 0) {
        LOGE("open bmp error, path = %s\n", SCALE_IMAGE);
        goto EXIT;
    }

    bmp_fmt = bmp_get_fmt(&bmp_head);
    bmp_buffer = ge_buf_malloc(bmp_head.width, abs(bmp_head.height), bmp_fmt);
    if (bmp_buffer == NULL) {
        LOGE("malloc bmp_buffer error\n");
        goto EXIT;
    }

    dst_buffer = ge_buf_malloc(fb_info->fb_data.width, fb_info->fb_data.height, fb_info->fb_data.format);
    if (dst_buffer == NULL) {
        LOGE("malloc dst_buffer error\n");
        goto EXIT;
    }

    ret = bmp_read(bmp_fd, (void *)((uintptr_t)bmp_buffer->buf.phy_addr[0]), &bmp_head);
    if (ret < 0) {
        LOGE("bmp_read error\n");
        goto EXIT;
    }
    ge_buf_clean_dcache(bmp_buffer);

    /* The ge operation here does not involve the CPU and does not required cleaning the dcache */
    ret = scale_run(ge, &blt, &bmp_head, fb_info,
                    bmp_buffer, dst_buffer, scale_type);
    if (ret < 0) {
        LOGE("scale_run task failed: %d\n", ret);
        goto EXIT;
    }

EXIT:
    if (bmp_fd > 0)
        bmp_close(bmp_fd);

    if (ge)
        mpp_ge_close(ge);

    if (fb_info)
        fb_close(fb_info);

    if (bmp_buffer)
        ge_buf_free(bmp_buffer);

    if (dst_buffer)
        ge_buf_free(dst_buffer);
}
MSH_CMD_EXPORT_ALIAS(scale_test, ge_scale, ge scale test);
