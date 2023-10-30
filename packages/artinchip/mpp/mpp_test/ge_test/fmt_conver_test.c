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

#define DITHER_IMAGE    "/sdcard/ge_test/image/singer_alpha.bmp"

/* format conversion type */
#define RGB_TO_RGB  0
#define RGB_TO_YUV  1
#define YUV_TO_YUV  2
#define YUV_TO_RGB  3
#define SRC_DISP_REGION 0
#define DST_DISP_REGION 1

#define LOGE(fmt, ...) aic_log(AIC_LOG_ERR, "E", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) aic_log(AIC_LOG_WARN, "W", fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) aic_log(AIC_LOG_INFO, "I", fmt, ##__VA_ARGS__)

/* str to format table struct */
struct StrToFormat {
    char *str;
    int format;
};

/* format conversion */
static int table_size = 0;
static struct StrToFormat *format_table = NULL;

static void usage(char *app)
{
    printf("Usage: %s [Options], built on %s %s\n", app, __DATE__, __TIME__);
    printf("\t-m, --mode, select format conversion type (default rgbtorgb), \n"
           "\tFormat conversion supports rgbtorgb, yuvtoyuv, yuvtorgb and rgbtoyuv.\n");
    printf("\t-u, --usage\n");
    printf("\t-h, --help to see more\n\n");
}

static void help(void)
{
    printf("\r--The src layer is displayed on the left side of the screen.\n");
    printf("\r--The dst layer is displayed on the right side of the screen.\n");
    printf("\r--RGB format supports: \n");
    printf("\r--ARGB8888, ABGR8888, RGBA8888, BGRA8888\n");
    printf("\r--XRGB8888, XBGR8888, RGBX8888, BGRX8888\n");
    printf("\r--ARGB4444, ABGR4444, RGBA4444, BGRA4444\n");
    printf("\r--ARGB1555, ABGR1555, RGBA5551, BGRA5551\n");
    printf("\r--RGB888,   BGR888,   RGB565,   BGR565\n\n");
    printf("\r--YUV format supports: \n");
    printf("\r--YUV420, NV12, NV21\n");
    printf("\r--YUV422, NV16, NV61 , YUYV, YVYU, UYVY, VYUY\n");
    printf("\r--YUV400, YUV444\n");
    printf("\r--Please use parameter -u or --usage to list other parameters.\n\n");
}

static int str_to_mode(char *str)
{
    static struct StrToMode {
        char *str;
        int mode;
    } str_mode[] = {
        {"rgbtorgb", RGB_TO_RGB},
        {"rgbtoyuv", RGB_TO_YUV},
        {"yuvtoyuv", YUV_TO_YUV},
        {"yuvtorgb", YUV_TO_RGB},
    };
    const int str_mode_size = sizeof(str_mode) / sizeof(str_mode[0]);
    for (int i = 0; i < str_mode_size; i++) {
        if (!strncmp(str, str_mode[i].str, strlen(str_mode[i].str)))
            return str_mode[i].mode;
    }
    return -1;
}

int str_to_format(char *str)
{
    int i = 0;
    static struct StrToFormat table[] = {
        {"argb8888", MPP_FMT_ARGB_8888},
        {"abgr8888", MPP_FMT_ABGR_8888},
        {"rgba8888", MPP_FMT_RGBA_8888},
        {"bgra8888", MPP_FMT_BGRA_8888},
        {"xrgb8888", MPP_FMT_XRGB_8888},
        {"xbgr8888", MPP_FMT_XBGR_8888},
        {"rgbx8888", MPP_FMT_RGBX_8888},
        {"bgrx8888", MPP_FMT_BGRX_8888},
        {"argb4444", MPP_FMT_ARGB_4444},
        {"abgr4444", MPP_FMT_ABGR_4444},
        {"rgba4444", MPP_FMT_RGBA_4444},
        {"bgra4444", MPP_FMT_BGRA_4444},
        {"rgb565", MPP_FMT_RGB_565},
        {"bgr565", MPP_FMT_BGR_565},
        {"argb1555", MPP_FMT_ARGB_1555},
        {"abgr1555", MPP_FMT_ABGR_1555},
        {"rgba5551", MPP_FMT_RGBA_5551},
        {"bgra5551", MPP_FMT_BGRA_5551},
        {"rgb888", MPP_FMT_RGB_888},
        {"bgr888", MPP_FMT_BGR_888},
        {"yuv420", MPP_FMT_YUV420P},
        {"nv12", MPP_FMT_NV12},
        {"nv21", MPP_FMT_NV21},
        {"yuv422", MPP_FMT_YUV422P},
        {"nv16", MPP_FMT_NV16},
        {"nv61", MPP_FMT_NV61},
        {"yuyv", MPP_FMT_YUYV},
        {"yvyu", MPP_FMT_YVYU},
        {"uyvy", MPP_FMT_UYVY},
        {"vyuy", MPP_FMT_VYUY},
        {"yuv400", MPP_FMT_YUV400},
        {"yuv444", MPP_FMT_YUV444P},
    };
    format_table = &(*table);
    table_size = sizeof(table) / sizeof(table[0]);
    for (i = 0; i < table_size; i++) {
        if (!strncmp(str, table[i].str, strlen(table[i].str)))
            return table[i].format;
    }
    return -1;
}

