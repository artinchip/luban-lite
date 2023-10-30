/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */

#include <stdio.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <string.h>
#include <aic_osal.h>
#include <getopt.h>

#include "hal_i2s.h"
#include "codec.h"

#define TX_FIFO_PERIOD_COUNT        4
#define TX_FIFO_SIZE                (RT_AUDIO_REPLAY_MP_BLOCK_SIZE *\
                                     TX_FIFO_PERIOD_COUNT)
#define RX_FIFO_PERIOD_COUNT        2
#define RX_FIFO_SIZE                (RT_AUDIO_RECORD_PIPE_SIZE *\
                                     RX_FIFO_PERIOD_COUNT)
static rt_uint8_t i2s_tx_fifo[TX_FIFO_SIZE] __attribute__((aligned(64)));
static rt_uint8_t i2s_rx_fifo[RX_FIFO_SIZE] __attribute__((aligned(64)));

struct aic_i2s_sound
{
    struct rt_audio_device audio;
    aic_i2s_ctrl i2s;
    struct codec *codec;
    i2s_format_t format;
    rt_uint8_t volume;
    char *name;
    uint32_t i2s_idx;
    uint8_t record_idx;
};

static struct aic_i2s_sound snd_dev[] =
{
#ifdef AIC_USING_I2S0
    {
        .name = "i2s0_sound",
        .i2s_idx = 0,
    },
#endif
#ifdef AIC_USING_I2S1
    {
        .name = "i2s1_sound",
        .i2s_idx = 1,
    },
#endif
};


static void drv_i2s_sound_callback(aic_i2s_ctrl *pi2s, void *arg);

rt_err_t drv_i2s_sound_init(struct rt_audio_device *audio)
{
    struct aic_i2s_sound *p_snd_dev;
    aic_i2s_ctrl *pi2s;

    p_snd_dev = (struct aic_i2s_sound *)audio;
    pi2s = &p_snd_dev->i2s;

    pi2s->tx_info.buf_info.buf = (void *)i2s_tx_fifo;
    pi2s->tx_info.buf_info.buf_len = TX_FIFO_SIZE;
    pi2s->tx_info.buf_info.period_len = TX_FIFO_SIZE / TX_FIFO_PERIOD_COUNT;

    pi2s->rx_info.buf_info.buf = (void *)i2s_rx_fifo;
    pi2s->rx_info.buf_info.buf_len = RX_FIFO_SIZE;
    pi2s->rx_info.buf_info.period_len = RX_FIFO_SIZE / RX_FIFO_PERIOD_COUNT;

    hal_i2s_attach_callback(pi2s, drv_i2s_sound_callback, NULL);

    codec_init(p_snd_dev->codec);

    return RT_EOK;
}

void drv_i2s_sound_buffer_info(struct rt_audio_device *audio,
                           struct rt_audio_buf_info *info)
{
    struct aic_i2s_sound *p_snd_dev;
    aic_i2s_ctrl *pi2s;

    p_snd_dev = (struct aic_i2s_sound *)audio;
    pi2s = &p_snd_dev->i2s;

    info->buffer = pi2s->tx_info.buf_info.buf;
    info->block_size = pi2s->tx_info.buf_info.period_len;
    info->block_count = TX_FIFO_PERIOD_COUNT;
    info->total_size = pi2s->tx_info.buf_info.buf_len;
}

