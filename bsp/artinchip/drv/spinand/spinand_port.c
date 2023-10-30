/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <finsh.h>
#include <drv_qspi.h>
#include <drv_spienc.h>
#include <partition_table.h>
#include "spinand.h"
#include "spinand_parts.h"
#include "spinand_block.h"

static struct rt_mtd_nand_device *g_mtd_partitions;
static int g_mtd_partitions_cnt;
struct aic_spinand *aic_nand;

void qspi_messages_init(struct rt_qspi_message *qspi_messages,
                        struct spi_nand_cmd_cfg *cfg, u32 addr, u8 *sendbuff,
                        u8 *recvbuff, u32 datacount)
{
    /* 1-bit mode */
    qspi_messages->instruction.content = cfg->opcode;
    qspi_messages->instruction.qspi_lines = cfg->opcode_bits;

    qspi_messages->address.content = addr;
    qspi_messages->address.size = cfg->addr_bytes;
    qspi_messages->address.qspi_lines = cfg->addr_bits;

    if (cfg->addr_bits)
        qspi_messages->dummy_cycles =
            (cfg->dummy_bytes * 8) / qspi_messages->address.qspi_lines;
    else
        qspi_messages->dummy_cycles =
            (cfg->dummy_bytes * 8) / qspi_messages->instruction.qspi_lines;

    /* 4-bit mode */
    qspi_messages->qspi_data_lines = cfg->data_bits;
    qspi_messages->parent.cs_take = 1;
    qspi_messages->parent.cs_release = 1;
    qspi_messages->parent.send_buf = sendbuff;
    qspi_messages->parent.recv_buf = recvbuff;
    qspi_messages->parent.length = datacount;
    qspi_messages->parent.next = NULL;
}

int aic_spinand_transfer_message(struct aic_spinand *flash,
                                 struct spi_nand_cmd_cfg *cfg, u32 addr,
                                 u8 *sendbuff, u8 *recvbuff, u32 datacount)
{
    int result;
    struct rt_qspi_message qspi_messages = { 0 };
    struct rt_qspi_device *device = (struct rt_qspi_device *)flash->user_data;

    RT_ASSERT(flash != RT_NULL);
    RT_ASSERT(device != RT_NULL);

    qspi_messages_init(&qspi_messages, cfg, addr, sendbuff, recvbuff,
                       datacount);

    result = rt_mutex_take(&(device->parent.bus->lock), RT_WAITING_FOREVER);
    if (result != SPINAND_SUCCESS) {
        rt_set_errno(-RT_EBUSY);
        return result;
    }

    /* reset errno */
    rt_set_errno(SPINAND_SUCCESS);

    /* configure SPI bus */
    if (device->parent.bus->owner != &device->parent) {
        /* not the same owner as current, re-configure SPI bus */
        result = device->parent.bus->ops->configure(&device->parent,
                                                    &device->config.parent);
        if (result == SPINAND_SUCCESS) {
            /* set SPI bus owner */
            device->parent.bus->owner = &device->parent;
        } else {
            pr_err("Configure SPI bus failed\n");
            rt_set_errno(-SPINAND_ERR);
            goto __exit;
        }
    }

    /* transmit each SPI message */
    if (device->parent.bus->ops->xfer(&device->parent, &qspi_messages.parent) <
        0) {
        pr_err("Xfer SPI bus failed\n");
        result = -SPINAND_ERR;
        rt_set_errno(-SPINAND_ERR);
        goto __exit;
    }

    result = SPINAND_SUCCESS;

__exit:
    /* release bus lock */
    rt_mutex_release(&(device->parent.bus->lock));

    return result;
}

static void spinand_dump_buffer(int page, rt_uint8_t *buf, int len,
                                const char *title)
{
#if defined(AIC_SPINAND_DRV_DEBUG)
    int i;

    RT_ASSERT(buf != RT_NULL);
    RT_ASSERT(len != 0);

    rt_kprintf("[%s-Page-%d]\n", title, page);

    for (i = 0; i < len; i++) {
        rt_kprintf("%02X ", buf[i]);
        if (i % 32 == 31)
            rt_kprintf("\n");
    }
    rt_kprintf("\n");
#endif
}

