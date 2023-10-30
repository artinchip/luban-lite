/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <rtconfig.h>
#include <aic_core.h>
#include "aic_tlsf.h"

//#define printf(fmt, ...)

typedef struct {
    char * name;
    aic_mem_region_t type;
    size_t start;
    size_t end;
} heap_def_t;

typedef struct {
    pool_t pool;
    struct list_head list;
} pool_list_t;

typedef struct {
    tlsf_t heap;
    pool_list_t pool_list;
    u32 total_mem;
    u32 used_mem;
    u32 free_mem;
    u32 max_used_mem;
    u32 max_free_mem;
} aic_tlsf_heap_t;

static heap_def_t heap_def[MAX_MEM_REGION] = {
    {
        .name = "sys",
        .type = MEM_DEFAULT,
        .start = (size_t)(&__heap_start),
        .end = (size_t)(&__heap_end),
    },

#if defined(AIC_CHIP_D21X)
    #ifdef AIC_DRAM_CMA_EN
    {
        .name = "cma",
        .type = MEM_DRAM_CMA,
        .start = (size_t)(&__dram_cma_heap_start),
        .end = (size_t)(&__dram_cma_heap_end),
    },
    #endif
#elif defined(AIC_CHIP_D13X)
    #ifdef AIC_TCM_EN
    {
        .name = "itcm",
        .type = MEM_ITCM,
        .start = (size_t)(&__itcm_heap_start),
        .end = (size_t)(&__itcm_heap_end),
    },
    {
        .name = "dtcm",
        .type = MEM_DTCM,
        .start = (size_t)(&__dtcm_heap_start),
        .end = (size_t)(&__dtcm_heap_end),
    },
    #endif
    #ifdef AIC_SRAM1_CMA_EN
    {
        .name = "s1cma",
        .type = MEM_SRAM1_CMA,
        .start = (size_t)(&__sram_s1_cma_heap_start),
        .end = (size_t)(&__sram_s1_cma_heap_end),
    },
    #endif
    #ifdef AIC_SRAM1_SW_EN
    {
        .name = "s1sw",
        .type = MEM_SRAM1_SW,
        .start = (size_t)(&__sram_s1_sw_heap_start),
        .end = (size_t)(&__sram_s1_sw_heap_end),
    },
    #endif
    #ifdef AIC_PSRAM_CMA_EN
    {
        .name = "cma",
        .type = MEM_PSRAM_CMA,
        .start = (size_t)(&__psram_cma_heap_start),
        .end = (size_t)(&__psram_cma_heap_end),
    },
    #endif
    #ifdef AIC_PSRAM_SW_EN
    {
        .name = "psw",
        .type = MEM_PSRAM_SW,
        .start = (size_t)(&__psram_sw_heap_start),
        .end = (size_t)(&__psram_sw_heap_end),
    },
    #endif
#elif defined(AIC_CHIP_D12X)
    #ifdef AIC_SRAM_CMA_EN
    {
        .name = "scma",
        .type = MEM_SRAM_CMA,
        .start = (size_t)(&__sram_cma_heap_start),
        .end = (size_t)(&__sram_cma_heap_end),
    },
        #endif
    #ifdef AIC_SRAM_SW_EN
    {
        .name = "ssw",
        .type = MEM_SRAM_SW,
        .start = (size_t)(&__sram_sw_heap_start),
        .end = (size_t)(&__sram_sw_heap_end),
    },
    #endif
    #ifdef AIC_PSRAM_CMA_EN
    {
        .name = "cma",
        .type = MEM_PSRAM_CMA,
        .start = (size_t)(&__psram_cma_heap_start),
        .end = (size_t)(&__psram_cma_heap_end),
    },
    #endif
#endif
};
static aic_tlsf_heap_t tlsf_heap[MAX_MEM_REGION];
static aicos_mutex_s heap_mutex;

#ifdef CONFIG_AICTLSF_USE_HOOK
static void (*tlsf_malloc_hook)(void *ptr, u32 size);
static void (*tlsf_free_hook)(void *ptr);

void aic_tlsf_malloc_sethook(void (*hook)(void *ptr, u32 size))
{
    tlsf_malloc_hook = hook;
}

void aic_tlsf_free_sethook(void (*hook)(void *ptr))
{
    tlsf_free_hook = hook;
}

#define TLSF_HOOK_CALL(__hook, argv) do {if ((__hook) != NULL) __hook argv; } while (0)
#else
#define TLSF_HOOK_CALL(__hook, argv)
#endif

#ifdef AIC_CONSOLE_BARE_DRV
#include <console.h>

