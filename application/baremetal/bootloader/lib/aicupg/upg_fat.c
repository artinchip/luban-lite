/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Jianfeng Li <jianfeng.li@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <upg_internal.h>
#include <aic_core.h>
#include <aicupg.h>
#include <fatfs.h>

#define FRAME_LIST_SIZE 4096
static s32 media_device_write(char *image_name, struct fwc_meta *pmeta)
{
    struct fwc_info *fwc;
    int offset, write_once_size, len, remaining_size;
    u8 *buf;
    s32 ret;
    ulong actread, total_len = 0;
    u64 start_us;

    fwc = NULL;
    buf = NULL;
    fwc = aicos_malloc_align(0, sizeof(struct fwc_info), FRAME_LIST_SIZE);
    if (!fwc) {
        pr_err("Error: malloc fwc failed.\n");
        ret = -1;
        goto err;
    }
    memset((void *)fwc, 0, sizeof(struct fwc_info));

    printf("Firmware component: %s\n", pmeta->name);
    printf("    partition: %s programming ...\n", pmeta->partition);
    /*config fwc */
    fwc_meta_config(fwc, pmeta);

    /*start write data*/
    start_us = aic_get_time_us();
    media_data_write_start(fwc);
    /*config write size once*/
    write_once_size = DATA_WRITE_ONCE_SIZE;
    if (write_once_size % fwc->block_size)
        write_once_size = (write_once_size / fwc->block_size) * fwc->block_size;

    /*malloc buf memory*/
    buf = aicos_malloc_align(0, write_once_size, FRAME_LIST_SIZE);
    if (!buf) {
        pr_err("Error: malloc buf failed.\n");
        ret = -1;
        goto err;
    }
    memset((void *)buf, 0, write_once_size);

    offset = 0;
    while (offset < pmeta->size) {
        remaining_size = pmeta->size - offset;
        len = min(remaining_size, write_once_size);
        if (len % fwc->block_size)
            len = ((len / fwc->block_size) + 1) * fwc->block_size;

        ret = aic_fat_read_file(image_name, (void *)buf,
                                pmeta->offset + offset, len, &actread);
        if (actread != len && actread != remaining_size) {
            pr_err("Error:read file failed!\n");
            goto err;
        }
        /*write data to media*/
        ret = media_data_write(fwc, buf, len);
        if (ret == 0) {
            pr_err("Error: media write failed!..\n");
            goto err;
        }
        total_len += ret;
        offset += len;
    }

    /*write data end*/
    media_data_write_end(fwc);
    start_us = aic_get_time_us() - start_us;
    /*check data */
    printf("    Partition: %s programming done.\n", pmeta->partition);
    pr_info("    Used time: %lld.%lld sec, Speed: %lld.%lld MB/s.\n",
            start_us / 1000000, start_us / 1000 % 1000,
            (total_len * 1000000 / start_us) / 1024 / 1024,
            (total_len * 1000000 / start_us) / 1024 % 1024);

    aicos_free_align(0, buf);
    aicos_free_align(0, fwc);

    return total_len;
err:
    if (buf)
        aicos_free_align(0, buf);
    if (fwc)
        aicos_free_align(0, fwc);
    return 0;
}

/*fat upgrade function*/
s32 aicupg_fat_write(char *image_name, char *protection,
                     struct image_header_upgrade *header)
{
    struct fwc_meta *p;
    struct fwc_meta *pmeta;
    int i, cnt, ret;
    u32 start_us;
    ulong actread, write_len = 0;
    u64 total_len = 0;

    pmeta = NULL;
    pmeta = (struct fwc_meta *)aicos_malloc_align(0, header->meta_size, FRAME_LIST_SIZE);
    if (!pmeta) {
        pr_err("Error: malloc for meta failed.\n");
        ret = -1;
        goto err;
    }
    memset((void *)pmeta, 0, header->meta_size);

    start_us = aic_get_time_us();
    /*read meta info*/
    ret = aic_fat_read_file(image_name, (void *)pmeta,
                            header->meta_offset, header->meta_size, &actread);
    if (actread != header->meta_size) {
        pr_err("Error:read file failed!\n");
        goto err;
    }

    /*prepare device*/
    ret = media_device_prepare(NULL, header);
    if (ret != 0) {
        pr_err("Error:prepare device failed!\n");
        goto err;
    }

    p = pmeta;
    cnt = header->meta_size / sizeof(struct fwc_meta);
    for (i = 0; i < cnt; i++) {
        if (!strcmp(p->partition, "") || strstr(protection, p->partition)) {
            p++;
            continue;
        }

        ret = media_device_write(image_name, p);
        if (ret == 0) {
            pr_err("media device write data failed!\n");
            goto err;
        }

        p++;
        write_len += ret;
    }

    total_len = write_len;
    start_us = aic_get_time_us() - start_us;
    printf("All firmaware components programming done.\n");
    printf("    Used time: %u.%u sec, Speed: %lu.%lu MB/s.\n",
           start_us / 1000000, start_us / 1000 % 1000,
           (ulong)((total_len * 1000000 / start_us) / 1024 / 1024),
           (ulong)((total_len * 1000000 / start_us) / 1024 % 1024));

    aicos_free_align(0, pmeta);
    return write_len;
err:
    if (pmeta)
        aicos_free_align(0, pmeta);
    return 0;
}