static rt_err_t spinand_read_id(struct rt_mtd_nand_device *device)
{
    rt_err_t result = RT_EOK;
    u32 id = 0;
    struct aic_spinand *flash = (struct aic_spinand *)device->priv;

    RT_ASSERT(device != RT_NULL);

    result = rt_mutex_take(flash->lock, RT_WAITING_FOREVER);
    RT_ASSERT(result == RT_EOK);

    spinand_read_id_op(flash, (u8 *)&id);

    result = rt_mutex_release(flash->lock);
    RT_ASSERT(result == RT_EOK);

    pr_info("Id: 0x%08x\n", id);

    result = (id != 0x0) ? RT_EOK : -RT_ERROR;
    return result;
}

static rt_err_t spinand_mtd_read(struct rt_mtd_nand_device *device,
                                 rt_off_t page, rt_uint8_t *data,
                                 rt_uint32_t data_len, rt_uint8_t *spare,
                                 rt_uint32_t spare_len)
{
    struct aic_spinand *flash;
    rt_err_t result;

    RT_ASSERT(device != RT_NULL);

    if (page / device->pages_per_block > device->block_end) {
        pr_err("[Error] read page:%d\n", page);
        return -RT_MTD_EIO;
    }

    flash = (struct aic_spinand *)device->priv;

    result = rt_mutex_take(flash->lock, RT_WAITING_FOREVER);
    RT_ASSERT(result == RT_EOK);

    result = spinand_read_page(flash, page, data, data_len, spare, spare_len);

    rt_mutex_release(flash->lock);

    return result;
}

#ifdef AIC_SPINAND_CONT_READ
static rt_err_t spinand_mtd_continuous_read(struct rt_mtd_nand_device *device,
                                            rt_off_t page, rt_uint8_t *data,
                                            rt_uint32_t size)
{
    rt_err_t result;
    struct aic_spinand *flash = (struct aic_spinand *)device->priv;

    result = rt_mutex_take(flash->lock, RT_WAITING_FOREVER);
    RT_ASSERT(result == RT_EOK);

    result = spinand_continuous_read(flash, page, data, size);

    rt_mutex_release(flash->lock);

    return result;
}
#else
static rt_err_t spinand_mtd_continuous_read(struct rt_mtd_nand_device *device,
                                            rt_off_t page, rt_uint8_t *data,
                                            rt_uint32_t size)
{
    pr_err("Please enable config AIC_SPINAND_CONT_READ!.\n");
    return -1;
}
#endif

static rt_err_t spinand_mtd_write(struct rt_mtd_nand_device *device,
                                  rt_off_t page, const rt_uint8_t *data,
                                  rt_uint32_t data_len, const rt_uint8_t *spare,
                                  rt_uint32_t spare_len)
{
    struct aic_spinand *flash = (struct aic_spinand *)device->priv;
    RT_ASSERT(device != RT_NULL);
    rt_err_t result = RT_EOK;

    if (page / device->pages_per_block > device->block_end) {
        pr_err("[Error] write page:%d\n", page);
        return -RT_MTD_EIO;
    }

    spinand_dump_buffer(page, (uint8_t *)data, data_len, "WRITE DATA");
    spinand_dump_buffer(page, (uint8_t *)spare, spare_len, "WRITE SPARE");

    result = rt_mutex_take(flash->lock, RT_WAITING_FOREVER);
    RT_ASSERT(result == RT_EOK);

    result = spinand_write_page(flash, page, data, data_len, spare, spare_len);
    rt_mutex_release(flash->lock);
    return result;
}

static rt_err_t spinand_mtd_erase(struct rt_mtd_nand_device *device,
                                  rt_uint32_t block)
{
    rt_err_t result = RT_EOK;
    struct aic_spinand *flash = (struct aic_spinand *)device->priv;

    RT_ASSERT(device != RT_NULL);

    if (block > device->block_end) {
        pr_err("[Error] block:%d block_end:%d\n", block, device->block_end);
        return -RT_MTD_EIO;
    }

    pr_debug("erase block: %d\n", block);

    result = rt_mutex_take(flash->lock, RT_WAITING_FOREVER);
    RT_ASSERT(result == RT_EOK);

    result = spinand_block_erase(flash, block);
    if (result != RT_EOK)
        goto exit_spinand_mtd_erase;

    result = RT_EOK;

exit_spinand_mtd_erase:

    rt_mutex_release(flash->lock);

    return result;
}

