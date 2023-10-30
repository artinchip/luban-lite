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
#include <sfud.h>
#include <mtd.h>
#include <image.h>
#include <boot.h>
#include <hexdump.h>
#include "aic_time.h"

#define APPLICATION_PART "os"

extern sfud_flash *sfud_probe(u32 spi_bus);

static int do_nor_boot(int argc, char *argv[])
{
    int ret = 0;
    struct image_header head;
    struct mtd_dev *mtd;
    void *la;
    u32 start_us;

    mtd_probe();

    mtd = mtd_get_device(APPLICATION_PART);
    if (!mtd) {
        printf("Failed to get application partition.\n");
        return -1;
    }
    ret = mtd_read(mtd, 0, (void *)&head, sizeof(head));
    if (ret < 0)
        return -1;

    ret = image_verify_magic((void *)&head, AIC_IMAGE_MAGIC);
    if (ret) {
        printf("Application header is unknown.\n");
        return -1;
    }

    la = (void *)(unsigned long)head.load_address;

    start_us =  aic_get_time_us();
    ret = mtd_read(mtd, 0, la, head.image_len);
    show_speed("nor read speed", head.image_len, aic_get_time_us() - start_us);

    if (ret < 0)
        return -1;

    boot_app(la);

    return ret;
}

CONSOLE_CMD(nor_boot, do_nor_boot, "Boot from NOR.");

