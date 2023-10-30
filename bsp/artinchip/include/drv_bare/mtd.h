/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wudehuang <dehuang.wu@artinchip.com>
 */

#ifndef __BL_MTD_H_
#define __BL_MTD_H_

#include <aic_common.h>
#include <aic_list.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MTD_NAME 64
struct mtd_partition {
    char name[MAX_MTD_NAME];
    u32 start;
    u32 size;
    struct mtd_partition *next;
};

struct mtd_dev;
struct mtd_drv_ops {
    int (*read)(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len);
    int (*erase)(struct mtd_dev *mtd, u32 offset, u32 len);
    int (*write)(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len);
    int (*read_oob)(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len,
                    u8 *spare_data, u32 spare_len);
    int (*write_oob)(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len,
                     u8 *spare_data, u32 spare_len);
    int (*block_isbad)(struct mtd_dev *mtd, u32 offset);
    int (*block_markbad)(struct mtd_dev *mtd, u32 offset);
    int (*cont_read)(struct mtd_dev *mtd, u32 offset, u8 *data, u32 size);
};

struct mtd_dev {
    struct list_head list;
    char *name;
    unsigned long start;
    unsigned long size;
    unsigned long erasesize;
    unsigned long writesize;
    unsigned long oobsize;
    struct mtd_drv_ops ops;
    void *priv;
};

int mtd_probe(void);
int mtd_add_device(struct mtd_dev *mtd);
int mtd_del_device(struct mtd_dev *mtd);
u32 mtd_get_device_count(void);
struct mtd_dev *mtd_get_device_by_id(u32 id);
struct mtd_dev *mtd_get_device(const char *name);
int mtd_read(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len);
int mtd_erase(struct mtd_dev *mtd, u32 offset, u32 len);
int mtd_write(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len);
int mtd_read_oob(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len,
                 u8 *spare_data, u32 spare_len);
int mtd_write_oob(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len,
                  u8 *spare_data, u32 spare_len);
int mtd_block_isbad(struct mtd_dev *mtd, u32 offset);
int mtd_block_markbad(struct mtd_dev *mtd, u32 offset);
struct mtd_partition *mtd_parts_parse(char *parts);
void mtd_parts_free(struct mtd_partition *head);
int mtd_contread(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len);

#ifdef __cplusplus
}
#endif

#endif /* __BL_MTD_H_ */
