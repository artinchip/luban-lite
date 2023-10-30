/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */

#include "hal_audio.h"

int hal_audio_init(aic_audio_ctrl *codec)
{
    int ret;
    codec->reg_base = AUDIO_BASE;
    codec->irq_num = AUDIO_IRQn;
    codec->clk_id = CLK_CODEC;

    ret = hal_clk_set_freq(codec->clk_id, DEFAULT_AUDIO_FREQ);
    if (ret)
        hal_log_err("Audio init set freq error!\n");

    ret = hal_clk_enable_deassertrst_iter(codec->clk_id);
    if (ret)
        hal_log_err("Audio init error!\n");

    return ret;
}

int hal_audio_uninit(aic_audio_ctrl *codec)
{
    int ret;

    ret = hal_clk_disable_assertrst(codec->clk_id);
    if (ret)
        hal_log_err("Audio uninit error!\n");

    return ret;
}

static int hal_audio_get_hw_rate(aic_audio_ctrl *codec, uint32_t samplerate)
{
    switch (samplerate)
    {
    case 48000:
    case 44100:
        return 0;
    case 32000:
        return 1;
    case 24000:
    case 22050:
        return 2;
    case 16000:
        return 3;
    case 12000:
    case 11025:
        return 4;
    case 8000:
        return 5;
    default:
        return -EINVAL;
    }
}

static uint32_t hal_audio_get_module_freq(aic_audio_ctrl *codec,
                                          uint32_t samplerate)
{
    uint32_t reg_val;

    switch (samplerate)
    {
    case 48000:
    case 32000:
    case 24000:
    case 16000:
    case 12000:
    case 8000:
        /* Set playback module clock */
        reg_val = readl(codec->reg_base + TX_PLAYBACK_CTRL_REG);
        reg_val &= ~TX_CLK_MASK;
        writel(reg_val, codec->reg_base + TX_PLAYBACK_CTRL_REG);
        /* Set dmic module clock */
        reg_val = readl(codec->reg_base + RX_DMIC_IF_CTRL_REG);
        reg_val &= ~RX_DMIC_IF_RX_CLK_MASK;
        writel(reg_val, codec->reg_base + RX_DMIC_IF_CTRL_REG);
        /* Set amic module clock */
        reg_val = readl(codec->reg_base + ADC_IF_CTRL_REG);
        reg_val &= ~ADC_IF_CTRL_RX_CLK_FRE_MASK;
        writel(reg_val, codec->reg_base + ADC_IF_CTRL_REG);
        return 24576000;
    case 44100:
    case 22050:
    case 11025:
        /* Set playback module clock */
        reg_val = readl(codec->reg_base + TX_PLAYBACK_CTRL_REG);
        reg_val |= TX_CLK_22579KHZ;
        writel(reg_val, codec->reg_base + TX_PLAYBACK_CTRL_REG);
        /* set dmic module clock */
        reg_val = readl(codec->reg_base + RX_DMIC_IF_CTRL_REG);
        reg_val |= RX_DMIC_IF_RX_CLK_22579KHZ;
        writel(reg_val, codec->reg_base + RX_DMIC_IF_CTRL_REG);
        /* set amic module clock */
        reg_val = readl(codec->reg_base + ADC_IF_CTRL_REG);
        reg_val |= ADC_IF_CTRL_RX_CLK_22579KHZ;
        writel(reg_val, codec->reg_base + ADC_IF_CTRL_REG);
        return 22579200;
    default:
        return 0;
    }
}

