/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <driver.h>
#include <uart.h>
#include <upg_uart.h>
#include <trans_rw_data.h>
#include <hexdump.h>
#include <crc16.h>

#ifndef AICUPG_UART_DEBUG
#undef pr_debug
#define pr_debug(fmt, ...)
#else
#undef pr_debug
#define pr_debug pr_err
#endif

static u8 g_uart_send_recv_buf[LNG_FRM_MAX_LEN];
static char uart_recv_buf[UART_RECV_BUF_LEN];
static int recv_buf_in_idx = 0;
static int recv_buf_out_idx = 0;

static int uart_conn_detect_proc(void);
static int uart_conn_normal_proc(void);
static int read_short_frame_data(u8 *buf, int len);
static int read_long_frame_data(u8 *buf, int len);
static int aic_upg_uart_recv(u8 *buf, int len);
static int aic_upg_uart_send(u8 *buf, int len);

struct upg_uart_device uart_upg;
static int upg_uart_id = 0;
static int update_baudrate = 0;

void recv_buf_init(void)
{
    recv_buf_in_idx = 0;
    recv_buf_out_idx = 0;
}

void recv_buf_reset(void)
{
    recv_buf_in_idx = 0;
    recv_buf_out_idx = 0;
}

int recv_buf_read_byte(int id)
{
    int newlen, ch;

    newlen = recv_buf_in_idx - recv_buf_out_idx;
    if (newlen <= 0) {
        /* No data in buffer, read it from UART port */
        ch = uart_getchar(id);
        if (ch != -1)
            update_last_recv_time();
        return ch;
    }

    ch = uart_recv_buf[recv_buf_out_idx];
    recv_buf_out_idx++;
    if ((ch == SIG_C) && (recv_buf_in_idx >= RESET_POS) && (recv_buf_out_idx ==
                recv_buf_in_idx)) {
        /* index reach reset position and no data in buffer, reset it */
        recv_buf_reset();
    }

    return ch;
}

int uart_getbytes(int id, char *buf, int len)
{
    int i = 0, ch;

    while (i < len) {
        ch = uart_getchar(id);
        if (ch == -1)
            break;
        buf[i++] = (char)ch;
    }

    return i;
}

int uart_putbytes(int id, char *buf, int len)
{
    int i = 0;

    while (i < len) {
        uart_putchar(id, buf[i++]);
    }

    return i;
}

#ifdef AICUPG_UART_DEBUG
static char *get_byte_str(u8 ch)
{
    switch (ch) {
    case SOH:
        return "SOH";
    case STX:
        return "STX";
    case ACK:
        return "ACK";
    case DC1_SEND:
        return "DC1_SEND(Dev RECV)";
    case DC2_RECV:
        return "DC2_RECV(Dev SEND)";
    case NAK:
        return "NAK";
    case CAN:
        return "CAN";
    case SIG_A:
        return "SIG_A";
    case SIG_C:
        return "SIG_C";
    default:
        return "";
    }

    return "";
}
#endif

static void send_byte(int id, u8 ch)
{
    pr_debug("---> Send 0x%x %s\n", ch, get_byte_str(ch));
    uart_putchar(id, ch);
    uart_upg.last_send_time = aic_get_time_ms();
}

int recv_buf_read(u8 *buf, int len)
{
    int newlen;

    newlen = recv_buf_in_idx - recv_buf_out_idx;
    if (newlen < len) {
        pr_debug("Data is not enough.\n");
        return 0;
    }

    memcpy(buf, &uart_recv_buf[recv_buf_out_idx], len);
    recv_buf_out_idx += len;

    if (buf[0] == SOH || buf[0] == STX)
        recv_buf_reset(); /* Reset it when got one frame */

    pr_debug("Read %d\n", len);

    return len;
}

