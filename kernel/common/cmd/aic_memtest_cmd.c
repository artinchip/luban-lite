/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "aic_osal.h"
#include "aic_common.h"

#ifdef AIC_CONSOLE_BARE_DRV
#include "console.h"
#endif

#ifdef RT_USING_FINSH
#include <rtconfig.h>
#include <rthw.h>
#include <rtthread.h>
#include <finsh.h>

#define printf              rt_kprintf
#define THREAD_NAME_LEN     64

struct memtest_para {
    u32 address;
    u32 size;
    char thread_name[THREAD_NAME_LEN];
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
        memset(para.thread_name, 0, THREAD_NAME_LEN);
    }
    snprintf(para.thread_name, THREAD_NAME_LEN, "memtest_0x%x-0x%x, size=0x%x",
             para.address, para.address + para.size, para.size);
    printf("memtest address 0x%08X, size 0x%08X\r\n", para.address, para.size);

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
    char thread_name[THREAD_NAME_LEN] = { 0 };
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
        snprintf(thread_name, THREAD_NAME_LEN, "memtest_auto_addr-0x%x", size);
    } else if (argc == 3) {
        address = strtol(argv[1], &str_tmp, 16);
        size = strtol(argv[2], &str_tmp, 16);
        para.address = address;
        snprintf(thread_name, THREAD_NAME_LEN, "memtest_0x%x-0x%x",
                 address, address + size);
    }
    snprintf(para.thread_name, THREAD_NAME_LEN, "%s", thread_name);
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

static int cmd_test_cache(int argc, char **argv)
{
    u32 addr = 0, len = 0;

    if (argc != 4) {
        printf("Invalid argument. Usage: \n");
        printf(" %s [i/c/f] [Phy addr] [len]\n", argv[0]);
        return -1;
    }

    addr = strtol(argv[2], NULL, 16);
    len  = atoi(argv[3]);
    switch (argv[1][0]) {
    case 'i':
        printf("Invalid Cache 0x%08x, len %d\n", addr, len);
        aicos_dcache_invalid_range((void *)(ptr_t)addr, len);
        break;

    case 'c':
        printf("Clean Cache 0x%08x, len %d\n", addr, len);
        aicos_dcache_clean_range((void *)(ptr_t)addr, len);
        break;

    case 'f':
        printf("Clean and invalid Cache 0x%08x, len %d\n", addr, len);
        aicos_dcache_clean_invalid_range((void *)(ptr_t)addr, len);
        break;

    default:
        printf("Invalid Cache operation: %s\n", argv[1]);
        return -1;
    }
    return 0;
}
#ifdef RT_USING_FINSH
MSH_CMD_EXPORT_ALIAS(cmd_test_cache, test_cache, Cache operation test);
#endif
#ifdef AIC_CONSOLE_BARE_DRV
CONSOLE_CMD(test_cache, cmd_test_cache, "Cache operation test");
#endif

static char *show_size(u32 size)
{
    static char str[32] = "";

    memset(str, 0, 32);
    if (size >= 0x100000) {
        if ((size >> 10) % 0x400)
            snprintf(str, 32, "%d MB %d KB", size >> 20, (size >> 10) % 0x400);
        else
            snprintf(str, 32, "%d MB", size >> 20);
    } else if (size >= 0x400) {
        snprintf(str, 32, "%d KB", size >> 10);
    } else {
        snprintf(str, 32, "%d B", size);
    }
    return str;
}

#define PRINT_MEM_ITEM(name, s, e) \
do { \
    int size = (int)(ptr_t)&e - (int)(ptr_t)&s; \
    if (size) \
        printf("%-16s 0x%08X 0x%08X 0x%08X (%s)\n", name, \
               (int)(ptr_t)&s, (int)(ptr_t)&e, size, show_size(size)); \
    else \
        printf("%-16s %10d %10d %10d \n", name, 0, 0, 0); \
} while (0)

#define PRINT_MEM_ITEM_EX(name, s, e) \
do { \
    extern int s, e; \
    PRINT_MEM_ITEM(name, s, e); \
} while (0)