rt_err_t drv_i2s_sound_start(struct rt_audio_device *audio, int stream)
{
    struct aic_i2s_sound *p_snd_dev;
    aic_i2s_ctrl *pi2s;
    i2s_format_t *pformat;
    unsigned int i;

    p_snd_dev = (struct aic_i2s_sound *)audio;
    pi2s = &p_snd_dev->i2s;
    pformat = &p_snd_dev->format;

    if (stream == AUDIO_STREAM_REPLAY)
    {
        for (i = 0; i < TX_FIFO_PERIOD_COUNT; i++)
            rt_audio_tx_complete(audio);

        hal_i2s_playback_start(pi2s, pformat);
        codec_start(p_snd_dev->codec, I2S_STREAM_PLAYBACK);
        /* for debug */
        // codec_dump_reg(p_snd_dev->codec);
        /* Enable PA */
#ifdef AIC_I2S_CODEC_PA_ENABLE_HIGH
        codec_pa_power(p_snd_dev->codec, 1);
#else
        codec_pa_power(p_snd_dev->codec, 0);
#endif
    }
    else if (stream == AUDIO_STREAM_RECORD)
    {
        /* May be need to do something for future */
    }
    else
    {
        hal_log_err("stream error\n");
        return -RT_EINVAL;
    }

    return RT_EOK;
}

rt_err_t drv_i2s_sound_stop(struct rt_audio_device *audio, int stream)
{
    struct aic_i2s_sound *p_snd_dev;
    aic_i2s_ctrl *pi2s;

    p_snd_dev = (struct aic_i2s_sound *)audio;
    pi2s = &p_snd_dev->i2s;

    if (stream == AUDIO_STREAM_REPLAY)
    {
        /* Disable PA first */
#ifdef AIC_I2S_CODEC_PA_ENABLE_HIGH
        codec_pa_power(p_snd_dev->codec, 0);
#else
        codec_pa_power(p_snd_dev->codec, 1);
#endif
        hal_i2s_playback_stop(pi2s);
    }
    else if (stream == AUDIO_STREAM_RECORD)
    {
        hal_i2s_record_stop(pi2s);
    }
    else
    {
        hal_log_err("stream error\n");
        return -RT_EINVAL;
    }

    return RT_EOK;
}

rt_err_t drv_i2s_sound_configure(struct rt_audio_device *audio,
                             struct rt_audio_caps *caps)
{
    rt_err_t ret = RT_EOK;
    struct aic_i2s_sound *p_snd_dev;
    aic_i2s_ctrl *pi2s;
    i2s_format_t *pformat;
    struct codec *codec;
    rt_uint32_t volume;

    p_snd_dev = (struct aic_i2s_sound *)audio;
    pi2s = &p_snd_dev->i2s;
    pformat = &p_snd_dev->format;
    codec = p_snd_dev->codec;

