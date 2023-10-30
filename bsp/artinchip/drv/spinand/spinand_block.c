/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <string.h>
#include <rtconfig.h>
#include <assert.h>
#include "spinand.h"
#include "spinand_block.h"
#include "spinand_parts.h"

struct spinand_blk_device {
    struct rt_device parent;
    struct rt_device_blk_geometry geometry;
    struct rt_mtd_nand_device *mtd_device;
    u8 *pagebuf;
};

static rt_err_t rt_spinand_control(rt_device_t dev, int cmd, void *args)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;

    assert(part != RT_NULL);

    if (cmd == RT_DEVICE_CTRL_BLK_GETGEOME) {
        struct rt_device_blk_geometry *geometry;

        geometry = (struct rt_device_blk_geometry *)args;
        if (geometry == RT_NULL) {
            return -RT_ERROR;
        }

        memcpy(geometry, &part->geometry,
               sizeof(struct rt_device_blk_geometry));
    } else {
        pr_warn("Invaild cmd = %d\n", cmd);
    }

    return RT_EOK;
}

static rt_size_t rt_spinand_read(rt_device_t dev, rt_off_t pos, void *buffer,
                                 rt_size_t size)
{
    int ret = 0;
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    struct rt_mtd_nand_device *device = part->mtd_device;
    int start_page;
    u8 *copybuf = NULL;
    u16 copysize;
    rt_size_t sectors_read = 0;
    u8 sectors_per_page = device->page_size / part->geometry.bytes_per_sector;
    rt_uint32_t block;

    assert(part != RT_NULL);

    pr_debug("pos = %d, size = %d\n", pos, size);

    start_page =
        pos / sectors_per_page + device->block_start * device->pages_per_block;

    block = start_page / device->pages_per_block;
    /* Search for the first good block after the given offset */
    while (device->ops->check_block(device, block)) {
        pr_warn("Find a bad block, pos adjust to the next block\n");
        pos += device->pages_per_block * sectors_per_page;
        block++;
    }

    start_page =
        pos / sectors_per_page + device->block_start * device->pages_per_block;

    /*pos is not aligned with page, read unalign part first*/
    if (pos % sectors_per_page) {
        memset(part->pagebuf, 0xFF, device->page_size);
        ret = device->ops->read_page(device, start_page, part->pagebuf,
                                     device->page_size, NULL, 0);
        if (ret != RT_EOK) {
            pr_err("read_page failed!\n");
            return -RT_ERROR;
        }

        copybuf = part->pagebuf +
                  (pos % sectors_per_page) * part->geometry.bytes_per_sector;
        if (size > (sectors_per_page - pos % sectors_per_page)) {
            copysize = (sectors_per_page - pos % sectors_per_page) *
                       part->geometry.bytes_per_sector;
            sectors_read += (sectors_per_page - pos % sectors_per_page);
        } else {
            copysize = size * part->geometry.bytes_per_sector;
            sectors_read += size;
        }

        rt_memcpy(buffer, copybuf, copysize);

        buffer += copysize;
        start_page++;
    }

    if (size - sectors_read == 0)
        return size;

#ifdef AIC_SPINAND_CONT_READ
    if ((size - sectors_read) > sectors_per_page) {
        rt_uint8_t *data_ptr = RT_NULL;
        rt_uint32_t copydata =
            (size - sectors_read) * part->geometry.bytes_per_sector;

        data_ptr = (rt_uint8_t *)rt_malloc_align(copydata, CACHE_LINE_SIZE);
        if (data_ptr == RT_NULL) {
            pr_err("data_ptr: no memory\n");
            goto exit_rt_spinand_read_malloc;
        }

        rt_memset(data_ptr, 0, copydata);

        ret = device->ops->continuous_read(device, start_page, data_ptr,
                                           copydata);
        if (ret != RT_EOK) {
            pr_err("continuous_read failed!\n");
            goto exit_rt_spinand_read;
        }

        rt_memcpy(buffer, data_ptr, copydata);

        if (data_ptr)
            rt_free_align(data_ptr);

        return size;

    exit_rt_spinand_read:
        if (data_ptr)
            rt_free_align(data_ptr);
    }
exit_rt_spinand_read_malloc:
#endif

    /*pos is aligned with page*/
    while (size > sectors_read) {
        if (start_page / device->pages_per_block != block) {
            block = start_page / device->pages_per_block;
            while (device->ops->check_block(device, block)) {
                pr_warn("Find a bad block, pos adjust to the next block\n");
                block++;
                start_page += device->pages_per_block;
            }
        }

        memset(part->pagebuf, 0xFF, device->page_size);
        ret = device->ops->read_page(device, start_page, part->pagebuf,
                                     device->page_size, NULL, 0);
        if (ret != RT_EOK) {
            pr_err("read_page failed!\n");
            return -RT_ERROR;
        }

        if ((size - sectors_read) > sectors_per_page) {
            copysize = sectors_per_page * part->geometry.bytes_per_sector;
            sectors_read += sectors_per_page;
        } else {
            copysize = (size - sectors_read) * part->geometry.bytes_per_sector;
            sectors_read += (size - sectors_read);
        }

        rt_memcpy(buffer, part->pagebuf, copysize);
        buffer += copysize;
        start_page++;
    }

    return size;
}

static rt_size_t rt_spinand_write(rt_device_t dev, rt_off_t pos,
                                  const void *buffer, rt_size_t size)
{
    return size;
}

#ifdef RT_USING_DEVICE_OPS
static struct rt_device_ops blk_dev_ops = {
    RT_NULL,         RT_NULL,          RT_NULL,
    rt_spinand_read, rt_spinand_write, rt_spinand_control
};
#endif

int rt_blk_nand_register_device(const char *name,
                                struct rt_mtd_nand_device *device)
{
    char str[32] = { 0 };
    struct spinand_blk_device *blk_dev;

    blk_dev = (struct spinand_blk_device *)rt_malloc(
        sizeof(struct spinand_blk_device));
    if (!blk_dev) {
        pr_err("Error: no memory for create SPI NAND block device");
    }

    blk_dev->mtd_device = device;
    blk_dev->parent.type = RT_Device_Class_Block;

#ifdef RT_USING_DEVICE_OPS
    blk_dev->parent.ops = &blk_dev_ops;
#else
    /* register device */
    blk_dev->parent.init = NULL;
    blk_dev->parent.open = NULL;
    blk_dev->parent.close = NULL;
    blk_dev->parent.read = rt_spinand_read;
    blk_dev->parent.write = rt_spinand_write;
    blk_dev->parent.control = rt_spinand_control;
#endif

    blk_dev->geometry.bytes_per_sector = 512;
    blk_dev->geometry.block_size = blk_dev->geometry.bytes_per_sector;
    blk_dev->geometry.sector_count =
        device->block_total * device->pages_per_block * device->page_size /
        blk_dev->geometry.bytes_per_sector;

    blk_dev->pagebuf =
        aicos_malloc_align(0, device->page_size, CACHE_LINE_SIZE);
    if (!blk_dev->pagebuf) {
        pr_err("malloc buf failed\n");
        return -1;
    }

    rt_sprintf(str, "blk_%s", name);
    /* register the device */
    rt_device_register(RT_DEVICE(blk_dev), str,
                       RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);
    return 0;
}
