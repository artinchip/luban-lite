/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#include <stdint.h>
#define LOG_TAG         "NEC"
#include "ir_raw.h"

#define NEC_NBITS       32
#define NEC_UNIT        21  /* sample cycles number */
#define NEC_HEADER_PULSE    342
#define NEC_HEADER_SPACE    171
#define NEC_REPEAT_SPACE    86
#define NEC_BIT_PULSE       NEC_UNIT
#define NEC_BIT_0_SPACE     NEC_UNIT
#define NEC_BIT_1_SPACE     64
#define NEC_TRAILER_PULSE   NEC_UNIT
#define NEC_TRAILER_SPACE   (10 * NEC_UNIT) /* even longer in reality */
#define NEC_MARGIN_CYCLES   8

enum nec_state {
    STATE_INACTIVE,
    STATE_HEADER_SPACE,
    STATE_BIT_PULSE,
    STATE_BIT_SPACE,
    STATE_TRAILER_PULSE,
    STATE_TRAILER_SPACE,
};

static inline uint32_t ir_nec_bytes_to_scancode(uint8_t address,
                                                uint8_t not_address,
                                                uint8_t command,
                                                uint8_t not_command)
{
    uint32_t scancode;

    /* Normal NEC */
    scancode = address << 8 | command;
    return scancode;
}

int ir_nec_decode(uint8_t * rx_data, uint32_t size, uint32_t *scancode)
{
    uint8_t previous_level = 0;
    static uint32_t last_scancode = 0;
    uint32_t i, data = 0, tmp_data = 0, data_count = 0;
    uint8_t address, not_address, command, not_command;
    enum nec_state state = STATE_INACTIVE;
    for (i = 0; i < size; i++) {
        if ((rx_data[i] & 0x80) == previous_level) {
            /* Level not flip */
            tmp_data += (rx_data[i] & 0x7F);
        } else {
            /* Level flip */
            switch(state) {
            case STATE_INACTIVE:
                if (cir_check_in_range(tmp_data, NEC_HEADER_PULSE,
                                       NEC_MARGIN_CYCLES))
                    state = STATE_HEADER_SPACE;
                break;
            case STATE_HEADER_SPACE:
                if (cir_check_in_range(tmp_data, NEC_HEADER_SPACE,
                                       NEC_MARGIN_CYCLES))
                    state = STATE_BIT_PULSE;
                else if (cir_check_in_range(tmp_data, NEC_REPEAT_SPACE,
                                       NEC_MARGIN_CYCLES))
                    state = STATE_TRAILER_PULSE;
                break;
            case STATE_BIT_PULSE:
                if (cir_check_in_range(tmp_data, NEC_BIT_PULSE,
                                       NEC_MARGIN_CYCLES))
                    state = STATE_BIT_SPACE;
                break;
            case STATE_BIT_SPACE:
                data <<= 1;
                if (cir_check_in_range(tmp_data, NEC_BIT_1_SPACE,
                                       NEC_MARGIN_CYCLES))
                    data |= 1;
                else if (!cir_check_in_range(tmp_data, NEC_BIT_0_SPACE,
                                             NEC_MARGIN_CYCLES))
                    break;
                data_count++;

                if (data_count == NEC_NBITS)
                    state = STATE_TRAILER_PULSE;
                else
                    state = STATE_BIT_PULSE;
                break;
            case STATE_TRAILER_PULSE:
                if (cir_check_in_range(tmp_data, NEC_TRAILER_PULSE,
                                       NEC_MARGIN_CYCLES))
                    state = STATE_TRAILER_SPACE;
                break;
            case STATE_TRAILER_SPACE:
                if (cir_check_in_range(tmp_data, NEC_TRAILER_SPACE,
                                       NEC_MARGIN_CYCLES))
                    state = STATE_INACTIVE;
            }

            tmp_data = rx_data[i] & 0x7F;
            previous_level = rx_data[i] & 0x80;
        }
    }

    if (i == size && state == STATE_TRAILER_SPACE) {
        if (cir_check_in_range(tmp_data, NEC_TRAILER_SPACE, NEC_MARGIN_CYCLES))
                    state = STATE_INACTIVE;
        if (data_count == NEC_NBITS) {
            address = bitrev8((data >> 24) & 0xff);
            not_address = bitrev8((data >> 16) & 0xff);
            command = bitrev8((data >> 8) & 0xff);;
            not_command = bitrev8((data >> 0) & 0xff);

            *scancode = ir_nec_bytes_to_scancode(address, not_address,
                                                 command, not_command);
            last_scancode = *scancode;
            return 0;
        } else {
            *scancode = last_scancode;
            return 0;
        }
    }

    return -EINVAL;
}