int recv_buf_peek(struct dma_input *in)
{
    int newlen, rdlen, timeout = 100;
    bool wait = false;
    volatile u64 start, cur;

    /* Read all data in uart fifo as fast as posible */
    if (recv_buf_in_idx >= UART_RECV_BUF_LEN) {
        pr_err("error, recv_buf_in_idx %d\n", recv_buf_in_idx);
        return -1;
    }

    start = aic_get_time_us();
    do {
        rdlen = uart_getbytes(upg_uart_id, &uart_recv_buf[recv_buf_in_idx], UART_RECV_BUF_LEN - recv_buf_in_idx);
        if (rdlen) {
            recv_buf_in_idx += rdlen;
            update_last_recv_time();
            pr_debug("in %d, i %d, o %d\n", rdlen, recv_buf_in_idx, recv_buf_out_idx);
        }

        newlen = recv_buf_in_idx - recv_buf_out_idx;
        if (newlen <= 0) {
            /* No new data */
            break;
        }

        cur = aic_get_time_us();
        /* Tell caller some key information */
        in->head = (u8)uart_recv_buf[recv_buf_out_idx];
        if (in->head == SOH) {
            if (newlen < 4) {
                /* Got frame, but data is not all arrived */
                wait = true;
                in->frm_len = 0;
                continue;
            }

            in->frm_len = (u8)uart_recv_buf[recv_buf_out_idx + 3];
            if (newlen < (in->frm_len + 6)) {
                /* Got frame, but data is not all arrived */
                wait = true;
                in->frm_len = 0;
                continue;
            }

            wait = false; /* all data recieved */
        } else if (in->head == STX) {
            if (newlen < LNG_FRM_MAX_LEN) {
                /* Got frame, but data is not all arrived */
                wait = true;
                continue;
            }
            in->frm_len = LNG_FRM_DLEN;
            wait = false; /* all data recieved */
        } else if (in->head != DC1_SEND) {
            /* Status is wrong, debug checking point */
            pr_err("Expect DC1_SEND but got 0x%x\n", in->head);
            hexdump((unsigned char*)&uart_recv_buf[recv_buf_out_idx], newlen, 1);
        }
    } while (wait && ((cur - start) < timeout));

    return newlen;
}

static int uart_conn_detect_proc(void)
{
    u64 delta, cur_tm;
    char ch = 0;

    ch = (char)recv_buf_read_byte(upg_uart_id);
    switch (ch) {
    case SIG_C:
        pr_debug("Got C from host, send ACK.\n");
        send_byte(upg_uart_id, ACK);
        uart_upg.state = CONN_STATE_CONNECTED;
        uart_upg.proc = uart_conn_normal_proc;
        break;
    case ACK:
        pr_debug("Got ACK for A?\n");
        if (uart_upg.wait_ack) {
            pr_debug("Yes. connected.\n");
            uart_upg.wait_ack = 0;
            uart_upg.state = CONN_STATE_CONNECTED;
            uart_upg.proc = uart_conn_normal_proc;
        }
        break;
    default:
        /* Unknown data and no data */
        cur_tm = aic_get_time_ms();
        delta = cur_tm - uart_upg.last_send_time;
        /* Timeout checking */
        if (uart_upg.last_send_time == 0 || delta >= SIG_A_INTERVAL_MS) {
            pr_debug("Silence for a long time, send A to host.\n");
            send_byte(upg_uart_id, SIG_A);
            uart_upg.wait_ack = 1;
        }
        break;
    }
    return 0;
}

