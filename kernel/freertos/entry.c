/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <aic_core.h>
#include <board.h>
#include <aic_tlsf.h>

#ifndef FREERTOS_MAIN_THREAD_STACK_SIZE
#define MAIN_TASK_STACK_SIZE 8192
#else
#define MAIN_TASK_STACK_SIZE FREERTOS_MAIN_THREAD_STACK_SIZE
#endif

#ifndef FREERTOS_MAIN_THREAD_PRIORITY
#define MAIN_TASK_PRI 32
#else
#define MAIN_TASK_PRI FREERTOS_MAIN_THREAD_PRIORITY
#endif

extern void aic_board_sysclk_init(void);
extern void aic_board_pinmux_init(void);
extern int main(void);

HeapRegion_t xHeapRegions[] =
{
    { NULL, 0},
    { NULL, 0 }
};

int freertos_heap_init (void)
{
    xHeapRegions[0].pucStartAddress = ( uint8_t * )(&__heap_start);
    xHeapRegions[0].xSizeInBytes = (size_t)(&__heap_end) - (size_t)(&__heap_start);
    vPortDefineHeapRegions( xHeapRegions );

    return 0;
}

void aic_hw_board_init(void)
{
    freertos_heap_init();
#ifdef TLSF_MEM_HEAP
    aic_tlsf_heap_init();
#endif
    aic_board_sysclk_init();
    aic_board_pinmux_init();
}

int entry(void)
{
    TaskHandle_t xHandle;
    BaseType_t ret;

    /* hw&heap init */
    aic_hw_board_init();

    /* kernel init */

    /* init task */
    ret = xTaskCreate((TaskFunction_t)main, "main", MAIN_TASK_STACK_SIZE/sizeof(StackType_t), NULL, configMAX_PRIORITIES-1-MAIN_TASK_PRI, &xHandle);
    if (ret != pdPASS) {
        printf("FreeRTOS create init main task fail.\n");
        return -1;
    }

    /* kernel start */
    vTaskStartScheduler();
    return 0;
}