static int cmd_meminfo(int argc, char **argv)
{
    printf("--------------------------------------------------------------\n");
    printf("Name             Start      End        Size \n");
    printf("---------------- ---------- ---------- -----------------------\n");

#ifdef AIC_DRAM_TOTAL_SIZE
    PRINT_MEM_ITEM_EX("DRAM Total", __dram_start, __dram_end);
#endif
#ifdef AIC_DRAM_CMA_EN
    PRINT_MEM_ITEM("DRAM CMA Heap", __dram_cma_heap_start, __dram_cma_heap_end);
#endif

/* Ignore the baremetal mode,
   because the link script is so simple that some label are undefined */
#ifndef KERNEL_BAREMETAL

#ifdef AIC_TCM_EN
    PRINT_MEM_ITEM_EX("iTCM Total", __itcm_start, __itcm_end);
    PRINT_MEM_ITEM("iTCM Heap", __itcm_heap_start, __itcm_heap_end);
    PRINT_MEM_ITEM_EX("dTCM Total", __dtcm_start, __dtcm_end);
    PRINT_MEM_ITEM("dTCM Heap", __dtcm_heap_start, __dtcm_heap_end);
#endif

#ifdef AIC_SRAM_TOTAL_SIZE
    PRINT_MEM_ITEM_EX("SRAM S0 Total", __sram_s0_start, __sram_s0_end);
    PRINT_MEM_ITEM_EX("SRAM S1 SW", __sram_s1_sw_start, __sram_s1_sw_end);
    PRINT_MEM_ITEM_EX("SRAM S1 CMA", __sram_s1_cma_start, __sram_s1_cma_end);
#endif

#ifdef AIC_SRAM_SIZE
#if (CONFIG_AIC_SRAM_SW_SIZE != 0)
    PRINT_MEM_ITEM("SRAM SW Heap", __sram_sw_heap_start, __sram_sw_heap_end);
#endif
#endif

#ifdef AIC_SRAM1_SW_EN
    PRINT_MEM_ITEM("SRAM1 SW Heap",
                   __sram_s1_sw_heap_start, __sram_s1_sw_heap_end);
#endif
#ifdef AIC_SRAM1_CMA_EN
    PRINT_MEM_ITEM("SRAM1 CMA Heap",
                   __sram_s1_cma_heap_start, __sram_s1_cma_heap_end);
#endif

#ifdef AIC_PSRAM_SIZE
    PRINT_MEM_ITEM_EX("PSRAM Total", __psram_start, __psram_end);
#endif
#ifdef AIC_PSRAM_SW_EN
    PRINT_MEM_ITEM_EX("PSRAM SW", __psram_sw_start, __psram_sw_end);
    PRINT_MEM_ITEM("PSRAM SW Heap",
                   __psram_sw_heap_start, __psram_sw_heap_end);
#endif
#ifdef AIC_PSRAM_CMA_EN
    PRINT_MEM_ITEM_EX("PSRAM CMA", __psram_cma_start, __psram_cma_end);
    PRINT_MEM_ITEM("PSRAM CMA Heap",
                   __psram_cma_heap_start, __psram_cma_heap_end);
#endif

#ifdef AIC_DRAM_CMA_EN
    PRINT_MEM_ITEM("CMA Heap", __cma_heap_start, __cma_heap_end);
#endif

#endif // end of ifndef KERNEL_BAREMETAL

    PRINT_MEM_ITEM("Heap", __heap_start, __heap_end);
    PRINT_MEM_ITEM_EX("Text section", __stext, __etext);
    PRINT_MEM_ITEM_EX("RO Data section", __srodata, __erodata);
    PRINT_MEM_ITEM_EX("Data section", __sdata, __edata);
    PRINT_MEM_ITEM_EX("BSS section", __sbss, __ebss);

    return 0;
}
#ifdef RT_USING_FINSH
MSH_CMD_EXPORT_ALIAS(cmd_meminfo, meminfo, Show the memory information);
#endif
#ifdef AIC_CONSOLE_BARE_DRV
CONSOLE_CMD(meminfo, cmd_meminfo, "Show the memory information");
#endif
