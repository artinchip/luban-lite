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
#include <aic_errno.h>
#include <hexdump.h>
#include <sfud.h>

#define SPINOR_HELP                           \
    "spinor read write command:\n"            \
    "  spinor init <spi bus id>\n"            \
    "  spinor dump  <offset> <size>\n"        \
    "  spinor read  <addr> <offset> <size>\n" \
    "  spinor erase <offset> <size>\n"        \
    "  spinor write <addr> <offset> <size>\n" \
    "e.g.: \n"                                \
    "  spinor read 0x40000000 0 256\n"

extern sfud_flash *sfud_probe(u32 spi_bus);

static void spinor_help(void)
{
    puts(SPINOR_HELP);
}

sfud_flash *g_spinor_flash;
static int do_spinor_init(int argc, char *argv[])
{
    unsigned long id;
    sfud_flash *flash;

    if (argc < 2) {
        spinor_help();
        return 0;
    }
    id = strtol(argv[1], NULL, 0);

    flash = sfud_probe(id);
    if (flash == NULL) {
        printf("Failed to probe spinor flash.\n");
        return -1;
    }

    printf("probe spinor flash success.\n");

    g_spinor_flash = flash;
    return 0;
}

static int do_spinor_dump(int argc, char *argv[])
{
    sfud_err err;
    sfud_flash *flash;
    u8 *data;
    unsigned long offset, size;

    flash = g_spinor_flash;
    if (flash == NULL) {
        printf("spinor init first.\n");
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

    err = sfud_read(flash, offset, size, data);
    if (!err)
        hexdump((void *)data, size, 1);
    free(data);
    return err;
}

static int do_spinor_read(int argc, char *argv[])
{
    sfud_err err;
    sfud_flash *flash;
    u8 *data;
    unsigned long addr, offset, size;

    flash = g_spinor_flash;
    if (flash == NULL) {
        printf("spinor init first.\n");
        return 0;
    }

    addr = strtol(argv[1], NULL, 0);
    offset = strtol(argv[2], NULL, 0);
    size = strtol(argv[3], NULL, 0);

    data = (u8 *)addr;
    if (data == NULL) {
        printf("Dest address is not correct.\n");
        return -1;
    }

    err = sfud_read(flash, offset, size, data);
    return err;
}

static int do_spinor_erase(int argc, char *argv[])
{
    sfud_err err;
    sfud_flash *flash;
    unsigned long offset, size;

    flash = g_spinor_flash;
    if (flash == NULL) {
        printf("spinor init first.\n");
        return 0;
    }

    offset = strtol(argv[1], NULL, 0);
    size = strtol(argv[2], NULL, 0);

    err = sfud_erase(flash, offset, size);
    return err;
}

static int do_spinor_write(int argc, char *argv[])
{
    sfud_err err;
    sfud_flash *flash;
    u8 *data;
    unsigned long addr, offset, size;

    flash = g_spinor_flash;
    if (flash == NULL) {
        printf("spinor init first.\n");
        return 0;
    }

    addr = strtol(argv[1], NULL, 0);
    offset = strtol(argv[2], NULL, 0);
    size = strtol(argv[3], NULL, 0);

    data = (u8 *)addr;
    if (data == NULL) {
        printf("Source address is not correct.\n");
        return -1;
    }

    err = sfud_write(flash, offset, size, data);
    return err;
}

/*
 * spinor init 0
 * spinor dump offset size
 * spinor read addr offset size
 * spinor write addr offset size
 */
static int do_spinor(int argc, char *argv[])
{
    if (argc < 3) {
        spinor_help();
        return 0;
    }

    if (!strncmp(argv[1], "init", 4))
        return do_spinor_init(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "dump", 4))
        return do_spinor_dump(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "read", 4))
        return do_spinor_read(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "write", 5))
        return do_spinor_write(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "erase", 5))
        return do_spinor_erase(argc - 1, &argv[1]);

    spinor_help();
    return 0;
}

CONSOLE_CMD(spinor, do_spinor,  "SPI NOR flash R/W command.");