static int bitblt_run(struct mpp_ge *ge, struct ge_bitblt *blt)
{
    int ret = 0;

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

static void fmt_conver_set(struct ge_bitblt *blt, struct ge_buf *src_buffer,
                           struct ge_buf *dst_buffer)
{
    memset(blt, 0, sizeof(struct ge_bitblt));
    /* src layer settings  */
    memcpy(&blt->src_buf, &src_buffer->buf, sizeof(struct mpp_buf));
    blt->src_buf.crop_en = 0;

    /* dst layer settings */
    memcpy(&blt->dst_buf, &dst_buffer->buf, sizeof(struct mpp_buf));
    blt->dst_buf.crop_en = 0;
}

static int fmt_conver_run(struct mpp_ge *ge, struct ge_bitblt *blt,
                          struct ge_buf *src_buffer, struct ge_buf *dst_buffer)
{
    int ret = -1;

    /* Run an bitlet operation without opening dither */
    fmt_conver_set(blt, src_buffer, dst_buffer);
    ret = bitblt_run(ge, blt);
    if (ret) {
        LOGE("bitblt_run task failed:%d\n", ret);
        return ret;
    }

    return 0;
}

static void display_set(struct ge_bitblt *blt, struct ge_fb_info *fb_info, struct ge_buf *buffer, int region)
{
    unsigned int fb_phy = 0;
    int dstx = 0;
    int dsty = 0;

    memset(blt, 0, sizeof(struct ge_bitblt));

    if (region == SRC_DISP_REGION) {
        dstx = 0;
        dsty = 0;
    } else {
        dstx = fb_info->fb_data.width / 2;
        dsty = 0;
    }

    /* Src layer set */
    memcpy(&blt->src_buf, &buffer->buf, sizeof(struct mpp_buf));
    blt->src_buf.crop_en = 0;

    fb_phy = fb_get_cur_frame(fb_info);
    /* Dst layer set */
    blt->dst_buf.buf_type = MPP_PHY_ADDR;
    blt->dst_buf.phy_addr[0] = fb_phy;
    blt->dst_buf.stride[0] = fb_info->fb_data.stride;
    blt->dst_buf.size.width = fb_info->fb_data.width;
    blt->dst_buf.size.height = fb_info->fb_data.height;
    blt->dst_buf.format = fb_info->fb_data.format;
    blt->dst_buf.crop_en = 1;
    blt->dst_buf.crop.x = dstx;
    blt->dst_buf.crop.y = dsty;
    blt->dst_buf.crop.width = fb_info->fb_data.width / 2;
    blt->dst_buf.crop.height = fb_info->fb_data.height;
}

static int display(struct mpp_ge *ge, struct ge_bitblt *blt, struct ge_fb_info *fb_info, struct ge_buf *buffer, int region)
{
    int ret = 0;

    display_set(blt, fb_info, buffer, region);

    ret = bitblt_run(ge, blt);
    if (ret < 0) {
        LOGE("bitblt_run failed\n");
        return ret;
    }

    return ret;
}

/* full format conversion */
static int format_conver_run(struct mpp_ge *ge, struct ge_bitblt *blt,
                             struct ge_fb_info *fb_info, struct ge_buf *bmp_buffer,
                             struct ge_buf *src_buffer, struct ge_buf *dst_buffer,
                             struct bmp_header *bmp_head, int mode)
{
    int ret = 0;
    int i = 0, j = 0;
    char *str_src_format = NULL;
    char *str_dst_format = NULL;
    int src_format = 0;
    int dst_format = 0;
    int src_select_region_left = 0;
    int dst_select_region_left = 0;
    int src_select_region_right = 0;
    int dst_select_region_right = 0;

    /* Set region according to mode */
    switch (mode)
    {
    case RGB_TO_RGB:
        str_src_format = "argb8888";
        str_dst_format = "argb8888";
        src_select_region_left = 0;
        dst_select_region_left = 0;
        src_select_region_right = 20;
        dst_select_region_right = 20;
        break;
    case RGB_TO_YUV:
        str_src_format = "argb8888";
        str_dst_format = "yuv420";
        src_select_region_left = 0;
        dst_select_region_left = 20;
        src_select_region_right = 20;
        dst_select_region_right = 32;
        break;
    case YUV_TO_YUV:
        str_src_format = "yuv420";
        str_dst_format = "yuv420";
        src_select_region_left = 20;
        dst_select_region_left = 20;
        src_select_region_right = 32;
        dst_select_region_right = 32;
        break;
    case YUV_TO_RGB:
        str_src_format = "yuv420";
        str_dst_format = "argb8888";
        src_select_region_left = 20;
        dst_select_region_left = 0;
        src_select_region_right = 32;
        dst_select_region_right = 20;
        break;
    default:
        LOGE("mode invalid, mode = %d\n",mode);
        break;
    }

    src_format = str_to_format(str_src_format);
    if (src_format < 0) {
        LOGE("str_to_format invalid\n");
        return -1;
    }
    dst_format = str_to_format(str_dst_format);
    if (dst_format < 0) {
        LOGE("dst_format invalid\n");
        return -1;
    }

    for (i = src_select_region_left; i < src_select_region_right; i++) {
        src_format = format_table[i].format;
        if (!dst_buffer)
            ge_buf_free(dst_buffer);

        src_buffer = ge_buf_malloc(bmp_head->width, abs(bmp_head->height), src_format);
        if (src_buffer == NULL) {
            LOGE("malloc src_buffer error\n");
            return -1;
        }

        /* convert the original image format to input format */
        ret = fmt_conver_run(ge, blt, bmp_buffer, src_buffer);
        if (ret < 0) {
            LOGE("fmt_conver_run failed\n");
            return -1;
        }

        ret = display(ge, blt, fb_info, src_buffer, SRC_DISP_REGION);
        if (ret < 0) {
            LOGE("display failed\n");
            return -1;
        }

        for (j = dst_select_region_left; j < dst_select_region_right; j++) {
            dst_format = format_table[j].format;
            if (!dst_buffer)
                ge_buf_free(dst_buffer);

            dst_buffer = ge_buf_malloc(bmp_head->width, abs(bmp_head->height), dst_format);
            if (dst_buffer == NULL) {
                LOGE("malloc dst_buffer error\n");
                return -1;
            }

            ret = fmt_conver_run(ge, blt, src_buffer, dst_buffer);
            if (ret < 0) {
                LOGE("fmt_conver_run failed\n");
                return -1;
            }

            ret = display(ge, blt, fb_info, dst_buffer, DST_DISP_REGION);
            if (ret < 0) {
                LOGE("display failed\n");
                return -1;
            }
            aicos_msleep(100);
        }
    }
    return 0;
}

int ge_format_test(int argc, char **argv)
{
    int ret = -1;
    int mode = RGB_TO_RGB;

    int bmp_fd = 0;
    enum mpp_pixel_format bmp_fmt = 0;
    struct ge_buf *bmp_buffer = NULL;
    struct ge_buf *src_buffer = NULL;
    struct ge_buf *dst_buffer = NULL;
    struct bmp_header bmp_head = {0};

    struct mpp_ge *ge = NULL;
    struct ge_bitblt blt = {0};
    struct ge_fb_info *fb_info = NULL;

    /* parameter supports settings */
    const char sopts[] = "uhm:";
    const struct option lopts[] = {
        {"usage",   no_argument,       NULL, 'u'},
        {"help",    no_argument,       NULL, 'h'},
        {"mode",    required_argument, NULL, 'm'},
        {0, 0, 0, 0}
    };
    optind = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'm':
            mode = str_to_mode(optarg);
            if (mode < 0) {
                printf("mode set error, please set against\n");
                goto EXIT;
            }
            break;
        case 'u':
            usage(argv[0]);
            goto EXIT;
        case 'h':
            help();
            goto EXIT;
        default:
            LOGE("Invalid parameter: %#x\n", ret);
            goto EXIT;
        }
    }

    ge = mpp_ge_open();
    if (!ge) {
        LOGE("open ge device error\n");
        goto EXIT;
    }

    fb_info = fb_open();

    bmp_fd = bmp_open(DITHER_IMAGE, &bmp_head);
    if (bmp_fd < 0) {
        LOGE("open bmp DITHER_IMAGE error\n");
        goto EXIT;
    }

    bmp_fmt = bmp_get_fmt(&bmp_head);
    bmp_buffer = ge_buf_malloc(bmp_head.width, abs(bmp_head.height), bmp_fmt);
    if (bmp_buffer == NULL) {
        LOGE("malloc bg buffer error\n");
        goto EXIT;
    }

    ret = bmp_read(bmp_fd, (void *)((uintptr_t)bmp_buffer->buf.phy_addr[0]), &bmp_head);
    if (ret < 0) {
        LOGE("src bmp_read error\n");
        goto EXIT;
    }
    ge_buf_clean_dcache(bmp_buffer);

    ret = format_conver_run(ge, &blt, fb_info, bmp_buffer,
                            src_buffer, dst_buffer, &bmp_head, mode);
    if (ret < 0) {
        LOGE("format_conver_run task failed\n");
        goto EXIT;
    }
EXIT:
    if (!bmp_fd)
        bmp_close(bmp_fd);

    if (!ge)
        mpp_ge_close(ge);

    if (!fb_info)
        fb_close(fb_info);

    if (!bmp_buffer)
        ge_buf_free(bmp_buffer);
    if (!src_buffer)
        ge_buf_free(src_buffer);
    if (!dst_buffer)
        ge_buf_free(dst_buffer);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(ge_format_test, ge_format, ge format test);
