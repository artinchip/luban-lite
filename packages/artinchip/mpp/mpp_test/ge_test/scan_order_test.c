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

/* scan order options */
#define UPPER_LEFT      0
#define UPPER_RIGHT     1
#define LOWER_RIGHT     2
#define LOWER_LEFT      3
#define STRAIGHT_LEFT   4
#define DIRECTLY_ABOVE  5
#define STRAIGHT_RIGHT  6
#define DIRECTLY_BELOW  7

#define SCAN_IMAGE      "/sdcard/ge_test/image/scan_order.bmp"

#define LOGE(fmt, ...) aic_log(AIC_LOG_ERR, "E", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) aic_log(AIC_LOG_WARN, "W", fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) aic_log(AIC_LOG_INFO, "I", fmt, ##__VA_ARGS__)

static void usage(char *app)
{
    printf("Usage: %s [Options]: \n", app);
    printf("\t-r, --region the  region of screen (default 0),The range of region is 0~7\n");
    printf("\t-m, --mode   scan order flags for GE ctrl (default 0)\n");
    printf("\t    --mode=0 scan order flags = MPP_LR_TB\n");
    printf("\t    --mode=1 scan order flags = MPP_RL_TB\n");
    printf("\t    --mode=2 scan order flags = MPP_LR_BT\n");
    printf("\t    --mode=3 scan order flags = MPP_RL_BT\n");

    printf("\t-u, --usage\n");
    printf("\t-h, --help\n\n");
}

