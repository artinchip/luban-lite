/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Hao Xiong <hao.xiong@artinchip.com>
 */

#include <aic_core.h>
#include <aic_hal.h>
#include <hal_spienc.h>
#include <drv_spienc.h>

int drv_spienc_init(void)
{
    return hal_spienc_init();
}

void drv_spienc_set_cfg(u32 spi_bus, u32 addr, u32 cpos, u32 clen)
{
    hal_spienc_set_cfg(spi_bus, addr, cpos, clen);
}

void drv_spienc_start(void)
{
    hal_spienc_start();
}

void drv_spienc_stop(void)
{
    hal_spienc_stop();
}

int drv_spienc_check_empty(void)
{
    return hal_spienc_check_empty();
}

