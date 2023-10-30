/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#ifndef _IR_RAW_H_
#define _IR_RAW_H_

#include <stdint.h>
#include <rtthread.h>
#include "drv_cir.h"

typedef struct ir_raw_handler {
	rt_slist_t list;
	cir_protocol_t protocol;
	int (*decode)(uint8_t * rx_data, uint32_t size, uint32_t *scancode);
	int (*encode)(cir_protocol_t protocol, uint32_t scancode,
		          void *tx_data, uint32_t max);
} ir_raw_handler_t;

int ir_raw_encode_scancode(cir_protocol_t protocol, uint32_t scancode,
                           void *tx_data, uint32_t max);
int ir_raw_decode_scancode(cir_protocol_t protocol, uint8_t * rx_data,
                           uint32_t size, uint32_t *scancode);
uint8_t cir_check_in_range(uint32_t raw_time, uint32_t target_time,
                           uint32_t margin_ticks);
uint8_t cir_check_greater(uint32_t raw_time, uint32_t target_time,
                          uint32_t margin_ticks);
void ir_raw_protocol_register(rt_slist_t *node);
void ir_raw_protocol_unregister(rt_slist_t *node);

#endif /* _IR_RAW_H_ */
