/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SPI_NAND_PARTS_H__
#define __SPI_NAND_PARTS_H__

#define MAX_MTD_NAME 32

#define MAX_NAND_NAME 32
struct nftl_volume {
    char name[MAX_NAND_NAME];
    int vol_type;
    int size;
    struct nftl_volume *next;
};

struct nftl_mtd {
    char name[MAX_NAND_NAME];
    struct nftl_mtd *next;
    struct nftl_volume *vols;
};

enum part_attr {
    PART_ATTR_MTD = 0,
    PART_ATTR_NFTL,
};

struct mtd_partition {
    char name[MAX_MTD_NAME];
    uint32_t start;
    uint32_t size;
    struct mtd_partition *next;
    enum part_attr attr;
};

struct mtd_partition *mtd_parts_parse(char *parts);
void mtd_parts_free(struct mtd_partition *head);

struct nftl_mtd *build_nftl_list(char *nftlvols);
void free_nftl_list(struct nftl_mtd *nftl);
uint8_t partition_nftl_is_exist(char *mtd_name, struct nftl_mtd *nftl_list);
#endif /* __SPI_NAND_PARTS_H__ */
