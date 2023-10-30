/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xiong Hao <hao.xiong@artinchip.com>
 */
#ifndef __AIC_UPG_NAND_FWC_SPL_H__
#define __AIC_UPG_NAND_FWC_SPL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>
#include "upg_internal.h"

#define SPL_NAND_IMAGE_BACKUP_NUM 4
#define PAGE_CNT_PER_BLOCK        64
#define PAGE_TABLE_MAX_ENTRY      101
#define SLICE_DEFAULT_SIZE        2048
#define PAGE_TABLE_USE_SIZE       2048
#define PAGE_MAX_SIZE             4096
#define SPL_CANDIDATE_BLOCK_NUM   18
#define SPL_INVALID_BLOCK_IDX     0xFFFFFFFF
#define SPL_INVALID_PAGE_ADDR     0xFFFFFFFF

#define ROUNDUP(a, b) ((((a)-1) / (b) + 1) * (b))

#define PAGE_SIZE_1KB 1
#define PAGE_SIZE_2KB 2
#define PAGE_SIZE_4KB 4

#define MAX_DUPLICATED_PART 4

struct aicupg_nand_priv {
    struct mtd_dev *mtd[MAX_DUPLICATED_PART];
    unsigned long start_offset[MAX_DUPLICATED_PART];
    unsigned long erase_offset[MAX_DUPLICATED_PART];
    unsigned char remain_data[PAGE_MAX_SIZE];
    unsigned int remain_len;
    int spl_flag;
};

struct nand_page_table_head {
    char magic[4]; /* AICP: AIC Page table */
    u32 entry_cnt;
    u8 page_size; /* 0: No page size info; 1: 1KB; 2: 2KB; 4: 4KB */
    u8 pad[11];   /* Padding it to fit size 20 bytes */
};

/*
 * FSBL page data store in 4 block's NAND Page. This structure is used to keep
 * all 4 NAND page address that storing FSBL page data, and keep that page data
 * checksum value(Actually is ~checksum (inverted)).
 */
struct nand_page_table_entry {
    u32 pageaddr[SPL_NAND_IMAGE_BACKUP_NUM]; /* Page address in block */
    u32 checksum; /* Page data checksum/crc32 value */
};

/*
 * Page Table
 *
 * Page table store page and data checksum information of FSBL.
 *
 * FSBL data will be splited into a lot of 2KB slices, each slice use one NAND
 * page to store it. Programmer and BROM don't care the actual NAND page size
 * is 2KB or 4KB or greater, just use the first 2KB(Not support page size less
 * than 2KB NAND).
 *
 * Every slice has 4 backup, that means 4 pages in 4 different good blocks.
 * Page table is used to keep these page address information and checksum for
 * every slice.
 *
 * Page table always store in the first page of blocks.
 */
struct nand_page_table {
    struct nand_page_table_head head;
    struct nand_page_table_entry entry[PAGE_TABLE_MAX_ENTRY];
};

s32 nand_fwc_spl_reserve_blocks(struct fwc_info *fwc);
s32 nand_fwc_spl_prepare(struct fwc_info *fwc);
s32 nand_fwc_spl_write(struct fwc_info *fwc, u8 *buf, s32 len);

#ifdef __cplusplus
}
#endif

#endif /* __AIC_UPG_NAND_FWC_SPL_H__ */
