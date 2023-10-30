/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     linpack_timer_port.c
 * @brief    the systimer for the linpack
 * @version  V1.0
 * @date     20. July 2016
 ******************************************************************************/
#include <sys_freq.h>
#include "drv_timer.h"
#include "soc.h"

/* APB frequence definition */
static uint32_t APB_FREQ;
static uint32_t TIMER_LOADTIMER;
static uint32_t TIMER_LOADCOUNT;

static unsigned int Timer_LoopCount = 0;

static timer_handle_t timer_handle;
static uint8_t timer_count_rise = 0;
/*
 * Callback function for TIMER0 interrupt, set timer_flag.
 */
static void timer_cb_fun(int32_t idx, timer_event_e event)
{
    Timer_LoopCount++;
}

unsigned  long long Timer_CurrentValue()
{
    unsigned int cv;

    csi_timer_get_current_value (timer_handle, &cv);

    if (timer_count_rise) {
        return (unsigned long long)(Timer_LoopCount) * (TIMER_LOADCOUNT + 1) + cv;
    } else {
        return (unsigned long long)(Timer_LoopCount + 1) * (TIMER_LOADCOUNT + 1) - cv -1;
    }
}

/*
 *start  systimer
 *
 */
void Timer_Open()
{
    timer_handle = csi_timer_initialize(0, timer_cb_fun);
    APB_FREQ = drv_get_timer_freq(0);
    TIMER_LOADTIMER  = 10000000; /* 10s */
    TIMER_LOADCOUNT  = TIMER_LOADTIMER * (APB_FREQ / 1000000);

    csi_timer_config(timer_handle, TIMER_MODE_RELOAD);
    csi_timer_set_timeout(timer_handle, TIMER_LOADTIMER);
}

void Timer_Start()
{
    csi_timer_start(timer_handle);
    unsigned int cv1, cv2;
    csi_timer_get_current_value (timer_handle, &cv1);
    csi_timer_get_current_value (timer_handle, &cv2);
    if (cv2 > cv1) {
        timer_count_rise = 1;
    }
}

void Timer_Stop()
{
    csi_timer_stop(timer_handle);
}

void Timer_Close()
{
    csi_timer_uninitialize(timer_handle);
}

/* time(uS) since run */
unsigned long long  clock()
{

    unsigned long long  systimer_val, systimer_us;
    systimer_val = Timer_CurrentValue();
    systimer_us  = systimer_val * 1000000/ APB_FREQ;
    return  systimer_us;
}

/* run time since run (seconds)*/
 long time()
{
    unsigned long long systimer_val;
    systimer_val = Timer_CurrentValue();
    return (long)(systimer_val / APB_FREQ);
}

