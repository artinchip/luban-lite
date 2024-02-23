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

/* ge dither test */
#define SRC_DISP_REGION 0
#define DST_DISP_REGION 1

#define DITHER_IMAGE    "/sdcard/ge_test/image/singer_alpha.bmp"

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
    printf("Usage: %s [Options]: \n", app);
    printf("\t-o, --dither_on,  Select open dither (default 0), 0 :close  1 :open\n");
    printf("\t-s, --src_format, Select src format  (default argb8888)\n");
    printf("\t-d, --dst_format, Select dst format  (default argb4444)\n");
    printf("\t-u, --usage\n");
    printf("\t-h, --help, list the supported format and the test instructions\n\n");
}
static void help(void)
{
    printf("\r--This is ge bitblt operation using dither function.\n");
    printf("\r--Dither supports input argb8888 and rgb888 formats, and output, argb4444, argb1555 and rgb565 formats.\n");
    printf("\r--In this example, the input layer is src, output layer is dst.\n");
    printf("\r--Please use parameter -u or --usage to list other parameters.\n\n");
}

static int str_to_format(char *str)
{
    int i = 0;
    static struct StrToFormat table[] = {
        {"argb8888", MPP_FMT_ARGB_8888},
        {"argb4444", MPP_FMT_ARGB_4444},
        {"rgb565", MPP_FMT_RGB_565},
        {"argb1555", MPP_FMT_ARGB_1555},
        {"rgb888", MPP_FMT_RGB_888}
    };
    format_table = &(*table);
    table_size = sizeof(table) / sizeof(table[0]);
    for (i = 0; i < table_size; i++) {
        if (!strncmp(str, table[i].str, strlen(table[i].str)))
            return table[i].format;
    }
    return -1;
}

