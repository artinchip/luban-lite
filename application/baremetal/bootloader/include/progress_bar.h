/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PROGRESS_BAR__
#define __PROGRESS_BAR__

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>

void aicfb_draw_bar(unsigned int value);

#ifdef __cplusplus
}
#endif

#endif /* __PROGRESS_BAR__ */
