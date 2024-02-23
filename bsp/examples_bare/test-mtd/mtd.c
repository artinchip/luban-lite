/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_time.h>
#include <aic_errno.h>
#include <hexdump.h>
#include <mtd.h>
#include <rtconfig.h>

#ifdef AIC_SPINAND_DRV
#define MTD_HELP                                     \
    "mtd read write command:\n"                      \
    "  mtd list\n"                                   \
    "  mtd dump  <part> <offset> <size>\n"           \
    "  mtd read  <part> <addr> <offset> <size>\n"    \
    "  mtd erase <part> <offset> <size>\n"           \
    "  mtd write <part> <addr> <offset> <size>\n"    \
    "  mtd oobdump  <part> <offset>\n"               \
    "  mtd oobread  <part> <addr> <offset>\n"        \
    "  mtd oobwrite <part> <addr> <offset>\n"        \
    "  mtd contread <part> <addr> <offset> <size>\n" \
    "e.g.:\n"                                        \
    "  mtd read spl 0x40000000 0 0x1000\n"
#else
#define MTD_HELP                                     \
    "mtd read write command:\n"                      \
    "  mtd list\n"                                   \
    "  mtd dump  <part> <offset> <size>\n"           \
    "  mtd read  <part> <addr> <offset> <size>\n"    \
    "  mtd erase <part> <offset> <size>\n"           \
    "  mtd write <part> <addr> <offset> <size>\n"    \
    "e.g.:\n"                                        \
    "  mtd read spl 0x40000000 0 0x1000\n"
#endif

static void mtd_help(void)
{
    puts(MTD_HELP);
}

static int do_mtd_list(int argc, char *argv[])
{
    u32 cnt, i;
    struct mtd_dev *mtd;

    if (mtd_probe()) {
        printf("Failed to probe mtd device.\n");
        return -1;
    }
    cnt = mtd_get_device_count();

    if (cnt == 0) {
        printf("No mtd devices.\n");
        return 0;
    }

    printf("MTD devices:\n");
    for (i = 0; i < cnt; i++) {
        mtd = mtd_get_device_by_id(i);
        if (i == 0)
            printf("%-24s 0x%08lx ~ 0x%08lx\n", mtd->name, mtd->start,
                   mtd->start + mtd->size);
        else
            printf("    %-20s 0x%08lx ~ 0x%08lx\n", mtd->name, mtd->start,
                   mtd->start + mtd->size);
    }
    return 0;
}

static int do_mtd_dump(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data = NULL;
    char *name;
    unsigned long offset, size;

    if (argc < 4) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    offset = strtol(argv[2], NULL, 0);
    size = strtol(argv[3], NULL, 0);

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }
    data = aicos_malloc_align(0, size, CACHE_LINE_SIZE);
    if (data == NULL) {
        printf("Out of memory.\n");
        return -1;
    }

    memset(data, 0, size);

    err = mtd_read(mtd, offset, data, size);
    if (!err)
        hexdump((void *)data, size, 1);
    aicos_free_align(0, data);
    return err;
}

static int do_mtd_read(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data;
    char *name;
    unsigned long addr, offset, size;
    u64 start_us;

    if (argc < 5) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    addr = strtol(argv[2], NULL, 0);
    offset = strtol(argv[3], NULL, 0);
    size = strtol(argv[4], NULL, 0);

    data = (u8 *)addr;
    if (data == NULL) {
        printf("Dest address is not correct.\n");
        return -1;
    }

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }
    start_us = aic_get_time_us();
    err = mtd_read(mtd, offset, data, size);
    show_speed("mtd_read speed", size, aic_get_time_us() - start_us);
    return err;
}

static int do_mtd_erase(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    char *name;
    unsigned long offset, size;

    if (argc < 4) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    offset = strtol(argv[2], NULL, 0);
    size = strtol(argv[3], NULL, 0);

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }
    err = mtd_erase(mtd, offset, size);
    return err;
}

