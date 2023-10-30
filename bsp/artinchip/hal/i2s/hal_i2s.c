/*
 * I2S driver of ArtInChip SoC
 *
 * Copyright (C) 2020-2021 ArtInChip Technology Co., Ltd.
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#include "hal_i2s.h"
#include "hal_dma.h"
#include "aic_dma_id.h"
#include "aic_hal_clk.h"

struct aic_i2s_clk_div {
    u8 div;
    u8 val;
};

static const struct aic_i2s_clk_div i2s_bmclk_div[] = {
    { .div = 1,   .val = 1 },
    { .div = 2,   .val = 2 },
    { .div = 4,   .val = 3 },
    { .div = 6,   .val = 4 },
    { .div = 8,   .val = 5 },
    { .div = 12,  .val = 6 },
    { .div = 16,  .val = 7 },
    { .div = 24,  .val = 8 },
    { .div = 32,  .val = 9 },
    { .div = 48,  .val = 10 },
    { .div = 64,  .val = 11 },
    { .div = 96,  .val = 12 },
    { .div = 128, .val = 13 },
    { .div = 176, .val = 14 },
    { .div = 192, .val = 15 },
};

int hal_i2s_init(aic_i2s_ctrl *i2s, uint32_t i2s_idx)
{
    int ret = 0;

    i2s->reg_base = I2S0_BASE + (0x1000 * i2s_idx);
    i2s->irq_num = I2S0_IRQn + i2s_idx;
    i2s->clk_id = CLK_I2S0 + i2s_idx;
    i2s->idx = i2s_idx;

    ret = hal_clk_enable_deassertrst(i2s->clk_id);
    if (ret)
        hal_log_err("I2S%u init error!\n", i2s_idx);

    return ret;
}

int hal_i2s_uninit(aic_i2s_ctrl *i2s)
{
    int ret;

    ret = hal_clk_disable_assertrst(i2s->clk_id);
    if (ret)
        hal_log_err("I2S%u uninit error!\n", i2s->idx);

    return ret;
}

int hal_i2s_protocol_select(aic_i2s_ctrl *i2s, i2s_protocol_t protocol)
{
    uint32_t reg_val, tx_offset, rx_offset;
    int ret = 0;

    reg_val = readl(i2s->reg_base + I2S_CTL_REG);
    tx_offset = readl(i2s->reg_base + I2S_TXCHSEL_REG);
    rx_offset = readl(i2s->reg_base + I2S_RXCHSEL_REG);

    switch (protocol) {
    case I2S_PROTOCOL_I2S:
        /* I2S protocol */
        reg_val &= ~I2S_CTL_MODE_MASK;
        reg_val |= I2S_CTL_LEFT_MODE;
        writel(reg_val, i2s->reg_base + I2S_CTL_REG);
        /* configure TX offset 1 */
        tx_offset &= ~I2S_TXCHSEL_TXOFFSET_MASK;
        tx_offset |= I2S_TXCHSEL_OFFSET_1;
        writel(tx_offset, i2s->reg_base + I2S_TXCHSEL_REG);
        /* configure RX offset 1 */
        rx_offset &= ~I2S_RXCHSEL_RXOFFSET_MASK;
        rx_offset |= I2S_RXCHSEL_RXOFFSET_1;
        writel(rx_offset, i2s->reg_base + I2S_RXCHSEL_REG);
        break;
    case I2S_PROTOCOL_LEFT_J:
        /* left justified protocol */
        reg_val &= ~I2S_CTL_MODE_MASK;
        reg_val |= I2S_CTL_LEFT_MODE;
        writel(reg_val, i2s->reg_base + I2S_CTL_REG);
        /* configure TX offset 0 */
        tx_offset &= ~I2S_TXCHSEL_TXOFFSET_MASK;
        writel(tx_offset, i2s->reg_base + I2S_TXCHSEL_REG);
        /* configure RX offset 0 */
        rx_offset &= ~I2S_RXCHSEL_RXOFFSET_MASK;
        writel(rx_offset, i2s->reg_base + I2S_RXCHSEL_REG);
        break;
    case I2S_PROTOCOL_RIGHT_J:
        /* right justified protocol */
        reg_val &= ~I2S_CTL_MODE_MASK;
        reg_val |= I2S_CTL_RIGHT_J_MODE;
        writel(reg_val, i2s->reg_base + I2S_CTL_REG);
        break;
    case I2S_PCM_LONG:
        /* PCM long protocol*/
        reg_val &= ~I2S_CTL_MODE_MASK;
        writel(reg_val, i2s->reg_base + I2S_CTL_REG);
        /* configure TX offset 0 */
        tx_offset &= ~I2S_TXCHSEL_TXOFFSET_MASK;
        writel(tx_offset, i2s->reg_base + I2S_TXCHSEL_REG);
        /* configure RX offset 0 */
        rx_offset &= ~I2S_RXCHSEL_RXOFFSET_MASK;
        writel(rx_offset, i2s->reg_base + I2S_RXCHSEL_REG);
        break;
    case I2S_PCM_SHORT:
        /* PCM short protocol */
        reg_val &= ~I2S_CTL_MODE_MASK;
        writel(reg_val, i2s->reg_base + I2S_CTL_REG);
        /* configure TX offset 1 */
        tx_offset &= ~I2S_TXCHSEL_TXOFFSET_MASK;
        tx_offset |= I2S_TXCHSEL_OFFSET_1;
        writel(tx_offset, i2s->reg_base + I2S_TXCHSEL_REG);
        /* configure RX offset 0 */
        rx_offset &= ~I2S_RXCHSEL_RXOFFSET_MASK;
        rx_offset |= I2S_RXCHSEL_RXOFFSET_1;
        writel(rx_offset, i2s->reg_base + I2S_RXCHSEL_REG);
        break;
    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

