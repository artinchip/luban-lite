/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#include <posix/string.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "aic_log.h"
#include "aic_osal.h"
#include "aic_drv_gpio.h"

#include "artinchip_fb.h"
#include "mpp_fb.h"

/* Global macro and variables */

#define AICFB_LAYER_MAX_NUM 2
#define FB_DEV "/dev/fb0"

static const char sopts[] = "nsflLaAkKedi:w:h:m:v:bu";
static const struct option lopts[] = {
    {"get_layer_num",       no_argument, NULL, 'n'},
    {"get_screen_size",     no_argument, NULL, 's'},
    {"get_fb_layer",        no_argument, NULL, 'f'},
    {"get_layer",           no_argument, NULL, 'l'},
    {"set_layer",           no_argument, NULL, 'L'},
    {"get_alpha",           no_argument, NULL, 'a'},
    {"set_alpha",     required_argument, NULL, 'A'},
    {"get_ck_cfg",          no_argument, NULL, 'k'},
    {"set_ck_cfg",    required_argument, NULL, 'K'},
    {"enable",              no_argument, NULL, 'e'},
    {"disable",             no_argument, NULL, 'd'},
    {"id",            required_argument, NULL, 'i'},
    {"width",         required_argument, NULL, 'w'},
    {"height",        required_argument, NULL, 'h'},
    {"mode",          required_argument, NULL, 'm'},
    {"value",         required_argument, NULL, 'v'},
    {"colorblock",          no_argument, NULL, 'b'},
    {"usage",               no_argument, NULL, 'u'},
    {0, 0, 0, 0}
};

static struct mpp_fb *g_fb = NULL;

/* Functions */

