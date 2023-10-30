/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <rtconfig.h>
#ifdef RT_USING_FINSH
#include <rthw.h>
#include <rtthread.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <aic_core.h>

#include "artinchip_fb.h"
#include "mpp_fb.h"

#define BLENDING_BG_COLOR   0x18a00100
#define DE_CONFIG_UPDATE    0x18a00008

#define BG_BLUE_MASK        GENMASK(7, 0)
#define BG_GREEN_MASK       GENMASK(15, 8)
#define BG_RED_MASK         GENMASK(23, 16)

#define PIXELS_BLUE(x)      (((x) & 0xff) << 0)
#define PIXELS_GREEN(x)     (((x) & 0xff) << 8)
#define PIXELS_RED(x)       (((x) & 0xff) << 16)

static inline void bg_update_bits(unsigned int mask, unsigned int value)
{
    void *base = (void *)BLENDING_BG_COLOR;
    void *update = (void *)DE_CONFIG_UPDATE;
    unsigned int tmp, orig;

    orig = readl(base);

    tmp = orig & ~mask;
    tmp |= value & mask;

    writel(tmp, base);
    writel(1, update);
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

static void usage(char *app)
{
    printf("Usage: %s [Options], built on %s %s\n", app, __DATE__, __TIME__);
    printf("\t-m, --mode, ");
    printf("\t0(default): disable background color, 1: enable background color");
    printf("\t-c, --ccm \n");
    printf("\t-r, --red \n");
    printf("\t-g, --green \n");
    printf("\t-b, --blue \n");
    printf("\t-u, --usage \n");
    printf("\n");
    printf("Example: %s -m 1 -r 45 -g 45 -b 45\n", app);
}

static int background_color_test(int argc, char **argv)
{
    struct mpp_fb *fb = NULL;
    int c, mode = 0, ret = 0;
    unsigned int red, green, blue;
    struct aicfb_layer_data layer = {0};

    const char sopts[] = "m:r:g:b:u";
    const struct option lopts[] = {
        {"mode",        required_argument, NULL, 'm'},
        {"red",         required_argument, NULL, 'r'},
        {"green",       required_argument, NULL, 'g'},
        {"blue",        required_argument, NULL, 'b'},
        {"usage",             no_argument, NULL, 'u'},
        {0, 0, 0, 0}
    };

    fb = mpp_fb_open();
    if(!fb) {
        pr_err("mpp fb open failed\n");
        return -1;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'u':
            usage(argv[0]);
            return 0;
        case 'm':
        {
            mode = str2int(optarg);
            break;
        }
        case 'r':
        {
            red = str2int(optarg);
            if (red > 255) {
                pr_err("Invaild red pixels: %d\n", red);
                break;
            }

            bg_update_bits(BG_RED_MASK, PIXELS_RED(red));
            break;
        }
        case 'g':
        {
            green = str2int(optarg);
            if (green > 255) {
                pr_err("Invaild blue pixels: %d\n", green);
                break;
            }

            bg_update_bits(BG_GREEN_MASK, PIXELS_GREEN(green));
            break;
        }
        case 'b':
        {
            blue = str2int(optarg);
            if (blue > 255) {
                pr_err("Invaild blue pixels: %d\n", blue);
                break;
            }

            bg_update_bits(BG_BLUE_MASK, PIXELS_BLUE(blue));
            break;
        }
        default:
            pr_err("Invalid parameter: %#x\n", ret);
            usage(argv[0]);
            return 0;
        }
    }

    layer.layer_id = AICFB_LAYER_TYPE_UI;
    layer.rect_id = 0;
    mpp_fb_ioctl(fb, AICFB_GET_LAYER_CONFIG, &layer);

    switch (mode) {
    case 0:
        layer.enable = 1;
        break;
    case 1:
        layer.enable = 0;
        break;
    default:
        printf("Invalid mode: %d\n", mode);
        break;
    }

    mpp_fb_ioctl(fb, AICFB_UPDATE_LAYER_CONFIG, &layer);

    mpp_fb_close(fb);
    return 0;
}
MSH_CMD_EXPORT_ALIAS(background_color_test, bg_color, config background color);
#endif /* RT_USING_FINSH */


