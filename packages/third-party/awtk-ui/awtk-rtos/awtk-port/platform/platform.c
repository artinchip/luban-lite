/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include <aic_core.h>
#include <aic_time.h>
#include "sys_freq.h"
#include "tkc/mem.h"
#include "base/timer.h"
#include "main_loop/main_loop_simple.h"

extern void aic_date_time_init(void);

static ret_t app_res_root_is_sdcard() {
#ifdef APP_RES_ROOT
  if (strstr(APP_RES_ROOT, "/sdcard") != NULL)
    return RET_OK;
#endif
  return RET_FAIL;
}

uint64_t get_time_us64() {
  return aic_get_time_us64();
}

uint64_t get_time_ms64(void) {
  return aic_get_time_ms64();
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

#ifndef HAS_STD_MALLOC
#define TK_HEAP_MEM_SIZE  1024 * 1024
  static uint32_t s_heap_mem[TK_HEAP_MEM_SIZE / 4];
  tk_mem_init(s_heap_mem, sizeof(s_heap_mem));
#endif /*HAS_STD_MALLOC*/

  aic_date_time_init();

/* boot up awtk, and if the resources are on the SD card, wait for SD card to be automatically mounted */
#ifdef AWTK_START_UP
  if (app_res_root_is_sdcard() == RET_OK)
    sleep_ms(500);
#endif
  return RET_OK;
}

uint8_t platform_disaptch_input(main_loop_t* loop) {
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
