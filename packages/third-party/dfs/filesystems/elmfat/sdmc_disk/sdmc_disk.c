/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <dfs_bare.h>

#include "aic_osal.h"
#include "sdmc_disk.h"
#include "mmc.h"

/*******************************************************************************
 * Definitons
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

struct aic_sdmc *g_sdmc_host = NULL;
static struct rt_device_blk_geometry info = {0};

/*******************************************************************************
 * Code
 ******************************************************************************/
DRESULT sdmc_disk_write(uint8_t pdrv, const char *device_name, const uint8_t *buf, uint32_t sector, uint8_t cnt)
{
    rt_size_t size = 0;

    if (!g_sdmc_host)
        return RES_NOTRDY;

    size = mmc_bwrite(g_sdmc_host, sector, cnt, buf);
    if (size != cnt) {
        pr_err("write() return %d/%d\n", size, cnt);
        return RES_ERROR;
    }

    return RES_OK;
}

DRESULT sdmc_disk_read(uint8_t pdrv, const char *device_name, uint8_t *buf, uint32_t sector, uint8_t cnt)
{
    rt_size_t size = 0;

    if (!g_sdmc_host)
        return RES_NOTRDY;

    size = mmc_bread(g_sdmc_host, sector, cnt, buf);
    if (size != cnt) {
        pr_err("read() return %d/%d\n", size, cnt);
        return RES_ERROR;
    }

    return RES_OK;
}

DRESULT sdmc_disk_ioctl(uint8_t pdrv, const char *device_name, uint8_t command, void *buf)
{
    DRESULT result = RES_OK;

    switch (command) {
    case GET_SECTOR_COUNT:
        if (buf) {
            *(uint32_t *)buf = info.sector_count;
        } else {
            result = RES_PARERR;
        }

        break;

    case GET_SECTOR_SIZE:
        if (buf) {
            *(uint32_t *)buf = info.bytes_per_sector;
        } else {
            result = RES_PARERR;
        }

        break;

    case GET_BLOCK_SIZE:
        if (buf) {
            *(uint32_t *)buf = info.block_size;
        } else {
            result = RES_PARERR;
        }

        break;

    case CTRL_SYNC:
        result = RES_OK;
        break;

    default:
        result = RES_PARERR;
        break;
    }

    return result;
}

DSTATUS sdmc_disk_status(uint8_t pdrv, const char *device_name)
{
    return RES_OK;
}

DSTATUS sdmc_disk_initialize(uint8_t pdrv, const char *device_name)
{
    g_sdmc_host = find_mmc_dev_by_index(pdrv);
    if (!g_sdmc_host)
        return RES_NOTRDY;

    info.block_size = g_sdmc_host->dev->max_blk_size;
    info.bytes_per_sector = info.block_size;
    info.sector_count = g_sdmc_host->dev->card_capacity * 2; /* unit: block */

    return RES_OK;
}
