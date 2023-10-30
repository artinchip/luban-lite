/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_MACRONIX		0xC2

struct spi_nand_cmd_cfg macronix_cmd_cfg_table[] = {
    /*opcode    opcode_bits addr_bytes	addr_bits	dummy_bytes	data_nbits*/
    { SPINAND_CMD_READ_FROM_CACHE, 1, 2, 1, 1, 1 },
    { SPINAND_CMD_READ_FROM_CACHE_X2, 1, 2, 1, 1, 2 },
    { SPINAND_CMD_READ_FROM_CACHE_X4, 1, 2, 1, 1, 4 },
    { SPINAND_CMD_PROG_LOAD, 1, 2, 1, 0, 1 },
    { SPINAND_CMD_PROG_LOAD_X4, 1, 2, 1, 0, 4 },
    { SPINAND_CMD_END },
};

const struct aic_spinand_info macronix_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock is_die_select*/
    /*MX35LF2G14AC-Z4I*/
    { 0x20, 2048, 64, 2048, 64, 0, "macronix 256MB: 2048+64@64@2048",
      macronix_cmd_cfg_table },
    /*MX35LF1G24AD-Z4I*/
    { 0x14, 2048, 128, 1024, 64, 0, "macronix 128MB: 2048+128@64@1024",
      macronix_cmd_cfg_table },
};

const struct aic_spinand_info *macronix_spinand_detect(struct aic_spinand *flash)
{
    u8 *Id = flash->id.data;

    if (Id[0] != SPINAND_MFR_MACRONIX)
        return NULL;

    return spinand_match_and_init(Id[1], macronix_spinand_table,
                                  ARRAY_SIZE(macronix_spinand_table));
};

static int macronix_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops macronix_spinand_manuf_ops = {
    .detect = macronix_spinand_detect,
    .init = macronix_spinand_init,
};

const struct spinand_manufacturer macronix_spinand_manufacturer = {
    .id = SPINAND_MFR_MACRONIX,
    .name = "macronix",
    .ops = &macronix_spinand_manuf_ops,
};