static uint32_t ir_nec_scancode_to_raw(cir_protocol_t protocol,
                                       uint32_t scancode)
{
    uint8_t addr, addr_inv, data, data_inv;

    /* Normal NEC */
    /* scan encoding: AADD */
    data = scancode & 0xff;
    addr       = (scancode >>  8) & 0xff;
    addr_inv   = addr ^ 0xff;
    data_inv   = data ^ 0xff;

    /* raw encoding: ddDDaaAA */
    return data_inv << 24 |
           data     << 16 |
           addr_inv <<  8 |
           addr;
}

int ir_raw_encode_nec(uint32_t raw, uint8_t *tx_data)
{
    int i, tx_idx = 0, duration;
    /* encode header pulse */
    duration = NEC_HEADER_PULSE;
    while (duration > 127) {
        tx_data[tx_idx++] = 0xff;
        duration -= 127;
    }
    tx_data[tx_idx++] = (1 << 7) | duration;

    /* encode header space */
    duration = NEC_HEADER_SPACE;
    while (duration > 127) {
        tx_data[tx_idx++] = 0x7f;
        duration -= 127;
    }
    tx_data[tx_idx++] = duration;

    /* encode address and cmd */
    for (i = 0; i < 32; i++) {
        if ((raw >> i) & 1) {
            /* bit 1 */
            tx_data[tx_idx++] = (1 << 7) | NEC_BIT_PULSE;
            tx_data[tx_idx++] = NEC_BIT_1_SPACE;
        } else {
            /* bit 0 */
            tx_data[tx_idx++] = (1 << 7) | NEC_BIT_PULSE;
            tx_data[tx_idx++] = NEC_BIT_0_SPACE;
        }
    }

    /* encode trailer pulse */
    tx_data[tx_idx++] = (1 << 7) | NEC_TRAILER_PULSE;

    /* encode trailer space */
    duration = NEC_TRAILER_SPACE;
    while (duration > 127) {
        tx_data[tx_idx++] = 0x7f;
        duration -= 127;
    }
    tx_data[tx_idx++] = duration;

    return tx_idx;
}

static int ir_nec_encode(cir_protocol_t protocol, uint32_t scancode,
                         void *tx_data, uint32_t max)
{
    int ret;
    uint32_t raw;

    /* Convert a NEC scancode to raw NEC data */
    raw = ir_nec_scancode_to_raw(protocol, scancode);
    ret = ir_raw_encode_nec(raw, (uint8_t *)tx_data);
    if (ret > max)
        return -ENOBUFS;
    else
        return ret;
}

ir_raw_handler_t nec_handler = {
    .protocol = CIR_PROTOCOL_NEC,
    .encode = ir_nec_encode,
    .decode = ir_nec_decode,
};

int ir_nec_decode_init(void)
{
    LOG_I("CIR NEC decoder register");
    ir_raw_protocol_register(&nec_handler.list);
    return 0;
}
INIT_COMPONENT_EXPORT(ir_nec_decode_init);

void ir_nec_decode_uninit(void)
{
    ir_raw_protocol_unregister(&nec_handler.list);
}
