/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include <aic_core.h>

#include "tkc/thread.h"
#include "platforms/common/rtos.h"

extern int gui_app_start(int lcd_w, int lcd_h);
extern ret_t platform_prepare(void);

void awtk_thread(void* args) {
  gui_app_start(1024, 600);

  return NULL;
}

static void awtk_start_ui_thread(void) {
  rt_thread_t ui_thread;
  ui_thread = rt_thread_create("awtk-ui",
                               awtk_thread,
                               NULL,
                               TK_UI_THREAD_STACK_SIZE,
                               TK_UI_THREAD_PRIORITY,
                               TK_UI_THREAD_TICK);
  rt_thread_startup(ui_thread);
}

int awtk_init(int argc, char **argv) {
  platform_prepare();
  awtk_start_ui_thread();

  return 0;
}

#ifdef AWTK_START_UP
INIT_APP_EXPORT(awtk_init);
#else
MSH_CMD_EXPORT_ALIAS(awtk_init, test_awtk, awtk chart demo);
#endif