int hal_i2s_sample_width_select(aic_i2s_ctrl *i2s, i2s_sample_width_t width)
{
    CHECK_PARAM(!(width < 8 || width > 32), -EINVAL);
    CHECK_PARAM(!(width % 4), -EINVAL);

    uint8_t reg_val, width_select;

    reg_val = readl(i2s->reg_base + I2S_FMT0_REG);
    reg_val &= ~(I2S_FMT0_SR_MASK | I2S_FMT0_SW_MASK);
    width_select = (width - 8) / 4 + 1;

    /* configure slot width equal to sample wodth */
    reg_val |= I2S_FMT0_SR(width_select) | I2S_FMT0_SW(width_select);
    writel(reg_val, i2s->reg_base + I2S_FMT0_REG);
    return 0;
}

int hal_i2s_mclk_set(aic_i2s_ctrl *i2s, i2s_sample_rate_t sample_rate,
                     uint32_t mclk_nfs)
{
    uint32_t mclk_div, module_rate, reg_val, i;

    module_rate = hal_clk_get_freq(i2s->clk_id);

    mclk_div = module_rate / sample_rate / mclk_nfs;

    for (i = 0; i < ARRAY_SIZE(i2s_bmclk_div); i++) {
        if (mclk_div == i2s_bmclk_div[i].div) {
            mclk_div = i2s_bmclk_div[i].val;
            break;
        }
    }

    if (i == ARRAY_SIZE(i2s_bmclk_div))
        return -EINVAL;

    reg_val = readl(i2s->reg_base + I2S_CLKD_REG);
    reg_val &= ~I2S_CLKD_MCLKDIV_MASK;
    reg_val |= I2S_CLKD_MCLKDIV(mclk_div);
    writel(reg_val, i2s->reg_base + I2S_CLKD_REG);
    return 0;
}

void hal_i2s_polarity_set(aic_i2s_ctrl *i2s, i2s_polarity_t polarity)
{
    uint32_t reg_val;

    reg_val = readl(i2s->reg_base + I2S_FMT0_REG);
    reg_val &= ~I2S_FMT0_LRCK_POL_MASK;
    if (polarity)
        reg_val |= I2S_FMT0_LRCK_POL_INVERTED;

    writel(reg_val, i2s->reg_base + I2S_FMT0_REG);
}

