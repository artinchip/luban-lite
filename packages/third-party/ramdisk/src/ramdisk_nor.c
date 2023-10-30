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
#include "drv_ramdisk.h"
#include <partition_table.h>
#include <aic_mtd_partition_parse.h>

#define DBG_TAG              "nor.ramdisk"
#ifdef DRV_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif /* DRV_DEBUG */
#include <rtdbg.h>

#define MTD_NOR_DEVICE_ROM LPKG_RAMDISK_INIT_DEVICE_NAME
#define SECTOR_SIZE_OFFSET 11
#define SECTOR_CNT_OFFSET1  19
#define SECTOR_CNT_OFFSET2  32

#if defined(LPKG_RAMDISK_TYPE_INITDATA) && defined(RT_USING_MTD_NOR)
#ifdef AIC_XIP
static int ramdisk_device_nor_init(void)
{
    struct mtd_partition *part;
    char *partstr;
    uint32_t sector_size = 0;
    uint32_t sector_cnt = 0;
    uint32_t total_size = 0;
    uint32_t part_size = 0;
    u32 rodata_addr = 0;
    rt_err_t ret;

    partstr = aic_get_part_str(0);
    part = aic_mtd_parts_parse(partstr);
    if (!part) {
        printf("aic_mtd_parts_parse failed\n");
        ret = -RT_ERROR;
    }

    part = aic_mtd_get_parts_byname(MTD_NOR_DEVICE_ROM);
    if (!part) {
        printf("aic_mtd_get_parts_byname failed, can't find %s partition\n",
               MTD_NOR_DEVICE_ROM);
        ret = -RT_ERROR;
    }

    rodata_addr = FLASH_XIP_BASE + part->start;
    part_size = part->size;

    memcpy(&sector_size, (const void *)rodata_addr + SECTOR_SIZE_OFFSET, 2);
    if (sector_size == 0)
        sector_size = 512;

    sector_cnt = 0;
    memcpy(&sector_cnt, (const void *)rodata_addr + SECTOR_CNT_OFFSET1, 2);
    if (sector_cnt == 0)
        memcpy(&sector_cnt, (const void *)rodata_addr + SECTOR_CNT_OFFSET2, 4);

    total_size = sector_cnt * sector_size;
    if (total_size > part_size) {
        LOG_E("Invalid FAT image.\n");
        ret = -RT_ERROR;
        goto out;
    }

    ret = ramdisk_init("ramdisk0", (uint8_t *)rodata_addr, sector_size,
                       sector_cnt);

out:
    return ret;
}
#else
static int ramdisk_device_nor_init(void)
{
    struct rt_mtd_nor_device *mtd;
    rt_device_t dev_id;
    uint32_t sector_size, sector_cnt, total_size;
    rt_size_t part_size;
    uint8_t *diskbuf;
    uint8_t tmp[512];
    rt_err_t ret;

    dev_id = rt_device_find(MTD_NOR_DEVICE_ROM);
    if (dev_id == RT_NULL) {
        LOG_E("Cannot find %s.\n", MTD_NOR_DEVICE_ROM);
        return -RT_ERROR;
    }
    ret = rt_device_open(dev_id, RT_DEVICE_OFLAG_RDWR);
    if (ret) {
        LOG_E("Open MTD %s failed.!\n", MTD_NOR_DEVICE_ROM);
        return ret;
    }
    mtd = (struct rt_mtd_nor_device *)dev_id;
    part_size = (mtd->block_end - mtd->block_start) * mtd->block_size;

    if (rt_mtd_nor_read(mtd, 0, tmp, 512) != 512) {
        LOG_E("Failed to read ramdisk from NOR.\n");
        ret = -RT_ERROR;
        goto out;
    }

    sector_size = 0;
    memcpy(&sector_size, tmp + SECTOR_SIZE_OFFSET, 2);
    if (sector_size == 0)
        sector_size = 512;

    sector_cnt = 0;
    memcpy(&sector_cnt, tmp + SECTOR_CNT_OFFSET1, 2);
    if (sector_cnt == 0)
        memcpy(&sector_cnt, tmp + SECTOR_CNT_OFFSET2, 4);
    total_size = sector_cnt * sector_size;
    if (total_size > part_size) {
        LOG_E("Invalid FAT image.\n");
        ret = -RT_ERROR;
        goto out;
    }
    diskbuf = rt_malloc_align(total_size, CACHE_LINE_SIZE);
    if (!diskbuf) {
        LOG_E("Malloc ramdisk buffer failed.\n");
        ret = -RT_ENOMEM;
        goto out;
    }

    if (rt_mtd_nor_read(mtd, 0, diskbuf, total_size) != total_size) {
        LOG_E("Failed to read ramdisk from NOR.\n");
        ret = -RT_ERROR;
        goto out;
    }

    ramdisk_init("ramdisk0", diskbuf, sector_size, sector_cnt);

    ret = RT_EOK;
out:
    rt_device_close(dev_id);
    return ret;
}
#endif

INIT_COMPONENT_EXPORT(ramdisk_device_nor_init);
#endif
