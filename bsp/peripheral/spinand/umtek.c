/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_UMTEK		0x52

struct spi_nand_cmd_cfg umtek_cmd_cfg_table[] = {
    /*opcode    opcode_bits addr_bytes	addr_bits	dummy_bytes	data_nbits*/
    { SPINAND_CMD_READ_FROM_CACHE, 1, 2, 1, 1, 1 },
    { SPINAND_CMD_READ_FROM_CACHE_X2, 1, 2, 1, 1, 2 },
    { SPINAND_CMD_READ_FROM_CACHE_X4, 1, 2, 1, 1, 4 },
    { SPINAND_CMD_PROG_LOAD, 1, 2, 1, 0, 1 },
    { SPINAND_CMD_PROG_LOAD_X4, 1, 2, 1, 0, 4 },
    { SPINAND_CMD_END },
};

const struct aic_spinand_info umtek_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock is_die_select*/
    /*GSS01GAK1*/
    { 0xBA, 2048, 64, 1024, 64, 0, "umtek 128MB: 2048+64@64@1024",
      umtek_cmd_cfg_table },
};

const struct aic_spinand_info *umtek_spinand_detect(struct aic_spinand *flash)
{
    u8 *Id = flash->id.data;

    if (Id[0] != SPINAND_MFR_UMTEK)
        return NULL;

    return spinand_match_and_init(Id[1], umtek_spinand_table,
                                  ARRAY_SIZE(umtek_spinand_table));
};

static int umtek_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops umtek_spinand_manuf_ops = {
    .detect = umtek_spinand_detect,
    .init = umtek_spinand_init,
};

const struct spinand_manufacturer umtek_spinand_manufacturer = {
    .id = SPINAND_MFR_UMTEK,
    .name = "umtek",
    .ops = &umtek_spinand_manuf_ops,
};
