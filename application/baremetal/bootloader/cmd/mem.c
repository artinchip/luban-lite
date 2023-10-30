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
    "  md [mode] <addr> <count>\n"                              \
    "    mode: should be \"b\" \"w\" \"l\", default is \"b\"\n" \
    "    addr: hex address string\n"                            \
    "    count: display unit count\n"                           \
    "  e.g.: \n"                                                \
    "    md 0x40000000 64\n"                                    \
    "    md w 0x40000000 64\n"                                  \
    "    md l 0x40000000 64\n"

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
        if (*argv[1] == 'b')
            groupsize = 1;
        else if (*argv[1] == 'w')
            groupsize = 2;
        else if (*argv[1] == 'l')
            groupsize = 4;
        else
            goto help;
        addr = strtol(argv[2], NULL, 0);
        cnt = strtol(argv[3], NULL, 0);
    } else {
        groupsize = 1;
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
    "  mw [mode] <addr> <value>\n"                              \
    "    mode: should be \"b\" \"w\" \"l\", default is \"b\"\n" \
    "    addr: hex address string\n"                            \
    "    value: value going to write\n"                         \
    "  e.g.: \n"                                                \
    "    mw 0x40000000 64\n"                                    \
    "    mw w 0x40000000 64\n"                                  \
    "    mw l 0x40000000 64\n"

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
        if (*argv[1] == 'b')
            groupsize = 1;
        else if (*argv[1] == 'w')
            groupsize = 2;
        else if (*argv[1] == 'l')
            groupsize = 4;
        else
            goto help;
        addr = strtol(argv[2], NULL, 0);
        val = strtol(argv[3], NULL, 0);
    } else {
        groupsize = 1;
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

CONSOLE_CMD(md, do_mem_display, "Memory display");
CONSOLE_CMD(mw, do_mem_write,   "Memory write");
