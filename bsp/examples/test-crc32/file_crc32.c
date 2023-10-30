/*
 * RT-Thread Device Interface for uffs
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_drv.h>
#include "crc32.h"
#include "aic_time.h"

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
    int fd, size;
    uint8_t *buffer;
    uint32_t result;
    uint32_t start_us;

    if (argc != 2) {
        pr_err("Usage %s: %s <file name>.\n", __func__, __func__);
        return;
    }

    buffer = (uint8_t *)aicos_malloc(MEM_CMA, 0x40000);
    if (buffer == RT_NULL) {
        pr_err("buffer: no memory\n");
        return;
    }

    /* open the ‘/text.txt’ file in read-only mode */
    fd = open(argv[1], O_RDONLY);
    if (fd >= 0) {
        start_us = aic_get_time_us();
        size = read(fd, buffer, 0x80000);
        show_speed("read speed", size, aic_get_time_us() - start_us);

        result = crc32(0, buffer, size);
        pr_info("result : 0x%x size : %d\n", result, size);

        close(fd);
        pr_info("Read from file : %s \n", argv[1]);
    }

    if (buffer)
        aicos_free(MEM_CMA, buffer);

    return;
}

MSH_CMD_EXPORT(file_crc32, Calculate the file crc32);
