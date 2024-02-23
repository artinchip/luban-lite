/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ardon <haidong.pan@artinchip.com>
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <aic_core.h>
#include "mpp_ge.h"
#include "mpp_fb.h"
#include "artinchip_fb.h"

#include <getopt.h>

#ifndef LOG_TAG
#define LOG_TAG "ge_test"
#endif

#define LOGE(fmt, ...) aic_log(AIC_LOG_ERR, "E", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) aic_log(AIC_LOG_WARN, "W", fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) aic_log(AIC_LOG_INFO, "I", fmt, ##__VA_ARGS__)

#define CLOCK_IMAGE_WIDTH   590
#define CLOCK_IMAGE_HEIGHT  600

#define SECOND_IMAGE_WIDTH  48
#define SECOND_IMAGE_HEIGHT 220

#define ROT_SRC_CENTER_X    24
#define ROT_SRC_CENTER_Y    194

#define CLOCK_IMAGE "/sdcard/ge_test/image/clock.bmp"
#define SECOND_IMAGE "/sdcard/ge_test/image/second.bmp"
#define BLT_BMP "/sdcard/ge_test/image/clock.bmp"

#define ROT_DST_CENTER_X    294
#define ROT_DST_CENTER_Y    297

#define APP_FB_NUM 2

#define PARA_NUM   0
#define PARA_CIR   1

static unsigned int g_src_phy = 0;
static unsigned int g_bg_phy = 0;
static unsigned int g_dst_phy = 0;

static struct aicfb_screeninfo g_info = {0};

static int degree_list[] = {
    0, 4096,
    428, 4073,
    851, 4006,
    1265, 3895,
    1665, 3741,
    2047, 3547,
    2407, 3313,
    2740, 3043,
    3043, 2740,
    3313, 2407,
    3547, 2048,
    3741, 1665,
    3895, 1265,
    4006, 851,
    4073, 428,
    4096, 0,
    4073, -428,
    4006, -851,
    3895, -1265,
    3741, -1665,
    3547, -2047,
    3313, -2407,
    3043, -2740,
    2740, -3043,
    2407, -3313,
    2048, -3547,
    1665, -3741,
    1265, -3895,
    851, -4006,
    428, -4073,
    0, -4096,
    -428, -4073,
    -851, -4006,
    -1265, -3895,
    -1665, -3741,
    -2047, -3547,
    -2407, -3313,
    -2740, -3043,
    -3043, -2740,
    -3313, -2407,
    -3547, -2048,
    -3741, -1665,
    -3895, -1265,
    -4006, -851,
    -4073, -428,
    -4096, 0,
    -4073, 428,
    -4006, 851,
    -3895, 1265,
    -3741, 1665,
    -3547, 2047,
    -3313, 2407,
    -3043, 2740,
    -2740, 3043,
    -2407, 3313,
    -2048, 3547,
    -1665, 3741,
    -1265, 3895,
    -851, 4006,
    -428, 4073,
    0, 4096,
};

