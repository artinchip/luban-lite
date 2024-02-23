/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_QUANXING 0x01

const struct aic_spinand_info quanxing_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    /*QXS99ML01G3*/
    { DEVID(0x15), PAGESIZE(2048), OOBSIZE(128), BPL(1024), PPB(64),
      PLANENUM(1), DIE(0), "quanxing 128MB: 2048+64@64@1024", cmd_cfg_table },
};

const struct aic_spinand_info *
quanxing_spinand_detect(struct aic_spinand *flash)
{
    u8 *Id = flash->id.data;

    if (Id[0] != SPINAND_MFR_QUANXING)
        return NULL;

    return spinand_match_and_init(Id[1], quanxing_spinand_table,
                                  ARRAY_SIZE(quanxing_spinand_table));
};

static int quanxing_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops quanxing_spinand_manuf_ops = {
    .detect = quanxing_spinand_detect,
    .init = quanxing_spinand_init,
};

const struct spinand_manufacturer quanxing_spinand_manufacturer = {
    .id = SPINAND_MFR_QUANXING,
    .name = "quanxing",
    .ops = &quanxing_spinand_manuf_ops,
};