static int uart_conn_normal_proc(void)
{
    u8 ch, frame[LNG_FRM_MAX_LEN];
    int cnt, ret, tmo_check;
    u64 delta, cur_tm;
    struct dma_input in;
    struct phy_data_rw rw;

    tmo_check = 0;
    memset(&in, 0, sizeof(in));
    cnt = recv_buf_peek(&in);
    if ((in.head == SOH || in.head == STX) && in.frm_len == 0)
        tmo_check = 1;
    if (cnt <= 0)
        tmo_check = 1;
    if (tmo_check) {
        /* No data, maybe need to check connection */
        cur_tm = aic_get_time_ms();
        delta = cur_tm - uart_upg.last_recv_time;
        if (delta >= 3000) {
            pr_err("Long time no data input, need to check the connection.\n");
            uart_upg.state = CONN_STATE_DETECTING;
            uart_upg.proc = uart_conn_detect_proc;
            recv_buf_reset(); /* Clear data */
        }
        return -1;
    }
    ch = in.head;

    switch (ch) {
    case SOH:
        if (uart_upg.direction == DIR_HOST_RECV) {
            recv_buf_reset();
            send_byte(upg_uart_id, CAN);
            break;
        }
        if (in.frm_len <= 0) /* Frame data recv not finish */
            break;
        ret = read_short_frame_data(frame, in.frm_len + 6);
        if (ret == UART_NO_DATA) {
            break;
        } else if (ret == UART_ERR_DATA) {
            ch = NAK;
        } else if (ret == UART_SKIP_DATA) {
            ch = ACK;
        } else {
            pr_debug("Recv %d\n", ret);
            ch = ACK;
        }

        send_byte(upg_uart_id, ch);
        pr_debug("Ack SOH frame.\n");
        if (ret > 0) {
            rw.recv = aic_upg_uart_recv;
            rw.send = aic_upg_uart_send;
            trans_layer_rw_proc(&rw, &frame[4], ret);
        }
        break;
    case STX:
        if (uart_upg.direction == DIR_HOST_RECV) {
            recv_buf_reset();
            send_byte(upg_uart_id, CAN);
            pr_debug("Send CAN.\n");
            break;
        }
        if (in.frm_len <= 0) /* Frame data recv not finish */
            break;
        ret = read_long_frame_data(frame, in.frm_len + 5);
        if (ret == UART_NO_DATA) {
            break;
        } else if (ret == UART_ERR_DATA) {
            ch = NAK;
        } else if (ret == UART_SKIP_DATA) {
            ch = ACK;
        } else {
            pr_debug("Recv %d\n", ret);
            ch = ACK;
        }

        send_byte(upg_uart_id, ch);
        pr_debug("Ack STX frame.\n");
        if (ret > 0) {
            rw.recv = aic_upg_uart_recv;
            rw.send = aic_upg_uart_send;
            trans_layer_rw_proc(&rw, &frame[3], ret);
        }
        break;
    case ACK:
        ch = recv_buf_read_byte(upg_uart_id);
        /* Just skip it */
        break;
    case DC1_SEND:
        uart_upg.direction = DIR_HOST_SEND;
        ch = recv_buf_read_byte(upg_uart_id);
        send_byte(upg_uart_id, ACK);
        pr_debug("Host switch to send mode.\n");
        break;
    case DC2_RECV:
        uart_upg.direction = DIR_HOST_RECV;
        ch = recv_buf_read_byte(upg_uart_id);
        send_byte(upg_uart_id, ACK);
        pr_debug("Host switch to recv mode.\n");
        break;
    case SIG_C:
        ch = recv_buf_read_byte(upg_uart_id);
        send_byte(upg_uart_id, ACK);
        pr_debug("Ack SIG_C\n");
        break;
    default:
        ch = recv_buf_read_byte(upg_uart_id);
        break;
    }

    if (update_baudrate) {
        if (uart_config_update(upg_uart_id, update_baudrate)) {
            pr_err("update uart baud rate %d failed.\n", update_baudrate);
            update_baudrate = 0;
            return -1;
        }
        update_baudrate = 0;
    }

    return 0;
}

