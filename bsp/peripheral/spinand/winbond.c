/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_WINBOND 0xEF

#define WINBOND_CFG_BUF_READ BIT(3)

struct spi_nand_cmd_cfg winbond_cmd_cfg_table[] = {
    /*opcode    opcode_bits addr_bytes	addr_bits	dummy_bytes	data_nbits*/
    { SPINAND_CMD_READ_FROM_CACHE, 1, 2, 1, 1, 1 },
    { SPINAND_CMD_READ_FROM_CACHE_X2, 1, 2, 1, 1, 2 },
    { SPINAND_CMD_READ_FROM_CACHE_X4, 1, 2, 1, 1, 4 },
    { SPINAND_CMD_PROG_LOAD, 1, 2, 1, 0, 1 },
    { SPINAND_CMD_PROG_LOAD_X4, 1, 2, 1, 0, 4 },
    { SPINAND_CMD_END },
};

const struct aic_spinand_info winbond_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock is_die_select*/
    { 0xAA, 2048, 64, 1024, 64, 0, "Winbond 128MB: 2048+64@64@1024",
      winbond_cmd_cfg_table },
    { 0xBF, 2048, 64, 2048, 64, 0, "Winbond 256MB: 2048+64@64@2048",
      winbond_cmd_cfg_table },
    { 0xAB, 2048, 64, 1024, 64, 1, "Winbond 256MB: 2048+64@64@1024, MCP",
      winbond_cmd_cfg_table },
    { 0xBA, 2048, 64, 1024, 64, 0, "Winbond 128MB: 2048+64@64@1024",
      winbond_cmd_cfg_table },
/*    { 0xAA, 2048, 128, 1024, 64, 1, "Winbond 256MB: 2048+64@64@1024",
      winbond_cmd_cfg_table },*/
};

const struct aic_spinand_info *winbond_spinand_detect(struct aic_spinand *flash)
{
    u8 *Id = flash->id.data;

    if (Id[0] != SPINAND_MFR_WINBOND)
        return NULL;

    return spinand_match_and_init(Id[1], winbond_spinand_table,
                                  ARRAY_SIZE(winbond_spinand_table));
};

static int winbond_spinand_init(struct aic_spinand *flash)
{
    return spinand_config_set(flash, WINBOND_CFG_BUF_READ,
                              WINBOND_CFG_BUF_READ);
};

static const struct spinand_manufacturer_ops winbond_spinand_manuf_ops = {
    .detect = winbond_spinand_detect,
    .init = winbond_spinand_init,
};

const struct spinand_manufacturer winbond_spinand_manufacturer = {
    .id = SPINAND_MFR_WINBOND,
    .name = "Winbond",
    .ops = &winbond_spinand_manuf_ops,
};
