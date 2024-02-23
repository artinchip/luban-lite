/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#ifndef _AIC_TK_TOUCH_THREAD_H
#define _AIC_TK_TOUCH_THREAD_H

#include "base/main_loop.h"
#include "main_loop/main_loop_simple.h"
#include "tkc/thread.h"
#include "input_dispatcher.h"

BEGIN_C_DECLS

int tk_touch_run(const char *name, main_loop_t* ctx);

END_C_DECLS

#endif /*_AIC_TK_TOUCH_THREAD_H*/
