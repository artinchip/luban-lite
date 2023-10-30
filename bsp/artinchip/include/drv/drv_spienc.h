/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Hao Xiong <hao.xiong@artinchip.com>
 */

#ifndef _AIC_DRV_SPIENC_H_
#define _AIC_DRV_SPIENC_H_

#include <hal_spienc.h>

int drv_spienc_init(void);
void drv_spienc_set_cfg(u32 spi_bus, u32 addr, u32 cpos, u32 clen);
void drv_spienc_start(void);
void drv_spienc_stop(void);
int drv_spienc_check_empty(void);

#endif