static rt_err_t spinand_mtd_block_isbad(struct rt_mtd_nand_device *device,
                                        rt_uint32_t block)
{
    struct aic_spinand *flash = (struct aic_spinand *)device->priv;
    rt_err_t result = RT_EOK;

    RT_ASSERT(device != RT_NULL);

    if (block > device->block_end) {
        pr_err("[Error] block:%d\n", block);
        return -RT_MTD_EIO;
    }

    pr_debug("check block status: %d\n", block);

    result = rt_mutex_take(flash->lock, RT_WAITING_FOREVER);
    RT_ASSERT(result == RT_EOK);

    result = spinand_block_isbad(flash, block);

    rt_mutex_release(flash->lock);

    return result;
}

static rt_err_t spinand_mtd_block_markbad(struct rt_mtd_nand_device *device,
                                          rt_uint32_t block)
{
    rt_err_t result = RT_EOK;
    struct aic_spinand *flash = (struct aic_spinand *)device->priv;

    RT_ASSERT(device != RT_NULL);

    if (block > device->block_end) {
        pr_err("[Error] block:%d\n", block);
        return -RT_MTD_EIO;
    }

    pr_info("mark bad block: %d\n", block);

    result = rt_mutex_take(flash->lock, RT_WAITING_FOREVER);
    RT_ASSERT(result == RT_EOK);

    /* Erase block after checking it is bad or not. */
    if (spinand_block_isbad(flash, block) != 0) {
        pr_warn("Block %d is bad.\n", block);
        result = RT_EOK;
    } else {
        result = spinand_block_markbad(flash, block);
    }

    rt_mutex_release(flash->lock);

    return result;
}

static struct rt_mtd_nand_driver_ops spinand_ops = {
    spinand_read_id,           spinand_mtd_read,
    spinand_mtd_write,         NULL,
    spinand_mtd_erase,         spinand_mtd_block_isbad,
    spinand_mtd_block_markbad, spinand_mtd_continuous_read
};

rt_err_t rt_hw_mtd_spinand_init(struct aic_spinand *flash)
{
    struct mtd_partition *parts, *p;
    rt_uint32_t blocksize;
    int i = 0, cnt;
    rt_err_t result;

    if (flash->IsInited)
        return RT_EOK;

    flash->lock = rt_mutex_create("spinand", RT_IPC_FLAG_PRIO);
    if (flash->lock == RT_NULL) {
        pr_err("Create mutex in rt_hw_mtd_spinand_init failed\n");
        return -1;
    }

    result = spinand_flash_init(flash);
    if (result != RT_EOK)
        return -RT_ERROR;

    parts = mtd_parts_parse(IMAGE_CFG_JSON_PARTS_MTD);
    p = parts;
    cnt = 0;
    while (p) {
        cnt++;
        p = p->next;
    }
    g_mtd_partitions_cnt = cnt;
    g_mtd_partitions = rt_malloc(sizeof(struct rt_mtd_nand_device) * cnt);
    if (!g_mtd_partitions) {
        pr_err("malloc buf failed\n");
        return -1;
    }

    blocksize = flash->info->page_size * flash->info->pages_per_eraseblock;
    p = parts;
    for (i = 0; i < cnt; i++) {
        g_mtd_partitions[i].page_size = flash->info->page_size;
        g_mtd_partitions[i].pages_per_block = flash->info->pages_per_eraseblock;
        g_mtd_partitions[i].oob_size = flash->info->oob_size;
        g_mtd_partitions[i].oob_free = 32;
        g_mtd_partitions[i].plane_num = flash->info->is_die_select;
        g_mtd_partitions[i].ops = &spinand_ops;
        g_mtd_partitions[i].block_start = p->start / blocksize;
        g_mtd_partitions[i].block_end = (p->start + p->size - 1) / blocksize;
        g_mtd_partitions[i].block_total = p->size / blocksize;
        g_mtd_partitions[i].priv = flash;

        result = rt_mtd_nand_register_device(p->name, &g_mtd_partitions[i]);
        RT_ASSERT(result == RT_EOK);

        p = p->next;
    }

    p = parts;
    for (i = 0; i < cnt; i++) {
        result = rt_blk_nand_register_device(p->name, &g_mtd_partitions[i]);
        RT_ASSERT(result == RT_EOK);

        p = p->next;
    }

    flash->databuf = aicos_malloc_align(
        0, flash->info->page_size + flash->info->oob_size, CACHE_LINE_SIZE);
    if (!flash->databuf) {
        pr_err("malloc buf failed\n");
        return -1;
    }

    flash->oobbuf = flash->databuf + flash->info->page_size;

    flash->IsInited = true;

    return result;
}

