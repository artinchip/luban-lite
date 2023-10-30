/*
 * I2S driver of ArtInChip SoC
 *
 * Copyright (C) 2020-2021 ArtInChip Technology Co., Ltd.
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#ifndef _HAL_I2S_H_
#define _HAL_I2S_H_

#include <stdint.h>
#include <aic_core.h>
#include "hal_i2s_format.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2S_CTL_REG                     0x00
#define I2S_CTL_GEN                     BIT(0)
#define I2S_CTL_RXEN                    BIT(1)
#define I2S_CTL_TXEN                    BIT(2)
#define I2S_CTL_LOOP                    BIT(3)
#define I2S_CTL_MODE_MASK               (3 << 4)
#define I2S_CTL_PCM_MODE                (0 << 4)
#define I2S_CTL_LEFT_MODE               (1 << 4)
#define I2S_CTL_RIGHT_J_MODE            (2 << 4)
#define I2S_CTL_OUTMUTE_MASK            BIT(6)
#define I2S_CTL_DOUT_EN                 BIT(8)
#define I2S_CTL_LRCK_MASK               BIT(17)
#define I2S_CTL_LRCK_SLAVE              (0 << 17)
#define I2S_CTL_LRCK_MASTER             (1 << 17)
#define I2S_CTL_BCLK_MASK               BIT(18)
#define I2S_CTL_BCLK_SLAVE              (0 << 18)
#define I2S_CTL_BCLK_MASTER             (1 << 18)

#define I2S_FMT0_REG                    (0x04)
#define I2S_FMT0_SW_MASK                (7)
#define I2S_FMT0_SW(sw)                 ((sw) << 0)
#define I2S_FMT0_EDGE_TRANS             BIT(3)
#define I2S_FMT0_SR_MASK                (7 << 4)
#define I2S_FMT0_SR(sr)                 ((sr) << 4)
#define I2S_FMT0_BCLK_POL_MASK          BIT(7)
#define I2S_FMT0_BCLK_POL_NORMAL        (0 << 7)
#define I2S_FMT0_BCLK_POL_INVERTED      (1 << 7)
#define I2S_FMT0_LRCK_PERIOD_MASK       (0x3FF << 8)
#define I2S_FMT0_LRCK_PERIOD(wid)       ((wid) << 8)
#define I2S_FMT0_LRCK_PERIOD_MAX        (1024)
#define I2S_FMT0_LRCK_POL_MASK          BIT(19)
#define I2S_FMT0_LRCK_POL_NORMAL        (0 << 19)
#define I2S_FMT0_LRCK_POL_INVERTED      (1 << 19)
#define I2S_FMT0_LRCK_WIDTH             BIT(30)

#define I2S_FMT1_REG                    (0x08)
#define I2S_FMT1_TX_PDM_MASK            (3)
#define I2S_FMT1_TX_PDM_LINEAR          (0 << 0)
#define I2S_FMT1_TX_PDM_ULAW            (2 << 0)
#define I2S_FMT1_TX_PDM_ALAW            (3 << 0)
#define I2S_FMT1_RX_PDM_MASK            (3 << 2)
#define I2S_FMT1_RX_PDM_LINEAR          (0 << 2)
#define I2S_FMT1_RX_PDM_ULAW            (2 << 2)
#define I2S_FMT1_RX_PDM_ALAW            (3 << 2)
#define I2S_FMT1_SEXT_MASK              (3 << 4)
#define I2S_FMT1_SEXT_ZERO_LSB          (0 << 4)
#define I2S_FMT1_SEXT_SE_MSB            (1 << 4)
#define I2S_FMT1_SEXT_TRANS0            (3 << 4)
#define I2S_FMT1_TXMLS                  BIT(6)
#define I2S_FMT1_RXMLS                  BIT(7)

#define I2S_ISTA_REG                    (0x0c)
#define I2S_RXFIFO_REG                  (0x10)

#define I2S_FCTL_REG                    (0x14)
#define I2S_FCTL_RXOM_MASK              (3)
#define I2S_FCTL_RXOM(mode)             ((mode) << 0)
#define I2S_FCTL_TXIM                   BIT(2)
#define I2S_FCTL_FRX                    BIT(24)
#define I2S_FCTL_FTX                    BIT(25)

#define I2S_FSTA_REG                    (0x18)
#define I2S_INT_REG                     (0x1c)
#define I2S_INT_RXDRQ_EN                BIT(3)
#define I2S_INT_TXDRQ_EN                BIT(7)

#define I2S_TXFIFO_REG                  (0x20)
#define I2S_CLKD_REG                    (0X24)
#define I2S_CLKD_MCLKDIV_MASK           (0xF)
#define I2S_CLKD_MCLKDIV(mdiv)          ((mdiv) << 0)
#define I2S_CLKD_BCLKDIV_MASK           (0xF << 4)
#define I2S_CLKD_BCLKDIV(bdiv)          ((bdiv) << 4)
#define I2S_CLKD_MCLKO_EN               BIT(8)

#define I2S_TXCNT_REG                   (0x28)
#define I2S_RXCNT_REG                   (0x2c)

#define I2S_CHCFG_REG                   (0x30)
#define I2S_CHCFG_TXSLOTNUM_MASK        (0xF)
#define I2S_CHCFG_TXSLOTNUM(num)        (((num) - 1) << 0)
#define I2S_CHCFG_RXSLOTNUM_MASK        (0xF << 4)
#define I2S_CHCFG_RXSLOTNUM(num)        (((num) - 1) << 4)

#define I2S_TXCHSEL_REG                 (0x34)
#define I2S_TXCHSEL_TXCHEN_MASK         (0xFFFF)
#define I2S_TXCHSEL_TXCHEN(ch)          ((1 << (ch)) - 1)
#define I2S_TXCHSEL_TXCHSEL_MASK        (0xF << 16)
#define I2S_TXCHSEL_TXCHSEL(num)        (((num) - 1) << 16)
#define I2S_TXCHSEL_TXOFFSET_MASK       (3 << 20)
#define I2S_TXCHSEL_OFFSET_0            (0 << 20)
#define I2S_TXCHSEL_OFFSET_1            (1 << 20)

#define I2S_TXCHMAP0_REG                (0x44)
#define I2S_TXCHMAP0_CHMAP_MASK(ch)     GENMASK(((ch) - 8) * 4 + 3, (ch) - 8)
#define I2S_TXCHMAP0_CHMAP(ch, chmap)   ((chmap) << ((ch) - 8) * 4)

#define I2S_TXCHMAP1_REG                (0x48)
#define I2S_TXCHMAP1_CHMAP_MASK(ch)     GENMASK((ch) * 4 + 3, (ch) * 4)
#define I2S_TXCHMAP1_CHMAP(ch, chmap)   ((chmap) << ((ch) * 4))

#define I2S_RXCHSEL_REG                 (0x64)
#define I2S_RXCHSEL_RXCHSEL_MASK        (0xF << 16)
#define I2S_RXCHSEL_RXCHSEL(num)        ((num - 1) << 16)
#define I2S_RXCHSEL_RXOFFSET_MASK       (3 << 20)
#define I2S_RXCHSEL_RXOFFSET_0          (0 << 20)
#define I2S_RXCHSEL_RXOFFSET_1          (1 << 20)

#define I2S_RXCHMAP0_REG                (0x68)
#define I2S_RXCHMAP0_CHMAP_MASK(ch)     GENMASK(((ch) - 8) * 4 + 3, (ch) - 8)
#define I2S_RXCHMAP0_CHMAP(ch, chmap)   ((chmap) << ((ch) - 8) * 4)

#define I2S_RXCHMAP1_REG                (0x6C)
#define I2S_RXCHMAP1_CHMAP_MASK(ch)     GENMASK((ch) * 4 + 3, (ch) * 4)
#define I2S_RXCHMAP1_CHMAP(ch, chmap)   ((chmap) << ((ch) * 4))

#define I2S_TX_PERIOD_INT               1
#define I2S_RX_PERIOD_INT               2

#define I2S_TRANSFER_TYPE_TX            3
#define I2S_TRANSFER_TYPE_RX            4

struct aic_i2s_buf_info
{
    void *buf;
    uint32_t buf_len;
    uint32_t period_len;
};

struct aic_i2s_transfer_info
{
    struct aic_dma_chan *dma_chan;
    struct aic_i2s_buf_info buf_info;
    int transfer_type;
};

typedef struct aic_i2s_ctrl aic_i2s_ctrl;
typedef void (*i2s_callback)(aic_i2s_ctrl *i2s, void *arg);

struct aic_i2s_ctrl
{
    unsigned long reg_base;
    uint32_t irq_num;
    uint32_t clk_id;
    uint32_t idx;
    struct aic_i2s_transfer_info tx_info;
    struct aic_i2s_transfer_info rx_info;
    i2s_callback callback;
    void *arg;
};

static inline void hal_i2s_clear_rx_fifo(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_FCTL_REG);
    reg_val |= I2S_FCTL_FRX;
    writel(reg_val, i2s->reg_base + I2S_FCTL_REG);
}

static inline void hal_i2s_clear_tx_fifo(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_FCTL_REG);
    reg_val |= I2S_FCTL_FTX;
    writel(reg_val, i2s->reg_base + I2S_FCTL_REG);
}

static inline void hal_i2s_module_enable(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_CTL_REG);
    reg_val |= I2S_CTL_GEN;
    writel(reg_val, i2s->reg_base + I2S_CTL_REG);
}

static inline void hal_i2s_module_disable(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_CTL_REG);
    reg_val &= ~I2S_CTL_GEN;
    writel(reg_val, i2s->reg_base + I2S_CTL_REG);
}

static inline void hal_i2s_master_mode(struct aic_i2s_ctrl *i2s)
{
        uint32_t reg_val;

        reg_val = readl(i2s->reg_base + I2S_CTL_REG);
        reg_val |= I2S_CTL_BCLK_MASTER | I2S_CTL_LRCK_MASTER;
        writel(reg_val, i2s->reg_base + I2S_CTL_REG);
}

static inline void hal_i2s_slave_mode(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_CTL_REG);
    reg_val &= ~(I2S_CTL_BCLK_MASTER | I2S_CTL_LRCK_MASTER);
    writel(reg_val, i2s->reg_base + I2S_CTL_REG);
}

static inline void hal_i2s_clear_tx_counter(struct aic_i2s_ctrl *i2s)
{
    writel(0, i2s->reg_base + I2S_TXCNT_REG);
}

static inline void hal_i2s_clear_rx_counter(struct aic_i2s_ctrl *i2s)
{
    writel(0, i2s->reg_base + I2S_RXCNT_REG);
}

static inline void hal_i2s_enable_tx_block(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_CTL_REG);
    reg_val |= I2S_CTL_TXEN;
    writel(reg_val, i2s->reg_base + I2S_CTL_REG);
}

static inline void hal_i2s_enable_rx_block(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_CTL_REG);
    reg_val |= I2S_CTL_RXEN;
    writel(reg_val, i2s->reg_base + I2S_CTL_REG);
}

static inline void hal_i2s_disable_tx_block(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_CTL_REG);
    reg_val &= ~I2S_CTL_TXEN;
    writel(reg_val, i2s->reg_base + I2S_CTL_REG);
}

static inline void hal_i2s_disable_rx_block(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_CTL_REG);
    reg_val &= ~I2S_CTL_RXEN;
    writel(reg_val, i2s->reg_base + I2S_CTL_REG);
}

static inline void hal_i2s_enable_tx_drq(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_INT_REG);
    reg_val |= I2S_INT_TXDRQ_EN;
    writel(reg_val, i2s->reg_base + I2S_INT_REG);
}

static inline void hal_i2s_enable_rx_drq(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_INT_REG);
    reg_val |= I2S_INT_RXDRQ_EN;
    writel(reg_val, i2s->reg_base + I2S_INT_REG);
}

static inline void hal_i2s_disable_tx_drq(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_INT_REG);
    reg_val &= ~I2S_INT_TXDRQ_EN;
    writel(reg_val, i2s->reg_base + I2S_INT_REG);
}

static inline void hal_i2s_disable_rx_drq(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_INT_REG);
    reg_val &= ~I2S_INT_RXDRQ_EN;
    writel(reg_val, i2s->reg_base + I2S_INT_REG);
}

static inline void hal_i2s_enable_data_out(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_CTL_REG);
    reg_val |= I2S_CTL_DOUT_EN;
    writel(reg_val, i2s->reg_base + I2S_CTL_REG);
}

static inline void hal_i2s_disable_data_out(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_CTL_REG);
    reg_val &= ~I2S_CTL_DOUT_EN;
    writel(reg_val, i2s->reg_base + I2S_CTL_REG);
}

static inline void hal_i2s_mclk_out_enable(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_CLKD_REG);
    reg_val |= I2S_CLKD_MCLKO_EN;
    writel(reg_val, i2s->reg_base + I2S_CLKD_REG);
}

static inline void hal_i2s_mclk_out_disable(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_CLKD_REG);
    reg_val &= ~I2S_CLKD_MCLKO_EN;
    writel(reg_val, i2s->reg_base + I2S_CLKD_REG);
}

static inline void hal_i2s_txfifo_input_mode(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_FCTL_REG);
    reg_val |= I2S_FCTL_TXIM;
    writel(reg_val, i2s->reg_base + I2S_FCTL_REG);
}

static inline void hal_i2s_rxfifo_output_mode(struct aic_i2s_ctrl *i2s)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_FCTL_REG);
    reg_val &= ~I2S_FCTL_RXOM_MASK;
    reg_val |= I2S_FCTL_RXOM(1);
    writel(reg_val, i2s->reg_base + I2S_FCTL_REG);
}

int hal_i2s_init(aic_i2s_ctrl *i2s, uint32_t i2s_idx);
int hal_i2s_uninit(aic_i2s_ctrl *i2s);
int hal_i2s_protocol_select(struct aic_i2s_ctrl *i2s, i2s_protocol_t protocol);
int hal_i2s_sample_width_select(aic_i2s_ctrl *i2s, i2s_sample_width_t width);
void hal_i2s_polarity_set(struct aic_i2s_ctrl *i2s, i2s_polarity_t polarity);
int hal_i2s_mclk_set(struct aic_i2s_ctrl *i2s,
                     i2s_sample_rate_t sample_rate, uint32_t mclk_nfs);
int hal_i2s_sclk_set(struct aic_i2s_ctrl *i2s,
                     i2s_sample_rate_t sample_rate, uint32_t sclk_nfs);
void hal_i2s_channel_select(struct aic_i2s_ctrl *i2s,
                            i2s_sound_channel_t channel, i2s_stream_t stream);
void hal_i2s_playback_start(aic_i2s_ctrl *i2s, i2s_format_t *format);
void hal_i2s_record_start(aic_i2s_ctrl *i2s, i2s_format_t *format);
void hal_i2s_playback_stop(aic_i2s_ctrl *i2s);
void hal_i2s_record_stop(aic_i2s_ctrl *i2s);
void hal_i2s_attach_callback(aic_i2s_ctrl *i2s, i2s_callback callback,
                             void *arg);
void hal_i2s_detach_callback(aic_i2s_ctrl *i2s);

#ifdef __cplusplus
}
#endif

#endif /* _HAL_I2S_H_ */