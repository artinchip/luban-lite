/*
    FreeRTOS V6.0.4 - Copyright (C) 2010 Real Time Engineers Ltd.

    ***************************************************************************
    *                                                                         *
    * If you are:                                                             *
    *                                                                         *
    *    + New to FreeRTOS,                                                   *
    *    + Wanting to learn FreeRTOS or multitasking in general quickly       *
    *    + Looking for basic training,                                        *
    *    + Wanting to improve your FreeRTOS skills and productivity           *
    *                                                                         *
    * then take a look at the FreeRTOS eBook                                  *
    *                                                                         *
    *        "Using the FreeRTOS Real Time Kernel - a Practical Guide"        *
    *                  http://www.FreeRTOS.org/Documentation                  *
    *                                                                         *
    * A pdf reference manual is also available.  Both are usually delivered   *
    * to your inbox within 20 minutes to two hours when purchased between 8am *
    * and 8pm GMT (although please allow up to 24 hours in case of            *
    * exceptional circumstances).  Thank you for your support!                *
    *                                                                         *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    ***NOTE*** The exception to the GPL is included to allow you to distribute
    a combined work that includes FreeRTOS without being obliged to provide the
    source code for proprietary components outside of the FreeRTOS kernel.
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
#include <stdio.h>
#include <rtconfig.h>

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/
#define configMTIME_BASE_ADDRESS 	( ( 0xE0000000UL ) + 0xBFF8UL )
#define configMTIMECMP_BASE_ADDRESS ( ( 0xE0000000UL ) + 0x4000UL )

#define portasmHANDLE_INTERRUPT   Default_IRQHandler

#ifdef FREERTOS_PREEMPTIVE_EN
#define configUSE_PREEMPTION        1
#else
#define configUSE_PREEMPTION        0
#endif
#ifdef AIC_CLK_CPU_FREQ
#define configCPU_CLOCK_HZ          ( ( unsigned long ) AIC_CLK_CPU_FREQ )
#else
#define configCPU_CLOCK_HZ          ( ( unsigned long ) 200000000 )
#endif
#ifdef FREERTOS_TICK_RATE_HZ
#define configTICK_RATE_HZ          FREERTOS_TICK_RATE_HZ
#else
#define configTICK_RATE_HZ          200
#endif
#ifdef FREERTOS_MAX_PRIORITIES
#define configMAX_PRIORITIES        FREERTOS_MAX_PRIORITIES
#else
#define configMAX_PRIORITIES        200
#endif
#ifdef FREERTOS_MINIMAL_STACK_SIZE
#define configMINIMAL_STACK_SIZE    ( ( unsigned short ) FREERTOS_MINIMAL_STACK_SIZE )
#else
#define configMINIMAL_STACK_SIZE    ( ( unsigned short ) (256*4) )
#endif
#ifdef FREERTOS_MAX_TASK_NAME_LEN
#define configMAX_TASK_NAME_LEN     ( FREERTOS_MAX_TASK_NAME_LEN )
#else
#define configMAX_TASK_NAME_LEN     ( 12 )
#endif
#define configUSE_16_BIT_TICKS                  0
#ifdef FREERTOS_IDLE_SHOULD_YIELD
#define configIDLE_SHOULD_YIELD                 1
#else
#define configIDLE_SHOULD_YIELD                 0
#endif
#ifdef FREERTOS_USE_TASK_NOTIFICATIONS
#define configUSE_TASK_NOTIFICATIONS            1
#else
#define configUSE_TASK_NOTIFICATIONS            0
#endif
#ifdef FREERTOS_USE_TASK_NOTIFICATION_ARRAY_ENTRIES
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   FREERTOS_USE_TASK_NOTIFICATION_ARRAY_ENTRIES
#else
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
#endif
#ifdef FREERTOS_USE_MUTEXES
#define configUSE_MUTEXES                       1
#ifdef FREERTOS_USE_RECURSIVE_MUTEXES
#define configUSE_RECURSIVE_MUTEXES             1
#else
#define configUSE_RECURSIVE_MUTEXES             0
#endif
#else
#define configUSE_MUTEXES                       0
#define configUSE_RECURSIVE_MUTEXES             0
#endif
#ifdef FREERTOS_USE_COUNTING_SEMAPHORES
#define configUSE_COUNTING_SEMAPHORES           1
#else
#define configUSE_COUNTING_SEMAPHORES           0
#endif
#ifdef FREERTOS_USE_ALTERNATIVE_API
#define configUSE_ALTERNATIVE_API               1 /* Deprecated! */
#else
#define configUSE_ALTERNATIVE_API               0 /* Deprecated! */
#endif
#ifdef FREERTOS_QUEUE_REGISTRY_SIZE
#define configQUEUE_REGISTRY_SIZE               FREERTOS_QUEUE_REGISTRY_SIZE
#else
#define configQUEUE_REGISTRY_SIZE               10
#endif
#ifdef FREERTOS_USE_QUEUE_SETS
#define configUSE_QUEUE_SETS                    1
#else
#define configUSE_QUEUE_SETS                    0
#endif
#ifdef FREERTOS_USE_TIME_SLICING
#define configUSE_TIME_SLICING                  1
#else
#define configUSE_TIME_SLICING                  0
#endif
#ifdef FREERTOS_USE_NEWLIB_REENTRANT
#define configUSE_NEWLIB_REENTRANT              1
#else
#define configUSE_NEWLIB_REENTRANT              0
#endif
#ifdef FREERTOS_ENABLE_BACKWARD_COMPATIBILITY
#define configENABLE_BACKWARD_COMPATIBILITY     1
#else
#define configENABLE_BACKWARD_COMPATIBILITY     0
#endif
#ifdef FREERTOS_NUM_THREAD_LOCAL_STORAGE_POINTERS
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS FREERTOS_NUM_THREAD_LOCAL_STORAGE_POINTERS
#else
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 4
#endif
//#define configSTACK_DEPTH_TYPE                  uint16_t
//#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t