int hal_i2s_sclk_set(aic_i2s_ctrl *i2s, i2s_sample_rate_t sample_rate,
                     uint32_t sclk_nfs)
{
    uint32_t module_rate, reg_val, bclk_div, i;

    module_rate = hal_clk_get_freq(i2s->clk_id);

    /* calculate lrck period */
    reg_val = readl(i2s->reg_base + I2S_CTL_REG);
    if (reg_val & I2S_CTL_MODE_MASK) {
        /* I2S mode */
        reg_val = readl(i2s->reg_base + I2S_FMT0_REG);
        reg_val &= ~I2S_FMT0_LRCK_PERIOD_MASK;
        reg_val |= I2S_FMT0_LRCK_PERIOD(sclk_nfs / 2 - 1);
        writel(reg_val, i2s->reg_base + I2S_FMT0_REG);
    } else {
        /* PCM mode */
        reg_val = readl(i2s->reg_base + I2S_FMT0_REG);
        reg_val &= ~I2S_FMT0_LRCK_PERIOD_MASK;
        reg_val |= I2S_FMT0_LRCK_PERIOD(sclk_nfs - 1);
        writel(reg_val, i2s->reg_base + I2S_FMT0_REG);
    }

    /* calculate bclk divider */
    bclk_div = module_rate / sample_rate / sclk_nfs;

    for (i = 0; i < ARRAY_SIZE(i2s_bmclk_div); i++) {
        if (bclk_div == i2s_bmclk_div[i].div) {
            bclk_div = i2s_bmclk_div[i].val;
            break;
        }
    }

    if (i == ARRAY_SIZE(i2s_bmclk_div))
        return -EINVAL;

    reg_val = readl(i2s->reg_base + I2S_CLKD_REG);
    reg_val &= ~I2S_CLKD_BCLKDIV_MASK;
    reg_val |= I2S_CLKD_BCLKDIV(bclk_div);
    writel(reg_val, i2s->reg_base + I2S_CLKD_REG);
    return 0;
}

void hal_i2s_channel_select(aic_i2s_ctrl *i2s,
                            i2s_sound_channel_t channel, i2s_stream_t stream)
{
    uint32_t reg_val;

    switch(channel) {
    case I2S_LEFT_CHANNEL:
        /* left channel */
        writel(0, i2s->reg_base + I2S_CHCFG_REG);
        if (!stream) {
            reg_val = readl(i2s->reg_base + I2S_TXCHSEL_REG);
            reg_val &= ~(I2S_TXCHSEL_TXCHEN_MASK | I2S_TXCHSEL_TXCHSEL_MASK);
            reg_val |= (I2S_TXCHSEL_TXCHEN(1) | I2S_TXCHSEL_TXCHSEL(1));
            writel(reg_val, i2s->reg_base + I2S_TXCHSEL_REG);
            writel(0, i2s->reg_base + I2S_TXCHMAP1_REG);
        } else {
            reg_val = readl(i2s->reg_base + I2S_RXCHSEL_REG);
            reg_val &= ~I2S_RXCHSEL_RXCHSEL_MASK;
            writel(reg_val, i2s->reg_base + I2S_RXCHSEL_REG);
            writel(0, i2s->reg_base + I2S_RXCHMAP1_REG);
        }
        break;
    case I2S_RIGHT_CHANNEL:
        /* right channel */
        writel(0, i2s->reg_base + I2S_CHCFG_REG);
        if (!stream) {
            reg_val = readl(i2s->reg_base + I2S_TXCHSEL_REG);
            reg_val &= ~(I2S_TXCHSEL_TXCHEN_MASK | I2S_TXCHSEL_TXCHSEL_MASK);
            reg_val |= ((1 << 1) | I2S_TXCHSEL_TXCHSEL(1));
            writel(reg_val, i2s->reg_base + I2S_TXCHSEL_REG);
            /* Map channel1 to first sample data */
            writel(0, i2s->reg_base + I2S_TXCHMAP1_REG);
        } else {
            reg_val = readl(i2s->reg_base + I2S_RXCHSEL_REG);
            reg_val &= ~I2S_RXCHSEL_RXCHSEL_MASK;
            writel(reg_val, i2s->reg_base + I2S_RXCHSEL_REG);
            writel(0, i2s->reg_base + I2S_RXCHMAP1_REG);
        }
        break;
    case I2S_LEFT_RIGHT_CHANNEL:
    default:
        writel(0x11, i2s->reg_base + I2S_CHCFG_REG);
        /* left right channel */
        if (!stream) {
            reg_val = readl(i2s->reg_base + I2S_TXCHSEL_REG);
            reg_val &= ~(I2S_TXCHSEL_TXCHEN_MASK | I2S_TXCHSEL_TXCHSEL_MASK);
            reg_val |= (I2S_TXCHSEL_TXCHEN(2) | I2S_TXCHSEL_TXCHSEL(2));
            writel(reg_val, i2s->reg_base + I2S_TXCHSEL_REG);
            /* Map channel*/
            writel(0x10, i2s->reg_base + I2S_TXCHMAP1_REG);
        } else {
            reg_val = readl(i2s->reg_base + I2S_RXCHSEL_REG);
            reg_val &= ~I2S_RXCHSEL_RXCHSEL_MASK;
            reg_val |= I2S_RXCHSEL_RXCHSEL(2);
            writel(reg_val, i2s->reg_base + I2S_RXCHSEL_REG);
            /* Map channel*/
            writel(0x10, i2s->reg_base + I2S_RXCHMAP1_REG);
        }
        break;
    }
}

