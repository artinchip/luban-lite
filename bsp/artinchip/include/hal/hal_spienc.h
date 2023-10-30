/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Hao Xiong <hao.xiong@artinchip.com>
 */

#ifndef _AIC_HAL_SPIENC_H_
#define _AIC_HAL_SPIENC_H_

#include <aic_core.h>

#define AIC_SPIENC_USER_TWEAK 0
#define AIC_SPIENC_HW_TWEAK   1

#define IOC_TYPE_SPIE 'E'
#define AIC_SPIENC_IOCTL_CRYPT_CFG \
    _IOW(IOC_TYPE_SPIE, 0x10, struct spienc_crypt_cfg)
#define AIC_SPIENC_IOCTL_START        _IOW(IOC_TYPE_SPIE, 0x11, u32)
#define AIC_SPIENC_IOCTL_STOP         _IOW(IOC_TYPE_SPIE, 0x12, u32)
#define AIC_SPIENC_IOCTL_CHECK_EMPTY  _IOW(IOC_TYPE_SPIE, 0x13, u32)
#define AIC_SPIENC_IOCTL_TWEAK_SELECT _IOW(IOC_TYPE_SPIE, 0x14, u32)

int hal_spienc_init(void);
void hal_spienc_set_cfg(u32 spi_bus, u32 addr, u32 cpos, u32 clen);
void hal_spienc_start(void);
void hal_spienc_stop(void);
int hal_spienc_check_empty(void);

#endif