static int wait_host_to_recv_mode(void)
{
    int retry = 1000;
    u8 ch = 0;

    while (retry) {
        /*
         * Two cases:
         * 1. Host is switching to RECV mode, it should send DC2_RECV to device
         * 2. Host is staying at SEND mode, here got other data, return error.
         */
        ch = recv_buf_read_byte(upg_uart_id);
        if ((char)ch != 0xFF && ch == DC2_RECV) {
            send_byte(upg_uart_id, ACK);
            uart_upg.direction = DIR_HOST_RECV;
            pr_debug("Host switch to recv mode.\n");
            break;
        }
        if ((char)ch == 0xFF) {
            pr_debug("Waiting DC2_RECV 0x12, but Got 0x%x\n", ch);
        }
        retry--;
        aicos_mdelay(1);
    }

    if (uart_upg.direction == DIR_HOST_RECV)
        return 0;

    return -1;
}

static int pack_frame(u8 blk, u8 *frame, u8 *data, int len)
{
    u16 crc16;
    int flen;

    if (len == LNG_FRM_DLEN) {
        pr_debug("pack long frame\n");
        frame[0] = STX;
        frame[1] = blk;
        frame[2] = 255 - blk;
        memcpy(&frame[3], data, len);
        crc16 = crc16_ccitt(0, data, len);
        frame[LNG_FRM_DLEN + 3] = (u8)(crc16 >> 8);
        frame[LNG_FRM_DLEN + 4] = (u8)(crc16 & 0xFF);
        flen = len + 5;
    } else if (len <= SHT_FRM_DLEN) {
        pr_debug("pack short frame\n");
        frame[0] = SOH;
        frame[1] = blk;
        frame[2] = 255 - blk;
        frame[3] = (u8)len;
        memcpy(&frame[4], data, len);
        crc16 = crc16_ccitt(0, data, len);
        frame[len + 4] = (u8)(crc16 >> 8);
        frame[len + 5] = (u8)(crc16 & 0xFF);
        flen = len + 6;
    } else {
        return 0;
    }

    return flen;
}

int aic_upg_uart_send(u8 *buf, int len)
{
    u8 ch, *pframe, *p;
    int ret, rest, slice, flen, resend_cnt;
    volatile u64 start, cur;

    pframe = g_uart_send_recv_buf;
    if (wait_host_to_recv_mode()) {
        pr_debug("Failed to send data because not switch to HOST RECV mode.\n");
        return -1;
    }

    p = buf;
    rest = len;
    resend_cnt = 0;
    while (rest > 0) {
        if (rest > LNG_FRM_DLEN)
            slice = LNG_FRM_DLEN;
        else if (rest > SHT_FRM_DLEN)
            slice = SHT_FRM_DLEN;
        else
            slice = rest;
        flen = pack_frame(uart_upg.send_blk_no, pframe, p, slice);
resend:
        if (resend_cnt >= 10) {
            pr_debug("Failed to read ACK, and resend not working.\n");
            break;
        }

        ret = uart_putbytes(upg_uart_id, (char *)pframe, flen);
        if (ret != flen) {
            pr_err("write frame data error.\n");
            break;
        }
        pr_debug("frame data is written.\n");
read_ack:
        start = aic_get_time_ms();
        do {
            ch = recv_buf_read_byte(upg_uart_id);
            cur = aic_get_time_ms();
        } while (ch == 0xFF && (cur - start) < 200);

        if ((cur - start) >= 200) {
            pr_err("Failed to read ACK, resend data.\n");
            resend_cnt++;
            goto resend;
        }

        if (ch == DC2_RECV) {
            pr_err("Got DC2_RECV when waiting ACK.\n");
            send_byte(upg_uart_id, ACK);
            goto read_ack;
        }

        if (ch != ACK) {
            pr_err("Expect ACK but got 0x%x\n", ch);
            hexdump(pframe, slice, 1);
            resend_cnt++;
            goto resend;
        }
        rest -= slice;
        p += slice;
        uart_upg.send_blk_no++;
    }
    pr_debug("Send out data len %d\n", len - rest);
    return len - rest;
}

