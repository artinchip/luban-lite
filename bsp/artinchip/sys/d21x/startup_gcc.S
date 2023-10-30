/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#include <cpuport.h>
#include <aic_soc.h>

.section .vectors, "aw", @progbits
    .align  6
    .globl  __Vectors
    .type   __Vectors, @object
__Vectors:
    j       Default_Handler      /* 0 */
    j       Default_Handler      /* 1 Supervisor-Mode Software Interrupt */
    j       Default_Handler      /* 2 */
    j       PendSV_Handler       /* 3 Machine-Mode Software Interrupt */
    j       Default_Handler      /* 4 */
    j       Default_Handler      /* 5 Supervisor-Mode Timer Interrupt */
    j       Default_Handler      /* 6 */
    j       Default_IRQHandler   /* 7 Machine-Mode Timer Interrupt */
    j       Default_Handler      /* 8 */
    j       Default_Handler      /* 9 Supervisor-Mode External Interrupt */
    j       Default_Handler      /* 10 */
    j       Default_IRQHandler   /* 11 Machine-Mode External Interrupt */
    j       Default_Handler      /* 12 */
    j       Default_Handler      /* 13 */
    j       Default_Handler      /* 14 */
    j       Default_Handler      /* 15 */
    .size __Vectors,.-__Vectors

    .text
    .align  2
_start:
    .text
    .align  2
    .globl  Reset_Handler
    .globl  save_boot_params_ret
    .type   save_boot_params_ret,%function
    .type   Reset_Handler, %function
Reset_Handler:
.option push
.option norelax
    j       save_boot_params
save_boot_params_ret:
    /* disable ie and clear all interrupts */
    csrw    mie, zero
    csrw    mip, zero
    /* Disable MIE to avoid triggering interrupts before the first task starts. */
    /* This bit is set when a task recovers context. */
    li      a0, 0x00000008UL     /*  Machine Interrupt Disable */
    csrc    mstatus, a0

    la      gp, __global_pointer$
.option pop
    la      a0, __Vectors
    ori     a0, a0, 0x1
    csrw    mtvec, a0
#ifndef KERNEL_BAREMETAL
    la      sp, g_top_irqstack
#else
    la      sp, g_top_normalstack
#endif
    csrw    mscratch, sp

    jal     icache_enable
    jal     dcache_enable

    /* Load data section */
#if defined(AIC_XIP)
    la      a0, __erodata
    la      a1, __data_start__
    la      a2, __data_end__
    bgeu    a1, a2, 2f
1:
    lw      t0, (a0)
    sw      t0, (a1)
    addi    a0, a0, 4
    addi    a1, a1, 4
    bltu    a1, a2, 1b
2:
#endif

    /* Clear bss section */
    la      a0, __bss_start__
    la      a1, __bss_end__
    bgeu    a0, a1, 2f
1:
    sw      zero, (a0)
    addi    a0, a0, 4
    bltu    a0, a1, 1b
2:

#ifndef __NO_SYSTEM_INIT
    jal     SystemInit
#endif

#if (defined(KERNEL_RTTHREAD) || defined(KERNEL_FREERTOS))
    jal     entry
#else
    jal     main
#endif

    .size   Reset_Handler, . - Reset_Handler

__exit:
    j      __exit

