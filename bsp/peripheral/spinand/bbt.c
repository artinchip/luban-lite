/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <spinand.h>
#include <bbt.h>

#define BITS_PER_BLOCK 2
/*a Byte can save 4 block bad block info*/

int nand_bbt_init(struct aic_spinand *flash)
{
    u32 nblocks = flash->info->block_per_lun;
    u32 bits_per_block = BITS_PER_BLOCK;

    flash->bbt.cache = calloc(1, nblocks * bits_per_block / 8);
    if (!flash->bbt.cache)
        return -SPINAND_ERR;

    return SPINAND_SUCCESS;
}

bool nand_bbt_is_initialized(struct aic_spinand *flash)
{
    return !!flash->bbt.cache;
}

void nand_bbt_cleanup(struct aic_spinand *flash)
{
    free(flash->bbt.cache);
}

int nand_bbt_get_block_status(struct aic_spinand *flash, u32 entry)
{
    u32 bits_per_block = BITS_PER_BLOCK;
    u8 *pos = flash->bbt.cache + entry * bits_per_block / 8;
    u32 offs = entry * bits_per_block % 8;

    pr_debug("block: %d.\n", entry);

    if (entry >= flash->info->block_per_lun)
        return 0;

    return (pos[0] >> offs) & 0x3;
}

void nand_bbt_set_block_status(struct aic_spinand *flash, u32 entry,
                               enum nand_bbt_block_status status)
{
    u32 bits_per_block = BITS_PER_BLOCK;
    u8 *pos = flash->bbt.cache + entry * bits_per_block / 8;
    u32 offs = entry * bits_per_block % 8;

    pr_debug("block: %d, status = 0x%d.\n", entry, status);

    if (entry >= flash->info->block_per_lun)
        return;

    pos[0] &= ~(0x3 << offs);
    pos[0] |= status << offs;
}
