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
#include <hexdump.h>

#define MD_HELP                                                 \
    "memory display command:\n"                                 \
    "  p <addr> <count> [mode]\n"                              \
    "    addr: hex address string\n"                            \
    "    count: display unit count\n"                           \
    "    mode: should be 1/2/4 (default is 4)\n" \
    "  e.g.: \n"                                                \
    "    p 0x40000000 64\n"                                    \
    "    p 0x40000000 64 1\n"                                  \
    "    p 0x40000000 64 2\n"

static void mem_display_help(void)
{
    puts(MD_HELP);
}

static int do_mem_display(int argc, char *argv[])
{
    int groupsize;
    unsigned long addr, cnt;

    cnt = 1;
    if (argc < 3) {
        goto help;
    }

    if (argc > 3) {
        addr = strtol(argv[1], NULL, 0);
        cnt = strtol(argv[2], NULL, 0);
        groupsize = strtol(argv[3], NULL, 0);
        if ((groupsize != 1) && (groupsize != 2) && (groupsize != 4))
            goto help;
    } else {
        groupsize = 4;
        addr = strtol(argv[1], NULL, 0);
        cnt = strtol(argv[2], NULL, 0);
    }

    hexdump((void *)addr, cnt * groupsize, groupsize);

    return 0;

help:
    mem_display_help();
    return 0;
}

#define MW_HELP                                                 \
    "memory write command:\n"                                   \
    "  m <addr> <value> [mode]\n"                              \
    "    addr: hex address string\n"                            \
    "    value: value going to write\n"                         \
    "    mode: should be 1/2/4 (default is 4)\n" \
    "  e.g.: \n"                                                \
    "    m 0x40000000 0x64\n"                                    \
    "    m 0x40000000 0x64 1\n"                                  \
    "    m 0x40000000 0x64 2\n"

static void mem_write_help(void)
{
    puts(MW_HELP);
}

static int do_mem_write(int argc, char *argv[])
{
    unsigned long addr, val;
    unsigned char *p;
    int groupsize;

    val = 0;
    if (argc < 3) {
        goto help;
    }

    if (argc > 3) {
        addr = strtol(argv[1], NULL, 0);
        val = strtol(argv[2], NULL, 0);
        groupsize = strtol(argv[3], NULL, 0);
        if ((groupsize != 1) && (groupsize != 2) && (groupsize != 4))
            goto help;
    } else {
        groupsize = 4;
        addr = strtol(argv[1], NULL, 0);
        val = strtol(argv[2], NULL, 0);
    }

    p = (unsigned char *)addr;
    memcpy(p, &val, groupsize);
    return 0;

help:
    mem_write_help();
    return 0;
}

CONSOLE_CMD(p, do_mem_display, "Memory display");
CONSOLE_CMD(m, do_mem_write,   "Memory write");
