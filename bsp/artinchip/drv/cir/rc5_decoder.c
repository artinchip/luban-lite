/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#include <stdint.h>
#define LOG_TAG         "RC5"
#include "ir_raw.h"

#define RC5_NBITS           14
#define RC5_UNIT            32 /* sample cycles number */
#define RC5_BIT_START       (1 * RC5_UNIT)
#define RC5_BIT_END         (1 * RC5_UNIT)
#define RC5_TRAILER         (10 * RC5_UNIT) /* In reality, approx 100 */
#define RC5_MARGIN_CYCLES   10

typedef enum rc5_state {
    RC5_STATE_INACTIVE,
    RC5_STATE_BIT_START,
    RC5_STATE_BIT_END,
    RC5_STATE_TRAILER,
}rc5_state;

int ir_rc5_decode(uint8_t * rx_data, uint32_t size, uint32_t *scancode)
{
    uint32_t i, j, data = 0, tmp_data = 0, data_count = 0;
    uint8_t command, system;
    rc5_state state = RC5_STATE_INACTIVE;
    for (i = 0; i < size; ) {
        tmp_data = rx_data[i] & 0x7F;
        for (j = i + 1; j < size; j++) {
            if ((rx_data[i] & 0x80) == (rx_data[j] & 0x80))
                tmp_data += (rx_data[j] & 0x7F);
            else
                break;
        }
again:
        switch (state) {
        case RC5_STATE_INACTIVE:
            if (cir_check_greater(tmp_data, RC5_UNIT, RC5_MARGIN_CYCLES))
                data_count = 1;
            else
                return -EINVAL;

            if (cir_check_in_range(tmp_data, 2 * RC5_UNIT, RC5_MARGIN_CYCLES)) {
                tmp_data -= RC5_UNIT;
                state = RC5_STATE_BIT_START;
                goto again;
            }

            if (cir_check_in_range(tmp_data, RC5_UNIT, RC5_MARGIN_CYCLES))
                state = RC5_STATE_BIT_START;
            break;
        case RC5_STATE_BIT_START:
            if (cir_check_greater(tmp_data, RC5_TRAILER, RC5_MARGIN_CYCLES)) {
                state = RC5_STATE_TRAILER;
                goto again;
            }

            if (!cir_check_in_range(tmp_data, RC5_UNIT, RC5_MARGIN_CYCLES))
                break;

            data <<= 1;
            if (!(rx_data[i] & 0x80))
                data |= 1;
            data_count++;
            state = RC5_STATE_BIT_END;
            break;
        case RC5_STATE_BIT_END:
            if (cir_check_in_range(tmp_data, 2 * RC5_UNIT, RC5_MARGIN_CYCLES)) {
                tmp_data -= RC5_UNIT;
                state = RC5_STATE_BIT_START;
                goto again;
            }

            if (cir_check_in_range(tmp_data, RC5_UNIT, RC5_MARGIN_CYCLES))
                state = RC5_STATE_BIT_START;
            break;
        case RC5_STATE_TRAILER:
            command = (data & 0x3f);
            system = (data & 0x7c0) >> 6;
            command += (data & 0x1000) ? 0 : 0x40;
            *scancode = (system << 8) | command;
            state = RC5_STATE_INACTIVE;
            break;
        }

        i = j;
    }

    return 0;
}

static uint32_t ir_rc5_scancode_to_raw(cir_protocol_t protocol,
                                       uint32_t scancode)
{
    uint32_t data, command, commandx, system;

    command = (scancode & 0x3f) >> 0;
    commandx = (scancode & 0x40) >> 6;
    system = (scancode & 0x1f00) >> 8;

    data = !commandx << 12 | system << 6 | command;
    return data;
}

int ir_raw_encode_rc5(uint32_t raw, uint8_t *tx_data)
{
    int i, tx_idx = 0;
    uint8_t isBit1, previous_level = 0;
    uint32_t duration;

    /* encode header pulse */
    tx_data[tx_idx] = (1 << 7) | RC5_UNIT;
    previous_level = tx_data[tx_idx] & 0x80;

    i = BIT(12);

    while (i > 0) {
        isBit1 = !!(raw & i);
        if (isBit1) {
            /* encode low level */
            if (previous_level == 0)
                tx_data[tx_idx] += RC5_UNIT;
            else {
                tx_idx++;
                tx_data[tx_idx] = RC5_UNIT;
                previous_level = tx_data[tx_idx] & 0x80;
            }

            /* encode high level */
            tx_idx++;
            tx_data[tx_idx] = (1 << 7) | RC5_UNIT;
            previous_level = tx_data[tx_idx] & 0x80;
        } else {
            /* encode high level */
            if (previous_level)
                tx_data[tx_idx] += RC5_UNIT;
            else {
                tx_idx++;
                tx_data[tx_idx] = (1 << 7) | RC5_UNIT;
                previous_level = tx_data[tx_idx] & 0x80;
            }

            /* encode low level */
            tx_idx++;
            tx_data[tx_idx] = RC5_UNIT;
            previous_level = tx_data[tx_idx] & 0x80;
        }

        i >>= 1;
    }

    /* encode trailer */
    duration = RC5_TRAILER;
    while (duration > 127) {
        tx_data[++tx_idx] = 0x7f;
        duration -= 127;
    }
    tx_data[++tx_idx] = duration;
    return tx_idx;
}

static int ir_rc5_encode(cir_protocol_t protocol, uint32_t scancode,
                         void *tx_data, uint32_t max)
{
    int ret;
    uint32_t raw;

    /* Convert a rc5 scancode to raw rc5 data */
    raw = ir_rc5_scancode_to_raw(protocol, scancode);
    ret = ir_raw_encode_rc5(raw, tx_data);
    if (ret > max)
        return -ENOBUFS;
    else
        return ret;
}

ir_raw_handler_t rc5_handler = {
    .protocol = CIR_PROTOCOL_RC5,
    .encode = ir_rc5_encode,
    .decode = ir_rc5_decode,
};

int ir_rc5_decode_init(void)
{
    LOG_I("CIR RC5 decoder register");
    ir_raw_protocol_register(&rc5_handler.list);
    return 0;
}
INIT_COMPONENT_EXPORT(ir_rc5_decode_init);

void ir_rc5_decode_uninit(void)
{
    ir_raw_protocol_unregister(&rc5_handler.list);
}
