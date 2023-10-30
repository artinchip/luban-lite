/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
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

#ifndef LOG_TAG
#define LOG_TAG "ge_test"
#endif

#define LOGE(fmt, ...) aic_log(AIC_LOG_ERR, "E", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) aic_log(AIC_LOG_WARN, "W", fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) aic_log(AIC_LOG_INFO, "I", fmt, ##__VA_ARGS__)

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
    printf("\t-b, --bright \n");
    printf("\t-c, --contrast \n");
    printf("\t-s, --saturation \n");
    printf("\t-h, --hue \n");
    printf("\t-u, --usage \n");
    printf("\n");
    printf("All parameter range in [0, 100], 50 means no effect, default is 50\n");
    printf("\n");
    printf("Example: %s -b 45 -c 40 -s 45 -h 44\n", app);
}

static int disp_prop_test(int argc, char **argv)
{
    struct mpp_fb *fb = NULL;
    struct aicfb_disp_prop disp_prop = {50, 50, 50, 50};
    int c, ret = 0;
    const char sopts[] = "b:c:s:h:u";
    const struct option lopts[] = {
        {"bright",     required_argument, NULL, 'b'},
        {"contrast",   required_argument, NULL, 'c'},
        {"saturation", required_argument, NULL, 's'},
        {"hue",        required_argument, NULL, 'h'},
        {"usage",            no_argument, NULL, 'u'},
        {0, 0, 0, 0}
    };

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'u':
            usage(argv[0]);
            return 0;
        case 'b':
            disp_prop.bright = str2int(optarg);
            break;
        case 'c':
            disp_prop.contrast = str2int(optarg);
            break;
        case 's':
            disp_prop.saturation = str2int(optarg);
            break;
        case 'h':
            disp_prop.hue = str2int(optarg);
            break;
        default:
            LOGE("Invalid parameter: %#x\n", ret);
            usage(argv[0]);
            return 0;
        }
    }

    fb = mpp_fb_open();
    if(!fb) {
        LOGE("mpp fb open failed\n");
        return -1;
    }

    ret = mpp_fb_ioctl(fb, AICFB_SET_DISP_PROP, &disp_prop);
    if (ret) {
        LOGE("mpp fb ioctl set disp prop failed\n");
        return ret;
    }

    LOGI("mpp fb update disp prop\n");

    mpp_fb_close(fb);
    return 0;
}
MSH_CMD_EXPORT_ALIAS(disp_prop_test, disp_prop, display property test);
#endif /* RT_USING_FINSH */
