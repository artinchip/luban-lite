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

#include "mpp_fb.h"

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
    printf("\tread a mipi-dsi screen register \n");
    printf("\t-r, --register \n");
    printf("\t-u, --usage \n");
    printf("\n");
    printf("Example: %s -r 0x04 \n", app);
}

static int screen_register_test(int argc, char **argv)
{
    struct mpp_fb *fb = NULL;
    int c, reg = 0, ret = 0;
    int val;

    const char sopts[] = "r:u";
    const struct option lopts[] = {
        {"register",    required_argument, NULL, 'r'},
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
        case 'r':
        {
            reg = str2int(optarg);
            break;
        }
        default:
            pr_err("Invalid parameter: %#x\n", ret);
            usage(argv[0]);
            return 0;
        }
    }

    val = mpp_fb_ioctl(fb, AICFB_GET_SCREENREG, &reg);

    printf("read %#x register value: %#x\n", reg, val);

    return 0;
}

MSH_CMD_EXPORT_ALIAS(screen_register_test, screen_register,
        test screen register value);
#endif /* RT_USING_FINSH */