rt_err_t rt_hw_mtd_spinand_register(const char *device_name)
{
    rt_device_t pDev;
    rt_err_t result;
    struct rt_qspi_device *device;
    struct aic_spinand *spinand;

    if ((pDev = rt_device_find(device_name)) == RT_NULL)
        return -RT_ERROR;

    spinand = rt_calloc(1, sizeof(struct aic_spinand));
    if (!spinand) {
        pr_err("malloc buf failed\n");
        return -RT_ERROR;
    }

    //SPINAND_FLASH_QSPI = (struct rt_qspi_device *)pDev;
    device = (struct rt_qspi_device *)pDev;
    spinand->user_data = device;

    device->config.parent.mode = RT_SPI_MODE_0;
    device->config.parent.data_width = 8;
    device->config.parent.max_hz = 100000000;
    device->config.ddr_mode = 0;
    device->config.qspi_dl_width = 4;

    aic_nand = spinand;

    result = rt_hw_mtd_spinand_init(spinand);
    return result;
}

#if defined(AIC_SPINAND_DRV_DEBUG)
static int nread(int argc, char **argv)
{
    int ret = -1;
    rt_uint8_t *spare = RT_NULL;
    rt_uint8_t *data_ptr = RT_NULL;
    struct rt_mtd_nand_device *device;
    rt_uint32_t partition, page;

    if (argc != 3) {
        pr_err("Usage %s: %s <partition_no> <page>.\n", __func__, __func__);
        goto exit_nread;
    }

    page = atoi(argv[2]);
    partition = atoi(argv[1]);

    if (partition >= g_mtd_partitions_cnt)
        goto exit_nread;

    device = &g_mtd_partitions[partition];
    data_ptr = (rt_uint8_t *)rt_malloc(flash->info->page_size);
    if (data_ptr == RT_NULL) {
        pr_err("data_ptr: no memory\n");
        goto exit_nread;
    }
    spare = (rt_uint8_t *)rt_malloc(flash->info->oob_size);
    if (spare == RT_NULL) {
        pr_err("spare: no memory\n");
        goto exit_nread;
    }

    rt_memset(spare, 0, flash->info->oob_size);
    rt_memset(data_ptr, 0, flash->info->page_size);

    page = page + device->block_start * device->pages_per_block;

    if (spinand_mtd_read(device, page, &data_ptr[0], flash->info->page_size,
                         &spare[0], flash->info->oob_size) != RT_EOK)
        goto exit_nread;

    pr_info("Partition:%d page-%d\n", partition, page);

    spinand_dump_buffer(page, data_ptr, flash->info->page_size, "Read Data");
    spinand_dump_buffer(page, spare, flash->info->oob_size, "Read Spare");

    ret = 0;

exit_nread:

    /* release memory */
    if (data_ptr)
        rt_free(data_ptr);

    if (spare)
        rt_free(spare);

    return ret;
}