static void usage(char *app)
{
    printf("Usage: %s [Options]: \n", app);
    printf("\t-c, --circle test\n");
    printf("\t-n, --number of run, default run once\n");

    printf("\t-u, --usage\n");
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

static void draw_clock(struct ge_bitblt *blt, int src_buf, int index)
{
    /* source buffer */
    blt->src_buf.buf_type = MPP_PHY_ADDR;
    blt->src_buf.phy_addr[0] = src_buf;
    blt->src_buf.stride[0] = CLOCK_IMAGE_WIDTH * 4;
    blt->src_buf.size.width = CLOCK_IMAGE_WIDTH;
    blt->src_buf.size.height = CLOCK_IMAGE_HEIGHT;
    blt->src_buf.format = MPP_FMT_ARGB_8888;

    blt->src_buf.crop_en = 1;
    blt->src_buf.crop.x = 0;
    blt->src_buf.crop.y = 0;
    blt->src_buf.crop.width = CLOCK_IMAGE_WIDTH;
    blt->src_buf.crop.height = CLOCK_IMAGE_HEIGHT;

    /* dstination buffer */
    blt->dst_buf.buf_type = MPP_PHY_ADDR;

    if (!index) {
        blt->dst_buf.phy_addr[0] = g_dst_phy;
    } else {
        if (APP_FB_NUM > 1) {
            blt->dst_buf.phy_addr[0] = g_dst_phy + g_info.smem_len;
        } else {
            blt->dst_buf.phy_addr[0] = g_dst_phy;
        }
    }
    blt->dst_buf.stride[0] = g_info.stride;
    blt->dst_buf.size.width = g_info.width;
    blt->dst_buf.size.height = g_info.height;
    blt->dst_buf.format = g_info.format;

    blt->ctrl.flags = 0;

    blt->dst_buf.crop_en = 1;
    blt->dst_buf.crop.x = 0;
    blt->dst_buf.crop.y = 0;
    blt->dst_buf.crop.width = CLOCK_IMAGE_WIDTH;
    blt->dst_buf.crop.height = CLOCK_IMAGE_HEIGHT;
}

static void move_second_hand(struct mpp_ge *ge, struct ge_rotation *rot,
                int sin, int cos,
                int src_buf, int index)
{
    /* source buffer */
    rot->src_buf.buf_type = MPP_PHY_ADDR;
    rot->src_buf.phy_addr[0] = src_buf;
    rot->src_buf.stride[0] = SECOND_IMAGE_WIDTH * 4;
    rot->src_buf.size.width = SECOND_IMAGE_WIDTH;
    rot->src_buf.size.height = SECOND_IMAGE_HEIGHT;
    rot->src_buf.format = MPP_FMT_ARGB_8888;
    rot->src_buf.crop_en = 0;

    rot->src_rot_center.x = ROT_SRC_CENTER_X;
    rot->src_rot_center.y = ROT_SRC_CENTER_Y;

    /* destination buffer */
    rot->dst_buf.buf_type = MPP_PHY_ADDR;
    if (!index) {
        rot->dst_buf.phy_addr[0] = g_dst_phy;
    } else {
        if (APP_FB_NUM > 1) {
            rot->dst_buf.phy_addr[0] = g_dst_phy + g_info.smem_len;
        } else {
            rot->dst_buf.phy_addr[0] = g_dst_phy;
        }
    }
    rot->dst_buf.stride[0] = g_info.stride;
    rot->dst_buf.size.width = g_info.width;
    rot->dst_buf.size.height = g_info.height;
    rot->dst_buf.format = g_info.format;
    rot->dst_buf.crop_en = 0;

    rot->dst_rot_center.x = ROT_DST_CENTER_X;
    rot->dst_rot_center.y = ROT_DST_CENTER_Y;

    rot->ctrl.alpha_en = 1;

    rot->angle_sin = sin;
    rot->angle_cos = cos;
}

static void ge_rotate_thread(void *arg)
{
    int ret = 0;
    int i = 0, num = 0, loops = 1;
    int src_fd = -1, bg_fd = -1;
    int fsize = 0;
    int index = 0;
    struct stat st;
    struct mpp_ge *ge = NULL;
    struct mpp_fb *fb = NULL;
    void *src_buf = NULL;
    void *bg_buf = NULL;
    struct ge_bitblt blt = {0};
    struct ge_rotation rot = {0};
    int *para = NULL;
    int circle = 0;

    para = arg;
    circle = para[PARA_CIR];
    if (para[PARA_NUM] >= 0)
        loops = para[PARA_NUM];

    src_fd = open(SECOND_IMAGE, O_RDONLY);
    if (src_fd < 0) {
        LOGE("open second_bmp fail, path = %s\n", SECOND_IMAGE);
        goto out;
    }

    lseek(src_fd, 54, SEEK_SET);
    stat(SECOND_IMAGE, &st);
    fsize = st.st_size;
    src_buf = aicos_malloc(MEM_CMA, fsize);
    if (!src_buf) {
        LOGE("malloc src_buf fail\n");
        return;
    }
    memset(src_buf, 0, fsize);

    ret = read(src_fd, src_buf, fsize - 54);
    aicos_dcache_clean_range((unsigned long *)src_buf, (unsigned long)fsize);
    close(src_fd);

    g_src_phy = (uintptr_t)src_buf;

    bg_fd = open(CLOCK_IMAGE, O_RDONLY);
    if (bg_fd < 0) {
        LOGE("open second_bmp fail, path = %s\n", CLOCK_IMAGE);
        goto out;
    }
    lseek(bg_fd, 54, SEEK_SET);

    stat(CLOCK_IMAGE, &st);
    fsize = st.st_size;
    bg_buf = aicos_malloc(MEM_CMA, fsize);
    if (!bg_buf) {
        LOGE("malloc bg_buf fail\n");
        goto out;
    }
    memset(bg_buf, 0, fsize);

    ret = read(bg_fd, bg_buf, fsize - 54);
    aicos_dcache_clean_range((unsigned long *)bg_buf, (unsigned long)fsize);
    close(bg_fd);

    g_bg_phy = (uintptr_t)bg_buf;

    fb = mpp_fb_open();
    if (!fb) {
        LOGE("mpp fb open failed\n");
        goto out;
    }

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO , &g_info);
    if (ret) {
        LOGE("get screen info failed\n");
        goto out;
    }
    g_dst_phy = (unsigned long)g_info.framebuffer;

    ge = mpp_ge_open();
    if (!ge) {
        LOGE("ge open fail\n");
        goto out;
    }
    while (1) {
        // LOGI("num : %d\n", num);
        draw_clock(&blt, g_bg_phy, index);

        ret = mpp_ge_bitblt(ge, &blt);
        if (ret < 0) {
            LOGE("ge blt fail\n");
        }

        ret = mpp_ge_emit(ge);
        if (ret < 0) {
            LOGE("ge emit fail\n");
        }

        ret = mpp_ge_sync(ge);
        if (ret < 0) {
            LOGE("ge sync fail\n");
            break;
        }

        move_second_hand(ge, &rot,
                    degree_list[i * 2],
                    degree_list[i * 2 + 1],
                    g_src_phy, index);

        ret = mpp_ge_rotate(ge, &rot);
        if (ret < 0) {
            LOGE("ge rotate fail\n");
        }

        ret = mpp_ge_emit(ge);
        if (ret < 0) {
            LOGE("ge emit fail\n");
        }

        ret = mpp_ge_sync(ge);
        if (ret < 0) {
            LOGE("ge sync fail\n");
            break;
        }

        //LOGI("index : %d\n", index);
        if (APP_FB_NUM > 1) {
            ret = mpp_fb_ioctl(fb, AICFB_PAN_DISPLAY, &index);
            if (ret == 0) {
                ret = mpp_fb_ioctl(fb, AICFB_WAIT_FOR_VSYNC, &index);
                if (ret < 0)
                    LOGE("wait for sync error\n");
            } else {
                LOGE("pan display fail\n");
            }
        }

        i++;
        if (i == 61) {
            i = 0;
        }
        index = !index;
        num++;

        if (circle == 0) {
            if (num == loops)
                break;
        }
        aicos_msleep(1000);
    }

out:
    mpp_ge_close(ge);
    mpp_fb_close(fb);

    if (bg_buf)
        aicos_free(MEM_CMA, bg_buf);

    if (src_buf)
        aicos_free(MEM_CMA, src_buf);
}

