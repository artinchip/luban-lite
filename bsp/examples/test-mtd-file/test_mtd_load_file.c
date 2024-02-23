/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_drv.h>
#include "aic_time.h"
#include <string.h>
#include <dfs_fs.h>
#include <sys/stat.h>

#define USAGE                                           \
    "test_mtd_load_file help : Get this information.\n" \
    "test_mtd_load_file os 0x4000 os.bin.\n"            \
    "test_mtd_load_file os 0x4000 spl.bin\n"

static void test_mtd_usage(void)
{
    printf("%s", USAGE);
}

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
            pr_err("The page id is not correct.\n");
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
            pr_err("Failed to read page data from NAND.\n");
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

static int test_mtd_load_file(int argc, char **argv)
{
    struct rt_mtd_nand_device *mtd;
    rt_device_t dev_id;
    rt_size_t file_size;
    uint8_t *tmp = NULL;
    rt_err_t ret;
    int fd;

    if (argc != 4) {
        pr_err("Usage: %s  <mtd name> <file size> <file name>.\n", __func__);
        test_mtd_usage();
        return -RT_ERROR;
    }

    dev_id = rt_device_find(argv[1]);
    if (dev_id == RT_NULL) {
        pr_err("Cannot find %s.\n", argv[1]);
        return -RT_ERROR;
    }

    ret = rt_device_open(dev_id, RT_DEVICE_OFLAG_RDWR);
    if (ret) {
        pr_err("Open MTD %s failed.!\n", argv[1]);
        return ret;
    }

    mtd = (struct rt_mtd_nand_device *)dev_id;
    file_size = strtol(argv[2], NULL, 0);

    tmp = malloc(file_size);
    if (!tmp) {
        pr_err("Malloc tmp buffer failed.\n");
        ret = -RT_ENOMEM;
        goto out;
    }

    ret = mtd_data_read(mtd, 0, tmp, file_size);
    if (ret) {
        pr_err("Failed to read data from NAND.\n");
        ret = -RT_ERROR;
        goto out;
    }

    fd = open(argv[3], O_WRONLY | O_CREAT);
    if (fd >= 0) {
        write(fd, tmp, file_size);
        close(fd);
    } else {
        pr_err("open file %s failed.\n", argv[3]);
    }

out:
    if (tmp)
        free(tmp);
    rt_device_close(dev_id);

    return ret;
}

MSH_CMD_EXPORT(test_mtd_load_file, mtd load data to a file);
