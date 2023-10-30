/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#include <stdint.h>
#include <aic_core.h>
#include "ir_raw.h"

static rt_slist_t cir_raw_handler_list;

int ir_raw_encode_scancode(cir_protocol_t protocol, uint32_t scancode,
                           void *tx_data, uint32_t max)
{
    ir_raw_handler_t *handler = NULL;
    int ret = -EINVAL;
    rt_slist_for_each_entry(handler, &cir_raw_handler_list, list) {
        if (handler && handler->protocol == protocol) {
            ret = handler->encode(protocol, scancode, tx_data, max);
            if (ret >= 0 || ret == -ENOBUFS)
                break;
        }
    }
    return ret;
}

int ir_raw_decode_scancode(cir_protocol_t protocol, uint8_t * rx_data,
                           uint32_t size, uint32_t *scancode)
{
    ir_raw_handler_t *handler = NULL;
    int ret = -EINVAL;
    rt_slist_for_each_entry(handler, &cir_raw_handler_list, list) {
        if (handler && handler->protocol == protocol) {
            ret = handler->decode(rx_data, size, scancode);
            if (ret)
                return ret;
        }
    }
    return ret;
}

uint8_t cir_check_in_range(uint32_t raw_time, uint32_t target_time,
                           uint32_t margin_ticks)
{
    return (raw_time < (target_time + margin_ticks)) &&
           (raw_time > (target_time - margin_ticks));
}

uint8_t cir_check_greater(uint32_t raw_time, uint32_t target_time,
                          uint32_t margin_ticks)
{
    return raw_time > (target_time - margin_ticks);
}

void ir_raw_protocol_register(rt_slist_t *node)
{
    rt_slist_append(&cir_raw_handler_list, node);
}

void ir_raw_protocol_unregister(rt_slist_t *node)
{
    rt_slist_remove(&cir_raw_handler_list, node);
}