static void usage(char *program)
{
    printf("Usage: %s [options]: \n", program);
    printf("\t -n, --get_layer_num \n");
    printf("\t -s, --get_screen_size \n");
    printf("\t -f, --get_fb_layer \n");
    printf("\t -l, --get_layer \n");
    printf("\t -L, --set_layer\tneed other options: -i x -e/d -w y -h z\n");
    printf("\t -a, --get_alpha \n");
    printf("\t -A, --set_alpha\tneed other options: -e/d -m x -v y \n");
    printf("\t -k, --get_ck_cfg \n");
    printf("\t -K, --set_ck_cfg\tneed other options: -e/d -v x \n");
    printf("\t -e, --enable \n");
    printf("\t -d, --disable \n");
    printf("\t -i, --id\t\tneed an integer argument of Layer ID [0, 1]\n");
    printf("\t -w, --width\t\tneed an integer argument\n");
    printf("\t -h, --height\t\tneed an integer argument\n");
    printf("\t -m, --mode\t\tneed an integer argument [0, 2]\n");
    printf("\t -v, --value\t\tneed an integer argument [0, 255]\n");
    printf("\t -b, --colorblock\tshow a color-block image\n");
    printf("\t -u, --usage \n");
    printf("\n");
    printf("Example: %s -l\n", program);
    printf("Example: %s -L -i 1 -e -w 800 -h 480\n", program);
    printf("Example: %s -A -e -w 800 -h 480\n", program);
    printf("Example: %s -K -e -v 0x3F\n", program);
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

int get_layer_num(int fd)
{
    int ret = 0;
    struct aicfb_layer_num n = {0};

    ret = mpp_fb_ioctl(g_fb, AICFB_GET_LAYER_NUM, &n);
    if (ret < 0) {
        pr_err("ioctl() return %d\n", ret);
    } else {
        printf("The number of video layer: %d\n", n.vi_num);
        printf("The number of UI layer: %d\n", n.ui_num);
    }
    return ret;
}

int get_one_layer_cfg(int fd, int cmd, int id)
{
    int ret = 0;
    struct aicfb_layer_data layer = {0};

    layer.layer_id = id;
    ret = mpp_fb_ioctl(g_fb, cmd, &layer);
    if (ret < 0) {
        pr_err("ioctl() return %d\n", ret);
        return ret;
    }

    printf("\n--------------- Layer %d ---------------\n", layer.layer_id);
    printf("Enable: %d\n", layer.enable);
    printf("Layer ID: %d\n", layer.layer_id);
    printf("Rect ID: %d\n", layer.rect_id);
    printf("Scale Size: w %d, h %d\n",
        layer.scale_size.width, layer.scale_size.height);
    printf("Position: x %d, y %d\n", layer.pos.x, layer.pos.y);
    printf("Buffer: \n");
    printf("\tPixel format: %#x\n", layer.buf.format);
    printf("\tPhysical addr: %#x %#x %#x\n",
        layer.buf.phy_addr[0], layer.buf.phy_addr[1],
        layer.buf.phy_addr[2]);
    printf("\tStride: %d %d %d\n", layer.buf.stride[0],
        layer.buf.stride[1], layer.buf.stride[2]);
    printf("\tSize: w %d, h %d\n",
        layer.buf.size.width, layer.buf.size.height);
    printf("\tCrop enable: %d\n", layer.buf.crop_en);
    printf("\tCrop: x %d, y %d, w %d, h %d\n",
        layer.buf.crop.x, layer.buf.crop.y,
        layer.buf.crop.width, layer.buf.crop.height);
    printf("\tFlag: %#x\n", layer.buf.flags);
    printf("---------------------------------------\n");

    return 0;
}

int get_fb_layer_cfg(int fd)
{
    return get_one_layer_cfg(fd, AICFB_GET_LAYER_CONFIG, 0);
}

int get_layer_cfg(int fd)
{
    int i;

    for (i = 0; i < AICFB_LAYER_MAX_NUM; i++)
        get_one_layer_cfg(fd, AICFB_GET_LAYER_CONFIG, i);

    return 0;
}

int set_layer_cfg(int fd, int id, int enable, int width, int height)
{
    int ret = 0;
    struct aicfb_layer_data layer = {0};

    if ((id != AICFB_LAYER_TYPE_UI) || (enable < 0) ||
        (width < 0) || (height < 0)) {
        pr_err("Invalid argument.\n");
        return -1;
    }

    /* Get the current configuration of UI layer */
    layer.layer_id = id;
    ret = mpp_fb_ioctl(g_fb, AICFB_GET_LAYER_CONFIG, &layer);
    if (ret < 0) {
        pr_err("ioctl() return %d\n", ret);
        return ret;
    }
    if (width > layer.buf.size.width || height > layer.buf.size.height) {
        pr_err("Width %d x Height %d is out of range.\n", width, height);
        return -1;
    }

    layer.enable      = enable;
    layer.buf.crop_en = 1;
    layer.buf.crop.x  = 0;
    layer.buf.crop.y  = 0;
    layer.buf.crop.width = width;
    layer.buf.crop.height = height;
    ret = mpp_fb_ioctl(g_fb, AICFB_UPDATE_LAYER_CONFIG, &layer);
    if (ret < 0)
        pr_err("ioctl() return %d\n", ret);

    return ret;
}

int get_layer_alpha(int fd)
{
    int i;
    int ret = 0;
    struct aicfb_alpha_config alpha = {0};

    for (i = 1; i < AICFB_LAYER_MAX_NUM; i++) {
        alpha.layer_id = i;
        ret = mpp_fb_ioctl(g_fb, AICFB_GET_ALPHA_CONFIG, &alpha);
        if (ret < 0) {
            pr_err("ioctl() return %d\n", ret);
            return ret;
        }

        printf("\n--------------- Layer %d ---------------\n", i);
        printf("Alpha enable: %d\n", alpha.enable);
        printf("Alpla mode: %d (0, pixel; 1, global; 2, mix)\n",
            alpha.mode);
        printf("Alpha value: %d (%#x)\n", alpha.value, alpha.value);
        printf("---------------------------------------\n");
    }

    return 0;
}

int set_layer_alpha(int fd, int enable, int mode, int val)
{
    int ret = 0;
    struct aicfb_alpha_config alpha = {0};

    if ((enable < 0) || (mode < 0) || (val < 0)) {
        pr_err("Invalid argument.\n");
        return -1;
    }

    alpha.layer_id = 1;
    alpha.enable = enable;
    alpha.mode = mode;
    alpha.value = val;
    ret = mpp_fb_ioctl(g_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    if (ret < 0)
        pr_err("ioctl() return %d\n", ret);

    return ret;
}

int get_ck_cfg(int fd)
{
    int i;
    int ret = 0;
    struct aicfb_ck_config ck = {0};

    for (i = 1; i < AICFB_LAYER_MAX_NUM; i++) {
        ck.layer_id = i;
        ret = mpp_fb_ioctl(g_fb, AICFB_GET_CK_CONFIG, &ck);
        if (ret < 0) {
            pr_err("ioctl() return %d\n", ret);
            return ret;
        }

        printf("\n--------------- Layer %d ---------------\n", i);
        printf("Color key enable: %d\n", ck.enable);
        printf("Color key value: R %#x, G %#x, B %#x\n",
            (ck.value >> 16) & 0xFF, (ck.value >> 8) & 0xFF,
            ck.value & 0xFF);
        printf("---------------------------------------\n");
    }
    return 0;
}

int set_ck_cfg(int fd, int enable, int val)
{
    int ret = 0;
    struct aicfb_ck_config ck = {0};

    if ((enable < 0) || (val < 0)) {
        pr_err("Invalid argument.\n");
        return -1;
    }

    ck.layer_id = 1;
    ck.enable = enable;
    ck.value = val;
    ret = mpp_fb_ioctl(g_fb, AICFB_UPDATE_CK_CONFIG, &ck);
    if (ret < 0)
        pr_err("ioctl() return %d\n", ret);

    return ret;
}

int get_screen_size(int fd)
{
    int ret = 0;
    struct aicfb_screeninfo s = {0};

    ret = mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &s);
    if (ret < 0) {
        pr_err("ioctl() return %d\n", ret);
        return ret;
    }

    printf("Screen width: %d (%#x)\n", s.width, s.width);
    printf("Screen height: %d (%#x)\n", s.height, s.height);
    return 0;
}

int show_color_block(int fd)
{
    struct aicfb_screeninfo s = {0};

    int i, j, color, step, blk_line, pixel_size, width, height, blk_height;
    int colors24[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF};
    int steps24[]  = {0x010000, 0x000100, 0x000001, 0x010101};
    int colors16[] = {0xF800, 0x07E0, 0x001F, 0xFFFF};
    int steps16[]  = {0x0800, 0x0020, 0x0001, 0x0821};
    int *colors = colors24;
    int *steps  = steps24;
    char *fb_buf = NULL, *line1 = NULL, *line2 = NULL;

    if (mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &s) < 0) {
        pr_err("ioctl AICFB_GET_SCREENINFO\n");
        return -1;
    }

    pixel_size = s.bits_per_pixel / 8;
    if (pixel_size == 2) {
        colors = colors16;
        steps  = steps16;
    } else {
        pixel_size = 4;
    }

    fb_buf = (char *)s.framebuffer;
    if (fb_buf == NULL) {
        pr_err("Invalid framebuffer\n");
        return -1;
    }
    memset(fb_buf, 0, s.smem_len);

    width  = s.width;
    height = s.height;
    printf("Framebuf: size %d, width %d, height %d, bits per pixel %d\n",
           s.smem_len, width, height, s.bits_per_pixel);

    blk_height = height / 4;
    line1 = fb_buf;
    for (i = 0; i < height; i++) {
        blk_line = i / blk_height;
        color = colors[blk_line];
        step = steps[blk_line];
        for (j = 0; j < width; j++) {
            memcpy(&line1[j * pixel_size], &color, pixel_size);
            color -= step;
            if (color == 0) {
                color = colors[blk_line];
                j++;
            }
        }
        line1 += width * pixel_size;
    }

    /* Draw the location line */

    line1 = &fb_buf[width * pixel_size] + pixel_size;
    line2 = &fb_buf[width * (height - 2) * pixel_size - 101 * pixel_size];
    for (i = 0; i < 100; i++) {
        memcpy(&line1[i * pixel_size], &colors[3], pixel_size);
        memcpy(&line2[i * pixel_size], &colors[3], pixel_size);
    }

    line1 = &fb_buf[width * pixel_size] + pixel_size;
    line2 = &fb_buf[width * (height - 101) * pixel_size - pixel_size];
    for (i = 0; i < 100; i++) {
        memcpy(&line1[0], &colors[3], pixel_size);
        line1 += width * pixel_size;
        memcpy(&line2[0], &colors[3], pixel_size);
        line2 += width * pixel_size;
    }

    aicos_dcache_clean_range(fb_buf, s.smem_len);
    return 0;
}

