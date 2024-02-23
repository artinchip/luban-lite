/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#if defined(RT_USING_FINSH) && defined(AIC_AXICFG_DRV)
#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <aic_common.h>
#include <hal_axicfg.h>

#define printf rt_kprintf

#define AXICFG_HELP                                                    \
    "axicfg command:\n"                                                \
    "  axicfg <device> <mode> <priority>\n"                            \
    "    device: dec value, get from chip spec (CPU=0, DE=2, GE=3) \n" \
    "    mode: should be \"w\" \"r\" \"a\" \n"                         \
    "    priority: dec value \n"                                       \
    "  e.g.: \n"                                                       \
    "    axicfg 2 a 10  (read and write mode)\n"                       \
    "    axicfg 2 w 10\n"                                              \
    "    axicfg 2 r 10\n"

static void axicfg_config_display_help(void)
{
    puts(AXICFG_HELP);
}

static void do_axicfg_config(int argc, char **argv)
{
    u8 device_p = 0;
    u8 priority = 0;
    u8 mode = 0;

    if (argc < 4) {
        goto help;
    }

    if (argc > 3) {
        device_p = atoi(argv[1]);
        priority = atoi(argv[3]);

        if (*argv[2] == 'w') {
            mode = 1;
        } else if (*argv[2] == 'r') {
            mode = 0;
        } else if (*argv[2] == 'a') {
            hal_axicfg_module_wr_init(0, device_p, priority);
            hal_axicfg_module_wr_init(1, device_p, priority);
            goto exit;
        } else {
            goto help;
        }
        hal_axicfg_module_wr_init(mode, device_p, priority);
    }

exit:
    return;
help:
    axicfg_config_display_help();
    return;
}

MSH_CMD_EXPORT_ALIAS(do_axicfg_config, axicfg, config IP access AXI priority);

#endif /* RT_USING_FINSH */