void hal_audio_set_samplerate(aic_audio_ctrl *codec, uint32_t samplerate)
{
    int hw_rate;
    uint32_t reg_val, module_freq;
    unsigned int pclk_id;

    hw_rate = hal_audio_get_hw_rate(codec, samplerate);
    /* Set playback samplerate */
    reg_val = readl(codec->reg_base + TX_PLAYBACK_CTRL_REG);
    reg_val &= ~TX_FS_OUT_MASK;
    reg_val |= TX_FS_OUT(hw_rate);
    writel(reg_val, codec->reg_base + TX_PLAYBACK_CTRL_REG);
    /* Set dmic samplerate */
    reg_val = readl(codec->reg_base + RX_DMIC_IF_CTRL_REG);
    reg_val &= ~RX_DMIC_IF_FS_IN_MASK;
    reg_val |= RX_DMIC_IF_FS_IN(hw_rate);
    writel(reg_val, codec->reg_base + RX_DMIC_IF_CTRL_REG);
    /* Set amic samplerate */
    reg_val = readl(codec->reg_base + ADC_IF_CTRL_REG);
    reg_val &= ~ADC_IF_CTRL_FS_ADC_IN_MASK;
    reg_val |= ADC_IF_CTRL_FS_ADC_IN(hw_rate);
    writel(reg_val, codec->reg_base + ADC_IF_CTRL_REG);

    module_freq = hal_audio_get_module_freq(codec, samplerate);
    pclk_id = hal_clk_get_parent(codec->clk_id);
#ifdef AIC_AUDIO_DRV_V10
    /* Set AudioCodec parent clock rate */
    hal_clk_set_freq(pclk_id, module_freq * 20);
    /* Set AudioCodec module clock rate */
    hal_clk_set_rate(codec->clk_id, module_freq, module_freq * 20);
#elif defined(AIC_AUDIO_DRV_V11)
    /* Audio has the same frequency with AUDIO_SCLK  */
    hal_clk_set_freq(pclk_id, module_freq);
#endif
}

void hal_audio_set_playback_channel(aic_audio_ctrl *codec, uint32_t ch)
{
    uint32_t reg_val;

    if (ch == 1)
    {
        /* Enable TX ch0 */
        reg_val = readl(codec->reg_base + TXFIFO_CTRL_REG);
        reg_val &= ~TXFIFO_CH_MASK;
        reg_val |= TXFIFO_CH0_EN;
        writel(reg_val, codec->reg_base + TXFIFO_CTRL_REG);
    }
    else if (ch == 2)
    {
        /* Enable TX ch0 ch1 */
        reg_val = readl(codec->reg_base + TXFIFO_CTRL_REG);
        reg_val &= ~TXFIFO_CH_MASK;
        reg_val |= (TXFIFO_CH0_EN | TXFIFO_CH1_EN);
        writel(reg_val, codec->reg_base + TXFIFO_CTRL_REG);
    }
}

void hal_audio_set_playback_by_spk0(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    /* Enable DVC3 */
    reg_val = readl(codec->reg_base + TX_DVC_3_4_CTRL_REG);
    reg_val &= ~TX_DVC3_MASK;
    reg_val |= TX_DVC3_EN;
    writel(reg_val, codec->reg_base + TX_DVC_3_4_CTRL_REG);
    /* Enable IF0 */
    reg_val = readl(codec->reg_base + TX_PLAYBACK_CTRL_REG);
    reg_val &= ~TX_IF_CH0_MASK;
    reg_val |= TX_PLAYBACK_IF_EN | TX_IF_CH0_EN;
    writel(reg_val, codec->reg_base + TX_PLAYBACK_CTRL_REG);
    /* Enable SDM0 */
    reg_val = readl(codec->reg_base + TX_SDM_CTRL_REG);
    reg_val &= ~TX_SDM_CH0_MASK;
    reg_val |= TX_SDM_CH0_EN;
    writel(reg_val, codec->reg_base + TX_SDM_CTRL_REG);
    /* Configure MIXER audio path */
    reg_val = readl(codec->reg_base + TX_MIXER_CTRL_REG);
    reg_val &= ~TX_MIXER0_PATH_MASK;
    reg_val |= TX_MIXER0_AUDOUTL_SEL | TX_MIXER0_AUDOUTR_SEL;
    writel(reg_val, codec->reg_base + TX_MIXER_CTRL_REG);
    /* Configure PWM0 single output */
    reg_val = readl(codec->reg_base + TX_PWM_CTRL_REG);
    reg_val &= ~TX_PWM0_MASK;
    reg_val |= TX_PWM0_EN;
    writel(reg_val, codec->reg_base + TX_PWM_CTRL_REG);
}

