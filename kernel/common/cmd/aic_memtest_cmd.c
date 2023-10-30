/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#ifdef RT_USING_FINSH
#include <rthw.h>
#include <rtthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <finsh.h>
#include "aic_osal.h"

#define printf rt_kprintf

struct memtest_para {
    u32 address;
    u32 size;
    char thread_name[64];
    bool auto_addr;
};

u8 do_mem_test(long address, u32 size)
{
    u32 i;

    /**< 8bit test */
    {
        u8 *p_u8 = (u8 *)address;
        for (i = 0; i < size / sizeof(u8); i++) {
            *p_u8++ = (u8)i;
        }

        p_u8 = (u8 *)address;
        for (i = 0; i < size / sizeof(u8); i++) {
            if (*p_u8 != (u8)i) {
                printf("8bit test fail @ 0x%08X\r\nsystem halt!!!!!",
                       (long)p_u8);
                return 1;
            }
            p_u8++;
        }
    }

    /**< 16bit test */
    {
        u16 *p_u16 = (u16 *)address;
        for (i = 0; i < size / sizeof(u16); i++) {
            *p_u16++ = (u16)i;
        }

        p_u16 = (u16 *)address;
        for (i = 0; i < size / sizeof(u16); i++) {
            if (*p_u16 != (u16)i) {
                printf("16bit test fail @ 0x%08X\r\nsystem halt!!!!!",
                       (long)p_u16);
                return 1;
            }
            p_u16++;
        }
    }

    /**< 32bit test */
    {
        u32 *p_u32 = (u32 *)address;
        for (i = 0; i < size / sizeof(u32); i++) {
            *p_u32++ = (u32)i;
        }

        p_u32 = (u32 *)address;
        for (i = 0; i < size / sizeof(u32); i++) {
            if (*p_u32 != (u32)i) {
                printf("32bit test fail @ 0x%08X\r\nsystem halt!!!!!",
                       (long)p_u32);
                return 1;
            }
            p_u32++;
        }
    }

    /**< 32bit Loopback test */
    {
        u32 *p_u32 = (u32 *)address;
        for (i = 0; i < size / sizeof(u32); i++) {
            *p_u32 = (long)p_u32;
            p_u32++;
        }

        p_u32 = (u32 *)address;
        for (i = 0; i < size / sizeof(u32); i++) {
            if (*p_u32 != (long)p_u32) {
                printf("32bit Loopback test fail @ 0x%08X", (long)p_u32);
                printf(" data:0x%08X \r\n", (u32)*p_u32);
                printf("system halt!!!!!", (long)p_u32);
                return 1;
            }
            p_u32++;
        }
    }
    return 0;
}

static void memtest_thread(void *arg)
{
    long loop_times = 0;
    u8 ret = 0;
    u32 *malloc_addr = NULL;
    struct memtest_para para;
    memset(&para, 0, sizeof(struct memtest_para));
    memcpy(&para, arg, sizeof(struct memtest_para));

    if (para.auto_addr) {
        malloc_addr = aicos_malloc(MEM_CMA, para.size);
        if (!malloc_addr) {
            goto memtest_thread_err;
        }
        para.address = (long)malloc_addr;
        memset(para.thread_name, 0, 64);
    }
    sprintf(para.thread_name, "memtest_0x%x-0x%x, size=0x%x", para.address,
            para.address + para.size, para.size);
    printf("memtest,address: 0x%08X size: 0x%08X\r\n", para.address, para.size);

    do {
        loop_times++;
        ret = do_mem_test(para.address, para.size);
        if (ret) {
            printf("[failed] %s, loop_times=%d\n", para.thread_name,
                   loop_times);
            goto memtest_thread_free;
        }
        aicos_msleep(1000);
        printf("[success] %s, loop_times=%d\n", para.thread_name, loop_times);
    } while (1);

memtest_thread_err:
    printf("memtest_thread aicos_malloc issue: ");
    printf(
        "The start addr: 0x%x, end addr: 0x%x, size: 0x%lx, please try small size.\n",
        para.address, para.address + para.size, para.size);
memtest_thread_free:
    aicos_free(MEM_CMA, malloc_addr);
}

static void cmd_mem_test(int argc, char **argv)
{
    u32 address = 0;
    u32 size = 0;
    rt_thread_t thid;
    char thread_name[32] = { 0 };
    struct memtest_para para;
    memset(&para, 0, sizeof(struct memtest_para));
    char *str_tmp;

    if (argc < 2) {
        printf("Invalid parameter, eg: \n");
        printf("1. lock addr mode: mem_test 0x40400000 0x100000\n");
        printf("2. auto malloc addr mode: mem_test 0x10000\n");
        return;
    }

    if (argc == 2) {
        para.auto_addr = 1;
        size = strtol(argv[1], &str_tmp, 16);
        sprintf(thread_name, "memtest_auto_addr-0x%x", size);
    } else if (argc == 3) {
        address = strtol(argv[1], &str_tmp, 16);
        size = strtol(argv[2], &str_tmp, 16);
        para.address = address;
        sprintf(thread_name, "memtest_0x%x-0x%x", address, address + size);
    }
    sprintf(para.thread_name, "%s", thread_name);
    para.size = size;

    thid = aicos_thread_create(thread_name, 4096, 10, memtest_thread, &para);
    if (thid == NULL) {
        printf("Failed to create \"memtest_0x%lx-0x%lx\" thread\n", address,
               address + size);
        return;
    }

    return;
}

MSH_CMD_EXPORT_ALIAS(cmd_mem_test, mem_test, memory test: mem_test address_hex size_hex);

#endif /* RT_USING_FINSH */
