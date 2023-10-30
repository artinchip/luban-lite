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
#include <image.h>
#include <boot.h>
#include "aic_time.h"

#define APPLICATION_PART "os"

static int do_ram_boot(int argc, char *argv[])
{
    int ret = 0;
    uint8_t *data;
    struct image_header head;
    void *la;
    unsigned long addr;

    addr = strtol(argv[1], NULL, 0);
    data = (uint8_t *)addr;
    memcpy(&head, data, sizeof(head));
    ret = image_verify_magic((void *)&head, AIC_IMAGE_MAGIC);
    if (ret) {
        printf("Application header is unknown.\n");
        return -1;
    }

    la = (void *)(unsigned long)head.load_address;

    memcpy(la, data, head.image_len);

    if (ret < 0)
        return -1;

    boot_app(la);

    return ret;
}

CONSOLE_CMD(ram_boot, do_ram_boot, "Boot from RAM.");