void hal_audio_set_playback_by_spk1(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    /* Enable DVC4 */
    reg_val = readl(codec->reg_base + TX_DVC_3_4_CTRL_REG);
    reg_val &= ~TX_DVC4_MASK;
    reg_val |= TX_DVC4_EN;
    writel(reg_val, codec->reg_base + TX_DVC_3_4_CTRL_REG);
    /* Enable IF1 */
    reg_val = readl(codec->reg_base + TX_PLAYBACK_CTRL_REG);
    reg_val &= ~TX_IF_CH1_MASK;
    reg_val |= TX_PLAYBACK_IF_EN | TX_IF_CH1_EN;
    writel(reg_val, codec->reg_base + TX_PLAYBACK_CTRL_REG);
    /* Enable SDM1 */
    reg_val = readl(codec->reg_base + TX_SDM_CTRL_REG);
    reg_val &= ~TX_SDM_CH1_MASK;
    reg_val |= TX_SDM_CH1_EN;
    writel(reg_val, codec->reg_base + TX_SDM_CTRL_REG);
    /* Configure MIXER audio path */
    reg_val = readl(codec->reg_base + TX_MIXER_CTRL_REG);
    reg_val &= ~TX_MIXER1_PATH_MASK;
    reg_val |= TX_MIXER1_AUDOUTL_SEL | TX_MIXER1_AUDOUTR_SEL;
    writel(reg_val, codec->reg_base + TX_MIXER_CTRL_REG);
    /* Configure PWM1 single output */
    reg_val = readl(codec->reg_base + TX_PWM_CTRL_REG);
    reg_val &= ~TX_PWM1_MASK;
    reg_val |= TX_PWM1_EN;
    writel(reg_val, codec->reg_base + TX_PWM_CTRL_REG);
}

void hal_audio_set_dmic_channel(aic_audio_ctrl *codec, uint32_t ch)
{
    uint32_t reg_val;

    if (ch == 1)
    {
        /* Enable HPF1 */
        reg_val = readl(codec->reg_base + RX_HPF_1_2_CTRL_REG);
        reg_val &= ~RX_HPF_MASK;
        reg_val |= RX_HPF1_EN;
        writel(reg_val, codec->reg_base + RX_HPF_1_2_CTRL_REG);
        /* Enable DF1 and DMIC interface*/
        reg_val = readl(codec->reg_base + RX_DMIC_IF_CTRL_REG);
        reg_val &= ~RX_DMIC_IF_DEC_EN_MASK;
        reg_val |= RX_DMIC_IF_DEC1_FLT | RX_DMIC_IF_EN;
        writel(reg_val, codec->reg_base + RX_DMIC_IF_CTRL_REG);
        /* Enable DVC1 */
        reg_val = readl(codec->reg_base + RX_DVC_1_2_CTRL_REG);
        reg_val &= ~RX_DVC_MASK;
        reg_val |= RX_DVC1_EN;
        writel(reg_val, codec->reg_base + RX_DVC_1_2_CTRL_REG);
        /* Enable ch0 */
        reg_val = readl(codec->reg_base + DMIC_RXFIFO_CTRL_REG);
        reg_val &= ~DMIC_RXFIFO_CH_MASK;
        reg_val |= DMIC_RXFIFO_CH0_EN;
        writel(reg_val, codec->reg_base + DMIC_RXFIFO_CTRL_REG);
    }
    else if (ch == 2)
    {
        /* Enable HPF1_2 */
        reg_val = readl(codec->reg_base + RX_HPF_1_2_CTRL_REG);
        reg_val &= ~RX_HPF_MASK;
        reg_val |= RX_HPF1_EN | RX_HPF2_EN;
        writel(reg_val, codec->reg_base + RX_HPF_1_2_CTRL_REG);
        /* Enable DF1_2 and DMIC interface*/
        reg_val = readl(codec->reg_base + RX_DMIC_IF_CTRL_REG);
        reg_val &= ~RX_DMIC_IF_DEC_EN_MASK;
        reg_val |= RX_DMIC_IF_DEC1_FLT | RX_DMIC_IF_DEC2_FLT | RX_DMIC_IF_EN;
        writel(reg_val, codec->reg_base + RX_DMIC_IF_CTRL_REG);
        /* Enable DVC1 DVC2 */
        reg_val = readl(codec->reg_base + RX_DVC_1_2_CTRL_REG);
        reg_val |= RX_DVC_MASK;
        writel(reg_val, codec->reg_base + RX_DVC_1_2_CTRL_REG);
        /* Enable ch0 ch1 */
        reg_val = readl(codec->reg_base + DMIC_RXFIFO_CTRL_REG);
        reg_val |= DMIC_RXFIFO_CH_MASK;
        writel(reg_val, codec->reg_base + DMIC_RXFIFO_CTRL_REG);
    }
}