static int cmd_mem_free(int argc, char *argv[])
{
    u32 i = 0;
    u32 total_m = 0;
    u32 used_m = 0;
    u32 free_m = 0;
    u32 max_used = 0;
    u32 max_free = 0;

    printf("memheap    pool size  used size  max used   free size  max free\n\n");
    printf("---------- ---------- ---------- ---------- ---------- ----------\n\n");
    for (i=0; i < MAX_MEM_REGION; i++) {
        aic_tlsf_mem_info(i, &total_m, &used_m, &max_used, &free_m, &max_free);
        printf("%-10s %-10d %-10d %-10d %-10d %-10d\n",
                heap_def[i].name, total_m, used_m, max_used, free_m, max_free);
    }

    return 0;
}

CONSOLE_CMD(free, cmd_mem_free, "Show the memory usage in the system.");
#endif

void aic_tlsf_heap_test(void)
{
    u32 i = 0;
    u32 total_m = 0;
    u32 used_m = 0;
    u32 free_m = 0;
    u32 max_used = 0;
    u32 max_free = 0;
    void * tmp[10];
    u32 len[10] = {13, 1000, 64, 300, 5000, 20000, 14, 5, 451, 1000};

    pr_debug("\n\nMem heap info:\n\n");
    for (i=0; i < MAX_MEM_REGION; i++) {
        aic_tlsf_mem_info(i, &total_m, &used_m, &max_used, &free_m, &max_free);
        pr_debug("%s: heap region %d total=%d, used=%d, max_used=%d, free=%d, max_free=%d.\n",
                __func__, i, total_m, used_m, max_used, free_m, max_free);
    }

    pr_debug("\n\nStart malloc:\n\n");
    for (i=0; i<10; i++){
        tmp[i] = aic_tlsf_malloc(MEM_CMA, len[i]);
        pr_debug("%s: heap region %d malloc %d len.\n ",  __func__, MEM_CMA, len[i]);
        aic_tlsf_mem_info(MEM_CMA, &total_m, &used_m, &max_used, &free_m, &max_free);
        pr_debug("%s: heap region %d total=%d, used=%d, max_used=%d, free=%d, max_free=%d.\n",
                __func__, MEM_CMA, total_m, used_m, max_used, free_m, max_free);
    }

    pr_debug("\n\nStart free:\n\n");
    for (i=0; i<10; i++){
        aic_tlsf_free(MEM_CMA, tmp[i]);
        pr_debug("%s: heap region %d free %d len.\n ",  __func__, MEM_CMA, len[i]);
        aic_tlsf_mem_info(MEM_CMA, &total_m, &used_m, &max_used, &free_m, &max_free);
        pr_debug("%s: heap region %d total=%d, used=%d, max_used=%d, free=%d, max_free=%d.\n",
                __func__, MEM_CMA, total_m, used_m, max_used, free_m, max_free);
    }
}

void aic_tlsf_heap_init(void)
{
    tlsf_t heap = NULL;
    u32 i = 0;
    size_t m_end;
    size_t m_start;

    for (; i < MAX_MEM_REGION; i++) {
        m_start = heap_def[i].start;
        m_end = heap_def[i].end;
        if (m_start >= m_end) {
            pr_err("%s: region %d addr err. start = 0x%lx, end = 0x%lx\n",
                   __func__, i, (unsigned long)m_start, (unsigned long)m_end);
            return;
        }

        heap = tlsf_create_with_pool((void *)m_start, (m_end - m_start));
        if (heap == NULL) {
            pr_err("%s: region %d tlsf_create_with_pool fial.\n", __func__, i);
            return;
        }

        tlsf_heap[i].heap = heap;
        tlsf_heap[i].pool_list.pool = tlsf_get_pool(heap);
        INIT_LIST_HEAD(&tlsf_heap[i].pool_list.list);
    }

     aicos_mutex_init(&heap_mutex);
}

void *aic_tlsf_malloc(u32 mem_type, u32 nbytes)
{
    void *ptr = NULL;
    tlsf_t heap = NULL;
    int i = 0;

    for (i=0; i<sizeof(heap_def)/sizeof(heap_def_t); i++){
        if (heap_def[i].type == mem_type)
            break;
    }

    if (i >= MAX_MEM_REGION){
        pr_err("%s: mem_type = %d err.\n", __func__, mem_type);
        return NULL;
    }

    heap = tlsf_heap[i].heap;
    if (heap) {
        aicos_mutex_take(&heap_mutex, AICOS_WAIT_FOREVER);

        ptr = tlsf_malloc(heap, nbytes);
        TLSF_HOOK_CALL(tlsf_malloc_hook, ((void *)ptr, nbytes));

        aicos_mutex_give(&heap_mutex);
    }
    return ptr;
}

