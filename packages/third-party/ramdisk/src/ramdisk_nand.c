/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-04-17    Jianjia Ma   first version
 * 2023-03-31    Vandoul      fix bug and add test cmd.
 */

#include "board.h"
#include <string.h>
#include <dfs_fs.h>
#include <sys/stat.h>
#include <aic_common.h>
#include "drv_ramdisk.h"

#define DBG_TAG              "nand.ramdisk"
#ifdef DRV_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif /* DRV_DEBUG */
#include <rtdbg.h>

#define MTD_NAND_DEVICE_ROM LPKG_RAMDISK_INIT_DEVICE_NAME

extern void mtd_dump(const rt_uint8_t *ptr, rt_size_t buflen);
#if defined(LPKG_RAMDISK_TYPE_INITDATA) && defined(RT_USING_MTD_NAND)

static int mtd_is_block_aligned(rt_off_t page, rt_uint32_t ppb)
{
    if (page & (ppb - 1))
        return 0;

    return 1;
}

static rt_err_t mtd_data_read(struct rt_mtd_nand_device *mtd, rt_off_t off,
                              rt_uint8_t *data, rt_uint32_t data_len)
{
    rt_err_t ret = RT_EOK;
    rt_off_t offset;
    rt_uint32_t page_id, blk, remain;

    remain = data_len;
    offset = off;
    while (remain) {
        page_id = offset / mtd->page_size;
        if (page_id > (mtd->block_total * mtd->pages_per_block)) {
            LOG_E("The page id is not correct.\n");
            return -RT_ERROR;
        }

        blk = page_id / mtd->pages_per_block;
        if (mtd_is_block_aligned(page_id, mtd->pages_per_block) &&
            rt_mtd_nand_check_block(mtd, blk) != RT_EOK) {
            offset += mtd->pages_per_block;
            continue;
        }
        ret = rt_mtd_nand_read(mtd, page_id, data, mtd->page_size, RT_NULL, 0);
        if (ret) {
            LOG_E("Failed to read page data from NAND.\n");
            return -RT_ERROR;
        }

        data += mtd->page_size;
        offset += mtd->page_size;
        if (remain >= mtd->page_size)
            remain -= mtd->page_size;
        else
            remain = 0;
    }

    return ret;
}

static int ramdisk_device_nand_init(void)
{
    struct rt_mtd_nand_device* mtd;
    rt_device_t dev_id;
    uint32_t sector_size, sector_cnt, total_size;
    rt_size_t part_size;
    uint8_t *diskbuf = NULL;
    uint8_t *tmp = NULL;
    rt_err_t ret;

    dev_id = rt_device_find(MTD_NAND_DEVICE_ROM);
    if (dev_id == RT_NULL) {
            LOG_E("Cannot find %s.\n", MTD_NAND_DEVICE_ROM);
            return -RT_ERROR;
    }
    ret = rt_device_open(dev_id, RT_DEVICE_OFLAG_RDWR);
    if (ret) {
            LOG_E("Open MTD %s failed.!\n", MTD_NAND_DEVICE_ROM);
            return ret;
    }
    mtd = (struct rt_mtd_nand_device *)dev_id;
    part_size = mtd->block_total * (mtd->pages_per_block * mtd->page_size);

    tmp = malloc(mtd->page_size);
    if (!tmp) {
            LOG_E("Malloc tmp buffer failed.\n");
            ret = -RT_ENOMEM;
            goto out;
    }

    ret = mtd_data_read(mtd, 0, tmp, mtd->page_size);
    if (ret) {
            LOG_E("Failed to read ramdisk from NAND.\n");
            ret = -RT_ERROR;
            goto out;
    }

    sector_size = 0;
    memcpy(&sector_size, tmp + 11, 2);
    if (sector_size == 0)
        sector_size = 512;
    sector_cnt = 0;
    memcpy(&sector_cnt, tmp + 19, 2);
    if (sector_cnt == 0)
        memcpy(&sector_cnt, tmp + 32, 4);
    total_size = sector_cnt * sector_size;
    if (total_size > part_size) {
            LOG_E("Invalid FAT image.\n");
            ret = -RT_ERROR;
            goto out;
    }
    total_size = ROUND(total_size, mtd->page_size);
    diskbuf = malloc(total_size);
    if (!diskbuf) {
            LOG_E("Malloc ramdisk buffer failed.\n");
            ret = -RT_ENOMEM;
            goto out;
    }

    ret = mtd_data_read(mtd, 0, diskbuf, total_size);
    if (ret) {
            LOG_E("Failed to read ramdisk from NAND.\n");
            ret = -RT_ERROR;
            goto out;
    }

    ramdisk_init("ramdisk0", diskbuf, sector_size, sector_cnt);

    ret = RT_EOK;
out:
    if (tmp)
        free(tmp);
    rt_device_close(dev_id);
    return ret;
}

INIT_COMPONENT_EXPORT(ramdisk_device_nand_init);
#endif