void hal_audio_set_amic_channel(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    /* Enable HPF0 */
    reg_val = readl(codec->reg_base + ADC_HPF0_CTRL_REG);
    reg_val |= ADC_HPF0_CTRL_HPF0_EN;
    writel(reg_val, codec->reg_base + ADC_HPF0_CTRL_REG);
    /* Enable MBIAS PGA ADC */
    reg_val |= ADC_CTL1_ADC_EN | ADC_CTL1_PGA_EN | ADC_CTL1_MBIAS_EN;
    writel(reg_val, codec->reg_base + ADC_HPF0_CTRL_REG);
    /* Enable DF0 */
    reg_val = readl(codec->reg_base + ADC_IF_CTRL_REG);
    reg_val |= ADC_IF_CTRL_EN_DEC0_MASK;
    writel(reg_val, codec->reg_base + ADC_IF_CTRL_REG);
    /* Enable DVC0 */
    reg_val = readl(codec->reg_base + ADC_DVC0_CTRL_REG);
    reg_val |= ADC_DVC0_CTRL_DVC0_EN;
    writel(reg_val, codec->reg_base + ADC_DVC0_CTRL_REG);
    /* Enable amic channel */
    reg_val = readl(codec->reg_base + ADC_RXFIFO_CTRL_REG);
    reg_val |= ADC_RXFIFO_EN;
    writel(reg_val, codec->reg_base + ADC_RXFIFO_CTRL_REG);
}

static void dma_transfer_period_callback(void *arg)
{
    struct aic_audio_transfer_info *info;
    aic_audio_ctrl *codec;

    info = (struct aic_audio_transfer_info *)arg;

    if (info->transfer_type == AUDIO_TRANSFER_TYPE_TX)
    {
        codec = container_of(info, aic_audio_ctrl, tx_info);
        if (codec->callback)
            codec->callback(codec, (void *)AUDIO_TX_PERIOD_INT);
    }
    else if (info->transfer_type == AUDIO_TRANSFER_TYPE_DMIC)
    {
        codec = container_of(info, aic_audio_ctrl, dmic_info);
        if (codec->callback)
            codec->callback(codec, (void *)AUDIO_RX_DMIC_PERIOD_INT);
    }
    else if (info->transfer_type == AUDIO_TRANSFER_TYPE_AMIC)
    {
        codec = container_of(info, aic_audio_ctrl, amic_info);
        if (codec->callback)
            codec->callback(codec, (void *)AUDIO_RX_AMIC_PERIOD_INT);
    }
}

void hal_audio_playback_start(aic_audio_ctrl *codec)
{
    struct dma_slave_config config;
    struct aic_audio_transfer_info *info;

    config.direction = DMA_MEM_TO_DEV;
    config.dst_addr = codec->reg_base + TXFIFO_DATA_REG;
    config.slave_id = 14;
    config.src_maxburst = 1;
    config.dst_maxburst = 1;

    /* AudioCodec only support 16bit sample width */
    if (codec->config.channel == 2)
    {
        config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
        config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    }
    else if (codec->config.channel == 1)
    {
        config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
        config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
    }

    info = &codec->tx_info;

    info->transfer_type = AUDIO_TRANSFER_TYPE_TX;

    info->dma_chan = hal_request_dma_chan();
    if (!info->dma_chan) {
        hal_log_err("playback request dma channel error\n");
        return;
    }

    hal_dma_chan_register_cb(info->dma_chan, dma_transfer_period_callback,
                             (void *)info);
    hal_dma_chan_config(info->dma_chan, &config);
    /* Configure DMA transfer */
    hal_dma_chan_prep_cyclic(info->dma_chan, (ulong)info->buf_info.buf,
                             info->buf_info.buf_len, info->buf_info.period_len,
                             DMA_MEM_TO_DEV);
    hal_dma_chan_start(info->dma_chan);
    hal_audio_disable_fade(codec);
    /* flush TXFIFO */
    hal_audio_flush_tx_fifo(codec);
    /* Enable TX global */
    hal_audio_enable_tx_global(codec);
    /* Enable AUDOUT DRQ */
    hal_audio_tx_enable_drq(codec);
}

