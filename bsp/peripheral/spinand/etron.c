/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_ETRON 0xD5

static int em73c044vcf_ecc_get_status(struct aic_spinand *flash, u8 status)
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

const struct aic_spinand_info etron_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    /*EM73C044VCF-H*/
    { DEVID(0x25), PAGESIZE(2048), OOBSIZE(64), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "etron 128MB: 2048+64@64@1024", cmd_cfg_table,
      em73c044vcf_ecc_get_status},
    /*EM73D044VCO-H*/
    { DEVID(0x3A), PAGESIZE(2048), OOBSIZE(128), BPL(2048), PPB(64),
      PLANENUM(1), DIE(0), "etron 256MB: 2048+128@64@2048", cmd_cfg_table,
      em73c044vcf_ecc_get_status},
    /*EM73E044VCE-H*/
    { DEVID(0x3B), PAGESIZE(2048), OOBSIZE(128), BPL(4096), PPB(64),
      PLANENUM(1), DIE(0), "etron 512MB: 2048+128@64@4096", cmd_cfg_table,
      em73c044vcf_ecc_get_status},
};

const struct aic_spinand_info *etron_spinand_detect(struct aic_spinand *flash)
{
    u8 *Id = flash->id.data;

    if (Id[0] != SPINAND_MFR_ETRON)
        return NULL;

    return spinand_match_and_init(Id[1], etron_spinand_table,
                                  ARRAY_SIZE(etron_spinand_table));
};

static int etron_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops etron_spinand_manuf_ops = {
    .detect = etron_spinand_detect,
    .init = etron_spinand_init,
};

const struct spinand_manufacturer etron_spinand_manufacturer = {
    .id = SPINAND_MFR_ETRON,
    .name = "etron",
    .ops = &etron_spinand_manuf_ops,
};
