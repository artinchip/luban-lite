/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <dfs_bare.h>
#include <rtconfig.h>
#include "aic_osal.h"
#include "spinand_disk.h"
#include "mtd.h"

static struct spinand_blk_device *blk_device = NULL;
static struct rt_device_blk_geometry info = { 0 };

/*******************************************************************************
 * Code
 ******************************************************************************/
DRESULT spinand_disk_write(const char *device_name, const uint8_t *buf,
                           uint32_t sector, uint8_t cnt)
{
    if (!blk_device->mtd_device)
        return RES_NOTRDY;

    return RES_OK;
}

DRESULT spinand_disk_read(const char *device_name, uint8_t *buf,
                          uint32_t sector, uint8_t cnt)
{
    rt_size_t sectors_per_page;
    rt_size_t start_page, block, offset;
    unsigned char *copybuf = NULL;
    rt_size_t sectors_read = 0;
    rt_size_t copysize = 0;
    rt_size_t pages_per_block;
    rt_size_t ret = 0;
    struct mtd_dev *mtd = blk_device->mtd_device;

    if (!blk_device->mtd_device)
        return RES_NOTRDY;

    pages_per_block = mtd->erasesize / mtd->writesize;
    sectors_per_page = mtd->writesize / info.bytes_per_sector;

    block = sector * info.bytes_per_sector / mtd->erasesize;
    offset = block * mtd->erasesize;

    /* Search for the first good block after the given offset */
    while (mtd->ops.block_isbad(mtd, offset)) {
        pr_warn("Find a bad block, sector adjust to the next block\n");
        sector += pages_per_block * sectors_per_page;
        block++;
    }

    start_page = sector / sectors_per_page;
    offset = start_page * mtd->writesize;

    /*sector is not aligned with page, read unalign part first*/
    if (sector % sectors_per_page) {
        memset(blk_device->pagebuf, 0xFF, mtd->writesize);
        ret = mtd->ops.read_oob(mtd, offset, blk_device->pagebuf,
                                mtd->writesize, NULL, 0);
        if (ret != RT_EOK) {
            pr_err("Mtd read page failed!\n");
            return -RT_ERROR;
        }

        copybuf = blk_device->pagebuf +
                  (sector % sectors_per_page) * info.bytes_per_sector;
        if (cnt > (sectors_per_page - sector % sectors_per_page)) {
            copysize = (sectors_per_page - sector % sectors_per_page) *
                       info.bytes_per_sector;
            sectors_read += (sectors_per_page - sector % sectors_per_page);
        } else {
            copysize = cnt * info.bytes_per_sector;
            sectors_read += cnt;
        }

        memcpy(buf, copybuf, copysize);

        buf += copysize;
        start_page++;
    }

    if (cnt - sectors_read == 0)
        return RES_OK;

#ifdef AIC_SPINAND_CONT_READ
    if ((cnt - sectors_read) > sectors_per_page) {
        uint8_t *data_ptr = RT_NULL;
        uint32_t copydata = (cnt - sectors_read) * info.bytes_per_sector;

        data_ptr = (uint8_t *)aicos_malloc_align(0, copydata, CACHE_LINE_SIZE);
        if (data_ptr == RT_NULL) {
            pr_err("Malloc buf data_ptr failed\n");
            goto exit_spinand_disk_read_malloc;
        }

        memset(data_ptr, 0, copydata);

        offset = start_page * mtd->writesize;
        ret = mtd->ops.cont_read(mtd, offset, data_ptr, copydata);
        if (ret != RT_EOK) {
            pr_err("continuous_read failed!\n");
            goto exit_spinand_disk_read;
        }

        memcpy(buf, data_ptr, copydata);

        if (data_ptr)
            aicos_free_align(0, data_ptr);

        return RES_OK;

    exit_spinand_disk_read:
        if (data_ptr)
            aicos_free_align(0, data_ptr);
    }
exit_spinand_disk_read_malloc:
#endif

    /*sector is aligned with page*/
    while (cnt > sectors_read) {
        if (start_page / pages_per_block != block) {
            block = start_page / pages_per_block;
            offset = block * mtd->erasesize;
            while (mtd->ops.block_isbad(mtd, offset)) {
                pr_warn("Find a bad block, sector adjust to the next block\n");
                block++;
                start_page += pages_per_block;
            }
        }

        memset(blk_device->pagebuf, 0xFF, mtd->writesize);

        offset = start_page * mtd->writesize;
        ret = mtd->ops.read_oob(mtd, offset, blk_device->pagebuf,
                                mtd->writesize, NULL, 0);
        if (ret != RT_EOK) {
            pr_err("Mtd read page failed!\n");
            return -RT_ERROR;
        }

        if ((cnt - sectors_read) > sectors_per_page) {
            copysize = sectors_per_page * info.bytes_per_sector;
            sectors_read += sectors_per_page;
        } else {
            copysize = (cnt - sectors_read) * info.bytes_per_sector;
            sectors_read += (cnt - sectors_read);
        }

        memcpy(buf, blk_device->pagebuf, copysize);

        buf += copysize;
        start_page++;
    }

    return RES_OK;
}

DRESULT spinand_disk_ioctl(const char *device_name, uint8_t command, void *buf)
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

DSTATUS spinand_disk_status(const char *device_name)
{
    return RES_OK;
}

DSTATUS spinand_disk_initialize(const char *device_name)
{
    blk_device = (struct spinand_blk_device *)aicos_malloc(
        MEM_CMA, sizeof(struct spinand_blk_device));
    if (!blk_device) {
        pr_err("Error: no memory for create SPI NAND block device");
        return RES_ERROR;
    }

    /*Obtain devices by part name*/
    blk_device->mtd_device = mtd_get_device(device_name);
    if (!blk_device->mtd_device) {
        pr_err("Failed to get mtd %s\n", device_name);
        return RES_NOTRDY;
    }

    blk_device->pagebuf = aicos_malloc_align(
        0, blk_device->mtd_device->writesize, CACHE_LINE_SIZE);
    if (!blk_device->pagebuf) {
        pr_err("Malloc buf failed\n");
        return RES_ERROR;
    }

    info.bytes_per_sector = 512;
    info.block_size = info.bytes_per_sector;
    info.sector_count = blk_device->mtd_device->size / info.bytes_per_sector;

    return RES_OK;
}
