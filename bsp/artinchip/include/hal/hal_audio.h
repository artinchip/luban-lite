/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */

#ifndef __HAL_AUDIOCODEC_H__
#define __HAL_AUDIOCODEC_H__

#include <aic_core.h>
#include "hal_dma.h"
#include "aic_hal_gpio.h"
#include "aic_hal_clk.h"
#include "hal_audio_reg.h"

#define AUDIO_TX_PERIOD_INT                 1
#define AUDIO_RX_DMIC_PERIOD_INT            2
#define AUDIO_RX_AMIC_PERIOD_INT            3

#define AUDIO_TRANSFER_TYPE_TX              4
#define AUDIO_TRANSFER_TYPE_DMIC            5
#define AUDIO_TRANSFER_TYPE_AMIC            6

#define MAX_VOLUME_0DB                      160

struct aic_audio_config
{
    uint32_t samplerate;
    uint32_t channel;
    uint32_t samplebits;
};

struct aic_audio_buf_info
{
    void *buf;
    uint32_t buf_len;
    uint32_t period_len;
};

struct aic_audio_transfer_info
{
    struct aic_dma_chan *dma_chan;
    struct aic_audio_buf_info buf_info;
    int transfer_type;
};

typedef struct aic_audio_ctrl aic_audio_ctrl;
typedef void (*audio_callback)(aic_audio_ctrl *codec, void *arg);

struct aic_audio_ctrl
{
    unsigned long reg_base;
    uint32_t irq_num;
    uint32_t clk_id;
    struct aic_audio_transfer_info tx_info;
    struct aic_audio_transfer_info dmic_info;
    struct aic_audio_transfer_info amic_info;
    audio_callback callback;
    void *arg;
    struct aic_audio_config config;
};

static inline void hal_audio_flush_tx_fifo(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + TXFIFO_CTRL_REG);
    reg_val |= TXFIFO_FLUSH;
    writel(reg_val, codec->reg_base + TXFIFO_CTRL_REG);
}

static inline void hal_audio_flush_dmic_fifo(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + DMIC_RXFIFO_CTRL_REG);
    reg_val |= DMIC_RXFIFO_FLUSH;
    writel(reg_val, codec->reg_base + DMIC_RXFIFO_CTRL_REG);
}

static inline void hal_audio_flush_amic_fifo(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + ADC_RXFIFO_CTRL_REG);
    reg_val |= ADC_RXFIFO_FLUSH;
    writel(reg_val, codec->reg_base + ADC_RXFIFO_CTRL_REG);
}

static inline void hal_audio_tx_enable_drq(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + FIFO_INT_EN_REG);
    reg_val |= FIFO_AUDOUT_DRQ_EN;
    writel(reg_val, codec->reg_base + FIFO_INT_EN_REG);
}

static inline void hal_audio_dmic_enable_drq(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + FIFO_INT_EN_REG);
    reg_val |= FIFO_DMICIN_DRQ_EN;
    writel(reg_val, codec->reg_base + FIFO_INT_EN_REG);
}

static inline void hal_audio_amic_enable_drq(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + ADC_RXFIFO_INT_EN_REG);
    reg_val |= ADC_RXFIFO_ADCIN_DRQ_EN;
    writel(reg_val, codec->reg_base + ADC_RXFIFO_INT_EN_REG);
}

static inline void hal_audio_tx_disable_drq(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + FIFO_INT_EN_REG);
    reg_val &= ~FIFO_AUDOUT_DRQ_EN;
    writel(reg_val, codec->reg_base + FIFO_INT_EN_REG);
}

static inline void hal_audio_dmic_disable_drq(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + FIFO_INT_EN_REG);
    reg_val &= ~FIFO_DMICIN_DRQ_EN;
    writel(reg_val, codec->reg_base + FIFO_INT_EN_REG);
}

static inline void hal_audio_amic_disable_drq(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + ADC_RXFIFO_INT_EN_REG);
    reg_val &= ~ADC_RXFIFO_ADCIN_DRQ_EN;
    writel(reg_val, codec->reg_base + ADC_RXFIFO_INT_EN_REG);
}

static inline void hal_audio_enable_tx_global(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + GLOBE_CTL_REG);
    reg_val |= GLOBE_TX_GLBEN;
    writel(reg_val, codec->reg_base + GLOBE_CTL_REG);
}

