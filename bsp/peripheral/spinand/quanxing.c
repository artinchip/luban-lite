/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_QUANXING		0x01

struct spi_nand_cmd_cfg quanxing_cmd_cfg_table[] = {
    /*opcode    opcode_bits addr_bytes	addr_bits	dummy_bytes	data_nbits*/
    { SPINAND_CMD_READ_FROM_CACHE, 1, 2, 1, 1, 1 },
    { SPINAND_CMD_READ_FROM_CACHE_X2, 1, 2, 1, 1, 2 },
    { SPINAND_CMD_READ_FROM_CACHE_X4, 1, 2, 1, 1, 4 },
    { SPINAND_CMD_PROG_LOAD, 1, 2, 1, 0, 1 },
    { SPINAND_CMD_PROG_LOAD_X4, 1, 2, 1, 0, 4 },
    { SPINAND_CMD_END },
};

const struct aic_spinand_info quanxing_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock is_die_select*/
    /*QXS99ML01G3*/
    { 0x15, 2048, 128, 1024, 64, 0, "quanxing 128MB: 2048+64@64@1024",
      quanxing_cmd_cfg_table },
};

const struct aic_spinand_info *quanxing_spinand_detect(struct aic_spinand *flash)
{
    u8 *Id = flash->id.data;

    if (Id[0] != SPINAND_MFR_QUANXING)
        return NULL;

    return spinand_match_and_init(Id[1], quanxing_spinand_table,
                                  ARRAY_SIZE(quanxing_spinand_table));
};

static int quanxing_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops quanxing_spinand_manuf_ops = {
    .detect = quanxing_spinand_detect,
    .init = quanxing_spinand_init,
};

const struct spinand_manufacturer quanxing_spinand_manufacturer = {
    .id = SPINAND_MFR_QUANXING,
    .name = "quanxing",
    .ops = &quanxing_spinand_manuf_ops,
};
