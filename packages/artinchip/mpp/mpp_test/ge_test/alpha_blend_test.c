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

#define ALPHA_SRC_IMAGE         "/sdcard/ge_test/image/alpha_src.bmp"
#define ALPHA_DST_IMAGE         "/sdcard/ge_test/image/alpha_dst.bmp"
#define ALPHA_BACK_GROUND_IMAGE "/sdcard/ge_test/image/alpha_back_ground.bmp"

/* ge alpha bending test */
#define ALPHA_RULE_NUM    14

#define LOGE(fmt, ...) aic_log(AIC_LOG_ERR, "E", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) aic_log(AIC_LOG_WARN, "W", fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) aic_log(AIC_LOG_INFO, "I", fmt, ##__VA_ARGS__)

/* alpha blending */
static unsigned int src_alpha_mode = 0;
static unsigned int src_global_alpha = 0;
static unsigned int dst_alpha_mode = 0;
static unsigned int dst_global_alpha = 0;

static void usage_alpha(char *app)
{
    printf("Usage: %s [Options]: \n", app);
    printf(
    "\t-s, --src_alpha_mode,   Select src_alpha_mode (default 0), "
    "0: pixel alpha mode, 1: global alpha mode, 2: mixded alpha mode"
    "\n"
    "\t-g, --src_global_alpha, Set src_global_alpha value (default 0), "
    "source global alpha value (0~255)"
    "\n\n"
    "\t-d, --dst_alpha_mode,   Select dst_alpha_mode (default 0), "
    "0: pixel alpha mode, 1: global alpha mode, 2: mixded alpha mode"
    "\n"
    "\t-p, --dst_global_alpha, Select dst_global_alpha value (default 0), "
    "destination global alpha value (0~255)"
    "\n"
    "\t-u, --usage\n");
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

/* select the displayed area based on alpha_rules */
static int set_display_pos(int *pos_x, int *pos_y, int alpha_rules)
{
    static int pos[][2] = {
        {450, 420}, {640, 420}, {70, 60}, {450, 60}, {640, 60},
        {830, 60}, {70, 240}, {260, 240}, {450, 240}, {640, 240},
        {830, 240}, {260, 420}, {70, 420}, {260, 60}, {0, 0}
    };

    if (alpha_rules > ALPHA_RULE_NUM || alpha_rules < 0) {
        LOGE("set_display_pos error, alpha_rules = %d\n", alpha_rules);
        return -1;
    }

    *pos_x = pos[alpha_rules][0];
    *pos_y = pos[alpha_rules][1];

    return 0;
}

static void display_set(struct ge_bitblt *blt, struct ge_fb_info *fb_info,
                        struct ge_buf *buffer, struct bmp_header *bmp_head, int alpha_rules)
{
    unsigned int fb_phy = 0;
    int x = 0;
    int y = 0;

    memset(blt, 0, sizeof(struct ge_bitblt));

    /* Src layer set */
    memcpy(&blt->src_buf, &buffer->buf, sizeof(struct mpp_buf));
    blt->src_buf.crop_en = 0;

    if (set_display_pos(&x, &y, alpha_rules) < 0)
        LOGE("ge_display_pos error.\n");

    fb_phy = fb_get_cur_frame(fb_info);
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
    blt->dst_buf.crop.width = bmp_head->width;
    blt->dst_buf.crop.height = abs(bmp_head->height);
}

static int display(struct mpp_ge *ge, struct ge_bitblt *blt,
                   struct ge_fb_info *fb_info, struct ge_buf *buffer,
                   struct bmp_header *bmp_head, int alpha_rules)
{
    int ret = 0;

    display_set(blt, fb_info, buffer, bmp_head, alpha_rules);

    ret = bitblt_run(ge, blt);
    if (ret < 0) {
        LOGE("bitblt_run failed\n");
        return ret;
    }

    return ret;
}

static void dst_buffer_copy_set(struct ge_bitblt *blt, struct ge_buf *src_buffer,
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

static int dst_buffer_copy_run(struct mpp_ge *ge,struct ge_bitblt *blt,
                                struct ge_buf *src_buffer, struct ge_buf *dst_buffer)
{
    int ret = 0;

    dst_buffer_copy_set(blt, src_buffer, dst_buffer);

    ret = bitblt_run(ge, blt);
    if (ret < 0) {
        LOGE("bitblt_run failed\n");
        return ret;
    }
    return ret;
}

static void porter_duff_set(struct ge_bitblt *blt, struct ge_buf *src_buffer,
                            struct ge_buf *dst_buffer, int alpha_rule)
{
    memset(blt, 0, sizeof(struct ge_bitblt));
    /* src layer settings  */
    memcpy(&blt->src_buf, &src_buffer->buf, sizeof(struct mpp_buf));
    blt->src_buf.crop_en = 0;
    blt->ctrl.src_alpha_mode = src_alpha_mode;
    blt->ctrl.src_global_alpha = src_global_alpha;

    /* dst layer settings */
    memcpy(&blt->dst_buf, &dst_buffer->buf, sizeof(struct mpp_buf));
    blt->dst_buf.crop_en = 0;
    blt->ctrl.dst_alpha_mode = dst_alpha_mode;
    blt->ctrl.dst_global_alpha = dst_global_alpha;

    blt->ctrl.alpha_en = 1;
    blt->ctrl.alpha_rules = alpha_rule;
}

static int ge_alpha_blending_test(int argc, char **argv)
{
    int ret = -1;
    int i = 0;

    int bg_fd = -1;
    int src_fd = -1;
    int dst_fd = -1;
    enum mpp_pixel_format bg_fmt = 0;
    enum mpp_pixel_format src_fmt = 0;
    enum mpp_pixel_format dst_fmt = 0;
    struct ge_buf *bg_buffer = NULL;
    struct ge_buf *src_buffer = NULL;
    struct ge_buf *dst_buffer = NULL;
    struct ge_buf *dst_copy_buffer = NULL;
    struct bmp_header bg_head = {0};
    struct bmp_header src_head = {0};
    struct bmp_header dst_head = {0};

    struct mpp_ge *ge = NULL;
    struct ge_bitblt blt = {0};
    struct ge_fb_info *fb_info = NULL;

    const char sopts[] = "us:d:g:p:";
    const struct option lopts[] = {
        {"usage",               no_argument,       NULL, 'u'},
        {"src_alpha_mode" ,     required_argument, NULL, 's'},
        {"dst_alpha_mode",      required_argument, NULL, 'd'},
        {"src_global_alpha",    required_argument, NULL, 'g'},
        {"dst_global_alpha",    required_argument, NULL, 'p'},
    };
    optind = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 's':
            src_alpha_mode = str2int(optarg);
            if ((src_alpha_mode > 3) || (src_alpha_mode < 0)) {
                printf("src_alpha_mode invalid, please input against\n");
                return 0;
            }
            break;
        case 'd':
            dst_alpha_mode = str2int(optarg);
            if ((dst_alpha_mode > 3) || (dst_alpha_mode < 0)) {
                printf("dst_alpha_mode invalid, please input against\n");
                return 0;
            }
            break;
        case 'g':
            src_global_alpha = str2int(optarg);
            if ((src_global_alpha > 255) || (src_global_alpha < 0)) {
                printf("src_global_alpha invalid, please input against\n");
                return 0;
            }
            break;
        case 'p':
            dst_global_alpha = str2int(optarg);
            if ((dst_global_alpha > 255) || (dst_global_alpha < 0)) {
                printf("dst_global_alpha invalid, please input against\n");
                return 0;
            }
            break;
        case 'u':
            usage_alpha(argv[0]);
            return 0;
        default:
            LOGE("Invalid parameter: %#x\n", ret);
            return 0;
        }
    }

    ge = mpp_ge_open();
    if (!ge) {
        LOGE("open ge device error\n");
        goto EXIT;
    }

    fb_info = fb_open();

    bg_fd = bmp_open(ALPHA_BACK_GROUND_IMAGE, &bg_head);
    if (bg_fd < 0) {
        LOGE("open bmp error, path = %s\n", ALPHA_BACK_GROUND_IMAGE);
        goto EXIT;
    }

    src_fd = bmp_open(ALPHA_SRC_IMAGE, &src_head);
    if (src_fd < 0) {
        LOGE("open bmp error, path = %s\n", ALPHA_SRC_IMAGE);
        goto EXIT;
    }

    dst_fd = bmp_open(ALPHA_DST_IMAGE, &dst_head);
    if (dst_fd < 0) {
        LOGE("open bmp error, path = %s\n", ALPHA_DST_IMAGE);
        goto EXIT;
    }

    bg_fmt = bmp_get_fmt(&bg_head);
    bg_buffer = ge_buf_malloc(bg_head.width, abs(bg_head.height), bg_fmt);
    if (bg_buffer == NULL) {
        LOGE("malloc bg buffer error\n");
        goto EXIT;
    }

    src_fmt = bmp_get_fmt(&src_head);
    src_buffer = ge_buf_malloc(src_head.width, abs(src_head.height), src_fmt);
    if (src_buffer == NULL) {
        LOGE("malloc src buffer error\n");
        goto EXIT;
    }

    dst_fmt = bmp_get_fmt(&dst_head);
    dst_buffer = ge_buf_malloc(dst_head.width, abs(dst_head.height), dst_fmt);
    if (dst_buffer == NULL) {
        LOGE("malloc dst buffer error\n");
        goto EXIT;
    }

    dst_copy_buffer = ge_buf_malloc(dst_head.width, abs(dst_head.height), dst_fmt);
    if (dst_copy_buffer == NULL) {
        LOGE("malloc dst copy buffer error\n");
        goto EXIT;
    }

    ret = bmp_read(bg_fd, (void *)((uintptr_t)bg_buffer->buf.phy_addr[0]), &bg_head);
    if (ret < 0) {
        LOGE("bg bmp_read error\n");
        goto EXIT;
    }
    ge_buf_clean_dcache(bg_buffer);

    ret = bmp_read(src_fd, (void *)((uintptr_t)src_buffer->buf.phy_addr[0]), &src_head);
    if (ret < 0) {
        LOGE("src bmp_read error\n");
        goto EXIT;
    }
    ge_buf_clean_dcache(src_buffer);

    ret = bmp_read(dst_fd, (void *)((uintptr_t)dst_buffer->buf.phy_addr[0]), &dst_head);
    if (ret < 0) {
        LOGE("dst bmp_read error\n");
        goto EXIT;
    }
    ge_buf_clean_dcache(dst_buffer);

    ret = dst_buffer_copy_run(ge, &blt, dst_buffer, dst_copy_buffer);
    if (ret < 0) {
        LOGE("dst_buffer_copy_run error\n");
        goto EXIT;
    }

    /* display background */
    display(ge, &blt, fb_info, bg_buffer, &bg_head, ALPHA_RULE_NUM);
    for (i = 0; i < ALPHA_RULE_NUM; i++) {
        /* set according to different porter duff rules */
        porter_duff_set(&blt, src_buffer, dst_buffer, i);
        ret = bitblt_run(ge, &blt);
        if (ret < 0) {
            LOGE("bitblt_run error\n");
            return -1;
        }

        display(ge, &blt, fb_info, dst_buffer, &dst_head, i);
        /* redraw dst image */
        ret = dst_buffer_copy_run(ge, &blt, dst_copy_buffer, dst_buffer);
        if (ret < 0) {
            LOGE("dst_buffer_copy_run error\n");
            goto EXIT;
        }
    }
EXIT:
    if (bg_fd > 0)
        bmp_close(bg_fd);
    if (src_fd > 0)
        bmp_close(src_fd);
    if (dst_fd > 0)
        bmp_close(dst_fd);

    if (ge)
        mpp_ge_close(ge);

    if (fb_info)
        fb_close(fb_info);

    if (bg_buffer)
        ge_buf_free(bg_buffer);
    if (src_buffer)
        ge_buf_free(src_buffer);
    if (dst_buffer)
        ge_buf_free(dst_buffer);
    if (dst_copy_buffer)
        ge_buf_free(dst_copy_buffer);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(ge_alpha_blending_test, ge_alpha_blending, ge alpha test);
