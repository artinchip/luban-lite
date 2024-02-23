/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: mingfeng.li <mingfeng.li@artinchip.com>
 */

#define _NFTL_NFTL_PORT_HW_C_

#include "../../../../kernel/rt-thread/components/drivers/include/drivers/mtd_nand.h"
#include <aic_common.h>
#include "nftl_api.h"
#include <stdarg.h>
#include <rtthread.h>

#if DRV_PORT_HW_DEBUG
#define PORT_HW_LOG rt_kprintf
#else
#define PORT_HW_LOG(...)
#endif

static u8 read_oob[64] = { 0 };
static u8 write_oob[64] = { 0 };

void *nftl_memcpy(void *str1, const void *str2, int size)
{
    return memcpy(str1, str2, size);
}

void nftl_memset(void *str, int c, int size)
{
    memset(str, c, size);
}

void *nftl_malloc(u32 size)
{
    if (size > 0x180000)
        PORT_HW_LOG("[NE]malloc size too large %d!\n", size);

    return aicos_malloc(MEM_CMA, size);
}

void nftl_free(const void *ptr)
{
    aicos_free(MEM_CMA, (void *)ptr);
}

int _nftl_port_hw_erase_block(void *device, struct physical_op_info *p)
{
    struct rt_mtd_nand_device *nand = (struct rt_mtd_nand_device *)device;

    rt_mtd_nand_erase_block(nand, p->physical_page.block_num);

    PORT_HW_LOG("%s:%d ...\n", __FUNCTION__, __LINE__);
    return 0;
}

int _nftl_port_hw_read_page(void *device, struct physical_op_info *p)
{
    struct rt_mtd_nand_device *nand = (struct rt_mtd_nand_device *)device;
    int ret;
    int page =
        p->physical_page.block_num * nand->pages_per_block + p->physical_page.page_num;
    //NFTL_INFO("%s:%d ...p->physical_page.block_num=%d, p->physical_page.page_num=%d page=%d\n", __FUNCTION__, __LINE__, p->physical_page.block_num, p->physical_page.page_num, page);
    ret = rt_mtd_nand_read(nand, page, p->user_data_addr, nand->page_size,
                           read_oob, 64);
    memcpy(p->spare_data_addr, read_oob, 8);
    memcpy(p->spare_data_addr + 8, read_oob + 16, 8);

    PORT_HW_LOG("%s:%d ...\n", __FUNCTION__, __LINE__);
    return ret;
}

int _nftl_port_hw_write_page(void *device, struct physical_op_info *p)
{
    struct rt_mtd_nand_device *nand = (struct rt_mtd_nand_device *)device;
    PORT_HW_LOG("%s:%d ...\n", __FUNCTION__, __LINE__);
    int ret;
    int page =
        p->physical_page.block_num * nand->pages_per_block + p->physical_page.page_num;
    //NFTL_INFO("%s:%d ...p->physical_page.block_num=%d, p->physical_page.page_num=%d page=%d\n", __FUNCTION__, __LINE__, p->physical_page.block_num, p->physical_page.page_num, page);

    memset(write_oob, 0xff, 64);
    memcpy(write_oob, p->spare_data_addr, 8);
    memcpy(write_oob + 16, p->spare_data_addr + 8, 8);
    ret = rt_mtd_nand_write(nand, page, p->user_data_addr, nand->page_size,
                            write_oob, 64);

    return ret;
}

#define IS_BAD          1
#define NFTL_BLOCK_GOOD 1
#define NFTL_BLOCK_BAD  0
int _nftl_port_hw_is_block_good(void *device, struct physical_op_info *p)
{
    struct rt_mtd_nand_device *nand = (struct rt_mtd_nand_device *)device;
    int ret = NFTL_BLOCK_GOOD;
    ret = rt_mtd_nand_check_block(nand, p->physical_page.block_num);
    PORT_HW_LOG("%s:%d ...\n", __FUNCTION__, __LINE__);
    return (ret == IS_BAD) ? NFTL_BLOCK_BAD : NFTL_BLOCK_GOOD;
}

int _nftl_port_hw_mark_bad_block(void *device, struct physical_op_info *p)
{
    struct rt_mtd_nand_device *nand = (struct rt_mtd_nand_device *)device;
    PORT_HW_LOG("%s:%d ...\n", __FUNCTION__, __LINE__);
    return rt_mtd_nand_mark_badblock(nand, p->physical_page.block_num);
}
