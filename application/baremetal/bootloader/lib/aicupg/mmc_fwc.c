/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aicupg.h>
#include <sparse_format.h>
#include <partition_table.h>
#include <disk_part.h>
#include "upg_internal.h"

#define MMC_BLOCK_SIZE   512
#define SPARSE_FILLBUF_SIZE (1024 * 1024)

struct aicupg_mmc_priv {
    struct aic_sdmc *host;
    struct aic_partition *parts;
    sparse_header_t sparse_header;
    chunk_header_t chunk_header;
    unsigned char remain_data[MMC_BLOCK_SIZE];
    unsigned int remain_len;
    ulong blkstart;
    int cur_chunk;
    int cur_chunk_remain_data_sz;
    int cur_chunk_burned_data_sz;
    int is_sparse;
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

static unsigned long mmc_write(struct blk_desc *block_dev, u64 start,
                               u64 blkcnt, void *buffer)
{
    return mmc_bwrite(block_dev->priv, start, blkcnt, buffer);
}

static unsigned long mmc_read(struct blk_desc *block_dev, u64 start, u64 blkcnt,
                              const void *buffer)
{
    return mmc_bread(block_dev->priv, start, blkcnt, (void *)buffer);
}

void mmc_fwc_start(struct fwc_info *fwc)
{
    struct aicupg_mmc_priv *priv;
    struct disk_blk_ops ops;
    struct blk_desc dev_desc;
    int mmc_id = 0, ret;

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

    ops.blk_write = mmc_write;
    ops.blk_read = mmc_read;
    aic_disk_part_set_ops(&ops);
    dev_desc.blksz = MMC_BLOCK_SIZE;
    dev_desc.lba_count = priv->host->dev->card_capacity * 2;
    dev_desc.priv = priv->host;

    ret = aic_disk_write_gpt(&dev_desc, priv->parts);
    if (ret) {
        printf("Write PART table failed.\n");
    }

    mmc_block_refresh(priv->host);
    return;
out:
    if (priv->parts)
        mmc_free_partition(priv->parts);

    if (priv)
        free(priv);
}

s32 mmc_fwc_sparse_write(struct fwc_info *fwc, u8 *buf, s32 len)
{
    struct aicupg_mmc_priv *priv;
    struct aic_partition *parts;
    sparse_header_t *sheader;
    chunk_header_t *cheader;
    u8 *wbuf, *p;
    s32 clen = 0, remain, total_len;
    u32 chunk;
    u64 chunk_data_sz, chunk_blkcnt, remain_blkcnt;
    u32 total_blocks = 0, blks;
    u32 remain_blks, redund_blks, erase_group;
    u32 *fill_buf, fill_val, fill_buf_num_blks;
    int i, j, break_flag = 0;

    wbuf = malloc(ROUNDUP(len + MMC_BLOCK_SIZE, fwc->block_size));
    if (!wbuf) {
        pr_err("malloc failed.\n");
        return 0;
    }
    p = wbuf;

    priv = (struct aicupg_mmc_priv *)fwc->priv;
    if (!priv) {
        pr_err("MMC FWC get priv failed.\n");
        goto out;
    }

    if (priv->remain_len > 0) {
        memcpy(wbuf, priv->remain_data, priv->remain_len);
        memcpy(wbuf + priv->remain_len, buf, len);
    } else {
        memcpy(wbuf, buf, len);
    }

    total_len = (priv->remain_len + len);
    remain = total_len;

    parts = priv->parts;
    while (parts) {
        if (!strcmp(parts->name, fwc->meta.partition))
            break;

        parts = parts->next;
    }

    if (!parts)
        pr_err("not find %s part info.\n", fwc->meta.partition);

    sheader = &(priv->sparse_header);
    if (is_sparse_image(wbuf)) {
        priv->is_sparse = 1;
        memcpy(sheader, wbuf, sizeof(sparse_header_t));
        wbuf += sheader->file_hdr_sz;
        clen += sheader->file_hdr_sz;
        remain -= sheader->file_hdr_sz;
        if (sheader->file_hdr_sz > sizeof(sparse_header_t)) {
            wbuf += (sheader->file_hdr_sz - sizeof(sparse_header_t));
            clen += (sheader->file_hdr_sz - sizeof(sparse_header_t));
            remain -= (sheader->file_hdr_sz - sizeof(sparse_header_t));
        }
        pr_info("=== Sparse Image Header ===\n");
        pr_info("magic: 0x%x\n", sheader->magic);
        pr_info("major_version: 0x%x\n", sheader->major_version);
        pr_info("minor_version: 0x%x\n", sheader->minor_version);
        pr_info("file_hdr_sz: %d\n", sheader->file_hdr_sz);
        pr_info("chunk_hdr_sz: %d\n", sheader->chunk_hdr_sz);
        pr_info("blk_sz: %d\n", sheader->blk_sz);
        pr_info("total_blks: %d\n", sheader->total_blks);
        pr_info("total_chunks: %d\n", sheader->total_chunks);
    }

    pr_debug("Flashing Sparse Image\n");

    /* Start processing chunks */
    for (chunk = priv->cur_chunk; chunk < sheader->total_chunks; chunk++) {
        /* Read and skip over chunk header */
        cheader = (chunk_header_t *)wbuf;

        if (cheader->chunk_type != CHUNK_TYPE_RAW) {
            pr_debug("=== Chunk Header ===\n");
            pr_debug("chunk_type: 0x%x\n", cheader->chunk_type);
            pr_debug("chunk_data_sz: 0x%x\n", cheader->chunk_sz);
            pr_debug("total_size: 0x%x\n", cheader->total_sz);
        }

        if (cheader->chunk_type != CHUNK_TYPE_RAW &&
            cheader->chunk_type != CHUNK_TYPE_FILL &&
            cheader->chunk_type != CHUNK_TYPE_DONT_CARE &&
            cheader->chunk_type != CHUNK_TYPE_CRC32) {
            cheader = &(priv->chunk_header);
            chunk_data_sz = priv->cur_chunk_remain_data_sz;
        } else {
            wbuf += sheader->chunk_hdr_sz;
            clen += sheader->chunk_hdr_sz;
            remain -= sheader->chunk_hdr_sz;
            memcpy(&(priv->chunk_header), cheader, sizeof(chunk_header_t));
            if (sheader->chunk_hdr_sz > sizeof(chunk_header_t)) {
                /*
                 * Skip the remaining bytes in a header that is longer
                 * than we expected.
                 */
                wbuf += (sheader->chunk_hdr_sz - sizeof(chunk_header_t));
                clen += (sheader->chunk_hdr_sz - sizeof(chunk_header_t));
                remain -= (sheader->chunk_hdr_sz - sizeof(chunk_header_t));
            }
            chunk_data_sz = ((u64)sheader->blk_sz) * cheader->chunk_sz;
            priv->cur_chunk_remain_data_sz = chunk_data_sz;
            priv->cur_chunk_burned_data_sz = 0;
        }

        chunk_blkcnt = DIV_ROUND_UP(chunk_data_sz, MMC_BLOCK_SIZE);
        remain_blkcnt = remain / MMC_BLOCK_SIZE;
        switch (cheader->chunk_type) {
            case CHUNK_TYPE_RAW:
                if (cheader->total_sz !=
                    (sheader->chunk_hdr_sz + chunk_data_sz +
                     priv->cur_chunk_burned_data_sz)) {
                    pr_err("Bogus chunk size for chunk type Raw\n");
                    goto out;
                }

                if (priv->blkstart + chunk_blkcnt >
                    (parts->size / MMC_BLOCK_SIZE)) {
                    pr_err("Request would exceed partition size!\n");
                    goto out;
                }

                if (remain_blkcnt > chunk_blkcnt &&
                    (remain - chunk_data_sz) >= 16) {
                    blks = mmc_bwrite(priv->host,
                                      (parts->start / MMC_BLOCK_SIZE) +
                                          priv->blkstart,
                                      chunk_blkcnt, wbuf);
                    if (blks <
                        chunk_blkcnt) { /* blks might be > blkcnt (eg. NAND bad-blocks) */
                        pr_err("Write failed, block %llu[%u]\n",
                               (parts->start / MMC_BLOCK_SIZE) + priv->blkstart,
                               blks);
                        goto out;
                    }
                    remain = remain - chunk_data_sz;
                    priv->cur_chunk_remain_data_sz = 0;
                    priv->cur_chunk_burned_data_sz += chunk_data_sz;
                } else {
                    blks = mmc_bwrite(priv->host,
                                      (parts->start / MMC_BLOCK_SIZE) +
                                          priv->blkstart,
                                      remain_blkcnt, wbuf);
                    if (blks <
                        remain_blkcnt) { /* blks might be > blkcnt (eg. NAND bad-blocks) */
                        pr_err("Write failed, block %llu[%u]\n",
                               (parts->start / MMC_BLOCK_SIZE) + priv->blkstart,
                               blks);
                        goto out;
                    }
                    priv->cur_chunk_remain_data_sz -=
                        remain_blkcnt * MMC_BLOCK_SIZE;
                    priv->cur_chunk_burned_data_sz +=
                        remain_blkcnt * MMC_BLOCK_SIZE;
                    remain = remain % MMC_BLOCK_SIZE;
                }

                priv->blkstart += blks;
                total_blocks += blks;
                wbuf += blks * MMC_BLOCK_SIZE;
                clen += blks * MMC_BLOCK_SIZE;
                if ((priv->cur_chunk_remain_data_sz > 0 &&
                     (remain > 0 && remain < MMC_BLOCK_SIZE)) ||
                    remain < sizeof(chunk_header_t))
                    break_flag = 1;

                break;

            case CHUNK_TYPE_FILL:
                if (cheader->total_sz !=
                    (sheader->chunk_hdr_sz + sizeof(uint32_t))) {
                    pr_err("Bogus chunk size for chunk type FILL\n");
                    goto out;
                }

                fill_buf = (u32 *)aicos_malloc_align(
                    0, ROUNDUP(SPARSE_FILLBUF_SIZE, CACHE_LINE_SIZE),
                    CACHE_LINE_SIZE);
                if (!fill_buf) {
                    pr_err("Malloc failed for: CHUNK_TYPE_FILL\n");
                    goto out;
                }

                fill_val = *(uint32_t *)wbuf;
                wbuf = (u8 *)wbuf + sizeof(u32);
                clen += sizeof(uint32_t);
                remain -= sizeof(uint32_t);
                if (remain < sizeof(chunk_header_t))
                    break_flag = 1;

                if (priv->blkstart + chunk_blkcnt >
                    (parts->size / MMC_BLOCK_SIZE)) {
                    pr_err("Request would exceed partition size!\n");
                    goto out;
                }

                for (i = 0; i < (SPARSE_FILLBUF_SIZE / sizeof(fill_val)); i++)
                    fill_buf[i] = fill_val;

                remain_blks =
                    ROUNDUP((parts->start / MMC_BLOCK_SIZE) + priv->blkstart,
                            0x400) -
                    ((parts->start / MMC_BLOCK_SIZE) + priv->blkstart);
                if (chunk_blkcnt >= (remain_blks + 0x400) &&
                    fill_val == 0x0) { // 512K
                    blks = mmc_bwrite(priv->host,
                                      (parts->start / MMC_BLOCK_SIZE) +
                                          priv->blkstart,
                                      remain_blks, (u8 *)fill_buf);
                    if (blks <
                        remain_blks) { /* blks might be > j (eg. NAND bad-blocks) */
                        pr_err("Write failed, block %llu[%d]\n",
                               (parts->start / MMC_BLOCK_SIZE) + priv->blkstart,
                               remain_blks);
                        free(fill_buf);
                        goto out;
                    }
                    priv->blkstart += blks;

                    erase_group = (chunk_blkcnt - remain_blks) / 0x400;
                    blks = mmc_berase(priv->host,
                                      (parts->start / MMC_BLOCK_SIZE) +
                                          priv->blkstart,
                                      erase_group * 0x400);
                    if (blks !=
                        (erase_group *
                         0x400)) { /* blks might be > j (eg. NAND bad-blocks) */
                        pr_err("Erase failed, block %llu[%d]\n",
                               (parts->start / MMC_BLOCK_SIZE) + priv->blkstart,
                               erase_group * 0x400);
                        free(fill_buf);
                        goto out;
                    }
                    priv->blkstart += blks;

                    redund_blks =
                        chunk_blkcnt - remain_blks - (erase_group * 0x400);
                    blks = mmc_bwrite(priv->host,
                                      (parts->start / MMC_BLOCK_SIZE) +
                                          priv->blkstart,
                                      redund_blks, (u8 *)fill_buf);
                    if (blks <
                        redund_blks) { /* blks might be > j (eg. NAND bad-blocks) */
                        pr_err("Write failed, block %llu[%d]\n",
                               (parts->start / MMC_BLOCK_SIZE) + priv->blkstart,
                               redund_blks);
                        free(fill_buf);
                        goto out;
                    }
                    priv->blkstart += blks;
                } else {
                    fill_buf_num_blks = SPARSE_FILLBUF_SIZE / MMC_BLOCK_SIZE;
                    for (i = 0; i < chunk_blkcnt;) {
                        j = chunk_blkcnt - i;
                        if (j > fill_buf_num_blks)
                            j = fill_buf_num_blks;

                        blks = mmc_bwrite(priv->host,
                                          (parts->start / MMC_BLOCK_SIZE) +
                                              priv->blkstart,
                                          j, (u8 *)fill_buf);
                        if (blks <
                            j) { /* blks might be > j (eg. NAND bad-blocks) */
                            pr_err("Write failed, block %llu[%d]\n",
                                   (parts->start / MMC_BLOCK_SIZE) +
                                       priv->blkstart,
                                   j);
                            free(fill_buf);
                            goto out;
                        }
                        priv->blkstart += blks;
                        i += j;
                    }
                }
                total_blocks += DIV_ROUND_UP(chunk_data_sz, sheader->blk_sz);

                free(fill_buf);
                break;

            case CHUNK_TYPE_DONT_CARE:
                priv->blkstart += chunk_blkcnt;
                total_blocks += cheader->chunk_sz;
                break;

            case CHUNK_TYPE_CRC32:
                if (cheader->total_sz != sheader->chunk_hdr_sz) {
                    pr_err("Bogus chunk size for chunk type Dont Care\n");
                    goto out;
                }
                total_blocks += cheader->chunk_sz;
                wbuf += chunk_data_sz;
                clen += chunk_data_sz;
                remain -= chunk_data_sz;
                break;

            default:
                printf("Unknown chunk type: %x\n", cheader->chunk_type);
                cheader = &(priv->chunk_header);
        }

        if (break_flag)
            break;
    }

    priv->remain_len = remain;
    if (priv->remain_len) {
        priv->cur_chunk = chunk;
        memcpy(priv->remain_data, wbuf, priv->remain_len);
    }

    fwc->trans_size += clen;

    pr_debug("%s, data len %d, trans len %d\n", __func__, len, fwc->trans_size);

out:
    if (p)
        free(p);
    return len;
}

s32 mmc_fwc_raw_write(struct fwc_info *fwc, u8 *buf, s32 len)
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

    if ((blkstart + blkcnt) > (parts->size / MMC_BLOCK_SIZE)) {
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

s32 mmc_fwc_data_write(struct fwc_info *fwc, u8 *buf, s32 len)
{
    struct aicupg_mmc_priv *priv;

    priv = (struct aicupg_mmc_priv *)fwc->priv;
    if (!is_sparse_image(buf) && !priv->is_sparse) {
        pr_debug("Not a sparse image\n");
        return mmc_fwc_raw_write(fwc, buf, len);
    } else {
        pr_debug("A sparse image\n");
        return mmc_fwc_sparse_write(fwc, buf, len);
    }
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
