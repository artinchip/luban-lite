/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-10-18     Meco Man     The first version
 * 2023-05-102    Ning Fang    Add ArtInChip input event
 */
#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lv_hal_indev.h>

void lv_port_indev_init(void);
void aic_touch_inputevent_cb(rt_int16_t x, rt_int16_t y, rt_uint8_t state);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