/* Memory allocation related definitions. */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) (24576 * 4))
#define configAPPLICATION_ALLOCATED_HEAP        0

/* Hook function related definitions. */
#ifdef FREERTOS_USE_IDLE_HOOK
#define configUSE_IDLE_HOOK                     1
#else
#define configUSE_IDLE_HOOK                     0
#endif
#ifdef FREERTOS_USE_TICK_HOOK
#define configUSE_TICK_HOOK                     1
#else
#define configUSE_TICK_HOOK                     0
#endif
#ifdef FREERTOS_USE_CHECK_STACK_OVERFLOW_HOOK
#define configCHECK_FOR_STACK_OVERFLOW          1
#else
#define configCHECK_FOR_STACK_OVERFLOW          0
#endif
#ifdef FREERTOS_USE_MALLOC_FAILED_HOOK
#define configUSE_MALLOC_FAILED_HOOK            1
#else
#define configUSE_MALLOC_FAILED_HOOK            0
#endif
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* Run time and task stats gathering related definitions. */
#ifdef FREERTOS_GENERATE_RUN_TIME_STATS
#define configGENERATE_RUN_TIME_STATS           1
#else
#define configGENERATE_RUN_TIME_STATS           0
#endif
#ifdef FREERTOS_USE_TRACE_FACILITY
#define configUSE_TRACE_FACILITY                1
#else
#define configUSE_TRACE_FACILITY                0
#endif
#ifdef FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
#define configUSE_STATS_FORMATTING_FUNCTIONS    1
#else
#define configUSE_STATS_FORMATTING_FUNCTIONS    0
#endif

/* Co-routine related definitions. */
#ifdef FREERTOS_USE_CO_ROUTINES
#define configUSE_CO_ROUTINES                   1
#else
#define configUSE_CO_ROUTINES                   0
#endif
#ifdef FREERTOS_MAX_CO_ROUTINE_PRIORITIES
#define configMAX_CO_ROUTINE_PRIORITIES         FREERTOS_MAX_CO_ROUTINE_PRIORITIES
#else
#define configMAX_CO_ROUTINE_PRIORITIES         2
#endif


/* Software timer related definitions. */
#ifdef FREERTOS_USE_TIMERS
#define configUSE_TIMERS                        1
#else
#define configUSE_TIMERS                        0
#endif
#ifdef FREERTOS_TIMER_TASK_PRIORITY
#define configTIMER_TASK_PRIORITY               FREERTOS_TIMER_TASK_PRIORITY
#else
#define configTIMER_TASK_PRIORITY               1
#endif
#ifdef FREERTOS_TIMER_QUEUE_LENGTH
#define configTIMER_QUEUE_LENGTH                FREERTOS_TIMER_QUEUE_LENGTH
#else
#define configTIMER_QUEUE_LENGTH                36
#endif
#ifdef FREERTOS_TIMER_TASK_STACK_DEPTH
#define configTIMER_TASK_STACK_DEPTH            FREERTOS_TIMER_TASK_STACK_DEPTH
#else
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE
#endif

/* Interrupt nesting behaviour configuration. */
#define configKERNEL_INTERRUPT_PRIORITY         ( ( unsigned char ) 7 << ( unsigned char ) 5 )  /* Priority 7, or 255 as only the top three bits are implemented.  This is the lowest priority. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( ( unsigned char ) 5 << ( unsigned char ) 5 )  /* Priority 5, or 160 as only the top three bits are implemented. */

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     0
#define INCLUDE_uxTaskGetStackHighWaterMark2    0
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xSemaphoreGetMutexHolder        1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  0
#define INCLUDE_xTaskResumeFromISR              1

#endif /* FREERTOS_CONFIG_H */
