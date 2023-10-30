/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <aicupg.h>
#include <mmc.h>
#include <partition_table.h>
#include "upg_internal.h"

#define MMC_BLOCK_SIZE   512

struct aicupg_mmc_priv {
    struct aic_sdmc *host;
    struct aic_partition *parts;
};

int mmc_is_exist()
{
    return 0;
}

s32 mmc_fwc_prepare(struct fwc_info *fwc, u32 mmc_id)
{
    int ret = 0;

    ret = mmc_init(mmc_id);
    if (ret) {
        pr_err("sdmc %d init failed.\n", mmc_id);
        return ret;
    }

    return ret;
}

void mmc_fwc_start(struct fwc_info *fwc)
{
    struct aicupg_mmc_priv *priv;
    int mmc_id = 0;

    mmc_id = get_current_device_id();

    priv = malloc(sizeof(struct aicupg_mmc_priv));
    if (!priv) {
        pr_err("malloc mmc priv failed.\n");
        goto out;
    }

    priv->host = find_mmc_dev_by_index(mmc_id);
    if (priv->host == NULL) {
        pr_err("find mmc dev failed.\n");
        goto out;
    }

    priv->parts = mmc_create_gpt_part();
    if (!priv->parts) {
        pr_err("sdmc %d create gpt part failed.\n", mmc_id);
        goto out;
    }
    fwc->priv = priv;
    fwc->block_size = MMC_BLOCK_SIZE;
    fwc->burn_result = 0;
    fwc->run_result = 0;

    return;
out:
    if (priv->parts)
        mmc_free_partition(priv->parts);

    if (priv)
        free(priv);
}

s32 mmc_fwc_data_write(struct fwc_info *fwc, u8 *buf, s32 len)
{
    struct aicupg_mmc_priv *priv;
    struct aic_partition *parts = NULL;
    u64 blkstart, blkcnt;
    long n;
    u32 clen = 0;

    priv = (struct aicupg_mmc_priv *)fwc->priv;
    if (!priv) {
        pr_err("MMC FWC get priv failed.\n");
        goto out;
    }

    parts = priv->parts;
    while (parts) {
        if (!strcmp(parts->name, fwc->meta.partition))
            break;

        parts = parts->next;
    }

    if (!parts)
        pr_err("not find %s part info.\n", fwc->meta.partition);

    blkstart = fwc->trans_size / MMC_BLOCK_SIZE;
    blkcnt = len / MMC_BLOCK_SIZE;
    if (len % MMC_BLOCK_SIZE)
        blkcnt++;

    if ((blkstart + blkcnt) > parts->size) {
        pr_err("Data size exceed the partition size.\n");
        goto out;
    }

    clen = len;
    n = mmc_bwrite(priv->host, (parts->start / MMC_BLOCK_SIZE) + blkstart, blkcnt, buf);
    if (n != blkcnt) {
        pr_err("Error, write to partition %s failed.\n", fwc->meta.partition);
        fwc->burn_result += 1;
        clen = n * MMC_BLOCK_SIZE;
        fwc->trans_size += clen;
    }

    fwc->trans_size += clen;

    pr_debug("%s, data len %d, trans len %d\n", __func__, len,
            fwc->trans_size);

out:
    return clen;
}

s32 mmc_fwc_data_read(struct fwc_info *fwc, u8 *buf, s32 len)
{
    return 0;
}

void mmc_fwc_data_end(struct fwc_info *fwc)
{
    struct aicupg_mmc_priv *priv;

    priv = (struct aicupg_mmc_priv *)fwc->priv;
    if (priv->parts)
        mmc_free_partition(priv->parts);

    if (fwc->priv) {
        free(priv);
        fwc->priv = 0;
    }
}
