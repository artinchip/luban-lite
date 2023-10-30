/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <aic_drv_irq.h>
#include <aic_hal.h>

extern void drv_irq_vectors_init(void);
extern void mm_heap_initialize(void);

int SystemCoreClock = IHS_VALUE;  /* System Core Clock Frequency      */
extern int __Vectors;

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = IHS_VALUE;
}

#ifndef QEMU_RUN
void aic_clk_lowpower(void)
{
    hal_clk_pll_lowpower();
    hal_clk_disable_assertrst(CLK_USB_PHY0);
    hal_clk_disable_assertrst(CLK_USBH0);
    hal_clk_disable_assertrst(CLK_USBD);
    hal_clk_disable_assertrst(CLK_USB_PHY1);
    hal_clk_disable_assertrst(CLK_USBH1);
}

void aic_gtc_enable(void)
{
    /* enable gtc clk */
    *(volatile uint32_t *)(CMU_BASE+0x090c) = 0x3100;
    /* enable gtc */
    *(volatile uint32_t *)GTC_BASE = 0x0001;
}
#endif

void dcache_enable(void)
{
    aicos_dcache_enable();
}

void icache_enable(void)
{
    aicos_icache_enable();
}

static void _system_init_for_kernel(void)
{
    drv_irq_vectors_init();

#ifndef QEMU_RUN
    aic_clk_lowpower();
    aic_gtc_enable();
#endif
    csi_plic_set_prio(PLIC_BASE, CORET_IRQn, 31U);
    csi_clint_config(CORET_BASE, (drv_get_sys_freq() / CONFIG_SYSTICK_HZ), CORET_IRQn);
    drv_irq_enable(CORET_IRQn);
}

static void interrupt_init(void)
{
    int i;

    for (i = 0; i < 1023; i++) {
        PLIC->PLIC_PRIO[i] = 31;
    }

    for (i = 0; i < 32; i++) {
        PLIC->PLIC_IP[i] = 0;
    }

    for (i = 0; i < 32; i++) {
        PLIC->PLIC_H0_MIE[i] = 0;
        PLIC->PLIC_H0_SIE[i] = 0;
    }

    /* set hart threshold 0, enable all interrupt */
    PLIC->PLIC_H0_MTH = 0;
    PLIC->PLIC_H0_STH = 0;

    for (i = 0; i < 1023; i++) {
        PLIC->PLIC_H0_MCLAIM = i;
        PLIC->PLIC_H0_SCLAIM = i;
    }

    /* set PLIC_PER */
    PLIC->PLIC_PER = 0x1;

    /* enable msoft interrupt ; Machine_Software_IRQn*/
    uint64_t mie = __get_MIE();
    mie |= (1 << 11 | 1 << 7 | 1 << 3);
    __set_MIE(mie);
}

/**
  * @brief  initialize the system
  *         Initialize the psr and vbr.
  * @param  None
  * @return None
  */
void SystemInit(void)
{
    /* enable mstatus FS & VS */
#ifdef ARCH_RISCV_FPU
    uint64_t mstatus = __get_MSTATUS();
    mstatus &= ~(0x3UL << 13);
    mstatus |= (0x1UL << 13);
    mstatus &= ~(0x3UL << 23);
    mstatus |= (0x1UL << 23);
    __set_MSTATUS(mstatus);
#endif

    /* enable mxstatus THEADISAEE */
    uint64_t mxstatus = __get_MXSTATUS();
    mxstatus |= (1 << 22);
    /* enable mxstatus MM */
    mxstatus |= (1 << 15);
    __set_MXSTATUS(mxstatus);

    interrupt_init();

    drv_irq_enable(Machine_Software_IRQn);

    _system_init_for_kernel();
}
