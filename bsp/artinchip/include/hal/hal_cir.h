/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#ifndef _HAL_CIR_H_
#define _HAL_CIR_H_

#include <aic_core.h>

#define AIC_IR_DEV      "aic-ir"
#define DEFAULT_FREQ    38000
#define US_PER_SEC      1000000UL

/* Register definition */
#define CIR_MCR_REG                     0x00
#define CIR_MCR_TXFIFO_CLR              BIT(17)
#define CIR_MCR_RXFIFO_CLR              BIT(16)
#define CIR_MCR_TX_STOP                 BIT(9)
#define CIR_MCR_TX_START                BIT(8)
#define CIR_MCR_TX_EN                   BIT(1)
#define CIR_MCR_RX_EN                   BIT(0)

#define CIR_INTR_REG                    0x04
#define CIR_INTR_TXB_AVL_INT            BIT(6)
#define CIR_INTR_TXEND_INT              BIT(5)
#define CIR_INTR_TX_UNF_INT             BIT(4)
#define CIR_INTR_RXB_AVL_INT            BIT(2)
#define CIR_INTR_RX_END_INT             BIT(1)
#define CIR_INTR_RX_OVF_INT             BIT(0)

#define CIR_INTEN_REG                   0x08
#define CIR_INTEN_TXB_EMPTY_LEVEL_POS   (16)
#define CIR_INTEN_TXB_EMPTY_LEVEL(x)    ((x) << CIR_INTEN_TXB_EMPTY_LEVEL_POS)
#define CIR_INTEN_RXB_AVL_LEVEL_POS     (8)
#define CIR_INTEN_RXB_AVL_LEVEL(x)      ((x) << CIR_INTEN_RXB_AVL_LEVEL_POS)
#define CIR_INTEN_TXB_AVL_EN            BIT(6)
#define CIR_INTEN_TXEND_EN              BIT(5)
#define CIR_INTEN_TX_UNF_EN             BIT(4)
#define CIR_INTEN_RXB_AVL_EN            BIT(2)
#define CIR_INTEN_RXEND_EN              BIT(1)
#define CIR_INTEN_RX_OVF_EN             BIT(0)
#define CIR_INTEN_RX_INT_EN             (7)

#define CIR_TXSTAT_REG                  0x0C
#define CIR_TXSTAT_TX_STA               (16)
#define CIR_TXSTAT_TXFIFO_ERR           (10)
#define CIR_TXSTAT_TXFIFO_FULL          (9)
#define CIR_TXSTAT_TXFIFO_EMPTY         (8)
#define CIR_TXSTAT_TXFIFO_DLEN          (0)

#define CIR_RXSTAT_REG                  0x10
#define CIR_RXSTAT_RX_STA               (16)
#define CIR_RXSTAT_RXFIFO_ERR           (10)
#define CIR_RXSTAT_RXFIFO_FULL          (9)
#define CIR_RXSTAT_RXFIFO_EMPTY         BIT(8)
#define CIR_RXSTAT_RXFIFO_DLEN          (0)

#define CIR_RXCLK_REG                   0x14
#define CIR_RX_THRES_REG                0x18
#define CIR_RX_THRES_ACTIVE             (16)
#define CIR_RX_THRES_ACTIVE_LEVEL(x)    ((x) << CIR_RX_THRES_ACTIVE)
#define CIR_RX_THRES_IDLE               (0)
#define CIR_RX_THRES_IDLE_LEVEL(x)      ((x) << CIR_RX_THRES_IDLE)

#define CIR_RX_CFG_REG                  0x1C
#define CIR_RX_CFG_NOISE                (16)
#define CIR_RX_CFG_NOISE_LEVEL(x)       ((x) << CIR_RX_CFG_NOISE)
#define CIR_RX_CFG_RX_LEVEL             BIT(1)
#define CIR_RX_CFG_RX_INVERT            BIT(0)

#define CIR_TX_CFG_REG                  0x20
#define CIR_TX_CFG_TX_MODE              BIT(2)
#define CIR_TX_CFG_TX_OUT_MODE          BIT(1)
#define CIR_TX_CFG_TX_INVERT            BIT(0)

#define CIR_TIDC_REG                    0x24
#define CIR_CARR_CFG_REG                0x2C
#define CIR_CARR_CFG_HIGH               (16)
#define CIR_CARR_CFG_HIGH_VAL(x)        ((x) << CIR_CARR_CFG_HIGH)
#define CIR_CARR_CFG_LOW                (0)
#define CIR_CARR_CFG_LOW_VAL(x)         ((x) << CIR_CARR_CFG_LOW)

#define CIR_RXFIFO_REG                  0x30
#define CIR_TXFIFO_REG                  0x80
#define CIR_VERSION_REG                 0xFFC

typedef enum {
    CIR_EVENT_RECEIVE_COMPLETE,
    CIR_EVENT_ERROR,
} cir_event_t;

typedef struct aic_cir_ctrl aic_cir_ctrl_t;
struct aic_cir_ctrl {
        unsigned long       cir_base;
        uint8_t             irq_num;
        uint8_t             clk_idx;
        void (*callback)(aic_cir_ctrl_t *aic_cir_ctrl, cir_event_t event,
                         void *arg);
        void                *arg;
        uint8_t             tx_data[128];
        uint8_t             rx_data[128];
        uint32_t            rx_idx;
        uint8_t             rx_level;
        uint8_t             rx_flag; /* Indicates if rxfifo has received data */
};

int hal_cir_init(aic_cir_ctrl_t *aic_cir_ctrl);
void hal_cir_uninit(aic_cir_ctrl_t *aic_cir_ctrl);
int hal_cir_set_tx_carrier(aic_cir_ctrl_t * aic_cir_ctrl, uint8_t protocol,
                           uint32_t tx_duty);
void hal_cir_set_rx_sample_clock(aic_cir_ctrl_t * aic_cir_ctrl,
                                 uint8_t protocol);
void hal_cir_set_rx_level(aic_cir_ctrl_t * aic_cir_ctrl, uint32_t rx_level);
void hal_cir_send_data(aic_cir_ctrl_t * aic_cir_ctrl, uint8_t * tx_data,
                       uint32_t size);
void hal_cir_enable_transmitter(aic_cir_ctrl_t * aic_cir_ctrl);
void hal_cir_disable_transmitter(aic_cir_ctrl_t * aic_cir_ctrl);
void hal_cir_enable_receiver(aic_cir_ctrl_t * aic_cir_ctrl);
void hal_cir_disable_receiver(aic_cir_ctrl_t * aic_cir_ctrl);
irqreturn_t hal_cir_irq(int irq_num, void *arg);
void hal_cir_attach_callback(aic_cir_ctrl_t * aic_cir_ctrl,
                             void *callback, void *arg);
void hal_cir_rx_reset_status(aic_cir_ctrl_t * aic_cir_ctrl);

#endif /* _HAL_CIR_H_ */
