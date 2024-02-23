/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <hexdump.h>
#include <mtd.h>
#include <hal_qspi.h>
#include <image.h>
#include <boot.h>
#include <aic_core.h>
#include <aic_flash_xip_def.h>
#include "fitimage.h"

#if defined(AIC_BOOTLOADER_CMD_XIP_BOOT) && defined(AIC_QSPI_DRV_V11)

#define APPLICATION_PART "os"
#define AIC_HEAD_SIZE    0x100

static int aic_xip_init(struct mtd_dev *mtd, u32 msk, u32 val)
{
    int ret = 0;
    u32 id;
    struct qspi_xip_burst_cfg burst_cfg;
    struct qspi_xip_read_cfg read_cfg;
    struct xip_device *xip_cfg;
    sfud_flash *flash = NULL;
    flash = (sfud_flash *)mtd->priv;
    const sfud_spi *spi = &flash->spi;
    struct aic_qspi_bus *qspi;
    qspi = (struct aic_qspi_bus *)spi->user_data;

    /*xip config*/
    hal_qspi_master_set_cs_owner(&qspi->handle, QSPI_CS_CTL_BY_HW);
    hal_qspi_master_set_qio_mode(&qspi->handle);

    flash->chip.mf_id = flash->chip.mf_id & 0xFFUL;
    flash->chip.type_id = flash->chip.type_id & 0xFFUL;
    flash->chip.capacity_id = flash->chip.capacity_id & 0xFFUL;
    id = ((flash->chip.mf_id << 16) | (flash->chip.type_id << 8) |
          (flash->chip.capacity_id << 0));

    printf("XIP flash ID: 0x%x\n", id);

    xip_cfg = get_xip_device_cfg(id, msk, val);
    if (xip_cfg == NULL) {
        printf("Not found xip cfg for this device.\n");
        ret = 1;
        goto out;
    }

    memcpy(&burst_cfg, &xip_cfg->burst_cfg, sizeof(burst_cfg));
    memcpy(&read_cfg, &xip_cfg->read_cfg, sizeof(read_cfg));

    hal_qspi_master_set_xip_burst_cfg(&qspi->handle, &burst_cfg);
    hal_qspi_master_set_xip_read_cfg(&qspi->handle, &read_cfg);
    hal_qspi_master_xip_enable(&qspi->handle, 1);

out:
    return ret;
}

static struct mtd_dev *nor_flash_init(void)
{
    struct mtd_dev *mtd;
    mtd_probe();
    mtd = mtd_get_device(APPLICATION_PART);
    if (!mtd) {
        printf("Failed to get application partition.\n");
        return NULL;
    }

    return mtd;
}

static int do_xip_boot(int argc, char *argv[])
{
    int ret = 0;
    struct mtd_dev *mtd;
    ulong entry_point;
    struct spl_load_info info;

    mtd = nor_flash_init();
    if (!mtd) {
        printf("XIP boot failed ...\n");
    }

    aic_xip_init(mtd, CMD_PROTO_QIO, CMD_PROTO_QIO);

    // need to delay, otherwise bootup unstable.
    aicos_udelay(1000 * 100);

    info.dev = (void *)mtd;
    info.dev_type = DEVICE_XIPNOR;
    info.bl_len = 1;
    info.priv = (void *)((unsigned long)mtd->start + FLASH_XIP_BASE);

    ret = spl_load_simple_fit(&info, &entry_point);
    if (ret < 0)
        goto out;

    /* boot */
    aicos_dcache_clean();
    boot_app((void *)entry_point);

out:
    return ret;
}

CONSOLE_CMD(xip_boot, do_xip_boot, "XIP boot from NOR.");
#endif // defined(AIC_BOOTLOADER_CMD_XIP_BOOT) && defined(AIC_QSPI_DRV_V11)
