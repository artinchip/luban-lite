/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <rtconfig.h>
#include <csi_core.h>

void (*trap_c_callback)(void);

void trap_c(uint64_t *regs)
{
    uint64_t vec = 0;

    vec = __get_MCAUSE() & 0x3FF;

    printf("CPU Exception: NO.%ld", vec);
    printf("\n");

    printf("x1(ra)   : %016lx\t", regs[0]);
    printf("x2(sp)   : %016lx\t", regs[1]);
    printf("x3(gp)   : %016lx\t", regs[2]);
    printf("x4(tp)   : %016lx\t", regs[3]);
    printf("\n");
    printf("x5(t0)   : %016lx\t", regs[4]);
    printf("x6(t1)   : %016lx\t", regs[5]);
    printf("x7(t2)   : %016lx\t", regs[6]);
    printf("x8(s0/fp): %016lx\t", regs[7]);
    printf("\n");
    printf("x9(s1)   : %016lx\t", regs[8]);
    printf("x10(a0)  : %016lx\t", regs[9]);
    printf("x11(a1)  : %016lx\t", regs[10]);
    printf("x12(a2)  : %016lx\t", regs[11]);
    printf("\n");
    printf("x13(a3)  : %016lx\t", regs[12]);
    printf("x14(a4)  : %016lx\t", regs[13]);
    printf("x15(a5)  : %016lx\t", regs[14]);
    printf("x16(a7)  : %016lx\t", regs[15]);
    printf("\n");
    printf("x17(a7)  : %016lx\t", regs[16]);
    printf("x18(s2)  : %016lx\t", regs[17]);
    printf("x19(s3)  : %016lx\t", regs[18]);
    printf("x20(s4)  : %016lx\t", regs[19]);
    printf("\n");
    printf("x21(s5)  : %016lx\t", regs[20]);
    printf("x22(s6)  : %016lx\t", regs[21]);
    printf("x23(s7)  : %016lx\t", regs[22]);
    printf("x24(s8)  : %016lx\t", regs[23]);
    printf("\n");
    printf("x25(s9)  : %016lx\t", regs[24]);
    printf("x26(s10) : %016lx\t", regs[25]);
    printf("x27(s11) : %016lx\t", regs[26]);
    printf("x28(t3)  : %016lx\t", regs[27]);
    printf("\n");
    printf("x29(t4)  : %016lx\t", regs[28]);
    printf("x30(t5)  : %016lx\t", regs[29]);
    printf("x31(t6)  : %016lx\t", regs[30]);
    printf("\n");
    printf("mcause   : %016lx\n", __get_MCAUSE());
    printf("mtval    : %016lx\n", __get_MTVAL());
    printf("mepc     : %016lx\n", regs[31]);
    printf("mstatus  : %016lx\n", regs[32]);

    if (trap_c_callback)
    {
        trap_c_callback();
    }

    while (1);
}