static int wait_host_to_send_mode(void)
{
    int retry = 1000;
    u8 ch;

    if (uart_upg.direction == DIR_HOST_SEND)
        return 0;

    while (retry) {
        /* Two cases:
		 * 1. Host is switching to SEND mode, it should send DC1_SEND to device
		 * 2. Host is staying at RECV mode, it should not send out any data
		 */
        ch = recv_buf_read_byte(upg_uart_id);
        if ((char)ch != 0xFF && ch == DC1_SEND) {
            send_byte(upg_uart_id, ACK);
            uart_upg.direction = DIR_HOST_SEND;
            pr_debug("Host switch to send mode.\n");
            break;
        } else {
            pr_err("Send CAN\n");
            send_byte(upg_uart_id, CAN);
            recv_buf_reset();
        }
        retry--;
        aicos_mdelay(1);
    }
    if (uart_upg.direction == DIR_HOST_SEND)
        return 0;
    return -1;
}

static int aic_upg_uart_recv(u8 *buf, int len)
{
    u8 ch, *pframe, *p;
    int ret, gotlen, timeout = 10000;
    volatile u64 start, cur;
    struct dma_input in;

    pframe = g_uart_send_recv_buf;
    if (wait_host_to_send_mode()) {
        pr_err("wait host to send mode failed.\n");
        return -1;
    }

    gotlen = 0;
    p = buf;
    start = aic_get_time_ms();
    while (gotlen < len) {
        /* Peek the input buffer */
        memset(&in, 0, sizeof(in));
        ret = recv_buf_peek(&in);
        if (ret == 0) {
            /* Waiting for data */
            cur = aic_get_time_ms();
            if ((cur - start) > timeout) {
                pr_err("Recv uart frame timeout, got len %d.\n", gotlen);
                return -1;
            }
            continue;
        }
        if (in.head == DC1_SEND) {
            ch = recv_buf_read_byte(upg_uart_id); /* Consume this byte */
            send_byte(upg_uart_id, ACK);
            pr_debug("Host switch to send mode.\n");
            continue;
        }
        if (in.head != SOH && in.head != STX) {
            ch = recv_buf_read_byte(upg_uart_id); /* Consume this byte */
            pr_err("Recv uart frame error. unknown frame data.\n");
            return -1;
        }

        if (in.head == SOH) {
            if (in.frm_len == 0) { /* frame is not ready, just wait */
                continue;
            }
            ret = read_short_frame_data(pframe, in.frm_len + 6);
            if (ret > 0) {
                pr_debug("Recv %d\n", ret);
                memcpy(p, &pframe[4], ret);
                gotlen += ret;
                p += ret;
                ch = ACK;
            } else if (ret == 0) {
                ch = ACK;
            } else {
                ch = NAK;
            }
            send_byte(upg_uart_id, ch);
            pr_debug("Ack SOH frame\n");
        } else {
            if (in.frm_len == 0) { /* frame is not ready, just wait */
                continue;
            }
            ret = read_long_frame_data(pframe, in.frm_len + 5);
            if (ret > 0) {
                pr_debug("Recv %d\n", ret);
                memcpy(p, &pframe[3], ret);
                gotlen += ret;
                p += ret;
                ch = ACK;
            } else if (ret == 0) {
                ch = ACK;
            } else {
                ch = NAK;
            }
            send_byte(upg_uart_id, ch);
            pr_debug("Ack STX frame\n");
        }
    }

    return gotlen;
}

