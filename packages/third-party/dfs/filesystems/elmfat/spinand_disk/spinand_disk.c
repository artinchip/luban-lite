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

static DRESULT spinand_disk_nonftl_read(struct spinand_blk_device *blk_device,
                                        uint8_t *buf, uint32_t sector,
                                        uint8_t cnt)
{
    rt_size_t sectors_per_page;
    rt_size_t start_page, block, offset;
    unsigned char *copybuf = NULL;
    rt_size_t sectors_read = 0;
    rt_size_t copysize = 0;
    rt_size_t pages_per_block;
    rt_size_t ret = 0;
    struct mtd_dev *mtd = blk_device->mtd_device;
    struct rt_device_blk_geometry *info;

    if (!mtd)
        return RES_NOTRDY;

    info = &blk_device->info;
    pages_per_block = mtd->erasesize / mtd->writesize;
    sectors_per_page = mtd->writesize / info->bytes_per_sector;

    block = sector * info->bytes_per_sector / mtd->erasesize;
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

        copybuf = blk_device->pagebuf;
        copybuf += (sector % sectors_per_page) * info->bytes_per_sector;
        if (cnt > (sectors_per_page - sector % sectors_per_page)) {
            copysize = (sectors_per_page - sector % sectors_per_page) *
                       info->bytes_per_sector;
            sectors_read += (sectors_per_page - sector % sectors_per_page);
        } else {
            copysize = cnt * info->bytes_per_sector;
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
        uint32_t copydata = (cnt - sectors_read) * info->bytes_per_sector;

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
            copysize = sectors_per_page * info->bytes_per_sector;
            sectors_read += sectors_per_page;
        } else {
            copysize = (cnt - sectors_read) * info->bytes_per_sector;
            sectors_read += (cnt - sectors_read);
        }

        memcpy(buf, blk_device->pagebuf, copysize);

        buf += copysize;
        start_page++;
    }

    return RES_OK;
}

#ifdef AIC_NFTL_SUPPORT
DRESULT spinand_disk_nftl_read(struct spinand_blk_device *blk_device,
                               uint8_t *buf, uint32_t sector, uint8_t cnt)
{
    if (!blk_device->nftl_handler)
        return RES_NOTRDY;

    nftl_api_read(blk_device->nftl_handler, sector, cnt, buf);

    return RES_OK;
}

static DRESULT spinand_disk_nftl_write(struct spinand_blk_device *blk_device,
                                       const uint8_t *buf, uint32_t sector,
                                       uint8_t cnt)
{
    if (!blk_device->nftl_handler)
        return RES_NOTRDY;

    nftl_api_write(blk_device->nftl_handler, sector, cnt, (uint8_t *)buf);
    return RES_OK;
}
#endif

DRESULT spinand_disk_write(void *hdisk, const uint8_t *buf, uint32_t sector,
                           uint8_t cnt)
{
    struct spinand_blk_device *dev;

    dev = hdisk;
    if (!dev)
        return RES_NOTRDY;

#ifdef AIC_NFTL_SUPPORT
    if (dev->attr == PART_ATTR_NFTL) {
        return spinand_disk_nftl_write(dev, buf, sector, cnt);
    }
#endif

    return RES_OK;
}

DRESULT spinand_disk_read(void *hdisk, uint8_t *buf, uint32_t sector,
                          uint8_t cnt)
{
    struct spinand_blk_device *dev;

    dev = hdisk;
    if (!dev)
        return RES_NOTRDY;

#ifdef AIC_NFTL_SUPPORT
    if (dev->attr == PART_ATTR_NFTL)
        return spinand_disk_nftl_read(dev, buf, sector, cnt);
#endif

    return spinand_disk_nonftl_read(dev, buf, sector, cnt);
}

