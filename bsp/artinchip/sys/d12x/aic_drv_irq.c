/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include <aic_hal.h>

extern void Default_Handler(void);
extern void SysTick_Handler(void);
extern void TIM4_NMIHandler(void);

#ifdef AIC_GPIO_IRQ_DRV_EN
#define MAX_IRQ_ENTRY (MAX_IRQn+GPIO_MAX_PIN)
#else
#define MAX_IRQ_ENTRY (MAX_IRQn)
#endif

void * g_irqvector[MAX_IRQ_ENTRY];
void * g_irqdata[MAX_IRQ_ENTRY];
u32 g_irqcnt[MAX_IRQ_ENTRY] = {0};
void (*g_nmivector)(void);

void drv_irq_vectors_init(void)
{
    int i;

    for (i = 0; i < MAX_IRQn; i++)
    {
        g_irqvector[i] = Default_Handler;
        g_irqdata[i] = NULL;
    }

    for (i = MAX_IRQn; i < MAX_IRQ_ENTRY; i++)
    {
        g_irqvector[i] = NULL;
        g_irqdata[i] = NULL;
    }

    g_irqvector[CORET_IRQn] = SysTick_Handler;
    g_nmivector = TIM4_NMIHandler;
}

void drv_irq_call_isr(uint32_t irq_num)
{
    irq_handler_t func = NULL;
    pin_irq_handler_t pin_func = NULL;

    if (irq_num >= MAX_IRQ_ENTRY)
        return;

    func = (irq_handler_t)g_irqvector[irq_num];
    if (func) {
        /* Extend gpio pin irq */
        g_irqcnt[irq_num]++;
        if (irq_num >= MAX_IRQn) {
            pin_func = (pin_irq_handler_t)func;
            pin_func(g_irqdata[irq_num]);
        /* Normal hw irq */
        } else {
            func(irq_num, g_irqdata[irq_num]);
        }
    }
}

/**
  \brief       enable irq.
  \param[in]   irq_num Number of IRQ.
  \return      None.
*/
void drv_irq_enable(uint32_t irq_num)
{
    if (irq_num >= MAX_IRQn)
        return;

    if (NMI_EXPn != irq_num)
    {
    #ifdef CONFIG_SYSTEM_SECURE
        csi_vic_enable_sirq(irq_num);
    #else
        csi_vic_enable_irq(irq_num);
    #endif
    }
}

/**
  \brief       disable irq.
  \param[in]   irq_num Number of IRQ.
  \return      None.
*/
void drv_irq_disable(uint32_t irq_num)
{
    if (irq_num >= MAX_IRQn)
            return;

    if (NMI_EXPn != irq_num)
    {
    #ifdef CONFIG_SYSTEM_SECURE
        csi_vic_disable_sirq(irq_num);
    #else
        csi_vic_disable_irq(irq_num);
    #endif
    }
}

/**
  \brief       register irq handler.
  \param[in]   irq_num Number of IRQ.
  \param[in]   irq_handler IRQ Handler.
  \return      None.
*/
void drv_irq_register(uint32_t irq_num, void *irq_handler, void *data)
{
    if ((irq_num >= MAX_IRQ_ENTRY) || (irq_handler == NULL))
            return;

    if (NMI_EXPn != irq_num)
    {
        g_irqvector[irq_num] = irq_handler;
        g_irqdata[irq_num] = data;
        g_irqcnt[irq_num] = 0;
    }
    else
    {
        g_nmivector = irq_handler;
    }
}

/**
  \brief       unregister irq handler.
  \param[in]   irq_num Number of IRQ.
  \return      None.
*/
void drv_irq_unregister(uint32_t irq_num)
{
    if (irq_num >= MAX_IRQ_ENTRY)
            return;

    if (NMI_EXPn != irq_num)
    {
        g_irqvector[irq_num] = (void *)Default_Handler;
		g_irqdata[irq_num] = NULL;
    }
    else
    {
        g_nmivector = (void *)Default_Handler;
    }
}

#if defined(RT_USING_FINSH)
#include <finsh.h>

static int cmd_list_irq(int argc, char **argv)
{
    int i;

    printf("IRQn Handler    Count   \n");
    printf("---- ---------- ----------\n");
    for (i = 0; i < MAX_IRQ_ENTRY; i++) {
        if (!g_irqvector[i] || g_irqvector[i] == Default_Handler)
            continue;

        printf("%3d: 0x%08x %10d\n", i, (u32)&g_irqvector[i], g_irqcnt[i]);
    }
    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_list_irq, list_irq, list system irq);
#endif
