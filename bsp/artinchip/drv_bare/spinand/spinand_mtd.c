/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xuan.Wen <xuan.wen@artinchip.com>
 */

#include <rtconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <aic_common.h>
#include <string.h>
#include <aic_core.h>
#include "spinand_port.h"
#include <mtd.h>
#include <partition_table.h>

#ifdef IMAGE_CFG_JSON_PARTS_MTD
#define NAND_MTD_PARTS IMAGE_CFG_JSON_PARTS_MTD
#else
#define NAND_MTD_PARTS ""
#endif

static int mtd_spinand_read(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len)
{
    u8 err;
    u32 start, dolen;
    struct aic_spinand *flash;

    if (!mtd)
        return -1;

    flash = (struct aic_spinand *)mtd->priv;

    start = mtd->start + offset;
    dolen = len;

    if ((mtd->size - offset) < dolen)
        dolen = mtd->size - offset;

    err = spinand_read(flash, data, start, dolen);
    return err;
}

static int mtd_spinand_read_oob(struct mtd_dev *mtd, u32 offset, u8 *data,
                                u32 len, u8 *spare_data, u32 spare_len)
{
    int err;
    u32 start;
    u32 page;
    struct aic_spinand *flash;

    if (!mtd)
        return -1;

    flash = (struct aic_spinand *)mtd->priv;

    if (offset % flash->info->page_size) {
        printf("Offset not aligned with a page (0x%x)\r\n",
               flash->info->page_size);
        return -1;
    }

    if ((mtd->size - offset) < flash->info->page_size) {
        printf("Offset: 0x%x is out of mtd size: 0x%lx.\n", offset, mtd->size);
        return -1;
    }

    start = mtd->start + offset;
    page = start / flash->info->page_size;

    err = spinand_read_page(flash, page, data, len, spare_data, spare_len);
    return err;
}

static int mtd_spinand_erase(struct mtd_dev *mtd, u32 offset, u32 len)
{
    u8 err;
    u32 start, dolen;
    struct aic_spinand *flash;

    if (!mtd)
        return -1;

    flash = (struct aic_spinand *)mtd->priv;

    start = mtd->start + offset;
    dolen = len;

    if ((mtd->size - offset) < dolen)
        dolen = mtd->size - offset;

    err = spinand_erase(flash, start, dolen);
    return err;
}

static int mtd_spinand_block_isbad(struct mtd_dev *mtd, u32 offset)
{
    u8 err;
    u32 start;
    u32 blk;
    struct aic_spinand *flash;
    u32 blocksize;

    if (!mtd)
        return -1;

    flash = (struct aic_spinand *)mtd->priv;

    blocksize = flash->info->page_size * flash->info->pages_per_eraseblock;

    if (offset % blocksize) {
        printf("Offset not aligned with a block (0x%x)\r\n", blocksize);
        return -1;
    }

    if ((mtd->size - offset) < blocksize) {
        printf("Offset: 0x%x is out of mtd size: 0x%lx.\n", offset, mtd->size);
        return -1;
    }

    start = mtd->start + offset;
    blk = start / blocksize;

    err = spinand_block_isbad(flash, blk);
    if (err != 0) {
        printf("Block %d is bad.\n", blk);
    }
    return err;
}

static int mtd_spinand_block_markbad(struct mtd_dev *mtd, u32 offset)
{
    u8 err;
    u32 start;
    u16 blk;
    struct aic_spinand *flash;
    u32 blocksize;

    if (!mtd)
        return -1;

    flash = (struct aic_spinand *)mtd->priv;

    blocksize = flash->info->page_size * flash->info->pages_per_eraseblock;

    if (offset % blocksize) {
        printf("Offset not aligned with a block (0x%x)\r\n", blocksize);
        return -1;
    }

    if ((mtd->size - offset) < blocksize) {
        printf("Offset: 0x%x is out of mtd size: 0x%lx.\n", offset, mtd->size);
        return -1;
    }

    start = mtd->start + offset;
    blk = start / blocksize;

    err = spinand_block_markbad(flash, blk);
    if (err != 0) {
        printf("Mark badblock %d failed.\n",blk);
    }
    return err;
}

static int mtd_spinand_write(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len)
{
    u8 err;
    u32 start, dolen;
    struct aic_spinand *flash;

    if (!mtd)
        return -1;

    flash = (struct aic_spinand *)mtd->priv;

    start = mtd->start + offset;
    dolen = len;

    if ((mtd->size - offset) < dolen)
        dolen = mtd->size - offset;

    err = spinand_write(flash, data, start, dolen);
    return err;
}