DRESULT spinand_disk_ioctl(void *hdisk, uint8_t command, void *buf)
{
    struct spinand_blk_device *dev;
    DRESULT result = RES_OK;

    dev = hdisk;
    if (!dev || !dev->mtd_device)
        return RES_NOTRDY;

    switch (command) {
        case GET_SECTOR_COUNT:
            if (buf) {
                *(uint32_t *)buf = dev->info.sector_count;
            } else {
                result = RES_PARERR;
            }

            break;

        case GET_SECTOR_SIZE:
            if (buf) {
                *(uint32_t *)buf = dev->info.bytes_per_sector;
            } else {
                result = RES_PARERR;
            }

            break;

        case GET_BLOCK_SIZE:
            if (buf) {
                *(uint32_t *)buf = dev->info.block_size;
            } else {
                result = RES_PARERR;
            }

            break;

        case CTRL_SYNC:
#ifdef AIC_NFTL_SUPPORT
            if (dev->attr == PART_ATTR_NFTL) {
                nftl_api_write_cache(dev->nftl_handler, 0xffff);
            }
#endif
            result = RES_OK;
            break;

        default:
            result = RES_PARERR;
            break;
    }

    return result;
}

DSTATUS spinand_disk_status(void *hdisk)
{
    return RES_OK;
}

void *spinand_disk_initialize(const char *device_name)
{
    struct spinand_blk_device *blk_device;
    struct mtd_dev *mtd;

    blk_device = (void *)malloc(sizeof(*blk_device));
    if (!blk_device) {
        pr_err("Error: no memory for create SPI NAND block device");
        return NULL;
    }

    memset(blk_device, 0, sizeof(*blk_device));
    /*Obtain devices by part name*/
    mtd = mtd_get_device(device_name);
    if (!mtd) {
        pr_err("Failed to get mtd %s\n", device_name);
        goto err;
    }

    blk_device->mtd_device = mtd;
#ifdef AIC_NFTL_SUPPORT
    if (blk_device->mtd_device->attr == PART_ATTR_NFTL) {
        struct nftl_api_handler_t *nftl_hdl;

        nftl_hdl = malloc(sizeof(struct nftl_api_handler_t));
        if (!nftl_hdl) {
            pr_err("Failed to allocate memory for nftl_handler");
            goto err;
        }
        blk_device->nftl_handler = nftl_hdl;
        memset(nftl_hdl, 0, sizeof(struct nftl_api_handler_t));

        nftl_hdl->priv_mtd = (void *)mtd;
        nftl_hdl->nandt = malloc(sizeof(struct nftl_api_nand_t));

        nftl_hdl->nandt->page_size = mtd->writesize;
        nftl_hdl->nandt->oob_size = mtd->oobsize;
        nftl_hdl->nandt->pages_per_block = mtd->erasesize / mtd->writesize;
        nftl_hdl->nandt->block_total = mtd->size / mtd->erasesize;
        nftl_hdl->nandt->block_start = mtd->start / mtd->erasesize;
        nftl_hdl->nandt->block_end = (mtd->start + mtd->size) / mtd->erasesize;
        if (nftl_api_init(nftl_hdl, 1)) {
            pr_err("[NE]nftl_initialize failed\n");
            goto err;
        }
    }
#endif

    blk_device->attr = mtd->attr;
    blk_device->pagebuf =
        aicos_malloc_align(0, mtd->writesize, CACHE_LINE_SIZE);
    if (!blk_device->pagebuf) {
        pr_err("Malloc buf failed\n");
        goto err;
    }

    blk_device->info.bytes_per_sector = 512;
    blk_device->info.block_size = blk_device->info.bytes_per_sector;
    blk_device->info.sector_count =
        mtd->size / blk_device->info.bytes_per_sector;

    return blk_device;
err:
    if (blk_device && blk_device->pagebuf)
        aicos_free_align(0, blk_device->pagebuf);
#ifdef AIC_NFTL_SUPPORT
    if (blk_device->nftl_handler && blk_device->nftl_handler->nandt)
        free(blk_device->nftl_handler->nandt);
    if (blk_device->nftl_handler)
        free(blk_device->nftl_handler);
#endif
    if (blk_device)
        free(blk_device);
    return NULL;
}
