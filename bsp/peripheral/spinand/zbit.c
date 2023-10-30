/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_ZBIT		0x5E

struct spi_nand_cmd_cfg zbit_cmd_cfg_table[] = {
    /*opcode    opcode_bits addr_bytes	addr_bits	dummy_bytes	data_nbits*/
    { SPINAND_CMD_READ_FROM_CACHE, 1, 2, 1, 1, 1 },
    { SPINAND_CMD_READ_FROM_CACHE_X2, 1, 2, 1, 1, 2 },
    { SPINAND_CMD_READ_FROM_CACHE_X4, 1, 2, 1, 1, 4 },
    { SPINAND_CMD_PROG_LOAD, 1, 2, 1, 0, 1 },
    { SPINAND_CMD_PROG_LOAD_X4, 1, 2, 1, 0, 4 },
    { SPINAND_CMD_END },
};

const struct aic_spinand_info zbit_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock is_die_select*/
    /*ZB35Q01A*/
    { 0x41, 2048, 64, 1024, 64, 0, "zbit 128MB: 2048+64@64@1024",
      zbit_cmd_cfg_table },
};

const struct aic_spinand_info *zbit_spinand_detect(struct aic_spinand *flash)
{
    u8 *Id = flash->id.data;

    if (Id[0] != SPINAND_MFR_ZBIT)
        return NULL;

    return spinand_match_and_init(Id[1], zbit_spinand_table,
                                  ARRAY_SIZE(zbit_spinand_table));
};

static int zbit_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops zbit_spinand_manuf_ops = {
    .detect = zbit_spinand_detect,
    .init = zbit_spinand_init,
};

const struct spinand_manufacturer zbit_spinand_manufacturer = {
    .id = SPINAND_MFR_ZBIT,
    .name = "zbit",
    .ops = &zbit_spinand_manuf_ops,
};
