/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_EFUSE_H__
#define _ARTINCHIP_HAL_EFUSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>

int hal_efuse_init(void);
int hal_efuse_deinit(void);
int hal_efuse_wait_ready(void);
int hal_efuse_read(u32 wid, u32 *wval);
int hal_efuse_write(u32 wid, u32 wval);

#ifdef __cplusplus
}
#endif

#endif
