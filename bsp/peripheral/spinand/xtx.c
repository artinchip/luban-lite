/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_XTX 0x0B

#define XTX_CFG_QUAD_ENABLE BIT(0)

#define XT26G01C_STATUS_ECC_MASK        (0xF << 4)
#define XT26G01C_STATUS_ECC_NO_BITFLIPS (0 << 4)
#define XT26G01C_STATUS_ECC_UNCOR_ERROR (0xF << 4)

int xt26g01c_ecc_get_status(struct aic_spinand *flash, u8 status)
{
    switch (status & XT26G01C_STATUS_ECC_MASK) {
        case XT26G01C_STATUS_ECC_NO_BITFLIPS:
            return 0;
        case XT26G01C_STATUS_ECC_UNCOR_ERROR:
            return -SPINAND_ERR_ECC;
        default:
            break;
    }

    return status & XT26G01C_STATUS_ECC_MASK;
}

const struct aic_spinand_info xtx_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    /*XT26G11C device*/
    { DEVID(0x15), PAGESIZE(2048), OOBSIZE(128), BPL(1024), PPB(64),
      PLANENUM(1), DIE(0), "XTX 128MB: 2048+128@64@1024", cmd_cfg_table },
    /*XT26G02C device*/
    { DEVID(0x12), PAGESIZE(2048), OOBSIZE(128), BPL(2048), PPB(64),
      PLANENUM(1), DIE(0), "XTX 256MB: 2048+128@64@2048", cmd_cfg_table },
    /*XT26G01C device*/
    { DEVID(0x11), PAGESIZE(2048), OOBSIZE(128), BPL(1024), PPB(64),
      PLANENUM(1), DIE(0), "XTX 128MB: 2048+128@64@1024", cmd_cfg_table,
      xt26g01c_ecc_get_status },
    /*XT26G01D device*/
    { DEVID(0x31), PAGESIZE(2048), OOBSIZE(128), BPL(1024), PPB(64),
      PLANENUM(1), DIE(0), "XTX 128MB: 2048+128@64@1024", cmd_cfg_table,
      xt26g01c_ecc_get_status },
    /*XT26G01B device*/
    { DEVID(0xF1), PAGESIZE(2048), OOBSIZE(64), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "XTX 128MB: 2048+64@64@1024", cmd_cfg_table },
    /*XT26G04D device
    { DEVID(0x33), PAGESIZE(4096), OOBSIZE(256), BPL(2048), PPB(64), PLANENUM(1),
      DIE(0), "XTX 512MB: 4096+256@64@2048", cmd_cfg_table, xt26g01c_ecc_get_status},
      */
};

const struct aic_spinand_info *xtx_spinand_detect(struct aic_spinand *flash)
{
    /*XT26G11C XT26G02C device id 1 Bity*/
    u8 *Id = flash->id.data;

    if (Id[0] != SPINAND_MFR_XTX)
        return NULL;

    return spinand_match_and_init(Id[1], xtx_spinand_table,
                                  ARRAY_SIZE(xtx_spinand_table));
};

static int xtx_spinand_init(struct aic_spinand *flash)
{
    /* Enable quad mode */
    return spinand_config_set(flash, XTX_CFG_QUAD_ENABLE, XTX_CFG_QUAD_ENABLE);
};

static const struct spinand_manufacturer_ops xtx_spinand_manuf_ops = {
    .detect = xtx_spinand_detect,
    .init = xtx_spinand_init,
};

const struct spinand_manufacturer xtx_spinand_manufacturer = {
    .id = SPINAND_MFR_XTX,
    .name = "XTX",
    .ops = &xtx_spinand_manuf_ops,
};