static int do_mtd_write(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data;
    char *name;
    unsigned long addr, offset, size;
    u64 start_us;

    if (argc < 4) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    addr = strtol(argv[2], NULL, 0);
    offset = strtol(argv[3], NULL, 0);
    size = strtol(argv[4], NULL, 0);

    data = (u8 *)addr;
    if (data == NULL) {
        printf("Source address is not correct.\n");
        return -1;
    }

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }

    start_us = aic_get_time_us();
    err = mtd_write(mtd, offset, data, size);
    show_speed("mtd_write speed", size, aic_get_time_us() - start_us);

    return err;
}

#ifdef AIC_SPINAND_DRV
static int do_mtd_oobdump(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data;
    char *name;
    unsigned long offset, size;

    if (argc < 3) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    offset = strtol(argv[2], NULL, 0);

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }

    size = mtd->oobsize + mtd->writesize;

    data = malloc(size);
    if (data == NULL) {
        printf("Out of memory.\n");
        return -1;
    }

    memset(data, 0, size);

    err = mtd_read_oob(mtd, offset, data, mtd->writesize, data + mtd->writesize,
                       mtd->oobsize);
    if (!err)
        hexdump((void *)data, size, 1);
    free(data);
    return err;
}

#ifdef AIC_SPINAND_CONT_READ
static int do_mtd_contread(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data;
    char *name;
    unsigned long addr, offset, size;
    u32 start_us;

    if (argc < 5) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    addr = strtol(argv[2], NULL, 0);
    offset = strtol(argv[3], NULL, 0);
    size = strtol(argv[4], NULL, 0);

    data = (u8 *)addr;
    if (data == NULL) {
        printf("Dest address is not correct.\n");
        return -1;
    }

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }
    start_us = aic_get_time_us();
    err = mtd_contread(mtd, offset, data, size);
    show_speed("mtd_contread speed", size, aic_get_time_us() - start_us);
    if (!err)
        hexdump((void *)data, size, 1);
    return err;
}
#endif

static int do_mtd_oobread(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data;
    char *name;
    unsigned long addr, offset;

    if (argc < 4) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    addr = strtol(argv[2], NULL, 0);
    offset = strtol(argv[3], NULL, 0);

    data = (u8 *)addr;
    if (data == NULL) {
        printf("Dest address is not correct.\n");
        return -1;
    }

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }
    err = mtd_read_oob(mtd, offset, data, mtd->writesize, data + mtd->writesize,
                       mtd->oobsize);
    return err;
}

static int do_mtd_oobwrite(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data;
    char *name;
    unsigned long addr, offset;

    if (argc < 4) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    addr = strtol(argv[2], NULL, 0);
    offset = strtol(argv[3], NULL, 0);

    data = (u8 *)addr;
    if (data == NULL) {
        printf("Source address is not correct.\n");
        return -1;
    }

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }
    err = mtd_write_oob(mtd, offset, data, mtd->writesize,
                        data + mtd->writesize, mtd->oobsize);
    return err;
}
#endif

static int do_mtd(int argc, char *argv[])
{
    if (argc < 2) {
        mtd_help();
        return 0;
    }

    if (!strncmp(argv[1], "list", 4))
        return do_mtd_list(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "dump", 4))
        return do_mtd_dump(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "read", 4))
        return do_mtd_read(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "write", 5))
        return do_mtd_write(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "erase", 5))
        return do_mtd_erase(argc - 1, &argv[1]);
#ifdef AIC_SPINAND_DRV
    else if (!strncmp(argv[1], "oobdump", 7))
        return do_mtd_oobdump(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "oobread", 7))
        return do_mtd_oobread(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "oobwrite", 8))
        return do_mtd_oobwrite(argc - 1, &argv[1]);
#ifdef AIC_SPINAND_CONT_READ
    else if (!strncmp(argv[1], "contread", 8))
        return do_mtd_contread(argc - 1, &argv[1]);
#endif
#endif
    mtd_help();
    return 0;
}

CONSOLE_CMD(mtd, do_mtd, "MTD R/W command.");
