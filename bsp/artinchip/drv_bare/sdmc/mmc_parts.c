/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xiong Hao <hao.xiong@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_soc.h>
#include <aic_hal.h>
#include <mmc.h>
#include <partition_table.h>
#include "sdmc.h"

#define MAX_PART_NAME    32
#define GPT_HEADER_SIZE  (34 * 512)

#ifdef IMAGE_CFG_JSON_PARTS_GPT
#define MMC_GPT_PARTS IMAGE_CFG_JSON_PARTS_GPT
#else
#define MMC_GPT_PARTS ""
#endif

struct aic_partition *mmc_new_partition(char *s, u64 start)
{
    struct aic_partition *part = NULL;
    int cnt = 0;
    char *p;

    part = (struct aic_partition *)malloc(sizeof(struct aic_partition));
    if (!part)
        return NULL;
    memset(part, 0, sizeof(struct aic_partition));

    p = s;
    part->start = start;
    if (*p == '-') {
        /* All remain space */
        part->size = 0;
        p++;
    } else if (*p == 's') {
        while (*p++ != ':')
            continue;
        part->size = strtoull(p, &p, 10);
    } else {
        part->size = strtoull(p, &p, 0);
    }
    if ((*p == 'k') || (*p == 'K')) {
        part->size *= 1024;
        p++;
    } else if ((*p == 'm') || (*p == 'M')) {
        part->size *= (1024 * 1024);
        p++;
    } else if ((*p == 'g') || (*p == 'G')) {
        part->size *= (1024 * 1024 * 1024);
        p++;
    }
    if (*p == '@') {
        p++;
        part->start = strtoull(p, &p, 0);
    }
    if (*p != '(') {
        pr_err("Partition name should be next of size.\n");
        goto err;
    }
    p++;

    while (*p != ')') {
        if (cnt >= MAX_PART_NAME)
            break;
        part->name[cnt++] = *p++;
    }
    p++;
    if (*p == ',') {
        p++;
        part->next = mmc_new_partition(p, part->start + part->size);
    }
    pr_info("part: %s, start "FMT_d64", size "FMT_d64"\n",
            part->name, part->start, part->size);
    return part;

err:
    if (part)
        free(part);
    return NULL;
}

void mmc_free_partition(struct aic_partition *part)
{
    struct aic_partition *next;

    if (!part)
        return;

    next = part->next;
    free(part);
    mmc_free_partition(next);
}

struct aic_partition *mmc_create_gpt_part(void)
{
    struct aic_partition *parts = NULL;

    /* Step1: Ensure MMC device is exist */

    /* Step2: Get partitions table from ENV */
    parts = mmc_new_partition(MMC_GPT_PARTS, (GPT_HEADER_SIZE));
    if (!parts)
        return NULL;

    if (parts->start != GPT_HEADER_SIZE) {
        pr_err("First partition start offset is not correct\n");
        goto out;
    }

    return parts;

out:
    if (parts)
        mmc_free_partition(parts);

    return NULL;
}