#ifdef AIC_SPINAND_CONT_READ
static int ncontread(int argc, char **argv)
{
    int ret = -1;
    rt_uint8_t *data_ptr = RT_NULL;
    struct rt_mtd_nand_device *device;
    rt_uint32_t partition, page, size;
    rt_uint32_t start_us;

    if (argc != 4) {
        pr_err("Usage %s: %s <partition_no> <page> <size>.\n", __func__,
               __func__, __func__);
        goto exit_ncontread;
    }

    partition = atoi(argv[1]);
    page = atoi(argv[2]);
    size = atoi(argv[3]);

    if (partition >= g_mtd_partitions_cnt)
        goto exit_ncontread;

    device = &g_mtd_partitions[partition];

    data_ptr = (rt_uint8_t *)rt_malloc_align(size, CACHE_LINE_SIZE);
    if (data_ptr == RT_NULL) {
        pr_err("data_ptr: no memory\n");
        goto exit_ncontread;
    }

    rt_memset(data_ptr, 0, size);

    page = page + device->block_start * device->pages_per_block;

    start_us = aic_get_time_us();
    if (spinand_mtd_continuous_read(device, page, data_ptr, size) != RT_EOK) {
        pr_err("spinand continuous read failed\n");
        goto exit_ncontread;
    }

    start_us = aic_get_time_us() - start_us;
    pr_info("start_us = %d size = %ud\n", start_us, size);

    pr_info("Partition:%d page-%d\n", partition, page);

    spinand_dump_buffer(page, (uint8_t *)data_ptr, size, "CONT READ");

    ret = 0;

exit_ncontread:

    /* release memory */
    if (data_ptr)
        rt_free_align(data_ptr);

    return ret;
}
#endif

static int nwrite(int argc, char **argv)
{
    int i, ret = -1;
    rt_uint8_t *data_ptr = RT_NULL;
    struct rt_mtd_nand_device *device;
    rt_uint32_t partition, page;

    if (argc != 3) {
        pr_err("Usage %s: %s <partition_no> <page>.\n", __func__, __func__);
        goto exit_nwrite;
    }

    partition = atoi(argv[1]);
    page = atoi(argv[2]);

    if (partition >= g_mtd_partitions_cnt)
        goto exit_nwrite;

    device = &g_mtd_partitions[partition];

    data_ptr = (rt_uint8_t *)rt_malloc(flash->info->page_size);
    if (data_ptr == RT_NULL) {
        pr_err("data_ptr: no memory\n");
        goto exit_nwrite;
    }

    /* Need random data to test ECC */
    for (i = 0; i < flash->info->page_size; i++)
        data_ptr[i] = i / 5 - i;

    page = page + device->block_start * device->pages_per_block;
    spinand_mtd_write(device, page, &data_ptr[0], flash->info->page_size, NULL,
                      0);

    pr_debug("Write data into %d in partition-index %d.\n", page, partition);

    ret = 0;

exit_nwrite:

    /* release memory */
    if (data_ptr)
        rt_free(data_ptr);

    return ret;
}
#endif

static int nerase(int argc, char **argv)
{
    struct rt_mtd_nand_device *device;
    int partition, block;

    if (argc != 3) {
        pr_err("Usage %s: %s <partition_no> <block_no>.\n", __func__, __func__);
        goto exit_nerase;
    }

    partition = atoi(argv[1]);
    block = atoi(argv[2]);

    if (partition >= g_mtd_partitions_cnt)
        goto exit_nerase;

    device = &g_mtd_partitions[partition];

    if (spinand_mtd_erase(device, block + device->block_start) != RT_EOK)
        goto exit_nerase;

    pr_info("Erased block %d in partition-index %d.\n",
            block + device->block_start, partition);

    return 0;

exit_nerase:

    return -1;
}

static rt_err_t nmarkbad(int argc, char **argv)
{
    struct rt_mtd_nand_device *device;
    int partition, block;

    if (argc != 3) {
        pr_err("Usage %s: %s <partition_no> <block_no>.\n", __func__, __func__);
        goto exit_nmarkbad;
    }

    partition = atoi(argv[1]);
    block = atoi(argv[2]);

    if (partition >= g_mtd_partitions_cnt)
        goto exit_nmarkbad;

    device = &g_mtd_partitions[partition];

    if (spinand_mtd_block_markbad(device, block + device->block_start) !=
        RT_EOK)
        goto exit_nmarkbad;

    pr_info("Marked block %d in partition-index %d.\n",
            block + device->block_start, partition);

    return 0;

exit_nmarkbad:

    return -1;
}

