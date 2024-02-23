/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BL_UPG_UART_H_
#define __BL_UPG_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_common.h>

#define CONN_STATE_DETECTING 0
#define CONN_STATE_CONNECTED 1
#define DIR_HOST_RECV        0
#define DIR_HOST_SEND        1

#define SOH      0x01
#define STX      0x02
#define ACK      0x06
#define DC1_SEND 0x11
#define DC2_RECV 0x12
#define NAK      0x15
#define CAN      0x18
#define SIG_A    0x41
#define SIG_C    0x43

#define LNG_FRM_DLEN     (1024)
#define LNG_FRM_MAX_LEN  (LNG_FRM_DLEN + 5)
#define SHT_FRM_DLEN     (176)
#define SHT_FRM_MAX_LEN  (SHT_FRM_DLEN + 6)
#define UART_RECV_BUF_LEN (1200)
#define RESET_POS        (128)

#define UART_SKIP_DATA 0
#define UART_NO_DATA   -1
#define UART_ERR_DATA  -2

#define SIG_A_INTERVAL_MS 500
#define update_last_recv_time() do {uart_upg.last_recv_time = aic_get_time_ms();} while(0)

struct upg_uart_device {
    int state;
    int direction;
    int (*proc)(void);
    int wait_ack;
    /* Time for connection checking */
    u32 last_recv_time;
    u32 last_send_time;
    u8 recv_blk_no;
    u8 send_blk_no;
    s32 recv_dma;
    s32 send_dma;
    s32 first_connect;
};

struct dma_input {
    u8 head;
    int frm_len;
};

void aic_upg_uart_init(int id);
void aic_upg_uart_loop(void);
bool aic_upg_uart_connect_check(void);

#ifdef __cplusplus
}
#endif

#endif /* __BL_UPG_UART_H_ */
