/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */
#include "spinand.h"
#include "manufacturer.h"

#define SPINAND_MFR_GIGADEVICE    0xC8

#define GD5FXGQXXEXXG_REG_STATUS2 0xf0

#define GIGADEVICE                _CFG_QUAD_ENABLE BIT(0)

struct spi_nand_cmd_cfg gigadevice_cmd_cfg_table[] = {
    /*opcode    opcode_bits addr_bytes	addr_bits	dummy_bytes	data_nbits*/
    { SPINAND_CMD_READ_FROM_CACHE, 1, 2, 1, 1, 1 },
    { SPINAND_CMD_READ_FROM_CACHE_X2, 1, 2, 1, 1, 2 },
    { SPINAND_CMD_READ_FROM_CACHE_X4, 1, 2, 1, 1, 4 },
    { SPINAND_CMD_PROG_LOAD, 1, 2, 1, 0, 1 },
    { SPINAND_CMD_PROG_LOAD_X4, 1, 2, 1, 0, 4 },
    { SPINAND_CMD_END },
};

const struct aic_spinand_info gigadevice_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock is_die_select*/
    /*GD5F2GM7UE*/
    { 0x92, 2048, 128, 2048, 64, 0, "GIGADEVICE 256MB: 2048+128@64@2048",
      gigadevice_cmd_cfg_table },
    /*GD5F4GM8UE*/
    { 0x95, 2048, 128, 4096, 64, 0, "GIGADEVICE 512MB: 2048+128@64@4096",
      gigadevice_cmd_cfg_table },
    /*GD5F1GQ5UE*/
    { 0x51, 2048, 128, 1024, 64, 0, "GIGADEVICE 128MB: 2048+128@64@1024",
      gigadevice_cmd_cfg_table },
    /*GD5F1GM7UE*/
    { 0x91, 2048, 128, 1024, 64, 0, "GIGADEVICE 128MB: 2048+128@64@1024",
      gigadevice_cmd_cfg_table },
    /*GD5F2GQ5UE*/
    { 0x52, 2048, 128, 2048, 64, 0, "GIGADEVICE 256MB: 2048+128@64@2048",
      gigadevice_cmd_cfg_table },
    /*GD5F1GQ5REYIGR*/
    { 0x41, 2048, 128, 1024, 64, 0, "GIGADEVICE 128MB: 2048+128@64@1024",
      gigadevice_cmd_cfg_table },
};

const struct aic_spinand_info *gigadevice_spinand_detect(struct aic_spinand *flash)
{
    u8 *Id = flash->id.data;

    if (Id[0] != SPINAND_MFR_GIGADEVICE)
        return NULL;

    return spinand_match_and_init(Id[1], gigadevice_spinand_table,
                                  ARRAY_SIZE(gigadevice_spinand_table));
};

static int gigadevice_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops gigadevice_spinand_manuf_ops = {
    .detect = gigadevice_spinand_detect,
    .init = gigadevice_spinand_init,
};

const struct spinand_manufacturer gigadevice_spinand_manufacturer = {
    .id = SPINAND_MFR_GIGADEVICE,
    .name = "GIGADEVICE	",
    .ops = &gigadevice_spinand_manuf_ops,
};
