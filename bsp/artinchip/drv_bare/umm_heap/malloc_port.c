#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <heap.h>
#include "umm_malloc.h"

int heap_init(void *ptr, size_t size)
{
    umm_init_heap(ptr, size);
    return 0;
}

#ifndef TLSF_MEM_HEAP
void *aic_tlsf_malloc(uint32_t mem_type, uint32_t nbytes)
{
    return umm_malloc(nbytes);
}

void aic_tlsf_free(uint32_t mem_type, void *ptr)
{
    umm_free(ptr);
}

void *aic_tlsf_malloc_align(uint32_t mem_type, uint32_t size, uint32_t align)
{
    return umm_malloc_align(size, align);
}

void aic_tlsf_free_align(uint32_t mem_type, void *ptr)
{
    umm_free_align(ptr);
}

void *aic_tlsf_realloc(uint32_t mem_type, void *ptr, uint32_t nbytes)
{
    return umm_realloc(ptr, nbytes);
}

void *aic_tlsf_calloc(uint32_t mem_type, uint32_t count, uint32_t size)
{
    return umm_calloc(count, size);
}

#endif
