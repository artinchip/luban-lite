/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_FORESEE 0xCD

struct spi_nand_cmd_cfg foresee_cmd_cfg_table[] = {
    /*opcode    opcode_bits addr_bytes	addr_bits	dummy_bytes	data_nbits*/
    { SPINAND_CMD_READ_FROM_CACHE, 1, 2, 1, 1, 1 },
    { SPINAND_CMD_READ_FROM_CACHE_X2, 1, 2, 1, 1, 2 },
    { SPINAND_CMD_READ_FROM_CACHE_X4, 1, 2, 1, 1, 4 },
    { SPINAND_CMD_PROG_LOAD, 1, 2, 1, 0, 1 },
    { SPINAND_CMD_PROG_LOAD_X4, 1, 2, 1, 0, 4 },
    { SPINAND_CMD_END },
};

const struct aic_spinand_info foresee_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock is_die_select*/
    /*F35SQA512M*/
    { 0x70, 2048, 64, 512, 64, 0, "foresee 64MB: 2048+64@64@512",
      foresee_cmd_cfg_table },
    /*F35SQA001G*/
    { 0x71, 2048, 64, 1024, 64, 0, "foresee 128MB: 2048+64@64@1024",
      foresee_cmd_cfg_table },
    /*F35SQA002G*/
    { 0x72, 2048, 64, 2048, 64, 0, "foresee 256MB: 2048+64@64@2048",
      foresee_cmd_cfg_table },
    /*FS35ND04G*/
    { 0xEC, 2048, 64, 4096, 64, 0, "foresee 512MB: 2048+64@64@4096",
      foresee_cmd_cfg_table },
};

const struct aic_spinand_info *foresee_spinand_detect(struct aic_spinand *flash)
{
    u8 *Id = flash->id.data;

    if (Id[0] != SPINAND_MFR_FORESEE)
        return NULL;

    return spinand_match_and_init(Id[1], foresee_spinand_table,
                                  ARRAY_SIZE(foresee_spinand_table));
};

static int foresee_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops foresee_spinand_manuf_ops = {
    .detect = foresee_spinand_detect,
    .init = foresee_spinand_init,
};

const struct spinand_manufacturer foresee_spinand_manufacturer = {
    .id = SPINAND_MFR_FORESEE,
    .name = "foresee",
    .ops = &foresee_spinand_manuf_ops,
};
