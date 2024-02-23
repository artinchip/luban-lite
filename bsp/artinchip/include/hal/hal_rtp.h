/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_RTP_H__
#define _ARTINCHIP_HAL_RTP_H__

#define AIC_RTP_NAME                "aic-rtp"
#define AIC_RTP_FIFO_DEPTH          16
#define AIC_RTP_DEFAULT_X_PLATE     280
#define AIC_RTP_DEFAULT_Y_PLATE     600
#define AIC_RTP_SCATTER_THD         48
#define AIC_RTP_MAX_VAL             0xFFF
#define AIC_RTP_VAL_RANGE           (AIC_RTP_MAX_VAL + 1)
#define AIC_RTP_INVALID_VAL         (AIC_RTP_MAX_VAL + 1)

#define AIC_RTP_EVT_BUF_SIZE        64

enum aic_rtp_mode {
    RTP_MODE_MANUAL = 0, /* Unsupported in RTOS */
    RTP_MODE_AUTO1,
    RTP_MODE_AUTO2, /* Default mode: AUTO2 + Period */
    RTP_MODE_AUTO3,
    RTP_MODE_AUTO4
};

struct aic_rtp_dat {
    u16 y_minus;
    u16 x_minus;
    u16 y_plus;
    u16 x_plus;
    u16 z_a;
    u16 z_b;
    u16 z_c;
    u16 z_d;
    u32 timestamp;
};

struct aic_rtp_event {
    u16 x;
    u16 y;
    u16 pressure;
    u16 down;
    u32 timestamp;
};

struct aic_rtp_ebuf {
    u16 rd_pos;
    u16 wr_pos;
    struct aic_rtp_event event[AIC_RTP_EVT_BUF_SIZE];
};

typedef s32 (*rtp_callback_t)(void);

struct aic_rtp_dev {
    u32 pclk_rate;
    rtp_callback_t callback;

    s32 pressure_det;
    s32 ignore_fifo_data;
    enum aic_rtp_mode mode;
    u32 max_press;
    u32 smp_period; /* unit: ms */
    u32 x_plate;
    u32 y_plate;
    u32 fuzz;
    u32 pdeb;
    u32 delay;

    u32 intr;
    u32 fcr;
    struct aic_rtp_dat latest;
    aicos_sem_t sem;
    struct aic_rtp_ebuf ebuf;
    u16 point_num;
};

typedef struct {
    int x[5], xfb[5];
    int y[5], yfb[5];
    int a[7];
} calibration;

void hal_rtp_status_show(struct aic_rtp_dev *rtp);

void hal_rtp_enable(struct aic_rtp_dev *rtp, int en);
void hal_rtp_int_enable(struct aic_rtp_dev *rtp, int en);
void hal_rtp_auto_mode(struct aic_rtp_dev *rtp);

irqreturn_t hal_rtp_isr(int irq, void *arg);

u32 hal_rtp_ebuf_read_space(struct aic_rtp_ebuf *ebuf);
#define hal_rtp_ebuf_write_space(buf) \
        (AIC_RTP_EVT_BUF_SIZE - hal_rtp_ebuf_read_space(buf))
#define hal_rtp_ebuf_full(buf) (hal_rtp_ebuf_write_space(buf) == 0)
#define hal_rtp_ebuf_empty(buf) (hal_rtp_ebuf_read_space((buf)) == 0)
s32 hal_rtp_ebuf_write(struct aic_rtp_ebuf *ebuf, struct aic_rtp_event *e);
s32 hal_rtp_ebuf_read(struct aic_rtp_ebuf *ebuf, struct aic_rtp_event *e);

s32 hal_rtp_clk_init(void);
s32 hal_rtp_register_callback(rtp_callback_t callback);
s32 hal_rtp_ebuf_sync(struct aic_rtp_ebuf *ebuf);
u32 hal_rtp_pdeb_valid_check(struct aic_rtp_dev *rtp);

#endif
