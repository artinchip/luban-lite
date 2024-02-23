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

#ifdef AIC_NFTL_SUPPORT
#include <nftl_api.h>
#endif

struct spinand_blk_device {
    struct rt_device parent;
    struct rt_device_blk_geometry geometry;
    struct rt_mtd_nand_device *mtd_device;
#ifdef AIC_NFTL_SUPPORT
    struct nftl_api_handler_t *nftl_handler;
#endif
    char name[32];
    enum part_attr attr;
    u8 *pagebuf;
};
#ifdef AIC_NFTL_SUPPORT
rt_size_t rt_spinand_read_nftl(rt_device_t dev, rt_off_t pos, void *buffer,
                                    rt_size_t size)
{
    rt_size_t ret;
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;

    ret = nftl_api_read(part->nftl_handler, pos, size, buffer);
    if (ret == 0) {
        return size;
    } else {
        return -1;
    }
}

rt_size_t rt_spinand_write_nftl(rt_device_t dev, rt_off_t pos,
                                     const void *buffer, rt_size_t size)
{
    rt_size_t ret;
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;

    ret = nftl_api_write(part->nftl_handler, pos, size, (u8 *)buffer);
    if (ret == 0) {
        return size;
    } else {
        return -1;
    }
}

rt_err_t rt_spinand_init_nftl(rt_device_t dev)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    part->nftl_handler =
        aicos_malloc(MEM_CMA, sizeof(struct nftl_api_handler_t));
    //part->nftl_handler = (struct nftl_api_handler_t *)rt_malloc(sizeof(struct nftl_api_handler_t));
    if (!part->nftl_handler) {
        pr_err(
            "Error: no memory for create SPI NAND block device . nftl_handler");
        return RT_ERROR;
    }
    memset(part->nftl_handler, 0, sizeof(struct nftl_api_handler_t));

    part->nftl_handler->priv_mtd = (void *)part->mtd_device;
    part->nftl_handler->nandt =
        aicos_malloc(MEM_CMA, sizeof(struct nftl_api_nand_t));

    part->nftl_handler->nandt->page_size = part->mtd_device->page_size;
    part->nftl_handler->nandt->oob_size = part->mtd_device->oob_size;
    part->nftl_handler->nandt->pages_per_block =
        part->mtd_device->pages_per_block;
    part->nftl_handler->nandt->block_total = part->mtd_device->block_total;
    part->nftl_handler->nandt->block_start = part->mtd_device->block_start;
    part->nftl_handler->nandt->block_end = part->mtd_device->block_end;

    if (nftl_api_init(part->nftl_handler, dev->device_id)) {
        pr_err("[NE]nftl_initialize failed\n");
        return RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t rt_spinand_nftl_close(rt_device_t dev)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    return nftl_api_write_cache(part->nftl_handler, 0xffff);
}
#endif

rt_size_t rt_spinand_read_nonftl(rt_device_t dev, rt_off_t pos,
                                       void *buffer, rt_size_t size)
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

rt_size_t rt_spinand_write_nonftl(rt_device_t dev, rt_off_t pos,
                                        const void *buffer, rt_size_t size)
{
    return size;
}

rt_err_t rt_spinand_init_nonftl(rt_device_t dev)
{
    return 0;
}

rt_err_t rt_spinand_nonftl_close(rt_device_t dev)
{
    return 0;
}

static rt_err_t rt_spinand_init(rt_device_t dev)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    if (part->attr == PART_ATTR_NFTL) {

#ifdef AIC_NFTL_SUPPORT
        return rt_spinand_init_nftl(dev);
#else
        return rt_spinand_init_nonftl(dev);
#endif
    } else {
        return rt_spinand_init_nonftl(dev);
    }
}

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
    } else if (cmd == RT_DEVICE_CTRL_BLK_SYNC) {
        if (part->attr == PART_ATTR_NFTL) {
#ifdef AIC_NFTL_SUPPORT
            nftl_api_write_cache(part->nftl_handler, 0xffff);
#else
            pr_warn("Invaild cmd = %d\n", cmd);
#endif
        } else {
            pr_warn("Invaild cmd = %d\n", cmd);
        }
    } else {
        pr_warn("Invaild cmd = %d\n", cmd);
    }

    return RT_EOK;
}

static rt_size_t rt_spinand_write(rt_device_t dev, rt_off_t pos,
                                  const void *buffer, rt_size_t size)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    if (part->attr == PART_ATTR_NFTL) {

#ifdef AIC_NFTL_SUPPORT
        return rt_spinand_write_nftl(dev, pos, buffer, size);
#else
        return rt_spinand_write_nonftl(dev, pos, buffer, size);
#endif
    } else {
        return rt_spinand_write_nonftl(dev, pos, buffer, size);
    }
}

static rt_size_t rt_spinand_read(rt_device_t dev, rt_off_t pos, void *buffer,
                                 rt_size_t size)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    if (part->attr == PART_ATTR_NFTL) {

#ifdef AIC_NFTL_SUPPORT
        return rt_spinand_read_nftl(dev, pos, buffer, size);
#else
        return rt_spinand_read_nonftl(dev, pos, buffer, size);
#endif
    } else {
        return rt_spinand_read_nonftl(dev, pos, buffer, size);
    }
}

static rt_err_t rt_spinand_close(rt_device_t dev)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    if (part->attr == PART_ATTR_NFTL) {

#ifdef AIC_NFTL_SUPPORT
        return rt_spinand_nftl_close(dev);
#else
        return rt_spinand_nonftl_close(dev);
#endif
    } else {
        return rt_spinand_nonftl_close(dev);
    }
}

#ifdef RT_USING_DEVICE_OPS
static struct rt_device_ops blk_dev_ops = {
    rt_spinand_init, RT_NULL,          rt_spinand_close,
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
    blk_dev->attr = device->attr;

#ifdef RT_USING_DEVICE_OPS
    blk_dev->parent.ops = &blk_dev_ops;
#else
    /* register device */
    blk_dev->parent.init = rt_spinand_init;
    blk_dev->parent.open = NULL;
    blk_dev->parent.close = rt_spinand_close;
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
    memset(blk_dev->name, 0, 32);
    rt_memcpy(blk_dev->name, str, 32);
    /* register the device */
    rt_device_register(RT_DEVICE(blk_dev), str,
                       RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);
    return 0;
}
