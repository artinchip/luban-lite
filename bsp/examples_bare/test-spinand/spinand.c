/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xuan.Wen <xuan.wen@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <spinand_port.h>
#include <hexdump.h>

#define SPINAND_HELP                       \
    "spinand read write command:\n"        \
    "  spinand init  <spi bus id>\n"       \
    "  spinand read <addr offset size>\n"  \
    "  spinand dump <offset size>\n"       \
    "  spinand oobdump <offset>\n"         \
    "  spinand write <addr offset size>\n" \
    "  spinand erase <offset size>\n"      \
    "  spinand bad <offset size>\n"        \
    "  spinand contread <offset size>\n"   \
    "  spinand read 0x40000000 0 0x20000\n"

struct aic_spinand *g_spinand_flash;

static void spinand_help(void)
{
    puts(SPINAND_HELP);
}

static int do_nand_id(int argc, char *argv[])
{
    int ret = 0;
    u32 id = 0;

    ret = spinand_read_id_op(g_spinand_flash, (u8 *)&id);

    printf("Id: 0x%08x\n", id);
    return ret;
}

CONSOLE_CMD(nid, do_nand_id, "Display NAND manu ID. need init first");

static int do_spinand_dump(int argc, char *argv[])
{
    int8_t err;
    uint8_t *data;
    unsigned long offset, size;

    if (argc < 3) {
        spinand_help();
        return 0;
    }

    offset = strtol(argv[1], NULL, 0);
    size = strtol(argv[2], NULL, 0);

    data = malloc(size);
    if (data == NULL) {
        printf("Out of memory.\n");
        return -1;
    }

    memset(data, 0, size);

    err = spinand_read(g_spinand_flash, (uint8_t *)data, offset, size);
    hexdump((void *)data, size, 1);

    free(data);
    return err;
}

static int do_spinand_read(int argc, char *argv[])
{
    int8_t err;
    unsigned long addr, offset, size;

    if (argc < 4) {
        spinand_help();
        return 0;
    }

    addr = strtol(argv[1], NULL, 0);
    offset = strtol(argv[2], NULL, 0);
    size = strtol(argv[3], NULL, 0);

    err = spinand_read(g_spinand_flash, (uint8_t *)addr, offset, size);
    //hexdump((void *)addr, size, 1);
    return err;
}

#ifdef AIC_SPINAND_CONT_READ
static int do_spinand_contread(int argc, char *argv[])
{
    int8_t err;
    unsigned long offset, size, page;
    uint8_t *data;
    uint32_t start_us;

    if (argc < 3) {
        spinand_help();
        return 0;
    }

    offset = strtol(argv[1], NULL, 0);
    size = strtol(argv[2], NULL, 0);

    data = aicos_malloc_align(0, size, CACHE_LINE_SIZE);
    if (data == NULL) {
        printf("Out of memory.\n");
        return -1;
    }

    page = offset / g_spinand_flash->info->page_size;

    start_us = aic_get_time_us();
    err = spinand_continuous_read(g_spinand_flash, page, (uint8_t *)data, size);
    start_us = aic_get_time_us() - start_us;
    printf("start_us = %ud size = %lu\n", start_us, size);

    hexdump((void *)data, size, 1);

    /* release memory */
    if (data)
        aicos_free_align(0, data);

    return err;
}
#endif

static int do_spinand_oobdump(int argc, char *argv[])
{
    int8_t err;
    unsigned long offset, size;
    uint8_t *data;

    if (argc < 2) {
        spinand_help();
        return 0;
    }

    offset = strtol(argv[1], NULL, 0);

    data = malloc(g_spinand_flash->info->page_size +
                  g_spinand_flash->info->oob_size);
    if (data == NULL) {
        printf("Out of memory.\n");
        return -1;
    }

    size = g_spinand_flash->info->page_size + g_spinand_flash->info->oob_size;

    err =
        spinand_read_page(g_spinand_flash,
                          offset / g_spinand_flash->info->page_size,
                          (uint8_t *)data, g_spinand_flash->info->page_size,
                          (uint8_t *)(data + g_spinand_flash->info->page_size),
                          g_spinand_flash->info->oob_size);
    hexdump((void *)data, size, 1);
    return err;
}

static int do_spinand_erase(int argc, char *argv[])
{
    int8_t err;
    unsigned long offset, size;

    if (argc < 3) {
        spinand_help();
        return 0;
    }

    offset = strtol(argv[1], NULL, 0);
    size = strtol(argv[2], NULL, 0);

    err = spinand_erase(g_spinand_flash, offset, size);
    return err;
}

static int do_spinand_write(int argc, char *argv[])
{
    int8_t err;
    unsigned long addr, offset, size;

    if (argc < 4) {
        spinand_help();
        return 0;
    }

    addr = strtol(argv[1], NULL, 0);
    offset = strtol(argv[2], NULL, 0);
    size = strtol(argv[3], NULL, 0);

    err = spinand_write(g_spinand_flash, (uint8_t *)addr, offset, size);
    return err;
}

static int do_spinand_bad(int argc, char **argv)
{
    int offset, size;
    u16 blk;
    u32 blk_size;

    if (argc < 3) {
        spinand_help();
        return 0;
    }

    offset = strtol(argv[1], NULL, 0);
    size = strtol(argv[2], NULL, 0);

    blk_size = g_spinand_flash->info->page_size *
               g_spinand_flash->info->pages_per_eraseblock;

    /* Search for the first good block after the given offset */
    while (size > 0) {
        blk = offset / blk_size;
        printf("offset = 0x%x\n", offset);
        if (spinand_block_isbad(g_spinand_flash, blk))
            printf("find a bad block, off adjust to the next block\n");

        offset += blk_size;
        blk = offset / blk_size;
        size -= blk_size;
    }

    return 0;
}

static int do_spinand_init(int argc, char *argv[])
{
    unsigned long spi_bus;

    if (argc < 2) {
        spinand_help();
        return -1;
    }

    spi_bus = strtol(argv[1], NULL, 0);

    g_spinand_flash = spinand_probe(spi_bus);
    if (g_spinand_flash == NULL) {
        printf("Failed to probe spinand flash.\n");
        return -1;
    }

    return 0;
}

/*
 * spinand init 0
 * spinand dump offset size
 * spinand read addr offset size
 * spinand write addr offset size
 */
static int do_spinand(int argc, char *argv[])
{
    if (argc < 2) {
        spinand_help();
        return 0;
    }

    if (!strncmp(argv[1], "init", 4))
        return do_spinand_init(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "dump", 4))
        return do_spinand_dump(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "read", 4))
        return do_spinand_read(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "write", 5))
        return do_spinand_write(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "erase", 5))
        return do_spinand_erase(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "oobdump", 7))
        return do_spinand_oobdump(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "bad", 3))
        return do_spinand_bad(argc - 1, &argv[1]);
#ifdef AIC_SPINAND_CONT_READ
    else if (!strncmp(argv[1], "contread", 8))
        return do_spinand_contread(argc - 1, &argv[1]);
#endif
    spinand_help();
    return 0;
}

CONSOLE_CMD(spinand, do_spinand, "SPI NAND flash R/W command.");