static int read_short_frame_data(u8 *buf, int len)
{
    int ret, flen;
    u16 crc16, crc16_i;
    u8 blk1, blk2, nextblk;

    ret = recv_buf_read(buf, len);
    if (ret != len) {
        pr_debug("Read data (len=%d)failed, no data. ret = %d\n", len, ret);
        return UART_NO_DATA;
    }

    pr_debug("SHORT: %llu\n", aic_get_time_us());
    // hexdump(buf, len, 1);
    blk1 = buf[1];
    blk2 = buf[2];
    flen = buf[3];
    if (blk1 != (255 - blk2)) {
        pr_err("blk1 %d != blk2 %d\n", blk1, (255 - blk2));
        return UART_ERR_DATA;
    }
    pr_debug("blk no %d\n", blk1);

    crc16_i = buf[4 + flen] << 8 | buf[4 + flen + 1];
    crc16 = crc16_ccitt(0, &buf[4], flen);
    if (crc16 != crc16_i) {
        pr_err("crc16 error: 0x%x != 0x%x\n", crc16, crc16_i);
        return UART_ERR_DATA;
    }

    nextblk = uart_upg.recv_blk_no + 1;
    if (blk1 != 1 && nextblk != blk1 && !uart_upg.first_connect) {
        pr_err("blk no discontinue should be %d, got %d\n", nextblk, blk1);
        return UART_ERR_DATA;
    }

    if (blk1 == uart_upg.recv_blk_no) {
        pr_debug("Repeat frame.\n");
        return UART_SKIP_DATA;
    }
    uart_upg.recv_blk_no = blk1;
    uart_upg.first_connect = 0;
    pr_debug("%s done\n", __func__);
    return flen;
}

/*
 * return > 0: ACK
 * return = 0: ACK
 * return < 0: NAK
 */
static int read_long_frame_data(u8 *buf, int len)
{
    int ret, dlen;
    u16 crc16, crc16_i;
    u8 blk1, blk2, nextblk;

    dlen = LNG_FRM_DLEN;
    ret = recv_buf_read(buf, LNG_FRM_MAX_LEN);
    if (ret != LNG_FRM_MAX_LEN) {
        pr_debug("Read data failed, no data.\n");
        return UART_NO_DATA;
    }

    blk1 = buf[1];
    blk2 = buf[2];
    if (blk1 != (255 - blk2)) {
        pr_err("blk1 %d != blk2 %d\n", blk1, (255 - blk2));
        return UART_ERR_DATA;
    }
    pr_debug("blk no %d\n", blk1);

    crc16_i = buf[3 + dlen] << 8 | buf[3 + dlen + 1];
    crc16 = crc16_ccitt(0, &buf[3], dlen);
    if (crc16 != crc16_i) {
        pr_err("crc16 error: 0x%x != 0x%x\n", crc16, crc16_i);
        hexdump(buf, LNG_FRM_MAX_LEN, 1);
        return UART_ERR_DATA;
    }

    nextblk = uart_upg.recv_blk_no + 1;
    if (blk1 != 1 && nextblk != blk1 && !uart_upg.first_connect) {
        pr_err("blk no discontinue should be %d, got %d\n", nextblk, blk1);
        return UART_ERR_DATA;
    }

    /* Recv repeat frame, just skip it */
    if (blk1 == uart_upg.recv_blk_no) {
        pr_err("Repeat frame.\n");
        return UART_SKIP_DATA;
    }
    uart_upg.recv_blk_no = blk1;
    uart_upg.first_connect = 0;
    pr_debug("%s done\n", __func__);
    return dlen;
}

void aic_upg_uart_baudrate_update(int baudrate)
{
    update_baudrate = baudrate;
    pr_debug("update burn baudrate:%d\n", baudrate);
}

void aic_upg_uart_init(int id)
{
    upg_uart_id = id;

    memset(&uart_upg, 0, sizeof(uart_upg));
    uart_upg.state = CONN_STATE_DETECTING;
    uart_upg.direction = DIR_HOST_SEND;
    uart_upg.proc = uart_conn_detect_proc;
    uart_upg.first_connect = 1;

    uart_init(upg_uart_id);
    recv_buf_init();
}

void aic_upg_uart_loop(void)
{
    uart_upg.proc();
}

bool aic_upg_uart_connect_check(void)
{
    if (uart_upg.state == CONN_STATE_CONNECTED)
        return 1;

    return 0;
}