static int cmd_test_fb(int argc, char **argv)
{
    int dev_fd = -1;
    int ret = -1;
    int c = 0;
    int layer_id = 0;
    int enable = 0;
    int mode = 0;
    int width = 0;
    int height = 0;
    int value = 0;

    g_fb = mpp_fb_open();
    if (!g_fb) {
        pr_err("Failed to open FB\n");
        goto end;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'n':
            ret = get_layer_num(dev_fd);
            goto end;
        case 's':
            ret = get_screen_size(dev_fd);
            goto end;
        case 'f':
            ret = get_fb_layer_cfg(dev_fd);
            goto end;
        case 'l':
            ret = get_layer_cfg(dev_fd);
            goto end;
        case 'a':
            ret = get_layer_alpha(dev_fd);
            goto end;
        case 'k':
            ret = get_ck_cfg(dev_fd);
            goto end;
        case 'e':
            enable = 1;
            continue;
        case 'd':
            enable = 0;
            continue;
        case 'i':
            layer_id = str2int(optarg);
            continue;
        case 'w':
            width = str2int(optarg);
            continue;
        case 'h':
            height = str2int(optarg);
            continue;
        case 'm':
            mode = str2int(optarg);
            continue;
        case 'v':
            value = str2int(optarg);
            continue;
        case 'b':
            show_color_block(dev_fd);
            goto end;
        case 'u':
            usage(argv[0]);
            goto end;
        default:
            continue;
        }
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'L':
            ret = set_layer_cfg(dev_fd, layer_id, enable, width, height);
            goto end;
        case 'A':
            ret = set_layer_alpha(dev_fd, enable, mode, value);
            goto end;
        case 'K':
            ret = set_ck_cfg(dev_fd, enable, value);
            goto end;
        default:
            continue;
        }
    }

end:
    rt_set_errno(ret);
    if (g_fb) {
        mpp_fb_close(g_fb);
        g_fb = NULL;
    }
    return ret;
}
MSH_CMD_EXPORT_ALIAS(cmd_test_fb, test_fb, Test Framebuffer);
