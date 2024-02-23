/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_DOSILICON 0xE5

const struct aic_spinand_info dosilicon_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    /*DS35Q1GA-IB*/
    { DEVID(0x71), PAGESIZE(2048), OOBSIZE(64), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "dosilicon 128MB: 2048+64@64@1024", cmd_cfg_table },
    /*DS35Q2GA-IB*/
    { DEVID(0x72), PAGESIZE(2048), OOBSIZE(64), BPL(2048), PPB(64), PLANENUM(2),
      DIE(0), "dosilicon 256MB: 2048+64@64@2048", cmd_cfg_table },
};

const struct aic_spinand_info *
dosilicon_spinand_detect(struct aic_spinand *flash)
{
    u8 *Id = flash->id.data;

    if (Id[0] != SPINAND_MFR_DOSILICON)
        return NULL;

    return spinand_match_and_init(Id[1], dosilicon_spinand_table,
                                  ARRAY_SIZE(dosilicon_spinand_table));
};

static int dosilicon_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops dosilicon_spinand_manuf_ops = {
    .detect = dosilicon_spinand_detect,
    .init = dosilicon_spinand_init,
};

const struct spinand_manufacturer dosilicon_spinand_manufacturer = {
    .id = SPINAND_MFR_DOSILICON,
    .name = "dosilicon",
    .ops = &dosilicon_spinand_manuf_ops,
};
