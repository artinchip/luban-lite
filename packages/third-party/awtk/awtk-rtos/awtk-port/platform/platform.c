/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

/**
 * History:
 * ================================================================
 * 2023-10-16 Zequan Liang <zequan.liang@artinchip.com> created
 * 2023-10-23 Zequan Liang <zequan.liang@artinchip.com> touch device adaptation
 * 2023-10-24 Zequan Liang <zequan.liang@artinchip.com> system time adaptation
 *
 */

#include <aic_core.h>
#include <aic_time.h>
#include "tkc/mem.h"
#include "base/timer.h"
#include "hwtimer.h"
#include "date_time.h"
#include "main_loop/main_loop_simple.h"

static int using_hw_timer;
uint64_t get_time_ms64() {
  uint64_t time_ms = 0;
  if (using_hw_timer)
    time_ms = (uint64_t)hw_get_time_ms64();
  else
    time_ms = aic_get_time_ms(); /* about 49 days to overflow */
  return time_ms;
}

void sleep_ms(uint32_t ms) {
  rt_thread_mdelay(ms);
}

ret_t platform_prepare(void) {
  static bool_t inited = FALSE;
  if (inited) {
    return RET_OK;
  }
  inited = TRUE;

  if (hwtimer_init() == 0) {
    using_hw_timer = 1;
  }
#ifndef HAS_STD_MALLOC
#define TK_HEAP_MEM_SIZE  1024 * 1024
  static uint32_t s_heap_mem[TK_HEAP_MEM_SIZE / 4];
  tk_mem_init(s_heap_mem, sizeof(s_heap_mem));
#endif /*HAS_STD_MALLOC*/

  aic_date_time_init();

  return RET_OK;
}

uint8_t platform_disaptch_input(main_loop_t* loop)
{
  static bool init_touch = FALSE;

  if (init_touch == TRUE)
    return RET_OK;
#ifdef AIC_TOUCH_PANEL_GT911
  tk_touch_run(AIC_TOUCH_PANEL_GT911_NAME, loop);
#endif

  init_touch = TRUE;
  return RET_OK;
}

#include "main_loop/main_loop_raw.inc"