static void ge_rotate(int argc, char **argv)
{
    aicos_thread_t thid = NULL;

    int ret = 0;
    int para[2] = {0};
    const char sopts[] = "ucn:";
    const struct option lopts[] = {
        {"usage",   no_argument, NULL, 'u'},
        {"circle",  no_argument, NULL, 'c'},
        {"number",  required_argument, NULL, 'n'},
        {0, 0, 0, 0}
    };

    optind = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'c':
            para[PARA_CIR] = 1;
            break;
        case 'n':
            if (str2int(optarg) <= 0) {
                LOGE("Invalid parameter: %#x\n", ret);
                return;
            }
            para[PARA_NUM] = str2int(optarg);
            break;
        case 'u':
            usage(argv[0]);
            return;
        default:
            LOGE("Invalid parameter: %#x\n", ret);
            return;
        }
    }

    thid = aicos_thread_create("ge_rotate", 8096, 0, ge_rotate_thread, para);
    if (thid == NULL) {
        LOGE("Failed to create thread\n");
        return;
    }
}
MSH_CMD_EXPORT_ALIAS(ge_rotate, ge_rotate, ge rotate test);

static void ge_bitblt(int argc, char **argv)
{
    LOGI("ge bitblt test app start........\n");

    int ret = -1, num = 0, loops = 1;
    struct mpp_ge *ge = NULL;
    int width = CLOCK_IMAGE_WIDTH;
    int height = CLOCK_IMAGE_HEIGHT;
    int src_fd = -1;
    int fsize = 0;
    void *src_buf = NULL;
    struct stat st;
    struct ge_bitblt blt = {0};
    struct mpp_fb *fb = NULL;

    int circle = 0;
    const char sopts[] = "ucn:";
    const struct option lopts[] = {
        {"usage",   no_argument, NULL, 'u'},
        {"circle",  no_argument, NULL, 'c'},
        {"number",  required_argument, NULL, 'n'},
        {0, 0, 0, 0}
    };

    optind = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'c':
            circle = 1;
            break;
        case 'n':
            if (str2int(optarg) <= 0) {
                LOGE("Invalid parameter: %#x\n", ret);
                return;
            }
            loops = str2int(optarg);
            break;
        case 'u':
            usage(argv[0]);
            return;
        default:
            LOGE("Invalid parameter: %#x\n", ret);
            return;
        }
    }

    src_fd = open(BLT_BMP, O_RDONLY);
    if (src_fd < 0) {
        LOGE("open blt_bmp fail, path = %s\n", BLT_BMP);
        return;
    }
    lseek(src_fd, 54, SEEK_SET);

    stat(BLT_BMP, &st);
    fsize = st.st_size;

    src_buf = aicos_malloc(MEM_CMA, fsize);
    if (!src_buf) {
        LOGE("malloc src fail, size: %d\n", fsize);
        return;
    }
    memset(src_buf, 0, fsize);

    ret = read(src_fd, src_buf, fsize - 54);
    LOGI("fsize: %d, ret: %d", fsize, ret);
    aicos_dcache_clean_range((unsigned long *)src_buf, (unsigned long)fsize);
    close(src_fd);

    g_src_phy = (uintptr_t)src_buf;

    fb = mpp_fb_open();
    if (!fb) {
        LOGE("mpp fb open failed\n");
        goto out;
    }

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO , &g_info);
    if (ret) {
        LOGE("get screen info failed\n");
        goto out;
    }
    g_dst_phy = (unsigned long)g_info.framebuffer;

    ge = mpp_ge_open();
    if (!ge) {
        LOGE("ge open fail\n");
        return;
    }

    while (1) {
        LOGI("num: %d\n", num);
        memset(&blt, 0, sizeof(struct ge_bitblt));

        /* source buffer */
        blt.src_buf.buf_type = MPP_PHY_ADDR;
        blt.src_buf.phy_addr[0] = g_src_phy;
        blt.src_buf.stride[0] = width * 4;
        blt.src_buf.size.width = width;
        blt.src_buf.size.height = height;
        blt.src_buf.format = MPP_FMT_ARGB_8888;
        blt.src_buf.crop_en = 0;

        /* dstination buffer */
        blt.dst_buf.buf_type = MPP_PHY_ADDR;
        blt.dst_buf.phy_addr[0] = g_dst_phy;
        blt.dst_buf.stride[0] = g_info.stride;
        blt.dst_buf.size.width = g_info.width;
        blt.dst_buf.size.height = g_info.height;
        blt.dst_buf.format = g_info.format;

        blt.dst_buf.crop_en = 1;
        blt.dst_buf.crop.x = 0;
        blt.dst_buf.crop.y = 0;
        blt.dst_buf.crop.width = width;
        blt.dst_buf.crop.height = height;

        ret =  mpp_ge_bitblt(ge, &blt);
        if (ret < 0) {
            LOGE("ge bitblt fail\n");
        }

        ret = mpp_ge_emit(ge);
        if (ret < 0) {
            LOGE("ge emit fail\n");
        }

        ret = mpp_ge_sync(ge);
        if (ret < 0) {
            LOGE("ge sync fail\n");
            break;
        }
        num++;
        aicos_msleep(1000);

        if (circle == 0) {
            if(num == loops)
                break;
        }
    }

