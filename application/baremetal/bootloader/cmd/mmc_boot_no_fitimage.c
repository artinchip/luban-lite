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

#define APPLICATION_PART "os"

static int do_mmc_boot(int argc, char *argv[])
{
    int ret = 0, mmc_id = 0;
    enum boot_device bd;
    struct image_header *head = NULL;
    struct aic_sdmc *host = NULL;
    struct aic_partition *part = NULL, *parts = NULL;
    void *la;
    u64 blkstart, blkcnt;
    u32 start_us;

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

    head = malloc(MMC_BLOCK_SIZE);
    blkstart = part->start / MMC_BLOCK_SIZE;
    ret = mmc_bread(host, blkstart, 1, (void *)head);
    if (ret < 0) {
        printf("Read image header failed.\n");
        goto out;
    }

    ret = image_verify_magic((void *)head, AIC_IMAGE_MAGIC);
    if (ret) {
        printf("Application header is unknown.\n");
        goto out;
    }

    la = (void *)(unsigned long)head->load_address;

    start_us =  aic_get_time_us();
    blkcnt = ROUNDUP(head->image_len, MMC_BLOCK_SIZE) / MMC_BLOCK_SIZE;
    ret = mmc_bread(host, blkstart, blkcnt, la);
    show_speed("mmc read speed", head->image_len, aic_get_time_us() - start_us);

    if (ret < 0) {
        printf("Read image failed.\n");
        goto out;
    }

    boot_app(la);

out:
    if (parts)
        mmc_free_partition(parts);
    if (head)
        free(head);
    return ret;
}

CONSOLE_CMD(mmc_boot, do_mmc_boot, "Boot from eMMC/SD.");
