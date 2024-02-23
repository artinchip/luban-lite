/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_TOSHIBA 0x98

int toshiba_ecc_get_status(struct aic_spinand *flash, u8 status)
{
    switch (status & STATUS_ECC_MASK) {
        case STATUS_ECC_NO_BITFLIPS:
            return 0;
        case STATUS_ECC_HAS_1_4_BITFLIPS:
            return 4;
        case STATUS_ECC_UNCOR_ERROR:
            return -SPINAND_ERR_ECC;
        case STATUS_ECC_MASK:
            return 4;
        default:
            break;
    }

    return -SPINAND_ERR;
}

const struct aic_spinand_info toshiba_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    /*TC58CVG1S3HRAIJ*/
    { DEVID(0xEB), PAGESIZE(2048), OOBSIZE(128), BPL(2048), PPB(64),
      PLANENUM(1), DIE(0), "toshiba 256MB: 2048+128@64@2048", cmd_cfg_table,
      toshiba_ecc_get_status },
};

const struct aic_spinand_info *toshiba_spinand_detect(struct aic_spinand *flash)
{
    u8 *Id = flash->id.data;

    if (Id[0] != SPINAND_MFR_TOSHIBA)
        return NULL;

    return spinand_match_and_init(Id[1], toshiba_spinand_table,
                                  ARRAY_SIZE(toshiba_spinand_table));
};

static int toshiba_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops toshiba_spinand_manuf_ops = {
    .detect = toshiba_spinand_detect,
    .init = toshiba_spinand_init,
};

const struct spinand_manufacturer toshiba_spinand_manufacturer = {
    .id = SPINAND_MFR_TOSHIBA,
    .name = "toshiba",
    .ops = &toshiba_spinand_manuf_ops,
};
