/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <aic_core.h>


unsigned long g_aicos_irq_state = 0;
unsigned int g_aicos_irq_nested_cnt = 0;

void aicos_irq_enter(void)
{
    g_aicos_irq_nested_cnt++;
}

void aicos_irq_exit(void)
{
    g_aicos_irq_nested_cnt--;
}

void *_aicos_malloc_align_(size_t size, size_t align, unsigned int type, void *func)
{
    void *ptr;
    void *align_ptr;
    int uintptr_size;
    size_t align_size;

    /* sizeof pointer */
    uintptr_size = sizeof(void*);
    uintptr_size -= 1;

    /* align the alignment size to uintptr size byte */
    align = ((align + uintptr_size) & ~uintptr_size);

    /* get total aligned size */
    align_size = ((size + uintptr_size) & ~uintptr_size) + align;
    /* allocate memory block from heap */
    if (type >= MAX_MEM_REGION) {
        aicos_malloc1_t func1 = (aicos_malloc1_t)func;
        ptr = (*func1)(align_size);
    } else {
        aicos_malloc2_t func2 = (aicos_malloc2_t)func;
        ptr = (*func2)(type, align_size);
    }
    if (ptr != NULL)
    {
        /* the allocated memory block is aligned */
        if (((unsigned long)ptr & (align - 1)) == 0)
        {
            align_ptr = (void *)((unsigned long)ptr + align);
        }
        else
        {
            align_ptr = (void *)(((unsigned long)ptr + (align - 1)) & ~(align - 1));
        }

        /* set the pointer before alignment pointer to the real pointer */
        *((unsigned long *)((unsigned long)align_ptr - sizeof(void *))) = (unsigned long)ptr;

        ptr = align_ptr;
    }

    return ptr;
}

void _aicos_free_align_(void *ptr, unsigned int type, void *func)
{
    void *real_ptr;

    /* NULL check */
    if (ptr == NULL) return;
    real_ptr = (void *) * (unsigned long *)((unsigned long)ptr - sizeof(void *));
    if (type >= MAX_MEM_REGION) {
        aicos_free1_t func1 = (aicos_free1_t)func;
        (*func1)(real_ptr);
    } else {
        aicos_free2_t func2 = (aicos_free2_t)func;
        (*func2)(type, real_ptr);
    }
}
