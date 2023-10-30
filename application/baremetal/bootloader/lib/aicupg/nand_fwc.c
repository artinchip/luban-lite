/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xiong Hao <hao.xiong@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <aicupg.h>
#include <spinand_port.h>
#include <mtd.h>
#include <aic_common.h>
#include "upg_internal.h"
#include "nand_fwc_spl.h"

#define MAX_NAND_NAME 32

struct aicupg_mtd_partition {
    char name[MAX_NAND_NAME];
    u64 size;
    struct aicupg_mtd_partition *next;
};

struct aicupg_nand_dev {
    char name[MAX_NAND_NAME];
    struct aicupg_nand_dev *next;
};

struct aicupg_mtd {
    char name[MAX_NAND_NAME];
};

static s32 nand_fwc_get_mtd_partitions(struct fwc_info *fwc,
                                       struct aicupg_nand_priv *priv)
{
    char name[MAX_NAND_NAME], *p;
    int cnt = 0, idx = 0;

    p = fwc->meta.partition;
    while (*p) {
        if (cnt >= MAX_NAND_NAME) {
            pr_err("Partition name is too long.\n");
            return -1;
        }

        name[cnt] = *p;
        p++;
        cnt++;
        if (*p == ';' || *p == '\0') {
            name[cnt] = '\0';
            priv->mtd[idx] = mtd_get_device(name);
            if (!priv->mtd[idx]) {
                pr_err("MTD %s part not found.\n", name);
                return -1;
            }
            idx++;
            cnt = 0;
        }
        if (*p == ';')
            p++;
        if (*p == '\0')
            break;
    }
    return 0;
}

int nand_is_exist()
{
    return 0;
}

s32 nand_fwc_prepare(struct fwc_info *fwc, u32 id)
{
    struct aic_spinand *flash;

    flash = spinand_probe(id);
    if (!flash) {
        printf("Failed to probe spinand flash.\n");
        return -1;
    }

    return 0;
}

/*
 * New FirmWare Component start, should prepare to burn FWC to NAND
 *  - Get FWC attributes
 *  - Parse MTD partitions or UBI Volumes the FWC going to upgrade
 *  - Erase MTD partitions(SPL is special, no need to erase here)
 */
void nand_fwc_start(struct fwc_info *fwc)
{
    struct aicupg_nand_priv *priv;
    int ret = 0;

    priv = malloc(sizeof(struct aicupg_nand_priv));
    if (!priv) {
        pr_err("Out of memory, malloc failed.\n");
        goto out;
    }
    memset(priv, 0, sizeof(struct aicupg_nand_priv));
    fwc->priv = priv;

    ret = nand_fwc_get_mtd_partitions(fwc, priv);
    if (ret) {
        pr_err("Get MTD partitions failed.\n");
        goto out;
    }

    if (aicupg_get_fwc_attr(fwc) & FWC_ATTR_DEV_UFFS) {
        fwc->block_size = priv->mtd[0]->writesize + 64;
    } else {
        fwc->block_size = priv->mtd[0]->writesize;
    }
    if (strstr(fwc->meta.name, "target.spl")) {
        ret = nand_fwc_spl_reserve_blocks(fwc);
        if (ret) {
            pr_err("Reserve blocks for SPL failed.\n");
            goto out;
        }

        ret = nand_fwc_spl_prepare(fwc);
        if (ret) {
            pr_err("Prepare to write SPL failed.\n");
            goto out;
        }
        priv->spl_flag = 1;
    }

    fwc->burn_result = 0;
    fwc->run_result = 0;

    return;
out:
    if (priv)
        free(priv);
    return;
}

s32 data_is_blank(s32 offset, u8 *buf, s32 len)
{
    int i = 0;
    u32 val = 0xff;

    for (i = 0; i < len; i++)
        val &= buf[i];

    pr_debug("offset = %0x val = 0x%x\n", offset, val);

    if (val == 0xff)
        return 1;

    return 0;
}

