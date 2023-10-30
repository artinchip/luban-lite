/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */


/******************************************************************************
 * @file     main.c
 * @brief    CSI Source File for main
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include <csi_config.h>
#include <stdint.h>
#include "linpack_timer_port.h"

extern void benchmark_linpack_main(void);

int linpack_test(void)
{
    benchmark_linpack_main();
    return 0;
}

int linpack_main(void)
{
    uint32_t ret = 0;

    Timer_Open();
    Timer_Start();
    ret = linpack_test();
    Timer_Stop();
    Timer_Close();
    return ret;
}
