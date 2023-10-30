/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>

#if defined(RT_USING_DFS_UFFS) && defined(RT_UFFS_ECC_MODE_3)
#include "dfs_uffs.h"

static const u8 MTD2K_LAYOUT_ECC[UFFS_SPARE_LAYOUT_SIZE] = {0xFF, 0x00};
static const u8 MTD2K_LAYOUT_DATA[UFFS_SPARE_LAYOUT_SIZE] = {0x10, 0x04, 0x20, 0x04, 0xFF, 0x00};
void uffs_setup_storage(struct uffs_StorageAttrSt *attr,
                        struct rt_mtd_nand_device *nand)
{
    RT_ASSERT(attr != RT_NULL);
    RT_ASSERT(nand != RT_NULL);

    rt_memset(attr, 0, sizeof(struct uffs_StorageAttrSt));

    attr->page_data_size = nand->page_size;                /* page data size */
    attr->pages_per_block = nand->pages_per_block;         /* pages per block */
    attr->spare_size = nand->oob_size;                     /* page spare size */
    attr->ecc_opt = RT_CONFIG_UFFS_ECC_MODE;               /* ecc option */
    attr->ecc_size = nand->oob_size - nand->oob_free;      /* ecc size */
    attr->block_status_offs = 0;                           /* indicate block bad or good, offset in spare */
    attr->layout_opt = RT_CONFIG_UFFS_LAYOUT;              /* let UFFS do the spare layout */

    /* initialize  _uffs_data_layout and _uffs_ecc_layout */
    rt_memcpy(attr->_uffs_data_layout, MTD2K_LAYOUT_DATA, UFFS_SPARE_LAYOUT_SIZE);
    rt_memcpy(attr->_uffs_ecc_layout, MTD2K_LAYOUT_ECC, UFFS_SPARE_LAYOUT_SIZE);

    attr->data_layout = MTD2K_LAYOUT_DATA;
    attr->ecc_layout = MTD2K_LAYOUT_ECC;
}
#endif