static void help(void)
{
    printf("\r--This is ge bitblt operation using scan order function.\n");
    printf("\r--Scan order function is used when src and dst memory overlap.\n");
    printf("\r--Scan order flags defaults to MPP_LR_TB whether the memory overlaps or not.\n");
    printf("\r--When scan order is not set to MPP_LR_TB,rot0 and dither are not supported, only rgb format is supported.\n\n");

    printf("\r--There are eight types of memory overlap in screen memory for reference.\n\n");
    printf("\r--In the case of SRC and DST memory overlap,it is recommended to set scan order flags as follows.\n");
    printf("\r--If the DST layer is in the upper left  corner of the SRC layer, scan order flags is recommended to be MPP_LR_TB.\n");
    printf("\r--If the DST layer is in the upper right corner of the SRC layer, scan order flags is recommended to be MPP_RL_TB.\n");
    printf("\r--If the DST layer is in the lower left  corner of the SRC layer, scan order flags is recommended to be MPP_LR_BT.\n");
    printf("\r--If the DST layer is in the lower right corner of the SRC layer, scan order flags is recommended to be MPP_RL_BT.\n");

    printf("\r--If the DST layer is in the directly above the SRC layer, scan order flags is recommended to be MPP_LR_TB or MPP_RL_TB.\n");
    printf("\r--If the DST layer is in the directly below the SRC layer, scan order flags is recommended to be MPP_LR_BT or MPP_RL_BT.\n");
    printf("\r--If the DST layer is on the straight left  side of the SRC layer, scan order flags is recommended to be MPP_LR_TB or MPP_LR_BT.\n");
    printf("\r--If the DST layer is on the straight right side of the SRC layer, scan order flags is recommended to be MPP_RL_TB or MPP_RL_BT.\n\n");

    printf("\r--Please use parameter -u or --usage to list other parameters.\n\n");
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


static void background_set(struct ge_bitblt *blt, struct ge_fb_info *fb_info, struct ge_buf *buffer)
{
    unsigned int fb_phy = 0;

    memset(blt, 0, sizeof(struct ge_bitblt));

    /* Src layer set */
    memcpy(&blt->src_buf, &buffer->buf, sizeof(struct mpp_buf));

    fb_phy = fb_get_cur_frame(fb_info);
    /* Dst layer set */
    blt->dst_buf.buf_type = MPP_PHY_ADDR;
    blt->dst_buf.phy_addr[0] = fb_phy;
    blt->dst_buf.stride[0] = fb_info->fb_data.stride;
    blt->dst_buf.size.width = fb_info->fb_data.width;
    blt->dst_buf.size.height = fb_info->fb_data.height;
    blt->dst_buf.format = fb_info->fb_data.format;
    blt->dst_buf.crop_en = 1;
    blt->dst_buf.crop.x = 0;
    blt->dst_buf.crop.y = 0;
    blt->dst_buf.crop.width = buffer->buf.size.width;
    blt->dst_buf.crop.height = buffer->buf.size.height;
}

static int background_fill(struct mpp_ge *ge, struct ge_bitblt *blt, struct ge_fb_info *fb_info, struct ge_buf *buffer)
{
    int ret = 0;

    background_set(blt, fb_info, buffer);

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

    return ret;
}

static void scan_order_set(struct ge_bitblt *blt, struct ge_fb_info *fb_info,
                           unsigned int mode, unsigned int dstx, unsigned int dsty)
{
    unsigned int fb_phy = 0;

    fb_phy = fb_get_cur_frame(fb_info);

    /* Src layer set */
    blt->src_buf.buf_type = MPP_PHY_ADDR;
    blt->src_buf.phy_addr[0] = fb_phy;
    blt->src_buf.stride[0] = fb_info->fb_data.stride;
    blt->src_buf.size.width = fb_info->fb_data.width;
    blt->src_buf.size.height = fb_info->fb_data.height;
    blt->src_buf.format = fb_info->fb_data.format;

    blt->src_buf.crop_en = 1;
    blt->src_buf.crop.x = fb_info->fb_data.width / 4;
    blt->src_buf.crop.y = fb_info->fb_data.height / 4;
    blt->src_buf.crop.width = fb_info->fb_data.width / 2;
    blt->src_buf.crop.height = fb_info->fb_data.height / 2;

    /* Dst layer set */
    blt->dst_buf.buf_type = MPP_PHY_ADDR;
    blt->dst_buf.phy_addr[0] = fb_phy;
    blt->dst_buf.stride[0] = fb_info->fb_data.stride;
    blt->dst_buf.size.width = fb_info->fb_data.width;
    blt->dst_buf.size.height = fb_info->fb_data.height;
    blt->dst_buf.format = fb_info->fb_data.format;

    /* only in this case,
     * dst_buf.crop.width==src_buf.size.width ||
     * dst_buf.crop.height==src_buf.size.height
     * scan order flags can use perameters MPP_RL_TB, MPP_LR_BT, MPP_RL_BT
    */
    blt->dst_buf.crop_en = 1;
    blt->dst_buf.crop.x = dstx;
    blt->dst_buf.crop.y = dsty;
    blt->dst_buf.crop.width = fb_info->fb_data.width / 2;
    blt->dst_buf.crop.height = fb_info->fb_data.height / 2;

    /* scan order flage */
    blt->ctrl.flags = mode;
}

static int scan_order_run(struct mpp_ge *ge, struct ge_bitblt *blt,
                          struct ge_fb_info *fb_info, int region, int mode)
{
    int ret = 0;
    unsigned int dstx = 0;
    unsigned int dsty = 0;

    /* choice dst region */
    switch (region) {
    case UPPER_LEFT:
        dstx = 0;
        dsty = 0;
        break;
    case UPPER_RIGHT:
        dstx = fb_info->fb_data.width / 2;
        dsty = 0;
        break;
    case LOWER_RIGHT:
        dstx = fb_info->fb_data.width / 2;
        dsty = fb_info->fb_data.height / 2;
        break;
    case LOWER_LEFT:
        dstx = 0;
        dsty = fb_info->fb_data.height / 2;
        break;
    case STRAIGHT_LEFT:
        dstx = 0;
        dsty = fb_info->fb_data.height / 4;
        break;
    case DIRECTLY_ABOVE:
        dstx = fb_info->fb_data.width / 4;
        dsty = 0;
        break;
    case STRAIGHT_RIGHT:
        dstx = fb_info->fb_data.width / 2;
        dsty = fb_info->fb_data.height / 4;
        break;
    case DIRECTLY_BELOW:
        dstx = fb_info->fb_data.width / 4;
        dsty = fb_info->fb_data.height / 2;
        break;
    default:
        break;
    }

    scan_order_set(blt, fb_info, mode<<16, dstx, dsty);
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

    return ret;
}

static void ge_scan_test(int argc, char **argv)
{
    int ret = -1;
    int region = 0;
    int mode = 0;

    int bmp_fd = -1;
    enum mpp_pixel_format bmp_fmt = 0;
    struct mpp_ge *ge = NULL;
    struct ge_bitblt blt = {0};
    struct ge_fb_info *fb_info = NULL;
    struct ge_buf *buffer = NULL;
    struct bmp_header bmp_head = {0};

    const char sopts[] = "uhr:m:";
    const struct option lopts[] = {
        {"usage",   no_argument,       NULL, 'u'},
        {"help",    no_argument,       NULL, 'h'},
        {"region",  required_argument, NULL, 'r'},
        {"mode",    required_argument, NULL, 'm'},
        {0, 0, 0, 0}
    };

    optind = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'r':
            region = str2int(optarg);
            if (region < 0 || region > 7)  region=0;
            break;
        case 'm':
            mode = str2int(optarg);
            if (mode < 0 || mode >3)  mode=0;
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

    bmp_fd = bmp_open(SCAN_IMAGE, &bmp_head);
    if (bmp_fd < 0) {
        LOGE("open bmp error, path = %s\n", SCAN_IMAGE);
        goto EXIT;
    }

    bmp_fmt = bmp_get_fmt(&bmp_head);
    buffer = ge_buf_malloc(bmp_head.width, abs(bmp_head.height), bmp_fmt);
    if (buffer == NULL) {
        LOGE("ge_buf_malloc error\n");
        goto EXIT;
    }

    ret = bmp_read(bmp_fd, (void *)((uintptr_t)buffer->buf.phy_addr[0]), &bmp_head);
    if (ret < 0) {
        LOGE("bmp_read error\n");
        goto EXIT;
    }

    ge_buf_clean_dcache(buffer);

    ret = background_fill(ge, &blt, fb_info, buffer);
    if (ret < 0) {
        LOGE("background_fill failed\n");
        goto EXIT;
    }

    aicos_msleep(1000);

    /* The ge operation here does not involve the CPU and does not required cleaning the dcache */
    ret = scan_order_run(ge, &blt, fb_info, region, mode);
    if (ret < 0) {
        LOGE("scan_order_run failed\n");
        goto EXIT;
    }

EXIT:
    if (bmp_fd > 0)
        bmp_close(bmp_fd);

    if (ge)
        mpp_ge_close(ge);

    if (fb_info)
        fb_close(fb_info);

    if (buffer)
        ge_buf_free(buffer);
}
MSH_CMD_EXPORT_ALIAS(ge_scan_test, ge_scan_order, ge scan test);
