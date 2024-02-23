/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * RT-Thread Device Interface for uffs
 *
 * Authors: Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <aic_crc32.h>
#include "aic_time.h"

#define BUFFER_SIZE 8192

static void show_speed(char *msg, u32 len, u32 us)
{
    u32 tmp, speed;

    /* Split to serval step to avoid overflow */
    tmp = 1000 * len;
    tmp = tmp / us;
    tmp = 1000 * tmp;
    speed = tmp / 1024;

    printf("%s: %d byte, %d us -> %d KB/s\n", msg, len, us, speed);
}

static void file_crc32(int argc, char **argv)
{
    int fd;
    uint8_t *buffer;
    uint64_t start_us;

    if (argc != 2) {
        pr_err("Usage %s: %s <file name>.\n", __func__, __func__);
        return;
    }

    buffer = (uint8_t *)aicos_malloc(MEM_CMA, BUFFER_SIZE);
    if (buffer == RT_NULL) {
        pr_err("buffer: no memory\n");
        return;
    }

    u32 crc32_val = crc32(0, NULL, 0);
    u32 read_bytes = 0;
    u32 file_sizes = 0;
    /* open the ‘/text.txt’ file in read-only mode */
    fd = open(argv[1], O_RDONLY);
    if (fd >= 0) {
        start_us = aic_get_time_us();
        while((read_bytes = read(fd, buffer, BUFFER_SIZE))) {
            crc32_val = crc32(crc32_val, buffer, read_bytes);
            file_sizes += read_bytes;
        }
        show_speed("read speed", file_sizes, aic_get_time_us() - start_us);

        pr_info("crc32_val: 0x%x sizes: %d\n", crc32_val, file_sizes);
        close(fd);
    }

    if (buffer)
        aicos_free(MEM_CMA, buffer);

    return;
}

MSH_CMD_EXPORT(file_crc32, Calculate the file crc32);
