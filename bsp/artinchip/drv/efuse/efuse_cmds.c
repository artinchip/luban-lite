/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <stdlib.h>
#include <drv_efuse.h>

static void cmd_efuse_help(void)
{
    printf("efuse command usage:\n");
    printf("  efuse help                     : Get this help.\n");
    printf("  efuse init                     : Initialize eFuse driver.\n");
    printf("  efuse dump     offset len      : Dump data from eFuse offset.\n");
    printf("  efuse read     addr offset len : Read eFuse data to RAM addr.\n");
    printf("  efuse write    addr offset len : Write data to eFuse from RAM addr.\n");
    printf("  efuse writehex offset data     : Write data to eFuse from input hex string.\n");
    printf("  efuse writestr offset data     : Write data to eFuse from input string.\n");
}

static void cmd_efuse_init(int argc, char **argv)
{
    drv_efuse_init();
}

static void cmd_efuse_read(int argc, char **argv)
{
    ulong addr, offset, len;
    int ret;

    if (argc != 4) {
        printf("Invalid parameter.\n");
        return;
    }
    addr = strtoul(argv[1], NULL, 16);
    offset = strtoul(argv[2], NULL, 16);
    len = strtoul(argv[3], NULL, 16);

    ret = drv_efuse_read(offset, (void *)addr, len);
    if (ret <= 0) {
        printf("Read efuse error.\n");
        return;
    }
    printf("Read efuse done.\n");
}

static void cmd_efuse_dump(int argc, char **argv)
{
    ulong offset, len;
    int i, j, ret;
    u8 data[256], c;

    if (argc != 3) {
        printf("Invalid parameter.\n");
        return;
    }
    offset = strtoul(argv[1], NULL, 16);
    len = strtoul(argv[2], NULL, 16);

    ret = drv_efuse_read(offset, (void *)data, len);
    if (ret <= 0) {
        printf("Read efuse error.\n");
        return;
    }
    for (i = 0; i < len; i += 16) {
        printf("0x%08lx: ", i + offset);
        for (j = i; j < i + 16; j++) {
            if (j < len)
                printf("%02X ", data[j]);
            else
                printf("   ");
        }
        printf("\t|");
        for (j = i; (j < len) && (j < i + 16); j++) {
            c = data[j] >= 32 && data[j] < 127 ? data[j] : '.';
            printf("%c", c);
        }
        printf("|\n");
    }
    printf("\n");
}

static void cmd_efuse_write(int argc, char **argv)
{
    ulong addr, offset, len;
    int ret;

    if (argc != 4) {
        printf("Invalid parameter.\n");
        return;
    }
    addr = strtoul(argv[1], NULL, 16);
    offset = strtoul(argv[2], NULL, 16);
    len = strtoul(argv[3], NULL, 16);

    ret = drv_efuse_program(offset, (const void *)addr, len);
    if (ret <= 0) {
        printf("Program 0x%lx failed.\n", offset);
        return;
    }

    printf("Program efuse done.\n");
}

static void cmd_efuse_writehex(int argc, char **argv)
{
    ulong offset, len;
    int ret, i, j;
    char *data, byte[3] = { 0x00, 0x00, 0x00 };
    u8 buf[256];

    if (argc != 3) {
        printf("Invalid parameter.\n");
        return;
    }
    offset = strtoul(argv[1], NULL, 16);
    data = argv[2];
    len = strlen(data) / 2;

    /* hex string to hex value */
    for (i = 0, j = 0; i < strlen(data) - 1; i += 2, j += 1) {
        byte[0] = data[i];
        byte[1] = data[i + 1];
        buf[j] = strtol(byte, NULL, 16);
    }

    ret = drv_efuse_program(offset, (const void *)buf, len);
    if (ret <= 0) {
        printf("Program 0x%lx failed.\n", offset);
        return;
    }

    printf("Program efuse done.\n");
}

static void cmd_efuse_writestr(int argc, char **argv)
{
    ulong offset, len;
    int ret;
    char *data;

    if (argc != 3) {
        printf("Invalid parameter.\n");
        return;
    }
    offset = strtoul(argv[1], NULL, 16);
    data = argv[2];
    len = strlen(data);

    ret = drv_efuse_program(offset, (const void *)data, len);
    if (ret <= 0) {
        printf("Program 0x%lx failed.\n", offset);
        return;
    }

    printf("Program efuse done.\n");
}

#if defined(RT_USING_FINSH)
#include <finsh.h>
#include <msh_parse.h>

static void cmd_efuse_do(int argc, char **argv)
{
    if (argc < 2) {
        return;
    }
    if (!strcmp(argv[1], "init")) {
        cmd_efuse_init(argc - 1, &argv[1]);
        return;
    }
    if (!strcmp(argv[1], "read")) {
        cmd_efuse_read(argc - 1, &argv[1]);
        return;
    }
    if (!strcmp(argv[1], "dump")) {
        cmd_efuse_dump(argc - 1, &argv[1]);
        return;
    }
    if (!strcmp(argv[1], "write")) {
        cmd_efuse_write(argc - 1, &argv[1]);
        return;
    }
    if (!strcmp(argv[1], "writehex")) {
        cmd_efuse_writehex(argc - 1, &argv[1]);
        return;
    }
    if (!strcmp(argv[1], "writestr")) {
        cmd_efuse_writestr(argc - 1, &argv[1]);
        return;
    }
    cmd_efuse_help();
}
MSH_CMD_EXPORT_ALIAS(cmd_efuse_do, efuse, efuse command);
#endif /* RT_USING_FINSH */
