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

void trap_c(uint32_t *regs)
{
    int i;
    uint32_t vec = 0;

    vec = __get_MCAUSE() & 0x3FF;

    printf("CPU Exception: NO.%ld", vec);
    printf("\n");

    for (i = 0; i < 31; i++)
    {
        printf("x%d: %08lx\t", i + 1, regs[i]);

        if ((i % 4) == 3)
        {
            printf("\n");
        }
    }

    printf("\n");
    printf("mcause : %08lx\n", __get_MCAUSE());
    printf("mtval  : %08lx\n", __get_MTVAL());
    printf("mepc   : %08lx\n", regs[31]);
    printf("mstatus: %08lx\n", regs[32]);

    if (trap_c_callback)
    {
        trap_c_callback();
    }

    while (1);
}