static long long int str2int(char *_str)
{
    if (_str == NULL) {
        pr_err("The string is empty!\n");
        return -1;
    }

    if (strncmp(_str, "0x", 2))
        return atoi(_str);
    else
        return strtoll(_str, NULL, 16);
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

static void bmp_to_src_set(struct ge_bitblt *blt, struct ge_buf *bmp_buffer, struct ge_buf *src_buffer)
{
    memset(blt, 0, sizeof(struct ge_bitblt));

    memcpy(&blt->src_buf, &bmp_buffer->buf, sizeof(struct mpp_buf));
    blt->src_buf.crop_en = 0;

    memcpy(&blt->dst_buf, &src_buffer->buf, sizeof(struct mpp_buf));
    blt->dst_buf.crop_en = 0;
}

static int bmp_to_src_run(struct mpp_ge *ge, struct ge_bitblt *blt, struct ge_buf *bmp_buffer, struct ge_buf *src_buffer)
{
    int ret = -1;

    bmp_to_src_set(blt, bmp_buffer, src_buffer);
    ret = bitblt_run(ge, blt);
    if (ret < 0) {
        LOGE("bitblt_run failed\n");
        return ret;
    }

    return ret;
}

static void dither_set(struct ge_bitblt *blt, struct ge_buf *src_buffer, struct ge_buf *dst_buffer, int dith_en)
{
    memset(blt, 0, sizeof(struct ge_bitblt));

    memcpy(&blt->src_buf, &src_buffer->buf, sizeof(struct mpp_buf));
    blt->src_buf.crop_en = 0;

    memcpy(&blt->dst_buf, &dst_buffer->buf, sizeof(struct mpp_buf));
    blt->dst_buf.crop_en = 0;

    blt->ctrl.dither_en = dith_en;
}

static int dither_run(struct mpp_ge *ge, struct ge_bitblt *blt,
                       struct ge_buf *src_buffer, struct ge_buf *dst_buffer, int dith_en)
{
    int ret = -1;

    dither_set(blt, src_buffer, dst_buffer, dith_en);
    ret = bitblt_run(ge, blt);
    if (ret < 0) {
        LOGE("bitblt_run failed\n");
        return ret;
    }

    return ret;
}

static void ge_dither_test(int argc, char **argv)
{
    int ret = -1;
    int dither_on = 0;
    int src_format = 0;
    int dst_format = 0;

    int bmp_fd = -1;
    enum mpp_pixel_format bmp_fmt = 0;
    struct mpp_ge *ge = NULL;
    struct ge_bitblt blt = {0};
    struct ge_fb_info *fb_info = NULL;
    struct ge_buf *bmp_buffer = NULL;
    struct ge_buf *src_buffer = NULL;
    struct ge_buf *dst_buffer = NULL;
    struct bmp_header bmp_head = {0};

    const char sopts[] = "uhs:d:o:";
    const struct option lopts[] = {
        {"usage",       no_argument,       NULL, 'u'},
        {"help",        no_argument,       NULL, 'h'},
        {"src_format",  required_argument, NULL, 's'},
        {"dst_format",  required_argument, NULL, 'd'},
        {"dither_on",   required_argument, NULL, 'o'},
        {0, 0, 0, 0}
    };
    src_format = str_to_format("argb8888");
    dst_format = str_to_format("argb4444");
    optind = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 's':
            src_format = str_to_format(optarg);
            if (src_format < 0) {
                printf("src format set error, please set against\n");
                return;
            }
            break;
        case 'd':
            dst_format = str_to_format(optarg);
            if (dst_format < 0) {
                printf("dst format set error, please set against\n");
                return;
            }
            break;
        case 'o':
            dither_on = str2int(optarg);
            if ((dither_on > 1) || (dither_on < 0)) {
                LOGE("dither switch set error, please set against\n");
                return;
            }
            break;
        case 'u':
            usage(argv[0]);
            return;
        case 'h':
            help();
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

    bmp_fd = bmp_open(DITHER_IMAGE, &bmp_head);
    if (bmp_fd < 0) {
        LOGE("open bmp error, path = %s\n", DITHER_IMAGE);
        goto EXIT;
    }

    bmp_fmt = bmp_get_fmt(&bmp_head);
    bmp_buffer = ge_buf_malloc(bmp_head.width, abs(bmp_head.height), bmp_fmt);
    if (bmp_buffer == NULL) {
        LOGE("malloc bmp_buffer error\n");
        goto EXIT;
    }

    src_buffer = ge_buf_malloc(bmp_head.width, abs(bmp_head.height), src_format);
    if (src_buffer == NULL) {
        LOGE("malloc src_buffer error\n");
        goto EXIT;
    }

    dst_buffer = ge_buf_malloc(bmp_head.width, abs(bmp_head.height), dst_format);
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
    ret = bmp_to_src_run(ge, &blt, bmp_buffer, src_buffer);
    if (ret < 0) {
        LOGE("bmp_to_src_run failed\n");
        goto EXIT;
    }

    ret = display(ge, &blt, fb_info, src_buffer, SRC_DISP_REGION);
    if (ret < 0) {
        LOGE("display failed\n");
        goto EXIT;
    }

    ret = dither_run(ge, &blt, src_buffer, dst_buffer, dither_on);
    if (ret < 0) {
        LOGE("dither_run failed\n");
        goto EXIT;
    }

    ret = display(ge, &blt, fb_info, dst_buffer, DST_DISP_REGION);
    if (ret < 0) {
        LOGE("display failed\n");
        goto EXIT;
    }
    fb_start_and_wait(fb_info);
    fb_swap_frame(fb_info);

EXIT:
    if (bmp_fd > 0)
        bmp_close(bmp_fd);

    if (ge)
        mpp_ge_close(ge);

    if (fb_info)
        fb_close(fb_info);

    if (bmp_buffer)
        ge_buf_free(bmp_buffer);

    if (src_buffer)
        ge_buf_free(src_buffer);

    if (dst_buffer)
        ge_buf_free(dst_buffer);
}
MSH_CMD_EXPORT_ALIAS(ge_dither_test, ge_dither, ge dithe test);
