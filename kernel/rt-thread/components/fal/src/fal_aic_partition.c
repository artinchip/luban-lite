/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Dehuang Wu <dehuang.wu@artinchip.com>
 */

#include <fal.h>
#include <aic_common.h>
#include <aic_image.h>
#include <private_param.h>
#include <aic_partition.h>
#include <partition_table.h>

void dump_hex(char *msg, unsigned char *data, int len)
{
    unsigned long i;

    printf("%s, 0x%lx:\n", msg, (unsigned long)data);
    i = 0;
    printf("%04lx: ", i);
    for (i = 0; i < len; i++) {
        if (i && (i % 16 == 0))
            printf("\n%04lx: ", i);
        printf("%02x ", data[i]);
    }
    printf("\n");
}

int aic_get_fal_partition_table(const char *dev_name,
                                struct fal_partition **table)
{
    const struct fal_flash_dev *flash_dev = NULL;
    struct aic_image_header head;
    struct aic_partition *list, *part;
    uint8_t head_buf[256], *res;
    char *part_str;
    struct fal_partition *fal;
    int i, cnt = 0;

    flash_dev = fal_flash_device_find(dev_name);
    if (flash_dev == NULL)
    {
        log_e("Initialize failed! Flash device (%s) NOT found.", dev_name);
        return -1;
    }

    if (flash_dev->ops.read(0, head_buf, sizeof(head_buf)) <= 0) {
        log_e("Failed to read aic image head.");
        return -1;
    }
    memcpy(&head, head_buf, sizeof(head));
    if (head.magic != AIC_IMAGE_MAGIC) {
        log_e("aic image head verify failure.");
        return -1;
    }
    if (head.private_data_offset) {
        res = malloc(head.private_data_len);
        if (res == NULL) {
            log_e("Failed to malloc resource buffer.\n");
            return -RT_ENOMEM;
        }
        if (flash_dev->ops.read(head.private_data_offset, res,
                                head.private_data_len) <= 0) {
            log_e("Failed to read aic image head.");
            return -1;
        }
        part_str = private_get_partition_string(res);
        if (part_str == NULL)
            part_str = IMAGE_CFG_JSON_PARTS_MTD;
        list = aic_part_mtd_parse((void *)part_str);
        part = list;
        while (part) {
            part = part->next;
            cnt++;
        }
        fal = malloc(sizeof(struct fal_partition) * cnt);
        *table = fal;
        part = list;
        i = 0;
        while (part) {
            fal[i].magic_word = FAL_PART_MAGIC_WORD;
            strcpy(fal[i].name, part->name);
            strcpy(fal[i].flash_name, dev_name);
            fal[i].offset = part->start;
            fal[i].len = part->size;
            part = part->next;
            i++;
        }
        free(res);
        aic_part_free(list);
    }
    return cnt;
}