void aic_tlsf_free(u32 mem_type, void *ptr)
{
    tlsf_t heap = NULL;
    int i = 0;

    for (i=0; i<sizeof(heap_def)/sizeof(heap_def_t); i++){
        if (heap_def[i].type == mem_type)
            break;
    }

    if (i >= MAX_MEM_REGION){
        pr_err("%s: mem_type = %d err.\n", __func__, mem_type);
        return;
    }

    heap = tlsf_heap[i].heap;
    if (heap) {
        aicos_mutex_take(&heap_mutex, AICOS_WAIT_FOREVER);

        tlsf_free(heap, ptr);
        TLSF_HOOK_CALL(tlsf_free_hook, (ptr));

        aicos_mutex_give(&heap_mutex);
    }
}

void *aic_tlsf_realloc(u32 mem_type, void *ptr, u32 nbytes)
{
    tlsf_t heap = NULL;
    int i = 0;

    for (i=0; i<sizeof(heap_def)/sizeof(heap_def_t); i++){
        if (heap_def[i].type == mem_type)
            break;
    }

    if (i >= MAX_MEM_REGION){
        pr_err("%s: mem_type = %d err.\n", __func__, mem_type);
        return NULL;
    }

    heap = tlsf_heap[i].heap;
    if (heap) {
        aicos_mutex_take(&heap_mutex, AICOS_WAIT_FOREVER);

        ptr = tlsf_realloc(heap, ptr, nbytes);

        aicos_mutex_give(&heap_mutex);
    }
    return ptr;
}

void *aic_tlsf_calloc(u32 mem_type, u32 count, u32 size)
{
    void *ptr = NULL;
    u32 total_size;

    if (mem_type >= MAX_MEM_REGION){
        pr_err("%s: mem_type = %d err.\n", __func__, mem_type);
        return NULL;
    }

    total_size = count * size;
    ptr = aic_tlsf_malloc(mem_type, total_size);
    if (ptr != NULL) {
        /* clean memory */
        memset(ptr, 0, total_size);
    }

    return ptr;
}

void *aic_tlsf_malloc_align(u32 mem_type, u32 size, u32 align)
{
    void *ptr = NULL;
    tlsf_t heap = NULL;
    int i = 0;

    for (i=0; i<sizeof(heap_def)/sizeof(heap_def_t); i++){
        if (heap_def[i].type == mem_type)
            break;
    }

    if (i >= MAX_MEM_REGION){
        pr_err("%s: mem_type = %d err.\n", __func__, mem_type);
        return NULL;
    }

    heap = tlsf_heap[i].heap;
    if (heap) {
        aicos_mutex_take(&heap_mutex, AICOS_WAIT_FOREVER);
        ptr = tlsf_memalign(heap, align, size);
        aicos_mutex_give(&heap_mutex);
    }
    return ptr;
}

void aic_tlsf_free_align(u32 mem_type, void *ptr)
{
    aic_tlsf_free(mem_type, ptr);
}

static void mem_info(void *ptr, size_t size, int used, void *user)
{
    aic_tlsf_heap_t *h = &tlsf_heap[(unsigned long)user];

    if (used) {
        h->used_mem += size;

        if (size > h->max_used_mem)
            h->max_used_mem = size;
    }else{
        h->free_mem += size;

        if (size > h->max_free_mem)
            h->max_free_mem = size;
    }
    h->total_mem += size;
}

void aic_tlsf_mem_info(u32 mem_type, u32 *total, u32 *used,
                            u32 *max_used, u32 *free, u32 *max_free)
{
    aic_tlsf_heap_t *h = NULL;
    struct list_head *pos;
    pool_list_t *pool_node;

    if (mem_type >= MAX_MEM_REGION){
        pr_err("%s: mem_type = %d err.\n", __func__, mem_type);
        return;
    }

    h = &tlsf_heap[mem_type];
    h->total_mem = 0;
    h->used_mem = 0;
    h->free_mem = 0;
    h->max_used_mem = 0;
    h->max_free_mem = 0;

    tlsf_walk_pool(h->pool_list.pool, mem_info, (void *)(unsigned long)mem_type);

    list_for_each(pos, &h->pool_list.list){
        pool_node = container_of(pos, pool_list_t, list);
        tlsf_walk_pool(pool_node->pool, mem_info, (void *)(unsigned long)mem_type);
    }

    *total = h->total_mem;
    *used = h->used_mem;
    *max_used = h->max_used_mem;
    *free = h->free_mem;
    *max_free = h->max_free_mem;
}