    switch (caps->main_type)
    {
    case AUDIO_TYPE_MIXER:
    {
        switch (caps->sub_type)
        {
        case AUDIO_MIXER_VOLUME:
            volume = caps->udata.value;
            codec_set_volume(codec, volume);
            p_snd_dev->volume = volume;
            break;

        default:
            ret = -RT_ERROR;
            break;
        }

        break;
    }

    case AUDIO_TYPE_OUTPUT:
    case AUDIO_TYPE_INPUT:
    {
        pformat->stream = I2S_STREAM_PLAYBACK;
        pformat->mclk_nfs = AIC_I2S_CODEC_MCLK_NFS;
        pformat->sclk_nfs = AIC_I2S_CODEC_SCLK_NFS;
        pformat->protocol = I2S_PROTOCOL_I2S;
        pformat->polarity = I2S_LEFT_POLARITY_LOW;

        hal_i2s_protocol_select(pi2s, pformat->protocol);
        hal_i2s_polarity_set(pi2s, pformat->polarity);
        codec_set_protocol(codec, pformat);
        codec_set_polarity(codec, pformat);

        switch (caps->sub_type)
        {
        case AUDIO_DSP_PARAM:
            pformat->rate = caps->udata.config.samplerate;
            pformat->channel = caps->udata.config.channels;
            pformat->width = caps->udata.config.samplebits;

            /* Configure channel */
            hal_i2s_channel_select(pi2s, pformat->channel, pformat->stream);
            codec_set_channel(codec, pformat);
            /* Configure samplebits */
            hal_i2s_sample_width_select(pi2s, pformat->width);
            codec_set_sample_width(codec, pformat);
            /* Configure sample rate */
            hal_i2s_mclk_set(pi2s, pformat->rate, pformat->mclk_nfs);
            hal_i2s_sclk_set(pi2s, pformat->rate, pformat->sclk_nfs);
            codec_set_mclk(codec, pformat);
            codec_set_sclk(codec, pformat);

            LOG_D("set samplerate %d, channel: %d, samplebits: %d\n",
                  pformat->rate, pformat->channel, pformat->width);
            break;

        case AUDIO_DSP_SAMPLERATE:
            // hal_i2s_set_samplerate(pi2s, caps->udata.config.samplerate);
            pformat->rate = caps->udata.config.samplerate;

            /* Configure sample rate */
            hal_i2s_mclk_set(pi2s, pformat->rate, pformat->mclk_nfs);
            hal_i2s_sclk_set(pi2s, pformat->rate, pformat->sclk_nfs);
            codec_set_mclk(codec, pformat);
            codec_set_sclk(codec, pformat);

            LOG_D("set samplerate %d\n", pformat->rate);
            break;

        case AUDIO_DSP_CHANNELS:
            pformat->channel = caps->udata.config.channels;

            hal_i2s_channel_select(pi2s, pformat->channel, pformat->stream);
            codec_set_channel(codec, pformat);

            LOG_D("set channel: %d\n", pformat->channel);
            break;

        case AUDIO_DSP_SAMPLEBITS:
            pformat->width = caps->udata.config.samplebits;
            /* Configure samplebits */
            hal_i2s_sample_width_select(pi2s, pformat->width);
            codec_set_sample_width(codec, pformat);

            LOG_D("set samplebits: %d\n", pformat->width);
            break;

        default:
            ret = -RT_ERROR;
        }

        if (caps->main_type == AUDIO_TYPE_INPUT)
        {
            hal_i2s_record_start(pi2s, pformat);
            codec_start(codec, I2S_STREAM_RECORD);
            /* for debug */
            //codec_dump_reg(codec);
        }

        break;
    }

    default:
        break;
    }

    return ret;
}

static rt_err_t drv_i2s_sound_getcaps(struct rt_audio_device *audio,
                                  struct rt_audio_caps *caps)
{
    rt_err_t ret = RT_EOK;
    struct aic_i2s_sound *p_snd_dev;
    i2s_format_t *pformat;

    p_snd_dev = (struct aic_i2s_sound *)audio;
    pformat = &p_snd_dev->format;

    switch (caps->main_type)
    {
    case AUDIO_TYPE_QUERY:
    {
        switch (caps->sub_type)
        {
        case AUDIO_TYPE_QUERY:
            caps->udata.mask = AUDIO_TYPE_OUTPUT | AUDIO_TYPE_MIXER;
            break;

        default:
            ret = -RT_ERROR;
            break;
        }

        break;
    }

    case AUDIO_TYPE_OUTPUT:
    case AUDIO_TYPE_INPUT:
    {
        switch (caps->sub_type)
        {
        case AUDIO_DSP_PARAM:
            caps->udata.config.samplerate = pformat->rate;
            caps->udata.config.channels = pformat->channel;
            caps->udata.config.samplebits = pformat->width;
            break;

        case AUDIO_DSP_SAMPLERATE:
            caps->udata.config.samplerate = pformat->rate;
            break;

        case AUDIO_DSP_CHANNELS:
            caps->udata.config.channels = pformat->channel;
            break;

        case AUDIO_DSP_SAMPLEBITS:
            caps->udata.config.samplebits = pformat->width;
            break;

        default:
            ret = -RT_ERROR;
            break;
        }

        break;
    }

    case AUDIO_TYPE_MIXER:
    {
        switch (caps->sub_type)
        {
        case AUDIO_MIXER_QUERY:
            caps->udata.mask = AUDIO_MIXER_VOLUME;
            break;

        case AUDIO_MIXER_VOLUME:
            caps->udata.value = p_snd_dev->volume;
            break;

        default:
            ret = -RT_ERROR;
            break;
        }

        break;
    }

    default:
        ret = -RT_ERROR;
        break;
    }

