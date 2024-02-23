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

const struct aic_spinand_info foresee_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    /*F35SQA512M*/
    { DEVID(0x70), PAGESIZE(2048), OOBSIZE(64), BPL(512), PPB(64), PLANENUM(1),
      DIE(0), "foresee 64MB: 2048+64@64@512", cmd_cfg_table },
    /*F35SQA001G*/
    { DEVID(0x71), PAGESIZE(2048), OOBSIZE(64), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "foresee 128MB: 2048+64@64@1024", cmd_cfg_table },
    /*F35SQA002G*/
    { DEVID(0x72), PAGESIZE(2048), OOBSIZE(64), BPL(2048), PPB(64), PLANENUM(1),
      DIE(0), "foresee 256MB: 2048+64@64@2048", cmd_cfg_table },
    /*FS35ND04G*/
    { DEVID(0xEC), PAGESIZE(2048), OOBSIZE(64), BPL(4096), PPB(64), PLANENUM(1),
      DIE(0), "foresee 512MB: 2048+64@64@4096", cmd_cfg_table },
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
