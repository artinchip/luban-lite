/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <rtdevice.h>
#include <aic_log.h>
#include <rtconfig.h>

#if defined(RT_USING_FAL)
#include <fal.h>

#ifndef RT_DFS_FAL_BLK_DEVICE_NANE
#define RT_DFS_FAL_BLK_DEVICE_NANE ""
#endif

int mnt_init_spiflash0(void)
{
    const struct fal_partition *parts;
    struct rt_device *flash_dev;
    int result = 0;
    size_t i, len;

    fal_init();

    parts = fal_get_partition_table(&len);
    if (!parts) {
        pr_info("No FAL partitions.\n");
        return result;
    }

    for (i = 0; i < len; i++) {
        flash_dev = fal_mtd_nor_device_create(parts[i].name);
        if (flash_dev == NULL) {
            pr_err("Can't create a mtd device on '%s' partition.",
                   parts[i].name);
            result = -1;
            break;
        }

        flash_dev = fal_blk_device_create(parts[i].name);
        if (flash_dev == NULL) {
            pr_err("Can't create a blk device on '%s' partition.",
                   parts[i].name);
            result = -1;
            break;
        }
    }

    return result;
}

INIT_DEVICE_EXPORT(mnt_init_spiflash0);
#endif
