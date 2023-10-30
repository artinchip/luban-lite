/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <rtconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <spinand_port.h>
#include <mtd.h>
#include <image.h>
#include <boot.h>
#include <hexdump.h>
#include "fitimage.h"

#define APPLICATION_PART "os"
#define PAGE_MAX_SIZE    4096

static int do_nand_boot(int argc, char *argv[])
{
    int ret = 0;
    struct mtd_dev *mtd;
    ulong entry_point;
    struct spl_load_info info;

    mtd_probe();
    mtd = mtd_get_device(APPLICATION_PART);
    if (!mtd) {
        printf("Failed to get application partition.\n");
        return -1;
    }

    info.dev = (void *)mtd;
    info.bl_len = mtd->writesize;

    spl_load_simple_fit(&info, &entry_point);

    boot_app((void *)entry_point);


    return ret;
}

CONSOLE_CMD(nand_boot, do_nand_boot, "Boot from NAND.");
