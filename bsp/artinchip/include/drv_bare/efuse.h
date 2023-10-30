/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef _AIC_EFUSE_H__
#define _AIC_EFUSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>

int efuse_init(void);
int efuse_read(u32 addr, void *data, u32 size);
int efuse_program(u32 addr, const void *data, u32 size);

#ifdef __cplusplus
}
#endif

#endif
