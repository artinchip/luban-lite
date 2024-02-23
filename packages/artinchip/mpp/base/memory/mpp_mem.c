/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  author: qi.xu@artinchip.com
 *  Desc: virtual memory allocator
 */

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "aic_core.h"
#include "mpp_mem.h"
#include "mpp_log.h"

#define DEBUG_MEM  (0)

#if DEBUG_MEM
#include <pthread.h>
#define MAX_NUM  20
struct memory_node {
	void* addr;
	int size;
	int used;
};

static pthread_mutex_t 	g_mem_mutex = PTHREAD_MUTEX_INITIALIZER;
int 			g_mem_count = 0;
int 			g_mem_total_size = 0;
struct memory_node	g_mem_info[MAX_NUM] = {{0,0,0}};

#endif

void *mpp_alloc(size_t size)
{
	void *ptr = NULL;

	ptr = aicos_malloc(MEM_DEFAULT, size);

#if DEBUG_MEM
	int i = 0;
	pthread_mutex_lock(&g_mem_mutex);
	g_mem_count ++;
	g_mem_total_size += size;

	for(i=0; i<MAX_NUM; i++) {
		if(g_mem_info[i].used == 0) {
			g_mem_info[i].addr = ptr;
			g_mem_info[i].size = size;
			g_mem_info[i].used = 1;
			logi("alloc memory, addr: %p, %d", ptr, size);
			break;
		}
	}
	if(i == MAX_NUM) {
		loge("exceed max number");
	}
	pthread_mutex_unlock(&g_mem_mutex);
#endif
	return ptr;
}

void mpp_free(void *ptr)
{
	if(ptr == NULL)
		return;

	aicos_free(MEM_DEFAULT, ptr);

#if DEBUG_MEM
	int i;
	pthread_mutex_lock(&g_mem_mutex);
	for(i=0; i<MAX_NUM; i++) {
		if(g_mem_info[i].used && g_mem_info[i].addr == ptr) {
			g_mem_count--;
			g_mem_total_size -= g_mem_info[i].size;
			logi("free memory, addr: %p, %d", ptr, g_mem_info[i].size);
			break;
		}
	}
	if(i == MAX_NUM) {
		loge("cannot find this memory");
	}
	logi("total memory size: %d", g_mem_total_size);
	pthread_mutex_unlock(&g_mem_mutex);
#endif
}

/***************************** physic memory *****************************************/
#define ALIGN_1024B(x) ((x+1023)&(~1023))
// base address of reserved buffer
#define BASE_ADDR 0x43a00000
#define MEMORY_NUM 32
//#define USE_CARVOUT

struct phy_mem_info {
    unsigned int addr;
    unsigned int align_addr; // 8 bytes align
    int size;
    int used;
};

struct phy_mem_info info[MEMORY_NUM];
unsigned int g_addr = BASE_ADDR;
int total_cnt = 0;
unsigned int mpp_phy_alloc(size_t size)
{
    int i;

    for(i=0; i<MEMORY_NUM; i++) {
        if(info[i].used == 0)
            break;
    }

    if(i == MEMORY_NUM) {
        loge("memory count exceed max number");
        return 0;
    }

#ifdef USE_CARVOUT
    info[i].addr = g_addr;
    info[i].align_addr = info[i].addr;
    info[i].size = ALIGN_1024B(size);
    g_addr += info[total_cnt].size;
#else
    info[i].addr = (unsigned long)aicos_malloc(MEM_CMA, ALIGN_UP(size, CACHE_LINE_SIZE) + 1024);
    info[i].align_addr = ALIGN_1024B(info[i].addr);
    info[i].size = size;
#endif
    info[i].used = 1;
    total_cnt ++;

    aicos_dcache_clean_invalid_range((unsigned long *)((unsigned long)info[i].align_addr),
                                     ALIGN_UP(info[i].size, CACHE_LINE_SIZE));

    logw("mpp_phy_alloc success, addr: %08x, align_addr: %08x, size: %d",
        info[i].addr, info[i].align_addr, info[i].size);
    return info[i].align_addr;
}

void mpp_phy_free(unsigned int addr)
{
    int i;

    for(i=0; i<MEMORY_NUM; i++) {
        if(info[i].align_addr == addr) {
            if(info[i].used == 0) {
                loge("the buffer not alloc, addr: %08x, size: %d", addr, info[i].size);
                return;
            }
            break;
        }
    }

    if(i == MEMORY_NUM) {
        loge("not found the addr: %08x, maybe error!", addr);
        return;
    }

    info[i].used = 0;
    total_cnt --;

    if(total_cnt == 0) {
        logw("reset base address");
        g_addr = BASE_ADDR;
    }

#ifndef USE_CARVOUT
    aicos_free(MEM_CMA, (void*)(unsigned long)info[i].addr);
#endif
    logw("phy_free success, addr: %08x, size: %d", info[i].addr, info[i].size);
}
