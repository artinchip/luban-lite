/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_soc.h>
#include <aic_hal.h>
#include <partition_table.h>

#ifdef IMAGE_CFG_JSON_PARTS_MTD
#define NOR_MTD_PARTS IMAGE_CFG_JSON_PARTS_MTD
#else
#define NOR_MTD_PARTS ""
#endif

#define MAX_MTD_NAME 64
struct mtd_partition {
    char name[MAX_MTD_NAME];
    u32 start;
    u32 size;
    struct mtd_partition *next;
};

struct mtd_partition *local_mtd = NULL;

static inline char *aic_get_part_str(u32 spi_bus)
{
    char name[8] = { 0 };

    strcpy(name, "spi0");
    name[3] += spi_bus;
    if (strncmp(name, NOR_MTD_PARTS, 4) == 0)
        return NOR_MTD_PARTS;
    return NULL;
}

static inline struct mtd_partition *aic_part_parse(char *parts, u32 start)
{
    struct mtd_partition *part;
    int cnt = 0;
    char *p;

    part = malloc(sizeof(struct mtd_partition));
    if (!part)
        return NULL;

    memset(part, 0, sizeof(struct mtd_partition));

    part->start = start;
    p = parts;
    if (*p == '-') {
        /* All remain space */
        part->size = 0;
        p++;
    } else {
        part->size = strtoul(p, &p, 0);
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
    }
    if (*p == '@') {
        p++;
        /* Don't care offset here, just skip it */
        part->start = strtoul(p, &p, 0);
    }
    if (*p != '(') {
        printf("%s: Partition name should be next of size.\n", __FUNCTION__);
        printf("%s\n", p);
        goto err;
    }
    p++;

    cnt = 0;
    while (*p != ')') {
        if (cnt >= MAX_MTD_NAME)
            break;
        part->name[cnt++] = *p++;
    }
    p++;

    /* Skip characters until '\0', ',', ';' */
    while ((*p != '\0') && (*p != ';') && (*p != ','))
        p++;

    if (*p == ',') {
        p++;
        part->next = aic_part_parse(p, part->start + part->size);
    }

    return part;
err:
    if (part)
        free(part);
    return NULL;
}

static inline struct mtd_partition *aic_mtd_parts_parse(char *parts)
{
    char *p;

    p = parts;

    while ((*p != '\0') && (*p != ':'))
        p++;
    if (*p != ':') {
        printf("%s: mtdparts is invalid: %s\n", __FUNCTION__, parts);
        return NULL;
    }
    p++;

    local_mtd = malloc(sizeof(struct mtd_partition));
    if (!local_mtd) {
        printf("%s: local_mtd is invalid, local_mtd malloc failed\n",
               __FUNCTION__);
        return NULL;
    }

    local_mtd = aic_part_parse(p, 0);

    return local_mtd;
}

static inline struct mtd_partition *aic_mtd_get_parts_byname(char *name)
{
    struct mtd_partition *part = local_mtd;
    if (!part) {
        printf(
            "%s: local_mtd is invalid, please use aic_mtd_parts_parse init the partition table\n",
            __FUNCTION__);
        return NULL;
    }
    while (part) {
        if (strncmp(name, part->name, strlen(name)) == 0) {
            return part;
        }
        part = part->next;
    }
    return NULL;
}

static inline void aic_mtd_parts_free(struct mtd_partition *head)
{
    struct mtd_partition *p, *n;

    p = head;
    n = p;
    while (p) {
        n = p->next;
        free(p);
        p = n;
    }
}
