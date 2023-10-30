/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SPI_NAND_PARTS_H__
#define __SPI_NAND_PARTS_H__

#define MAX_MTD_NAME 32
struct mtd_partition {
	char name[MAX_MTD_NAME];
	uint32_t start;
	uint32_t size;
	struct mtd_partition *next;
};

struct mtd_partition *mtd_parts_parse(char *parts);
void mtd_parts_free(struct mtd_partition *head);
#endif /* __SPI_NAND_PARTS_H__ */