static void i2s_dma_transfer_period_callback(void *arg)
{
    struct aic_i2s_transfer_info *info;
    aic_i2s_ctrl *i2s;

    info = (struct aic_i2s_transfer_info *)arg;

    if (info->transfer_type == I2S_TRANSFER_TYPE_TX)
    {
        i2s = container_of(info, aic_i2s_ctrl, tx_info);
        if (i2s->callback)
            i2s->callback(i2s, (void *)I2S_TX_PERIOD_INT);
    }
    else if (info->transfer_type == I2S_TRANSFER_TYPE_RX)
    {
        i2s = container_of(info, aic_i2s_ctrl, rx_info);
        if (i2s->callback)
            i2s->callback(i2s, (void *)I2S_RX_PERIOD_INT);
    }
}

void hal_i2s_playback_start(aic_i2s_ctrl *i2s, i2s_format_t *format)
{
    struct dma_slave_config config;
    struct aic_i2s_transfer_info *info;

    config.direction = DMA_MEM_TO_DEV;
    config.dst_addr = i2s->reg_base + I2S_TXFIFO_REG;
    config.slave_id = DMA_ID_I2S0 + i2s->idx;
    config.src_maxburst = 1;
    config.dst_maxburst = 1;

    switch (format->width)
    {
    case 8:
        config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
        config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
        break;
    case 16:
        config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
        config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
        break;
    case 24:
        config.src_addr_width = DMA_SLAVE_BUSWIDTH_3_BYTES;
        config.dst_addr_width = DMA_SLAVE_BUSWIDTH_3_BYTES;
        break;
    case 32:
        config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
        config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
        break;
    default:
        hal_log_err("I2S%u not support %u sample rate\n",
                    i2s->idx, format->width);
        return;
    }

    info = &i2s->tx_info;

    info->transfer_type = I2S_TRANSFER_TYPE_TX;

    info->dma_chan = hal_request_dma_chan();
    if (!info->dma_chan) {
        hal_log_err("I2S%u request dma channel error\n", i2s->idx);
        return;
    }

    hal_dma_chan_register_cb(info->dma_chan, i2s_dma_transfer_period_callback,
                             (void *)info);
    hal_dma_chan_config(info->dma_chan, &config);
    /* Configure DMA transfer */
    hal_dma_chan_prep_cyclic(info->dma_chan, (ulong)info->buf_info.buf,
                             info->buf_info.buf_len, info->buf_info.period_len,
                             DMA_MEM_TO_DEV);
    hal_dma_chan_start(info->dma_chan);
    /* flush TXFIFO */
    hal_i2s_clear_tx_fifo(i2s);
    /* clear TX counter */
    hal_i2s_clear_tx_counter(i2s);
    /* Enable MCLK OUT */
    hal_i2s_mclk_out_enable(i2s);
    /* configure TXFIFO input mode */
    hal_i2s_txfifo_input_mode(i2s);
    hal_i2s_enable_tx_block(i2s);
    hal_i2s_enable_data_out(i2s);
    hal_i2s_enable_tx_drq(i2s);
}

