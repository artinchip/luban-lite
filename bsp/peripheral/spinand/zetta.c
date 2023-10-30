/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

//#define SPINAND_MFR_ZETTA		0xBA
#define SPINAND_MFR_ZETTA		0x2C

struct spi_nand_cmd_cfg zetta_cmd_cfg_table[] = {
    /*opcode    opcode_bits addr_bytes	addr_bits	dummy_bytes	data_nbits*/
    { SPINAND_CMD_READ_FROM_CACHE, 1, 2, 1, 1, 1 },
    { SPINAND_CMD_READ_FROM_CACHE_X2, 1, 2, 1, 1, 2 },
    { SPINAND_CMD_READ_FROM_CACHE_X4, 1, 2, 1, 1, 4 },
    { SPINAND_CMD_PROG_LOAD, 1, 2, 1, 0, 1 },
    { SPINAND_CMD_PROG_LOAD_X4, 1, 2, 1, 0, 4 },
    { SPINAND_CMD_END },
};

const struct aic_spinand_info zetta_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock is_die_select*/
    /*ZD35Q1GC-IB*/
    { 0x71, 2048, 64, 1024, 64, 0, "zetta 128MB: 2048+64@64@1024",
      zetta_cmd_cfg_table },
    /*ZD35Q2GC-IB*/
    { 0x24, 2048, 128, 2048, 64, 0, "zetta 256MB: 2048+128@64@2048",
      zetta_cmd_cfg_table },
};

const struct aic_spinand_info *zetta_spinand_detect(struct aic_spinand *flash)
{
    u8 *Id = flash->id.data;

    if (Id[0] != SPINAND_MFR_ZETTA)
        return NULL;

    return spinand_match_and_init(Id[1], zetta_spinand_table,
                                  ARRAY_SIZE(zetta_spinand_table));
};

static int zetta_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops zetta_spinand_manuf_ops = {
    .detect = zetta_spinand_detect,
    .init = zetta_spinand_init,
};

const struct spinand_manufacturer zetta_spinand_manufacturer = {
    .id = SPINAND_MFR_ZETTA,
    .name = "zetta",
    .ops = &zetta_spinand_manuf_ops,
};