static inline void hal_audio_enable_rx_global(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + GLOBE_CTL_REG);
    reg_val |= GLOBE_RX_GLBEN;
    writel(reg_val, codec->reg_base + GLOBE_CTL_REG);
}

static inline void hal_audio_disable_tx_global(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + GLOBE_CTL_REG);
    reg_val &= ~GLOBE_TX_GLBEN;
    writel(reg_val, codec->reg_base + GLOBE_CTL_REG);
}

static inline void hal_audio_disable_rx_global(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + GLOBE_CTL_REG);
    reg_val &= ~GLOBE_RX_GLBEN;
    writel(reg_val, codec->reg_base + GLOBE_CTL_REG);
}

static inline void hal_audio_global_reset(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + GLOBE_CTL_REG);
    reg_val |= GLOBE_GLB_RST;
    writel(reg_val, codec->reg_base + GLOBE_CTL_REG);
}

static inline void hal_audio_set_playback_volume(aic_audio_ctrl *codec,
                                                 uint32_t volume)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + TX_DVC_3_4_CTRL_REG);
    reg_val &= ~(0xFFFF << TX_DVC3_GAIN);
    reg_val |= (volume << TX_DVC4_GAIN | volume << TX_DVC3_GAIN);
    writel(reg_val, codec->reg_base + TX_DVC_3_4_CTRL_REG);
}

static inline void hal_audio_set_dmic_volume(aic_audio_ctrl *codec,
                                             uint32_t volume)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + RX_DVC_1_2_CTRL_REG);
    reg_val &= ~(0xFFFF << RX_DVC1_GAIN);
    reg_val |= (volume << RX_DVC2_GAIN | volume << RX_DVC1_GAIN);
    writel(reg_val, codec->reg_base + RX_DVC_1_2_CTRL_REG);
}

static inline void hal_audio_set_amic_volume(aic_audio_ctrl *codec,
                                             uint32_t volume)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + ADC_DVC0_CTRL_REG);
    reg_val &= ~(0xFF << ADC_DVC0_CTRL_DVC0_GAIN);
    reg_val |= (volume << ADC_DVC0_CTRL_DVC0_GAIN);
    writel(reg_val, codec->reg_base + ADC_DVC0_CTRL_REG);
}

static inline void hal_audio_set_pwm0_differential(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + TX_PWM_CTRL_REG);
    reg_val &= ~(TX_PWM0_MASK | TX_PWM1_MASK);
    reg_val |= TX_PWM0_EN | TX_PWM0_DIFEN;
    writel(reg_val, codec->reg_base + TX_PWM_CTRL_REG);
}

static inline void hal_audio_set_pwm1_differential(aic_audio_ctrl *codec)
{
    uint32_t reg_val;

    reg_val = readl(codec->reg_base + TX_PWM_CTRL_REG);
    reg_val &= ~(TX_PWM0_MASK | TX_PWM1_MASK);
    reg_val |= TX_PWM1_EN | TX_PWM1_DIFEN;
    writel(reg_val, codec->reg_base + TX_PWM_CTRL_REG);
}

static inline void hal_audio_disable_fade(aic_audio_ctrl *codec)
{
    writel(0, codec->reg_base + FADE_CTRL0_REG);
}

int hal_audio_init(aic_audio_ctrl *codec);
int hal_audio_uninit(aic_audio_ctrl *codec);
void hal_audio_set_samplerate(aic_audio_ctrl *codec, uint32_t samplerate);
void hal_audio_set_playback_channel(aic_audio_ctrl *codec, uint32_t ch);
void hal_audio_set_playback_by_spk0(aic_audio_ctrl *codec);
void hal_audio_set_playback_by_spk1(aic_audio_ctrl *codec);
void hal_audio_set_dmic_channel(aic_audio_ctrl *codec, uint32_t ch);
void hal_audio_set_amic_channel(aic_audio_ctrl *codec);
void hal_audio_playback_start(aic_audio_ctrl *codec);
void hal_audio_playback_stop(aic_audio_ctrl *codec);
void hal_audio_dmic_start(aic_audio_ctrl *codec);
void hal_audio_dmic_stop(aic_audio_ctrl *codec);
void hal_audio_amic_start(aic_audio_ctrl *codec);
void hal_audio_amic_stop(aic_audio_ctrl *codec);
void hal_audio_attach_callback(aic_audio_ctrl *codec,
                               audio_callback callback, void *arg);
void hal_audio_detach_callback(aic_audio_ctrl *codec);

#endif
