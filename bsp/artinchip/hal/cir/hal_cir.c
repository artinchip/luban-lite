/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#include "hal_cir.h"
#include "aic_hal_clk.h"

#define USEC_PER_SEC    (1000000)
#define gen_reg(val)    (volatile void *)(val)

int hal_cir_init(aic_cir_ctrl_t *aic_cir_ctrl)
{
    int ret = 0;
    uint32_t reg_val;

    aic_cir_ctrl->cir_base = CIR_BASE;
    aic_cir_ctrl->irq_num = CIR_IRQn;
    aic_cir_ctrl->clk_idx = CLK_CIR;
    ret = hal_clk_enable_deassertrst(aic_cir_ctrl->clk_idx);

    /* Set noise thresholld */
    reg_val = readl(gen_reg(aic_cir_ctrl->cir_base + CIR_RX_CFG_REG));
    reg_val &= ~(0xffff << CIR_RX_CFG_NOISE);
    reg_val |= CIR_RX_CFG_NOISE_LEVEL(0x1770);
    writel(reg_val, gen_reg(aic_cir_ctrl->cir_base + CIR_RX_CFG_REG));
    /* Set rx active threshold and idle threshold */
    writel((CIR_RX_THRES_ACTIVE_LEVEL(0x10) |
        CIR_RX_THRES_IDLE_LEVEL(0x1C0)),
        gen_reg(aic_cir_ctrl->cir_base + CIR_RX_THRES_REG));
    /* Flush TX and RX FIFO */
    writel((CIR_MCR_RXFIFO_CLR | CIR_MCR_TXFIFO_CLR),
        gen_reg(aic_cir_ctrl->cir_base + CIR_MCR_REG));
    /* Set TX and RX FIFO level */
    writel((CIR_INTEN_TXB_EMPTY_LEVEL(0x40) |
        CIR_INTEN_RXB_AVL_LEVEL(0x20)),
        gen_reg(aic_cir_ctrl->cir_base + CIR_INTEN_REG));
    /* Clear pending interrupt flags */
    writel(0xff, gen_reg(aic_cir_ctrl->cir_base + CIR_INTR_REG));
    /* Enable RX interrupt */
    reg_val = readl(gen_reg(aic_cir_ctrl->cir_base + CIR_INTEN_REG));
    reg_val |= CIR_INTEN_RX_INT_EN;
    writel(reg_val, gen_reg(aic_cir_ctrl->cir_base + CIR_INTEN_REG));

    return ret;
}

void hal_cir_uninit(aic_cir_ctrl_t *aic_cir_ctrl)
{
    hal_clk_disable_assertrst(aic_cir_ctrl->clk_idx);
}

int hal_cir_set_tx_carrier(aic_cir_ctrl_t * aic_cir_ctrl,
                           uint8_t protocol, uint32_t tx_duty)
{
    uint32_t mod_clk, clk_div, carrier_high, carrier_low, val;

    mod_clk = hal_clk_get_freq(CLK_CIR);
    if (protocol == 1) {
        /* RC5 protocol */
        clk_div = DIV_ROUND_UP(mod_clk, 36000);
        carrier_high = clk_div * tx_duty / 100;
        carrier_low = clk_div - carrier_high;
    } else {
        /* NEC protocol */
        clk_div = DIV_ROUND_UP(mod_clk, 38000);
        carrier_high = clk_div * tx_duty / 100;
        carrier_low = clk_div - carrier_high;
    }

    val = ((carrier_high - 1) << 16) | (carrier_low - 1);
    writel(val, gen_reg(aic_cir_ctrl->cir_base + CIR_CARR_CFG_REG));
    return 0;
}

void hal_cir_set_rx_sample_clock(aic_cir_ctrl_t * aic_cir_ctrl,
                                 uint8_t protocol)
{
    uint32_t mod_clk, clk_div;

    mod_clk = hal_clk_get_freq(CLK_CIR);
    if (protocol == 1) {
        /* RC5 protocol */
        clk_div = DIV_ROUND_UP(mod_clk, 36000);
    } else {
        /* NEC protocol */
        clk_div = DIV_ROUND_UP(mod_clk, 38000);
    }

    writel(clk_div - 1, gen_reg(aic_cir_ctrl->cir_base + CIR_RXCLK_REG));
}

void hal_cir_set_rx_level(aic_cir_ctrl_t * aic_cir_ctrl, uint32_t rx_level)
{
    uint32_t reg_val;

    aic_cir_ctrl->rx_level = rx_level;
    reg_val = readl(gen_reg(aic_cir_ctrl->cir_base + CIR_RX_CFG_REG));
    reg_val &= ~CIR_RX_CFG_RX_LEVEL;
    reg_val |= (rx_level << 1);
    writel(reg_val, gen_reg(aic_cir_ctrl->cir_base + CIR_RX_CFG_REG));
}

