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
    printf("\t0(default): disable ccm and gamma, 1: just enable ccm\n");
    printf("\t\t2: just enable gamma, 3: enable ccm and gamma\n");
    printf("\t-c, --ccm \n");
    printf("\t-r, --gammared \n");
    printf("\t-g, --gammagreen \n");
    printf("\t-b, --gammablue \n");
    printf("\t-u, --usage \n");
    printf("\n");
    printf("Example: %s -m 1 -c 45 \n", app);
}

#define FLAGS_NONE  0x00
#define FLAGS_CCM   (0x1 << 1)
#define FLAGS_GAMMA (0x1 << 2)

static int ccm_gamma_test(int argc, char **argv)
{
    struct mpp_fb *fb = NULL;
    int c, ret = 0, mode = 0, num = 0;
    struct aicfb_ccm_config ccm;
    struct aicfb_gamma_config gamma;
    unsigned int flags = FLAGS_NONE;
    int value;

    const char sopts[] = "m:c:n:r:g:b:u";
    const struct option lopts[] = {
        {"mode",       required_argument, NULL, 'm'},
        {"ccm",        required_argument, NULL, 'c'},
        {"num",        required_argument, NULL, 'n'},
        {"gammared",   required_argument, NULL, 'r'},
        {"gammagreen", required_argument, NULL, 'g'},
        {"gammablue",  required_argument, NULL, 'b'},
        {"usage",            no_argument, NULL, 'u'},
        {0, 0, 0, 0}
    };

    fb = mpp_fb_open();
    if(!fb) {
        pr_err("mpp fb open failed\n");
        return -1;
    }

    mpp_fb_ioctl(fb, AICFB_GET_GAMMA_CONFIG, &gamma);
    mpp_fb_ioctl(fb, AICFB_GET_CCM_CONFIG, &ccm);

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'u':
            usage(argv[0]);
            return 0;
        case 'm':
        {
            mode = str2int(optarg);
            switch (mode) {
            case 0:
                ccm.enable = 0;
                gamma.enable = 0;
                break;
            case 1:
                ccm.enable = 0;
                break;
            case 2:
                gamma.enable = 0;
                break;
            default:
                break;
            }

            break;
        }
        case 'n':
        {
            num = str2int(optarg);
            break;
        }
        case 'c':
        {
            int len, i;
            char str[5] = {0};

            len = strlen(optarg);
            if (len != 12 * 4) {
                pr_err("Invaild ccm table, table len %d\n", len);
                break;
            }

            for (i = 0; i < 12; i++) {
                strncpy(str, optarg + (i * 4), 4);

                ccm.ccm_table[i] = strtoll(str, NULL, 16);
            }

            flags |= FLAGS_CCM;
            break;
        }
        case 'r':
        {
            int len, i;
            char str[3] = {0};

            len = strlen(optarg);
            if (len != 16 * 2) {
                pr_err("Invaild gamma table, table len %d\n", len);
                break;
            }

            for (i = 0; i < 16; i++) {
                strncpy(str, optarg + (i * 2), 2);

                value = strtoll(str, NULL, 16);
                gamma.gamma_lut[GAMMA_RED][num * 16 + i] = value;
            }
            break;
        }
        case 'g':
        {
            int len, i;
            char str[3] = {0};

            len = strlen(optarg);
            if (len != 16 * 2) {
                pr_err("Invaild gamma table, table len %d\n", len);
                break;
            }

            for (i = 0; i < 16; i++) {
                strncpy(str, optarg + (i * 2), 2);

                value = strtoll(str, NULL, 16);
                gamma.gamma_lut[GAMMA_BLUE][num * 16 + i] = value;
            }
            break;
        }
        case 'b':
        {
            int len, i;
            char str[3] = {0};

            len = strlen(optarg);
            if (len != 16 * 2) {
                pr_err("Invaild gamma table, table len %d\n", len);
                break;
            }

            for (i = 0; i < 16; i++) {
                strncpy(str, optarg + (i * 2), 2);

                value = strtoll(str, NULL, 16);
                gamma.gamma_lut[GAMMA_GREEN][num * 16 + i] = value;
            }

            if (num == 3)
                flags |= FLAGS_GAMMA;

            break;
        }
        default:
            pr_err("Invalid parameter: %#x\n", ret);
            usage(argv[0]);
            return 0;
        }
    }

    if (flags & FLAGS_GAMMA)
        gamma.enable = 1;

    mpp_fb_ioctl(fb, AICFB_SET_GAMMA_CONFIG, &gamma);

    if (flags & FLAGS_CCM)
        ccm.enable = 1;

    mpp_fb_ioctl(fb, AICFB_SET_CCM_CONFIG, &ccm);

    mpp_fb_close(fb);
    return 0;
}
MSH_CMD_EXPORT_ALIAS(ccm_gamma_test, ccm_gamma, CCM or Gamma test);
#endif /* RT_USING_FINSH */

