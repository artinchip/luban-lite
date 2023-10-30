/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_OSAL_PLATFORM_LBL_H_
#define _ARTINCHIP_AIC_OSAL_PLATFORM_LBL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <csi_core.h>
#include <aic_drv_irq.h>
//#include <aic_tlsf.h>

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
    __disable_irq();
}

static inline void aicos_local_irq_enable(void)
{
    __enable_irq();
}

enum irqreturn
{
    IRQ_NONE        = (0 << 0),
    IRQ_HANDLED     = (1 << 0),
    IRQ_WAKE_THREAD = (1 << 1),
};
typedef enum irqreturn irqreturn_t;
typedef irqreturn_t (*irq_handler_t)(int, void *);
typedef irqreturn_t (*pin_irq_handler_t)(void *);


static inline int aicos_request_irq(unsigned int irq, irq_handler_t handler, unsigned int flags,
                                 const char *name, void *data)
{
    drv_irq_register(irq, handler, data);
    drv_irq_enable(irq);

    return 0;
}

static inline void aicos_irq_enable(unsigned int irq)
{
    drv_irq_enable(irq);
}

static inline void aicos_irq_disable(unsigned int irq)
{
    drv_irq_disable(irq);
}

extern unsigned int g_aicos_irq_nested_cnt;
static inline int aicos_in_irq(void)
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

static inline void aicos_dcache_invalid_range(void *addr, u32 size)
{
    csi_dcache_invalid_range((phy_addr_t)(ptr_t)addr, size);
}

static inline void aicos_dcache_clean_range(void *addr, u32 size)
{
    csi_dcache_clean_range((phy_addr_t)(ptr_t)addr, size);
}

static inline void aicos_dcache_clean_invalid_range(void *addr, u32 size)
{
    csi_dcache_clean_invalid_range((phy_addr_t)(ptr_t)addr, size);
}

//--------------------------------------------------------------------+
// Delay API
//--------------------------------------------------------------------+
extern void aic_udelay(u32 us);

static inline void aicos_mdelay(unsigned long msecs)
{
    aic_udelay(msecs * 1000);
}

static inline void aicos_udelay(unsigned long usecs)
{
    aic_udelay(usecs);
}

#ifdef __cplusplus
}
#endif

#endif /* _ARTINCHIP_AIC_OSAL_PLATFORM_LBL_H_ */