void hal_cir_send_data(aic_cir_ctrl_t * aic_cir_ctrl,
                       uint8_t * tx_data, uint32_t size)
{
    int i;
    uint32_t reg_val;

    for (i = 0; i < size; i++)
        writel(tx_data[i], gen_reg(aic_cir_ctrl->cir_base + CIR_TXFIFO_REG));

    /* Start to transfer */
    reg_val = readl(gen_reg(aic_cir_ctrl->cir_base + CIR_MCR_REG));
    reg_val |= CIR_MCR_TX_START;
    writel(reg_val, gen_reg(aic_cir_ctrl->cir_base + CIR_MCR_REG));
}

void hal_cir_enable_transmitter(aic_cir_ctrl_t * aic_cir_ctrl)
{
    uint32_t reg_val;

    reg_val = readl(gen_reg(aic_cir_ctrl->cir_base + CIR_MCR_REG));
    reg_val |= CIR_MCR_TX_EN;
    writel(reg_val, gen_reg(aic_cir_ctrl->cir_base + CIR_MCR_REG));
}

void hal_cir_disable_transmitter(aic_cir_ctrl_t * aic_cir_ctrl)
{
    uint32_t reg_val;

    reg_val = readl(gen_reg(aic_cir_ctrl->cir_base + CIR_MCR_REG));
    reg_val &= ~CIR_MCR_TX_EN;
    writel(reg_val, gen_reg(aic_cir_ctrl->cir_base + CIR_MCR_REG));
}

void hal_cir_enable_receiver(aic_cir_ctrl_t * aic_cir_ctrl)
{
    uint32_t reg_val;

    reg_val = readl(gen_reg(aic_cir_ctrl->cir_base + CIR_MCR_REG));
    reg_val |= CIR_MCR_RX_EN;
    writel(reg_val, gen_reg(aic_cir_ctrl->cir_base + CIR_MCR_REG));
}

void hal_cir_disable_receiver(aic_cir_ctrl_t * aic_cir_ctrl)
{
    uint32_t reg_val;

    reg_val = readl(gen_reg(aic_cir_ctrl->cir_base + CIR_MCR_REG));
    reg_val &= ~CIR_MCR_RX_EN;
    writel(reg_val, gen_reg(aic_cir_ctrl->cir_base + CIR_MCR_REG));
}

irqreturn_t hal_cir_irq(int irq_num, void *arg)
{
    unsigned int int_status;
    unsigned int rx_status;
    unsigned int i, count;
    aic_cir_ctrl_t *aic_cir_ctrl = (aic_cir_ctrl_t *)arg;
    uint8_t *rx_data = &aic_cir_ctrl->rx_data[aic_cir_ctrl->rx_idx];
    uint8_t need_inverse;

    int_status = readl(gen_reg(aic_cir_ctrl->cir_base + CIR_INTR_REG)) & 7;
    rx_status = readl(gen_reg(aic_cir_ctrl->cir_base + CIR_RXSTAT_REG));

    /* clear all pending status */
    /* RX Available interrupt is pulse interrupt */
    writel(0xff, gen_reg(aic_cir_ctrl->cir_base + CIR_INTR_REG));
    /* Handle Receive data */
    if (int_status & (CIR_INTR_RXB_AVL_INT | CIR_INTR_RX_END_INT)) {
        /* Get the number of data in RXFIFO */
        count = rx_status & 0x3f;

        /* Confirm if RXFIFO has data */
        if (!(rx_status & (0x1 << 8))) {
            for (i = 0; i < count; i++) {
                rx_data[i] = readl(gen_reg(aic_cir_ctrl->cir_base +
                                           CIR_RXFIFO_REG));
                need_inverse = (rx_data[i] >> 7) ^ aic_cir_ctrl->rx_level;
                rx_data[i] = (need_inverse << 7) | (rx_data[i] & 0x7f);
                aic_cir_ctrl->rx_idx++;
            }
            aic_cir_ctrl->rx_flag = 1;
        }
    }

    if (int_status & CIR_INTR_RX_OVF_INT) {
        aic_cir_ctrl->rx_flag = 0;
        if (aic_cir_ctrl->callback)
            aic_cir_ctrl->callback(aic_cir_ctrl, CIR_EVENT_ERROR,
                                   aic_cir_ctrl->arg);
    } else if ((int_status & CIR_INTR_RX_END_INT) && aic_cir_ctrl->rx_flag) {
        aic_cir_ctrl->rx_flag = 0;
        if (aic_cir_ctrl->callback)
            aic_cir_ctrl->callback(aic_cir_ctrl, CIR_EVENT_RECEIVE_COMPLETE,
                                   aic_cir_ctrl->arg);
    }

    return IRQ_HANDLED;
}

void hal_cir_attach_callback(aic_cir_ctrl_t * aic_cir_ctrl,
                             void *callback, void *arg)
{
    aic_cir_ctrl->callback = callback;
    aic_cir_ctrl->arg = arg;
}

void hal_cir_rx_reset_status(aic_cir_ctrl_t * aic_cir_ctrl)
{
    aic_cir_ctrl->rx_idx = 0;
}