s32 nand_fwc_uffs_write(struct fwc_info *fwc, u8 *buf, s32 len)
{
    struct aicupg_nand_priv *priv;
    struct mtd_dev *mtd;
    unsigned long offset, erase_offset;
    int i, dolen = 0, ret = 0, pages = 0;
    int total_len = 0, data_len = 0, remain_offset = 0;
    u8 *wbuf = NULL, *pbuf = NULL;

    wbuf = malloc(ROUNDUP(len, fwc->block_size));
    if (!wbuf) {
        pr_err("malloc failed.\n");
        return 0;
    }

    priv = (struct aicupg_nand_priv *)fwc->priv;
    if (priv->remain_len > 0) {
        memcpy(wbuf, priv->remain_data, priv->remain_len);
        memcpy(wbuf + priv->remain_len, buf, len);
    } else {
        memcpy(wbuf, buf, len);
    }

    total_len = (priv->remain_len + len);
    pages = total_len / fwc->block_size;
    if (total_len % fwc->block_size) {
        priv->remain_len = total_len % fwc->block_size;
        remain_offset = total_len - priv->remain_len;
        memcpy(priv->remain_data, wbuf + remain_offset, priv->remain_len);
    } else {
        remain_offset = total_len;
        priv->remain_len = 0;
    }

    for (i = 0; i < MAX_DUPLICATED_PART; i++) {
        mtd = priv->mtd[i];
        if (!mtd)
            continue;

        data_len = pages * mtd->writesize;
        offset = priv->start_offset[i];
        if ((offset + data_len) > (mtd->size)) {
            pr_err("Not enough space to write mtd %s\n", mtd->name);
            goto out;
        }

        /* erase 1 sector when offset+len more than erased address */
        erase_offset = priv->erase_offset[i];
        while ((offset + data_len) > erase_offset) {
            if (mtd_block_isbad(mtd, erase_offset)) {
                pr_err("Erase block is bad, skip it.\n");
                priv->erase_offset[i] = erase_offset + mtd->erasesize;
                erase_offset = priv->erase_offset[i];
            }

            ret = mtd_erase(mtd, erase_offset, mtd->erasesize);
            if (ret) {
                pr_err("Erase block is bad, mark it.\n");
                ret = mtd_block_markbad(mtd, erase_offset);
                if (ret)
                    pr_err("Mark block is bad.\n");

                continue;
            }
            priv->erase_offset[i] = erase_offset + mtd->erasesize;
            erase_offset = priv->erase_offset[i];
        }

        pbuf = wbuf;
        while (dolen < data_len) {
            if (!data_is_blank(offset + dolen, pbuf + mtd->writesize, 64)) {
                if (mtd_block_isbad(mtd, ALIGN_DOWN(offset, mtd->erasesize))) {
                    pr_err(" Write block is bad, skip it.\n");
                    priv->start_offset[i] = offset + mtd->erasesize;
                    offset = priv->start_offset[i];
                }

                ret = mtd_write_oob(mtd, offset + dolen, pbuf, mtd->writesize,
                                    pbuf + mtd->writesize, 64);
                if (ret) {
                    pr_err("Write mtd oob %s block error, mark it.\n", mtd->name);
                    ret = mtd_block_markbad(mtd, erase_offset);
                    if (ret)
                        pr_err("Mark block is bad.\n");

                    continue;
                }
            }

            pbuf += fwc->block_size;
            dolen += mtd->writesize;
        }
        priv->start_offset[i] = offset + data_len;
    }

    pr_debug("%s, data len %d, trans len %d\n", __func__, len, fwc->trans_size);

    free(wbuf);

    return len;

out:
    if (wbuf)
        free(wbuf);

    return ret;
}

s32 nand_fwc_mtd_write(struct fwc_info *fwc, u8 *buf, s32 len)
{
    struct aicupg_nand_priv *priv;
    struct mtd_dev *mtd;
    unsigned long offset, erase_offset;
    int i, ret = 0;

    priv = (struct aicupg_nand_priv *)fwc->priv;
    for (i = 0; i < MAX_DUPLICATED_PART; i++) {
        mtd = priv->mtd[i];
        if (!mtd)
            continue;

        offset = priv->start_offset[i];
        if ((offset + len) > (mtd->size)) {
            pr_err("Not enough space to write mtd %s\n", mtd->name);
            return 0;
        }

        /* erase 1 sector when offset+len more than erased address */
        erase_offset = priv->erase_offset[i];
        while ((offset + len) > erase_offset) {
            if (mtd_block_isbad(mtd, erase_offset)) {
                pr_err("Erase block is bad, skip it.\n");
                priv->erase_offset[i] = erase_offset + mtd->erasesize;
                erase_offset = priv->erase_offset[i];
            }

            ret = mtd_erase(mtd, erase_offset, mtd->erasesize);
            if (ret) {
                pr_err("Erase block is bad, mark it.\n");
                ret = mtd_block_markbad(mtd, erase_offset);
                if (ret)
                    pr_err("Mark block is bad.\n");

                continue;
            }
            priv->erase_offset[i] = erase_offset + mtd->erasesize;
            erase_offset = priv->erase_offset[i];
        }

        if (mtd_block_isbad(mtd, erase_offset)) {
            pr_err(" Write block is bad, skip it.\n");
            priv->start_offset[i] = offset + mtd->erasesize;
            offset = priv->start_offset[i];
        }

        ret = mtd_write(mtd, offset, buf, len);
        if (ret) {
            pr_err("Write mtd %s block error, mark it.\n", mtd->name);
            ret = mtd_block_markbad(mtd, erase_offset);
            if (ret)
                pr_err("Mark block is bad.\n");

            continue;
        }
        priv->start_offset[i] = offset + len;
    }

    pr_debug("%s, data len %d, trans len %d\n", __func__, len, fwc->trans_size);

    return len;
}

s32 nand_fwc_data_write(struct fwc_info *fwc, u8 *buf, s32 len)
{
    struct aicupg_nand_priv *priv;

    if (aicupg_get_fwc_attr(fwc) & FWC_ATTR_DEV_UFFS) {
        len = nand_fwc_uffs_write(fwc, buf, len);
    } else if (aicupg_get_fwc_attr(fwc) & FWC_ATTR_DEV_MTD) {
        priv = (struct aicupg_nand_priv *)fwc->priv;
        if (priv->spl_flag)
            len = nand_fwc_spl_write(fwc, buf, len);
        else
            len = nand_fwc_mtd_write(fwc, buf, len);
    } else {
        /* Do nothing */
    }

    if (len < 0) {
        return -1;
    } else {
        fwc->burn_result = 0;
        fwc->run_result = 0;
        fwc->trans_size += len;
    }
    fwc->calc_partition_crc = fwc->meta.crc;

    pr_debug("%s, data len %d, trans len %d\n", __func__, len, fwc->trans_size);

    return len;
}

s32 nand_fwc_data_read(struct fwc_info *fwc, u8 *buf, s32 len)
{
    return 0;
}

void nand_fwc_data_end(struct fwc_info *fwc)
{
    struct aicupg_nand_priv *priv;

    priv = (struct aicupg_nand_priv *)fwc->priv;
    if (!priv)
        return;

    pr_debug("trans len all %d\n", fwc->trans_size);
    free(priv);
}
