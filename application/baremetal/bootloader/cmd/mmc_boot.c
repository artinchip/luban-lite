/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <boot_param.h>
#include <mmc.h>
#include <image.h>
#include <boot.h>
#include <hexdump.h>
#include "fitimage.h"

#define APPLICATION_PART "os"

static int do_mmc_boot(int argc, char *argv[])
{
    int ret = 0, mmc_id = 0;
    enum boot_device bd;
    struct aic_sdmc *host = NULL;
    struct aic_partition *part = NULL, *parts = NULL;
    struct spl_load_info info;
    ulong entry_point;

    bd = aic_get_boot_device();
    if (BD_SDMC0 == bd) {
        mmc_id = 0;
    } else if (BD_SDMC1 == bd) {
        mmc_id = 1;
    }

    ret = mmc_init(mmc_id);
    if (ret) {
        printf("sdmc %d init failed.\n", mmc_id);
        return ret;
    }

    host = find_mmc_dev_by_index(mmc_id);
    if (!host) {
        pr_err("find mmc dev failed.\n");
        return -1;
    }

    parts = mmc_create_gpt_part();
    if (!parts) {
        pr_err("sdmc %d create gpt part failed.\n", mmc_id);
        goto out;
    }

    part = parts;
    while (part) {
        if (!strcmp(part->name, APPLICATION_PART))
            break;

        part = part->next;
    }

    if (!part) {
        printf("Failed to get application partition.\n");
        goto out;
    }

    info.dev = (void *)host;
    info.priv = (void *)part;
    info.dev_type = DEVICE_MMC;
    info.bl_len = MMC_BLOCK_SIZE;

    ret = spl_load_simple_fit(&info, &entry_point);
    if (ret < 0)
        goto out;

    boot_app((void *)entry_point);

out:
    if (parts)
        mmc_free_partition(parts);
    return ret;
}

CONSOLE_CMD(mmc_boot, do_mmc_boot, "Boot from eMMC/SD.");
