/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_OSAL_PLATFORM_YOC_H_
#define _ARTINCHIP_AIC_OSAL_PLATFORM_YOC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aos/aos.h>
#include <csi_core.h>
#include <drv/irq.h>
#include "aic_tlsf.h"

//--------------------------------------------------------------------+
// Irq API
//--------------------------------------------------------------------+

static inline void aicos_local_irq_save(unsigned long *state)
{
    *state = csi_irq_save();
}

static inline void aicos_local_irq_restore(unsigned long state)
{
    csi_irq_restore(state);
}

extern unsigned long g_aicos_irq_state;

static inline void aicos_local_irq_disable(void)
{
    g_aicos_irq_state = csi_irq_save();
}

static inline void aicos_local_irq_enable(void)
{
    csi_irq_restore(g_aicos_irq_state);
}

enum irqreturn
{
	IRQ_NONE		= (0 << 0),
	IRQ_HANDLED		= (1 << 0),
	IRQ_WAKE_THREAD		= (1 << 1),
};
typedef enum irqreturn irqreturn_t;
typedef irqreturn_t (*irq_handler_t)(int, void *);

static inline int aicos_request_irq(unsigned int irq, irq_handler_t handler, unsigned int flags,
                                 const char *name, void *dev)
{
    csi_dev_t * pdev;

    pdev = aos_malloc(sizeof(csi_dev_t));
    if (pdev == NULL)
        return -1;

    pdev->irq_num = irq;
    pdev->dev_tag = flags;

    csi_irq_attach(irq, handler, pdev);
    csi_irq_enable(irq);

    return 0;
}

static inline void aicos_irq_enable(unsigned int irq)
{
    csi_irq_enable(irq);
}

static inline void aicos_irq_disable(unsigned int irq)
{
    csi_irq_disable(irq);
}

extern unsigned int g_aicos_irq_nested_cnt;

static inline void aicos_irq_enter(void)
{
    g_aicos_irq_nested_cnt++;
    aos_kernel_intrpt_enter();
}

static inline void aicos_irq_exit(void)
{
    g_aicos_irq_nested_cnt--;
    aos_kernel_intrpt_exit();
}

static inline bool aicos_in_irq(void)
{
    return g_aicos_irq_nested_cnt;
}

//--------------------------------------------------------------------+
// Cache API
//--------------------------------------------------------------------+

static inline void aicos_icache_enable(void)
{
    csi_icache_enable();
}

static inline void aicos_icache_disable(void)
{
    csi_icache_disable();
}

static inline void aicos_icache_invalid(void)
{
    csi_icache_invalid();
}

static inline void aicos_dcache_enable(void)
{
    csi_dcache_enable();
}

static inline void aicos_dcache_disable(void)
{
    csi_dcache_disable();
}

static inline void aicos_dcache_invalid(void)
{
    csi_dcache_invalid();
}

static inline void aicos_dcache_clean(void)
{
    csi_dcache_clean();
}

static inline void aicos_dcache_clean_invalid(void)
{
    csi_dcache_clean_invalid();
}

static inline void aicos_dcache_invalid_range(unsigned long *addr, unsigned long size)
{
    csi_dcache_invalid_range((dma_addr_t *)addr, (int32_t)size);
}

static inline void aicos_dcache_clean_range(unsigned long *addr, unsigned long size)
{
    csi_dcache_clean_range((dma_addr_t *)addr, (int32_t)size);
}

static inline void aicos_dcache_clean_invalid_range(unsigned long *addr, unsigned long size)
{
    csi_dcache_clean_invalid_range((dma_addr_t *)addr, (int32_t)size);
}


//--------------------------------------------------------------------+
// Mem API
//--------------------------------------------------------------------+

static inline void *aicos_malloc(unsigned int mem_type, size_t size)
{
#ifdef CONFIG_DEFAULT_MALLOC_ONLY
    return aos_malloc(size);
#else
    if (mem_type == MEM_DEFAULT)
        return aos_malloc(size);
    else
        return aic_tlsf_malloc(mem_type, size);
#endif
}

static inline void aicos_free(unsigned int mem_type, void *mem)
{
#ifdef CONFIG_DEFAULT_MALLOC_ONLY
    aos_free(mem);
#else
    if (mem_type == MEM_DEFAULT)
        aos_free(mem);
    else
        aic_tlsf_free(mem_type, mem);
#endif
}


#ifdef __cplusplus
}
#endif

#endif /* _ARTINCHIP_AIC_OSAL_PLATFORM_YOC_H_ */