static int nerase_all(int argc, char **argv)
{
    rt_uint32_t index;
    rt_uint32_t partition;
    struct rt_mtd_nand_device *device;

    if (argc != 2) {
        pr_err("Usage %s: %s <partition_no>.\n", __func__, __func__);
        goto exit_nerase_all;
    }

    partition = atoi(argv[1]);

    if (partition >= g_mtd_partitions_cnt)
        goto exit_nerase_all;

    device = &g_mtd_partitions[partition];

    for (index = 0; index < device->block_total; index++) {
        spinand_mtd_erase(device, index + device->block_start);
    }

    pr_info("Erased all block in partition-index %d.\n", partition);

    return 0;

exit_nerase_all:

    return -1;
}

static int ncheck_all(int argc, char **argv)
{
    rt_uint32_t index;
    rt_uint32_t partition;
    struct rt_mtd_nand_device *device;

    if (argc != 2) {
        pr_err("Usage %s: %s <partition_no>.\n", __func__, __func__);
        return -1;
    }

    partition = atoi(argv[1]);

    if (partition >= g_mtd_partitions_cnt)
        return -1;

    device = &g_mtd_partitions[partition];

    for (index = 0; index < device->block_total; index++) {
        pr_info("Partition:%d Block-%d is %s\n", partition, index,
                spinand_mtd_block_isbad(device, index + device->block_start) ?
                    "bad" :
                    "good");
    }

    return 0;
}

static int nid(int argc, char **argv)
{
    rt_uint32_t id;
    if (NULL == aic_nand)
        return -1;
    spinand_read_id_op(aic_nand, (u8 *)&id);
    rt_kprintf("nid: 0x%x\n", id);
    return 0;
}

static int nlist(int argc, char **argv)
{
    rt_uint32_t index;
    struct rt_mtd_nand_device *device;

    rt_kprintf("\n");
    for (index = 0; index < g_mtd_partitions_cnt; index++) {
        device = &g_mtd_partitions[index];
        rt_kprintf("[Partition #%d]\n", index);
        rt_kprintf("Name: %s\n", device->parent.parent.name);
        rt_kprintf("Start block: %d\n", device->block_start);
        rt_kprintf("End block: %d\n", device->block_end);
        rt_kprintf("Block number: %d\n", device->block_total);
        rt_kprintf("Plane number: %d\n", device->plane_num);
        rt_kprintf("Pages per Block: %d\n", device->pages_per_block);
        rt_kprintf("Page size: %d bytes\n", device->page_size);
        rt_kprintf("Spare size: %d bytes\n", device->oob_size);
        rt_kprintf("Total size: %d bytes (%d KB)\n",
                   device->block_total * device->pages_per_block *
                       device->page_size,
                   device->block_total * device->pages_per_block *
                       device->page_size / 1024);
        rt_kprintf("\n");
    }
    return 0;
}

#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(nid, nand id);
MSH_CMD_EXPORT(nlist, list all partition information on nand);
MSH_CMD_EXPORT(nerase, nand erase a block of a partiton);
MSH_CMD_EXPORT(nerase_all, erase all blocks of a partition);
MSH_CMD_EXPORT(ncheck_all, check all blocks of a partition);
MSH_CMD_EXPORT(nmarkbad, nand mark a bad block of a partition);
#if defined(AIC_SPINAND_DRV_DEBUG)
MSH_CMD_EXPORT(nwrite, test nand write page);
MSH_CMD_EXPORT(nread, test nand read page);
#ifdef AIC_SPINAND_CONT_READ
MSH_CMD_EXPORT(ncontread, test nand cont read);
#endif
#endif
#endif

static int rt_hw_spinand_register(void)
{
    aic_qspi_bus_attach_device("qspi0", "spinand0", 0, 4, RT_NULL, RT_NULL);
    rt_hw_mtd_spinand_register("spinand0");
    return RT_EOK;
}

INIT_DEVICE_EXPORT(rt_hw_spinand_register);
