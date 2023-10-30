/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_core.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <xspi_psram.h>
#include <hexdump.h>
#include <aic_time.h>

#define BASE_DRAM     (0x40000000)
#define PARALLEL_MODE 1
#define SINGLE_MODE   0
static u8 xspi_get_parallel_mode(void)
{
    u32 val = readl(0x10300000);
    return (u8)((val >> 6) & 0x01);
}

s32 cpu_stream_double(u32 addr, u32 len, u32 loop)
{
    const char *name[4] = { "copy", "scalar", "add", "triad" };
    const char bytes[4] = { 2, 2, 3, 3 };
    double *stream_a = (double *)(u32)(addr);
    double *stream_b = (double *)(u32)(addr + len / 4);
    double *stream_c = (double *)(u32)(addr + len / 2);
    double check_a, check_b, check_c;
    double check_aerr, check_berr, check_cerr;
    double scalar = 3.0;
    u32 size = len / 4 / sizeof(double);

    u32 t0[4];
    u32 t1[4];
    u32 tt;
    u32 bw;
    u32 i = 0, j, k;
    s32 ret = 1;

    for (j = 0; j < size; j++) {
        stream_a[j] = 1.0;
        stream_b[j] = 2.0;
        stream_c[j] = 0.0;
    }

    for (i = 0; i < loop; i++) {
        //copy
        t0[0] = aic_get_time_us();
        for (j = 0; j < size; j++)
            stream_c[j] = stream_a[j];
        t1[0] = aic_get_time_us();

        //scalar
        t0[1] = aic_get_time_us();
        for (j = 0; j < size; j++)
            stream_b[j] = scalar * stream_c[j];
        t1[1] = aic_get_time_us();

        //add
        t0[2] = aic_get_time_us();
        for (j = 0; j < size; j++)
            stream_c[j] = stream_a[j] + stream_b[j];
        t1[2] = aic_get_time_us();

        //triad
        t0[3] = aic_get_time_us();
        for (j = 0; j < size; j++)
            stream_a[j] = stream_b[j] + scalar * stream_c[j];
        t1[3] = aic_get_time_us();

        //end
        printf("------------------------------\n");
        printf(" Stream Test Loop %d Result\n", i);
        printf("------------------------------\n");
        for (k = 0; k < 4; k++) {
            tt = (u32)t1[k] - (u32)t0[k];
            bw = size * (sizeof(double) * bytes[k]) / tt;
            printf(
                "sizeof(double) : %d stream_a=0x%x, stream_b=0x%x, stream_c=0x%x, array_size=%d\n",
                sizeof(double), (u32)stream_a, (u32)stream_b, (u32)stream_c,
                size);
            printf("Test Pat : %s \n", name[k]);
            printf("Time Beg : %7d us\n", (u32)t0[k]);
            printf("Time End : %7d us\n", (u32)t1[k]);
            printf("Time Use : %7d us\n", tt);
            printf("Bandwidth: %7d MB/s\n", bw);
            printf("------------------------------\n");
        }
    }

    // check
    check_a = 1.0;
    check_b = 2.0;
    check_c = 0.0;
    for (i = 0; i < loop; i++) {
        check_c = check_a;
        check_b = scalar * check_c;
        check_c = check_a + check_b;
        check_a = check_b + scalar * check_c;
    }

    check_aerr = 0.0;
    check_berr = 0.0;
    check_cerr = 0.0;
    for (j = 0; j < size; j++) {
        check_aerr += abs(stream_a[j] - check_a);
        check_berr += abs(stream_b[j] - check_b);
        check_cerr += abs(stream_c[j] - check_c);
    }

    check_aerr /= size;
    check_berr /= size;
    check_cerr /= size;
    if (abs(check_aerr / check_a) > 1e-13) {
        printf("Verify Fail: A Error !\n");
        ret = 0;
    }
    if (abs(check_berr / check_b) > 1e-13) {
        printf("Verify Fail: B Error !\n");
        ret = 0;
    }
    if (abs(check_cerr / check_c) > 1e-13) {
        printf("Verify Fail: C Error !\n");
        ret = 0;
    }
    if (ret == 1)
        printf("Verify Pass !\n");
    printf("------------------------------\n");

    return ret;
}

static int do_psram_test(int argc, char *argv[])
{
    int len = 0x400000;
    int loops = 1;

    printf(" %s:%d BASE_DRAM=0x%x...\n", __FUNCTION__, __LINE__, BASE_DRAM);

    if (xspi_get_parallel_mode() == PARALLEL_MODE) {
        len = 0x800000;
        printf(" %s:%d PARALLEL_MODE len=0x%x...\n", __FUNCTION__, __LINE__,
               len);
    } else {
        printf(" %s:%d SINGLE_MODE len=0x%x...\n", __FUNCTION__, __LINE__, len);
    }

    if (argc == 2) {
        loops = atoi(argv[1]);
    }

    cpu_stream_double(BASE_DRAM + len / 2, len / 2, loops);

    return 0;
}

CONSOLE_CMD(test_psram, do_psram_test, "psram memory test ...");