void hal_i2s_record_start(aic_i2s_ctrl *i2s, i2s_format_t *format)
{
    struct dma_slave_config config;
    struct aic_i2s_transfer_info *info;

    config.direction = DMA_DEV_TO_MEM;
    config.src_addr = i2s->reg_base + I2S_RXFIFO_REG;
    config.slave_id = DMA_ID_I2S0 + i2s->idx;
    config.src_maxburst = 1;
    config.dst_maxburst = 1;

    switch (format->width)
    {
    case 8:
        config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
        config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
        break;
    case 16:
        config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
        config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
        break;
    case 24:
        config.src_addr_width = DMA_SLAVE_BUSWIDTH_3_BYTES;
        config.dst_addr_width = DMA_SLAVE_BUSWIDTH_3_BYTES;
        break;
    case 32:
        config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
        config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
        break;
    default:
        hal_log_err("I2S%u not support %u sample rate\n",
                    i2s->idx, format->width);
        return;
    }

    info = &i2s->rx_info;

    info->transfer_type = I2S_TRANSFER_TYPE_RX;

    info->dma_chan = hal_request_dma_chan();
    if (!info->dma_chan) {
        hal_log_err("I2S%u request dma channel error\n", i2s->idx);
        return;
    }

    hal_dma_chan_register_cb(info->dma_chan, i2s_dma_transfer_period_callback,
                             (void *)info);
    hal_dma_chan_config(info->dma_chan, &config);
    /* Configure DMA transfer */
    hal_dma_chan_prep_cyclic(info->dma_chan, (ulong)info->buf_info.buf,
                             info->buf_info.buf_len, info->buf_info.period_len,
                             DMA_DEV_TO_MEM);
    hal_dma_chan_start(info->dma_chan);
    /* flush RXFIFO */
    hal_i2s_clear_rx_fifo(i2s);
    /* clear RX counter */
    hal_i2s_clear_rx_counter(i2s);
    /* Enable MCLK OUT */
    hal_i2s_mclk_out_enable(i2s);
    /* configure RXFIFO output mode */
    hal_i2s_rxfifo_output_mode(i2s);
    /* Enable RX block */
    hal_i2s_enable_rx_block(i2s);
    /* Enable RX DRQ */
    hal_i2s_enable_rx_drq(i2s);
}

void hal_i2s_playback_stop(aic_i2s_ctrl *i2s)
{
    struct aic_i2s_transfer_info *info;

    info = &i2s->tx_info;

    hal_i2s_disable_tx_drq(i2s);
    hal_i2s_disable_tx_block(i2s);
    hal_dma_chan_stop(info->dma_chan);
    hal_release_dma_chan(info->dma_chan);
}

void hal_i2s_record_stop(aic_i2s_ctrl *i2s)
{
    struct aic_i2s_transfer_info *info;

    info = &i2s->rx_info;

    hal_i2s_disable_rx_drq(i2s);
    hal_i2s_disable_rx_block(i2s);
    hal_dma_chan_stop(info->dma_chan);
    hal_release_dma_chan(info->dma_chan);
}

void hal_i2s_attach_callback(aic_i2s_ctrl *i2s, i2s_callback callback,
                             void *arg)
{
    i2s->callback = callback;
    i2s->arg = arg;
}

void hal_i2s_detach_callback(aic_i2s_ctrl *i2s)
{
    i2s->callback = NULL;
    i2s->arg = NULL;
}
