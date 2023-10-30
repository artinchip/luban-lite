/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#ifndef __BBT_H__
#define __BBT_H__

#include <spinand.h>

/* BBT related functions */
enum nand_bbt_block_status {
    NAND_BBT_BLOCK_STATUS_UNKNOWN,
    NAND_BBT_BLOCK_GOOD,
    NAND_BBT_BLOCK_RESERVED,
    NAND_BBT_BLOCK_FACTORY_BAD,
};

int nand_bbt_init(struct aic_spinand *flash);
bool nand_bbt_is_initialized(struct aic_spinand *flash);
void nand_bbt_cleanup(struct aic_spinand *flash);
int nand_bbt_get_block_status(struct aic_spinand *flash, u32 entry);
void nand_bbt_set_block_status(struct aic_spinand *flash, u32 entry,
                               enum nand_bbt_block_status status);

#endif /* __BBT_H__ */