    return ret;
}

static void drv_i2s_sound_callback(aic_i2s_ctrl *pi2s, void *arg)
{
    unsigned long event = (unsigned long)arg;
    struct aic_i2s_sound *p_snd_dev;
    struct rt_audio_device *audio;
    uint32_t period_len = 0;

    p_snd_dev = rt_container_of(pi2s, struct aic_i2s_sound, i2s);
    audio = (struct rt_audio_device *)p_snd_dev;

    switch (event)
    {
    case I2S_TX_PERIOD_INT:
        rt_audio_tx_complete(audio);
        break;

    case I2S_RX_PERIOD_INT:
        period_len = pi2s->rx_info.buf_info.period_len;
        if (!p_snd_dev->record_idx)
        {
            rt_audio_rx_done(audio, &i2s_rx_fifo[0], period_len);
            p_snd_dev->record_idx = 1;
        }
        else
        {
            rt_audio_rx_done(audio, &i2s_rx_fifo[period_len], period_len);
            p_snd_dev->record_idx = 0;
        }
        break;

    default:
        hal_log_err("not support event\n");
        break;
    }
}

/* Get the queued space for playback */
static rt_size_t drv_i2s_sound_get_playback_avail(struct rt_audio_device *audio)
{
    rt_uint16_t data_queue_item;
    u32 dma_pos;
    struct aic_i2s_sound *p_snd_dev;
    aic_i2s_ctrl *pi2s;
    uint32_t cache_in_tx_buf, cache_in_data_queue, cache_in_mempool;

    p_snd_dev = (struct aic_i2s_sound *)audio;
    pi2s = &p_snd_dev->i2s;

    data_queue_item = rt_data_queue_len(&audio->replay->queue);

    hal_dma_chan_tx_status(pi2s->tx_info.dma_chan, &dma_pos);

    cache_in_data_queue = RT_AUDIO_REPLAY_MP_BLOCK_SIZE * data_queue_item;
    cache_in_mempool = audio->replay->write_index;
    cache_in_tx_buf = TX_FIFO_SIZE -
                      (pi2s->tx_info.buf_info.period_len - dma_pos);

    return cache_in_data_queue + cache_in_mempool + cache_in_tx_buf;
}

rt_err_t drv_i2s_sound_pause(struct rt_audio_device *audio, int enable)
{
    struct aic_i2s_sound *p_snd_dev;
    aic_i2s_ctrl *pi2s;

    p_snd_dev = (struct aic_i2s_sound *)audio;
    pi2s = &p_snd_dev->i2s;

    if(enable)
        hal_dma_chan_pause(pi2s->tx_info.dma_chan);
    else
        hal_dma_chan_resume(pi2s->tx_info.dma_chan);

    return RT_EOK;
}


struct rt_audio_ops aic_i2s_ops =
{
    .getcaps     = drv_i2s_sound_getcaps,
    .configure   = drv_i2s_sound_configure,
    .init        = drv_i2s_sound_init,
    .start       = drv_i2s_sound_start,
    .stop        = drv_i2s_sound_stop,
    .transmit    = NULL,
    .buffer_info = drv_i2s_sound_buffer_info,
    .get_avail   = drv_i2s_sound_get_playback_avail,
    .pause       = drv_i2s_sound_pause,
};

int rt_hw_i2s_sound_init(void)
{
    rt_err_t ret = RT_EOK;
    int i;

    for (i = 0; i < ARRAY_SIZE(snd_dev); i++)
    {
        hal_i2s_init(&snd_dev[i].i2s, snd_dev[i].i2s_idx);

        snd_dev[i].audio.ops = &aic_i2s_ops;
        snd_dev[i].record_idx = 0;

        ret = rt_audio_register(&snd_dev[i].audio, snd_dev[i].name,
                            RT_DEVICE_FLAG_RDWR, &snd_dev[i]);
    }

    return ret;
}

INIT_DEVICE_EXPORT(rt_hw_i2s_sound_init);