out:
    mpp_ge_close(ge);
    mpp_fb_close(fb);
    aicos_free(MEM_CMA, src_buf);

    return;
}
MSH_CMD_EXPORT_ALIAS(ge_bitblt, ge_bitblt, ge bitblit test);

static void ge_fillrect(int argc, char **argv)
{
    LOGI("ge fillrect app test start........\n");

    int ret = -1, num = 0, loops = 1;
    int index = 0;
    struct mpp_ge *ge = NULL;
    struct mpp_fb *fb = NULL;
    struct ge_fillrect fill;
    int width;
    int height;

    int circle = 0;
    const char sopts[] = "ucn:";
    const struct option lopts[] = {
        {"usage",   no_argument, NULL, 'u'},
        {"circle",  no_argument, NULL, 'c'},
        {"number",  required_argument, NULL, 'n'},
        {0, 0, 0, 0}
    };

    optind = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'c':
            circle = 1;
            break;
        case 'n':
            if (str2int(optarg) <= 0) {
                LOGE("Invalid parameter: %#x\n", ret);
                return;
            }
            loops = str2int(optarg);
            break;
        case 'u':
            usage(argv[0]);
            return;
        default:
            LOGE("Invalid parameter: %#x\n", ret);
            return;
        }
    }

    fb = mpp_fb_open();
    if (!fb) {
        LOGE("open mpp fb failed\n");
        return;
    }

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO , &g_info);
    if (ret) {
        LOGE("get screen info failed\n");
        goto out;
    }

    g_dst_phy = (unsigned long)g_info.framebuffer;
    width = g_info.width;
    height = g_info.height;
    LOGI("g_dst_phy: 0x%08x, width: %d, height: %d\n", g_dst_phy, width, height);

    ge = mpp_ge_open();
    if (!ge) {
        LOGE("ge open fail\n");
        goto out;
    }

    while (1) {
        LOGI("fill: %d\n", num);
        memset(&fill, 0, sizeof(struct ge_fillrect));

        fill.type = GE_NO_GRADIENT;
        if (num%2 == 0)
            fill.start_color = 0xff11ff11;
        else
            fill.start_color = 0x11ff11ff;
        fill.end_color = 0;
        fill.dst_buf.buf_type = MPP_PHY_ADDR;

        if (!index) {
            fill.dst_buf.phy_addr[0] = g_dst_phy;
        } else {
            if (APP_FB_NUM > 1) {
                fill.dst_buf.phy_addr[0] = g_dst_phy + g_info.smem_len;
            } else {
                fill.dst_buf.phy_addr[0] = g_dst_phy;
            }
        }

        fill.dst_buf.stride[0] = g_info.stride;
        fill.dst_buf.size.width = g_info.width;
        fill.dst_buf.size.height = g_info.height;
        fill.dst_buf.format = g_info.format;
        fill.ctrl.flags = 0;

        fill.dst_buf.crop_en = 0;
        fill.dst_buf.crop.x = 0;
        fill.dst_buf.crop.y = 0;
        fill.dst_buf.crop.width = width;
        fill.dst_buf.crop.height = height;

        ret =  mpp_ge_fillrect(ge, &fill);
        if (ret < 0) {
            LOGE("ge fillrect fail\n");
        }

        ret = mpp_ge_emit(ge);
        if (ret < 0) {
            LOGE("ge emit fail\n");
        }

        ret = mpp_ge_sync(ge);
        if (ret < 0) {
            LOGE("ge sync fail\n");
            break;
        }
        num++;
        aicos_msleep(200);

        if (APP_FB_NUM > 1) {
            ret = mpp_fb_ioctl(fb, AICFB_PAN_DISPLAY, &index);
            if (ret == 0) {
                ret = mpp_fb_ioctl(fb, AICFB_WAIT_FOR_VSYNC, &index);
                if (ret < 0)
                    LOGE("wait for sync error\n");
            } else {
                LOGE("pan display fail\n");
            }
        }

        if (circle == 0) {
            if(num == loops)
                break;
        }

        index = !index;
    }

out:
    mpp_ge_close(ge);
    mpp_fb_close(fb);

    return;
}
MSH_CMD_EXPORT_ALIAS(ge_fillrect, ge_fill, ge fill test);