void hal_audio_dmic_start(aic_audio_ctrl *codec)
{
    struct dma_slave_config config;
    struct aic_audio_transfer_info *info;

    config.direction = DMA_DEV_TO_MEM;
    config.src_addr = codec->reg_base + DMIC_RXFIFO_DATA_REG;
    config.slave_id = 14;
    config.src_maxburst = 1;
    config.dst_maxburst = 1;

    /* AudioCodec only support 16bit sample width */
    if (codec->config.channel == 2)
    {
        config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
        config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    }
    else if (codec->config.channel == 1)
    {
        config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
        config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
    }

    info = &codec->dmic_info;

    info->transfer_type = AUDIO_TRANSFER_TYPE_DMIC;

    info->dma_chan = hal_request_dma_chan();
    if (!info->dma_chan) {
        hal_log_err("dmic request dma channel error\n");
        return;
    }

    hal_dma_chan_register_cb(info->dma_chan, dma_transfer_period_callback,
                             (void *)info);
    hal_dma_chan_config(info->dma_chan, &config);
    /* Configure DMA transfer */
    hal_dma_chan_prep_cyclic(info->dma_chan, (ulong)info->buf_info.buf,
                             info->buf_info.buf_len, info->buf_info.period_len,
                             DMA_DEV_TO_MEM);
    hal_dma_chan_start(info->dma_chan);
    /* flush DMIC FIFO */
    hal_audio_flush_dmic_fifo(codec);
    /* Enable RX global */
    hal_audio_enable_rx_global(codec);
    /* Enable DMIC DRQ */
    hal_audio_dmic_enable_drq(codec);
}

void hal_audio_amic_start(aic_audio_ctrl *codec)
{
    struct dma_slave_config config;
    struct aic_audio_transfer_info *info;

    config.direction = DMA_DEV_TO_MEM;
    config.src_addr = codec->reg_base + DMIC_RXFIFO_DATA_REG;
    config.slave_id = 15;
    config.src_maxburst = 1;
    config.dst_maxburst = 1;

    /* amic only support mono */
    config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
    config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;

    info = &codec->amic_info;

    info->transfer_type = AUDIO_TRANSFER_TYPE_AMIC;

    info->dma_chan = hal_request_dma_chan();
    if (!info->dma_chan) {
        hal_log_err("amic request dma channel error\n");
        return;
    }

    hal_dma_chan_register_cb(info->dma_chan, dma_transfer_period_callback,
                             (void *)info);

    hal_dma_chan_config(info->dma_chan, &config);
    /* Configure DMA transfer */
    hal_dma_chan_prep_cyclic(info->dma_chan, (ulong)info->buf_info.buf,
                             info->buf_info.buf_len, info->buf_info.period_len,
                             DMA_DEV_TO_MEM);
    hal_dma_chan_start(info->dma_chan);
    /* flush AMIC FIFO */
    hal_audio_flush_amic_fifo(codec);
    /* Enable RX global */
    hal_audio_enable_rx_global(codec);
    /* Enable DMIC DRQ */
    hal_audio_amic_enable_drq(codec);
}

void hal_audio_playback_stop(aic_audio_ctrl *codec)
{
    struct aic_audio_transfer_info *info;

    info = &codec->tx_info;

    hal_audio_tx_disable_drq(codec);
    hal_audio_disable_tx_global(codec);
    hal_dma_chan_stop(info->dma_chan);
    hal_release_dma_chan(info->dma_chan);
}

void hal_audio_dmic_stop(aic_audio_ctrl *codec)
{
    struct aic_audio_transfer_info *info;

    info = &codec->dmic_info;

    hal_audio_dmic_disable_drq(codec);
    hal_audio_disable_rx_global(codec);
    hal_dma_chan_stop(info->dma_chan);
    hal_release_dma_chan(info->dma_chan);
}

void hal_audio_amic_stop(aic_audio_ctrl *codec)
{
    struct aic_audio_transfer_info *info;

    info = &codec->amic_info;

    hal_audio_amic_disable_drq(codec);
    hal_audio_disable_rx_global(codec);
    hal_dma_chan_stop(info->dma_chan);
    hal_release_dma_chan(info->dma_chan);
}

void hal_audio_attach_callback(aic_audio_ctrl *codec, audio_callback callback,
                               void *arg)
{
    codec->callback = callback;
    codec->arg = arg;
}

void hal_audio_detach_callback(aic_audio_ctrl *codec)
{
    codec->callback = NULL;
    codec->arg = NULL;
}