static int mtd_spinand_write_oob(struct mtd_dev *mtd, u32 offset, u8 *data,
                                 u32 len, u8 *spare_data, u32 spare_len)
{
    u8 err;
    u32 start;
    u32 page;
    struct aic_spinand *flash;

    if (!mtd)
        return -1;

    flash = (struct aic_spinand *)mtd->priv;

    if (offset % flash->info->page_size) {
        printf("Offset not aligned with a page (0x%x)\r\n",
               flash->info->page_size);
        return -1;
    }

    if ((mtd->size - offset) < flash->info->page_size) {
        printf("Offset: 0x%x is out of mtd size: 0x%lx.\n", offset, mtd->size);
        return -1;
    }

    start = mtd->start + offset;
    page = start / flash->info->page_size;

    err = spinand_write_page(flash, page, data, len, spare_data, spare_len);
    return err;
}

#ifdef AIC_SPINAND_CONT_READ
static int mtd_spinand_continuous_read(struct mtd_dev *mtd, u32 offset,
                                       u8 *data, u32 size)
{
    struct aic_spinand *flash;
    u32 start;
    u32 page;

    if (!mtd)
        return -1;

    flash = (struct aic_spinand *)mtd->priv;

    start = mtd->start + offset;
    page = start / flash->info->page_size;

    return spinand_continuous_read(flash, page, data, size);
}
#else
static int mtd_spinand_continuous_read(struct mtd_dev *mtd, u32 offset,
                                       u8 *data, u32 size)
{
    pr_err("Please enable config AIC_SPINAND_CONT_READ!.\n");
    return -1;
}
#endif

static char *get_part_str(u32 spi_bus)
{
    char name[8] = {0};

    strcpy(name, "spi0");
    name[3] += spi_bus;
    if (strncmp(name, NAND_MTD_PARTS, 4) == 0)
        return NAND_MTD_PARTS;
    return NULL;
}

struct aic_spinand *spinand_probe(u32 spi_bus)
{
    u8 err = 0;
    struct mtd_dev *mtd;
    struct mtd_partition *part, *p;
    struct aic_spinand *flash = NULL;
    struct aic_qspi *qspi = NULL;
    char *partstr = NULL;

    qspi = get_qspi_by_index(spi_bus);
    if (!qspi) {
        pr_err("spi bus is invalid: %d\n", spi_bus);
        return NULL;
    }

    if ((qspi->inited) && (qspi->attached_flash))
        return qspi->attached_flash;

    flash = malloc(sizeof(struct aic_spinand));
    if (!flash) {
        pr_err("malloc buf failed\n");
        return NULL;
    }

    flash->user_data = qspi;
    flash->qspi_dl_width = 4;

    qspi_configure(qspi, NULL);

    err = spinand_flash_init(flash);
    if (err < 0) {
        printf("Failed to probe spinand flash.\n");
        return NULL;
    }

    mtd = malloc(sizeof(*mtd));
    mtd->name = strdup("nand0");
    mtd->name[4] += spi_bus;
    mtd->start = 0;
    mtd->size = flash->info->page_size * flash->info->block_per_lun *
                flash->info->pages_per_eraseblock;
    mtd->erasesize = flash->info->page_size * flash->info->pages_per_eraseblock;
    mtd->writesize = flash->info->page_size;
    mtd->oobsize = flash->info->oob_size;
    mtd->ops.erase = mtd_spinand_erase;
    mtd->ops.block_isbad = mtd_spinand_block_isbad;
    mtd->ops.block_markbad = mtd_spinand_block_markbad;
    mtd->ops.read = mtd_spinand_read;
    mtd->ops.write = mtd_spinand_write;
    mtd->ops.read_oob = mtd_spinand_read_oob;
    mtd->ops.write_oob = mtd_spinand_write_oob;
    mtd->ops.cont_read = mtd_spinand_continuous_read;
    mtd->priv = (void *)flash;
    mtd_add_device(mtd);

    partstr = get_part_str(spi_bus);
    part = mtd_parts_parse(partstr);
    p = part;
    while (p) {
        mtd = malloc(sizeof(*mtd));
        mtd->name = strdup(p->name);
        mtd->start = p->start;
        mtd->size = p->size;
        if (p->size == 0)
            mtd->size = flash->info->page_size * flash->info->block_per_lun *
                            flash->info->pages_per_eraseblock -
                        p->start;
        mtd->erasesize =
            flash->info->page_size * flash->info->pages_per_eraseblock;
        mtd->writesize = flash->info->page_size;
        mtd->oobsize = flash->info->oob_size;
        mtd->ops.erase = mtd_spinand_erase;
        mtd->ops.block_isbad = mtd_spinand_block_isbad;
        mtd->ops.block_markbad = mtd_spinand_block_markbad;
        mtd->ops.read = mtd_spinand_read;
        mtd->ops.write = mtd_spinand_write;
        mtd->ops.read_oob = mtd_spinand_read_oob;
        mtd->ops.write_oob = mtd_spinand_write_oob;
        mtd->ops.cont_read = mtd_spinand_continuous_read;
        mtd->priv = (void *)flash;
        mtd_add_device(mtd);
        p = p->next;
    }

    if (part)
        mtd_parts_free(part);

    qspi->attached_flash = flash;

    return flash;
}
