/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <hal_audio.h>
#include <unistd.h>
#include "ringbuffer.h"
#include "sound.h"

#ifdef LPKG_USING_DFS
#include <dfs.h>
#include <dfs_fs.h>
#ifdef LPKG_USING_DFS_ELMFAT
#include <dfs_elm.h>
#endif
#endif

aic_audio_ctrl audio_ctrl;
ringbuf_t *ring_buf;
#define TX_FIFO_SIZE                    4096
#define TX_FIFO_PERIOD_COUNT            2
#define BUFFER_SIZE                     2048
rt_uint8_t audio_tx_fifo[TX_FIFO_SIZE] __attribute__((aligned(64)));

static void drv_audio_callback(aic_audio_ctrl *pcodec, void *arg)
{
    unsigned long event = (unsigned long)arg;

    switch (event)
    {
    case AUDIO_TX_PERIOD_INT:
        ring_buf->read += pcodec->tx_info.buf_info.period_len;
        ring_buf->data_len -= pcodec->tx_info.buf_info.period_len;
        aicos_dcache_clean_range((void *)audio_tx_fifo, TX_FIFO_SIZE);
        break;
    default:
        hal_log_err("%s(%d)\n", __func__, __LINE__);
        break;
    }

}

static int sound_aplay(int argc, char *argv[])
{
    int fd;
    struct wav_header *info = NULL;
    uint8_t *buffer = NULL;
    unsigned int pa, group, pin;
    uint8_t start_flag = 0;

    if (argc != 2)
    {
        hal_log_err("Usage:\n");
        hal_log_err("aplay song.wav\n");
        return 0;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        hal_log_err("open file failed!\n");
        goto __exit;
    }

    buffer = aicos_malloc(MEM_CMA, BUFFER_SIZE);
    if (!buffer)
    {
        hal_log_err("buffer malloc error!\n");
        goto __exit;
    }

    ring_buf = aicos_malloc(MEM_CMA, sizeof(ringbuf_t));
    if (!ring_buf)
    {
        hal_log_err("ring_buf malloc error!\n");
        goto __exit;
    }

    ring_buf->buffer = audio_tx_fifo;
    ring_buf->size = TX_FIFO_SIZE;
    ring_buf->write = 0;
    ring_buf->read = 0;
    ring_buf->data_len = 0;

    info = aicos_malloc(MEM_CMA, sizeof(struct wav_header));
    if (!info)
    {
        hal_log_err("malloc error!\n");
        goto __exit;
    }

    if (read(fd, info, sizeof(struct wav_header)) <= 0)
    {
        hal_log_err("read info error\n");
        goto __exit;
    }

    hal_log_info("wav information:\n");
    hal_log_info("samplerate %u\n", info->fmt_sample_rate);
    hal_log_info("channel %u\n", info->fmt_channels);

    audio_ctrl.tx_info.buf_info.buf = (void *)audio_tx_fifo;
    audio_ctrl.tx_info.buf_info.buf_len = TX_FIFO_SIZE;
    audio_ctrl.tx_info.buf_info.period_len = TX_FIFO_SIZE / TX_FIFO_PERIOD_COUNT;

    hal_audio_init(&audio_ctrl);
    hal_audio_attach_callback(&audio_ctrl, drv_audio_callback, NULL);

    /* Configure audio format */
    /* AudioCodec only support 16 bits */
    hal_audio_set_playback_channel(&audio_ctrl, info->fmt_channels);
    hal_audio_set_samplerate(&audio_ctrl, info->fmt_sample_rate);
    audio_ctrl.config.samplerate = info->fmt_sample_rate;
    audio_ctrl.config.channel = info->fmt_channels;
    audio_ctrl.config.samplebits = 16;

#ifdef AIC_AUDIO_SPK_0
    hal_audio_set_playback_by_spk0(&audio_ctrl);
#endif
#ifdef AIC_AUDIO_SPK_1
    hal_audio_set_playback_by_spk1(&audio_ctrl);
#endif
#ifdef AIC_AUDIO_SPK_0_1
    hal_audio_set_playback_by_spk0(&audio_ctrl);
    hal_audio_set_playback_by_spk1(&audio_ctrl);
#ifdef AIC_AUDIO_SPK0_OUTPUT_DIFFERENTIAL
    hal_audio_set_pwm0_differential(&audio_ctrl);
#endif
#ifdef AIC_AUDIO_SPK1_OUTPUT_DIFFERENTIAL
    hal_audio_set_pwm1_differential(&audio_ctrl);
#endif
#endif

    pa = hal_gpio_name2pin(AIC_AUDIO_PA_ENABLE_GPIO);
    group = GPIO_GROUP(pa);
    pin = GPIO_GROUP_PIN(pa);
    hal_gpio_set_func(group, pin, 1);
    hal_gpio_direction_output(group, pin);
#ifdef AIC_AUDIO_EN_PIN_HIGH
    hal_gpio_set_output(group, pin);
#else
    hal_gpio_clr_output(group, pin);
#endif

    hal_dma_init();
    aicos_request_irq(DMA_IRQn, hal_dma_irq, 0, NULL, NULL);

    uint32_t total_len = 0;
    while (1)
    {
        int length, wr_size;
        uint8_t *tmp_buf;

        length = read(fd, buffer, BUFFER_SIZE);
        if (length <= 0)
            break;

        if (!start_flag)
        {
            wr_size = 0;
            tmp_buf = buffer;
            wr_size = ringbuf_in(ring_buf, tmp_buf, length);
            total_len += wr_size;
            if (total_len >= TX_FIFO_SIZE)
            {
                start_flag = 1;
                hal_audio_playback_start(&audio_ctrl);
            }
        }
        else
        {
            wr_size = 0;
            tmp_buf = buffer;
            while (length)
            {
                wr_size = ringbuf_in(ring_buf, tmp_buf, length);
                tmp_buf += wr_size;
                length -= wr_size;
            }
        }
    }

#ifdef AIC_AUDIO_EN_PIN_HIGH
    hal_gpio_clr_output(group, pin);
#else
    hal_gpio_set_output(group, pin);
#endif
    hal_audio_playback_stop(&audio_ctrl);

__exit:
    if (fd >= 0)
        close(fd);
    if (buffer != NULL)
        aicos_free(MEM_CMA, buffer);
    if (ring_buf != NULL)
        aicos_free(MEM_CMA, ring_buf);
    if (info != NULL)
        aicos_free(MEM_CMA, info);
    return 0;
}

CONSOLE_CMD(aplay, sound_aplay, "aplay song.wav");
