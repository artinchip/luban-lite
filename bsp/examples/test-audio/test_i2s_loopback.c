/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#include <rtthread.h>
#include <stdio.h>
#include <hal_i2s.h>
#include <hal_dma.h>

#define I2S_PERIOD_COUNT                2
#define I2S_BUFFER_SIZE                 128
#define I2S_PERIOD_SIZE                 (I2S_BUFFER_SIZE / I2S_PERIOD_COUNT)
#define MAX_COUNTER_VALUE               200

#ifdef AIC_USING_I2S0
#define I2S_IDX                         0
#elif defined(AIC_USING_I2S1)
#define I2S_IDX                         1
#endif

aic_i2s_ctrl i2s_ctrl;
uint8_t i2s_rx_in[I2S_BUFFER_SIZE] __attribute__((aligned(64)));
uint8_t i2s_tx_out[I2S_BUFFER_SIZE] __attribute__((aligned(64)));
volatile int val = 0;
volatile uint32_t tx_pos = 0;
volatile uint8_t rx_pos = 0;

static void drv_i2s_callback(aic_i2s_ctrl *pi2s, void *arg)
{
    int i;
    unsigned long event = (unsigned long)arg;

    switch (event)
    {
    case I2S_TX_PERIOD_INT:
        for (i = 0; i < I2S_PERIOD_SIZE; i++) {
            i2s_tx_out[tx_pos++] = val++;
            tx_pos %= I2S_BUFFER_SIZE;
            val %= MAX_COUNTER_VALUE;
        }
        aicos_dcache_clean_range((void *)i2s_tx_out, I2S_BUFFER_SIZE);
        break;
    case I2S_RX_PERIOD_INT:
        aicos_dcache_invalid_range((void *)i2s_rx_in, I2S_BUFFER_SIZE);
        for (i = 0; i < I2S_PERIOD_SIZE; i++) {
            rt_kprintf("%u: %u\n", rx_pos, i2s_rx_in[rx_pos]);
            rx_pos++;
        }
        rx_pos %= I2S_BUFFER_SIZE;
        break;
    default:
        hal_log_err("unknown event\n");
        break;
    }
}

void i2s_set_format(aic_i2s_ctrl *pi2s, i2s_format_t *format)
{
    format->mode = I2S_MODE_MASTER;
    format->protocol = I2S_PROTOCOL_I2S;
    format->polarity = I2S_LEFT_POLARITY_LOW;
    format->rate = I2S_SAMPLE_RATE_48000;
    format->width = I2S_SAMPLE_WIDTH_32BIT;
    format->slot_width = I2S_SAMPLE_WIDTH_32BIT;
    format->channel = I2S_TDM_CHANNEL_2;
    format->sclk_nfs = 64;
    format->mclk_nfs = 256;
    format->stream = I2S_STREAM_PLAYBACK;
    hal_i2s_set_format(pi2s, format);

    format->stream = I2S_STREAM_RECORD;
    hal_i2s_set_format(pi2s, format);
}

static int test_i2s_loopback(int argc, char *argv[])
{
    int i;
    i2s_format_t format;

    i2s_ctrl.rx_info.buf_info.buf = (void *)i2s_rx_in;
    i2s_ctrl.rx_info.buf_info.buf_len = I2S_BUFFER_SIZE;
    i2s_ctrl.rx_info.buf_info.period_len = I2S_BUFFER_SIZE / I2S_PERIOD_COUNT;

    i2s_ctrl.tx_info.buf_info.buf = (void *)i2s_tx_out;
    i2s_ctrl.tx_info.buf_info.buf_len = I2S_BUFFER_SIZE;
    i2s_ctrl.tx_info.buf_info.period_len = I2S_BUFFER_SIZE / I2S_PERIOD_COUNT;

    /*
     * TX use dual buffer mode, ecah buffer 64bytes.
     * Fill the first buffer.
     */
    for (tx_pos = 0; tx_pos < I2S_PERIOD_SIZE; tx_pos++) {
        i2s_tx_out[tx_pos] = val++;
    }

    /*
     * RX use dual buffer mode, ecah buffer 64bytes.
     * Clear dual buffer data to 0.
     */
    for (i = 0; i < I2S_BUFFER_SIZE; i++)
        i2s_rx_in[i] = 0;

    hal_i2s_init(&i2s_ctrl, I2S_IDX);
    /* Enable I2S loopback */
    hal_i2s_enable_loopback(&i2s_ctrl);
    hal_i2s_attach_callback(&i2s_ctrl, drv_i2s_callback, NULL);

    i2s_set_format(&i2s_ctrl, &format);

    hal_dma_init();
    aicos_request_irq(DMA_IRQn, hal_dma_irq, 0, NULL, NULL);
    /* Start capture */
    hal_i2s_record_start(&i2s_ctrl, &format);
    /* Start playback */
    hal_i2s_playback_start(&i2s_ctrl, &format);

    while (1)
    {

    }

    return RT_EOK;
}

MSH_CMD_EXPORT_ALIAS(test_i2s_loopback, test_i2s_loopback, test i2s loopback);
