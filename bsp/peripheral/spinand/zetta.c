/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_ZETTA		0xBA

static int zd35q1gc_ecc_get_status(struct aic_spinand *flash, u8 status)
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

const struct aic_spinand_info zetta_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    /*ZD35Q1GC-IB*/
    { DEVID(0x71), PAGESIZE(2048), OOBSIZE(64), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "zetta 128MB: 2048+64@64@1024", cmd_cfg_table,
      zd35q1gc_ecc_get_status},
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
