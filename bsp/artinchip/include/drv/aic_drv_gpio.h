/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_DRV_GPIO_H__
#define __AIC_DRV_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "aic_hal_gpio.h"

void drv_pin_bias_set(unsigned int pin, unsigned int pull);
void drv_pin_drive_set(unsigned int pin, unsigned int strength);
void drv_pin_mux_set(unsigned int pin, unsigned int func);
unsigned int drv_pin_mux_get(unsigned int pin);
long drv_pin_get(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* __AIC_LL_GPIO_H__ */
