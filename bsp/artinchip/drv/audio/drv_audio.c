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

#include "hal_audio.h"

#define TX_FIFO_PERIOD_COUNT        2
#define TX_FIFO_SIZE                (RT_AUDIO_REPLAY_MP_BLOCK_SIZE *\
                                     TX_FIFO_PERIOD_COUNT)
static rt_uint8_t audio_tx_fifo[TX_FIFO_SIZE] __attribute__((aligned(64)));

struct aic_audio
{
    struct rt_audio_device audio;
    aic_audio_ctrl codec;
    rt_uint8_t volume;
    char *pa_name;
    unsigned int gpio_pa;
};

static struct aic_audio snd_dev;
static void drv_audio_callback(aic_audio_ctrl *pcodec, void *arg);

rt_err_t drv_audio_init(struct rt_audio_device *audio)
{
    struct aic_audio *p_snd_dev;
    aic_audio_ctrl *pcodec;

    p_snd_dev = (struct aic_audio *)audio;
    pcodec = &p_snd_dev->codec;

    pcodec->tx_info.buf_info.buf = (void *)audio_tx_fifo;
    pcodec->tx_info.buf_info.buf_len = TX_FIFO_SIZE;
    pcodec->tx_info.buf_info.period_len = TX_FIFO_SIZE / TX_FIFO_PERIOD_COUNT;

    hal_audio_attach_callback(pcodec, drv_audio_callback, NULL);

    return RT_EOK;
}

void drv_audio_buffer_info(struct rt_audio_device *audio,
                           struct rt_audio_buf_info *info)
{
    struct aic_audio *p_snd_dev;
    aic_audio_ctrl *pcodec;

    p_snd_dev = (struct aic_audio *)audio;
    pcodec = &p_snd_dev->codec;

    info->buffer = pcodec->tx_info.buf_info.buf;
    info->block_size = pcodec->tx_info.buf_info.period_len;
    info->block_count = TX_FIFO_PERIOD_COUNT;
    info->total_size = pcodec->tx_info.buf_info.buf_len;
}

rt_err_t drv_audio_start(struct rt_audio_device *audio, int stream)
{
    struct aic_audio *p_snd_dev;
    aic_audio_ctrl *pcodec;
    unsigned int group, pin;

    p_snd_dev = (struct aic_audio *)audio;
    pcodec = &p_snd_dev->codec;
    group = GPIO_GROUP(p_snd_dev->gpio_pa);
    pin = GPIO_GROUP_PIN(p_snd_dev->gpio_pa);

    if (stream == AUDIO_STREAM_REPLAY)
    {
        #ifdef AIC_AUDIO_SPK_0
        hal_audio_set_playback_by_spk0(pcodec);
        #endif
        #ifdef AIC_AUDIO_SPK_1
        hal_audio_set_playback_by_spk1(pcodec);
        #endif
        #ifdef AIC_AUDIO_SPK_0_1
        hal_audio_set_playback_by_spk0(pcodec);
        hal_audio_set_playback_by_spk1(pcodec);
        #ifdef AIC_AUDIO_SPK0_OUTPUT_DIFFERENTIAL
        hal_audio_set_pwm0_differential(pcodec);
        #endif
        #ifdef AIC_AUDIO_SPK1_OUTPUT_DIFFERENTIAL
        hal_audio_set_pwm1_differential(pcodec);
        #endif
        #endif

        rt_audio_tx_complete(audio);

        hal_audio_playback_start(pcodec);
        /* Delay 10ms and then enable the PA to prevent pop */
        rt_thread_mdelay(10);
        /* Enable PA */
#ifdef AIC_AUDIO_EN_PIN_HIGH
        hal_gpio_set_output(group, pin);
#else
        hal_gpio_clr_output(group, pin);
#endif
    }
    else
    {
        hal_log_err("stream error\n");
        return -RT_EINVAL;
    }

    return RT_EOK;
}

rt_err_t drv_audio_stop(struct rt_audio_device *audio, int stream)
{
    struct aic_audio *p_snd_dev;
    aic_audio_ctrl *pcodec;
    unsigned int group, pin;

    p_snd_dev = (struct aic_audio *)audio;
    pcodec = &p_snd_dev->codec;
    group = GPIO_GROUP(p_snd_dev->gpio_pa);
    pin = GPIO_GROUP_PIN(p_snd_dev->gpio_pa);

    if (stream == AUDIO_STREAM_REPLAY)
    {
        /* Disable PA first */
        #ifdef AIC_AUDIO_EN_PIN_HIGH
            hal_gpio_clr_output(group, pin);
        #else
            hal_gpio_set_output(group, pin);
        #endif
        hal_audio_playback_stop(pcodec);
    }
    else
    {
        hal_log_err("stream error\n");
        return -RT_EINVAL;
    }

    return RT_EOK;
}

rt_err_t drv_audio_configure(struct rt_audio_device *audio,
                             struct rt_audio_caps *caps)
{
    rt_err_t ret = RT_EOK;
    struct aic_audio *p_snd_dev;
    aic_audio_ctrl *pcodec;
    rt_uint32_t volume, reg_volume;

    p_snd_dev = (struct aic_audio *)audio;
    pcodec = &p_snd_dev->codec;

    switch (caps->main_type)
    {
    case AUDIO_TYPE_MIXER:
    {
        switch (caps->sub_type)
        {
        case AUDIO_MIXER_VOLUME:
            volume = caps->udata.value;
            /*
             * Set max user volume to 0db
             */
            reg_volume = volume * MAX_VOLUME_0DB / 100;

            hal_audio_set_playback_volume(pcodec, reg_volume);
            hal_audio_set_dmic_volume(pcodec, reg_volume);
            p_snd_dev->volume = volume;
            break;

        default:
            ret = -RT_ERROR;
            break;
        }

        break;
    }

    case AUDIO_TYPE_OUTPUT:
    {
        switch (caps->sub_type)
        {
        case AUDIO_DSP_PARAM:
            /* AudioCodec only support 16 bits */
            hal_audio_set_playback_channel(pcodec, caps->udata.config.channels);
            hal_audio_set_samplerate(pcodec, caps->udata.config.samplerate);
            pcodec->config.samplerate = caps->udata.config.samplerate;
            pcodec->config.channel = caps->udata.config.channels;
            pcodec->config.samplebits = 16;
            LOG_D("set samplerate %d, channel: %d\n",
                  caps->udata.config.samplerate, caps->udata.config.channels);
            break;

        case AUDIO_DSP_SAMPLERATE:
            hal_audio_set_samplerate(pcodec, caps->udata.config.samplerate);
            pcodec->config.samplerate = caps->udata.config.samplerate;
            LOG_D("set samplerate %d\n", caps->udata.config.samplerate);
            break;

        case AUDIO_DSP_CHANNELS:
            hal_audio_set_playback_channel(pcodec, caps->udata.config.channels);
            pcodec->config.channel = caps->udata.config.channels;
            LOG_D("set channel: %d\n", caps->udata.config.channels);
            break;

        case AUDIO_DSP_SAMPLEBITS:
            LOG_D("AudioCodec only support 16 sample bits\n");
            break;

        default:
            ret = -RT_ERROR;
        }

        break;
    }

    default:
        break;
    }

    return ret;
}

static rt_err_t drv_audio_getcaps(struct rt_audio_device *audio,
                                  struct rt_audio_caps *caps)
{
    rt_err_t ret = RT_EOK;
    struct aic_audio *p_snd_dev;
    aic_audio_ctrl *pcodec;

    p_snd_dev = (struct aic_audio *)audio;
    pcodec = &p_snd_dev->codec;

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
    {
        switch (caps->sub_type)
        {
        case AUDIO_DSP_PARAM:
            caps->udata.config.samplerate = pcodec->config.samplerate;
            caps->udata.config.channels = pcodec->config.channel;
            caps->udata.config.samplebits = 16;
            break;

        case AUDIO_DSP_SAMPLERATE:
            caps->udata.config.samplerate = pcodec->config.samplerate;
            break;

        case AUDIO_DSP_CHANNELS:
            caps->udata.config.channels = pcodec->config.channel;
            break;

        case AUDIO_DSP_SAMPLEBITS:
            caps->udata.config.samplebits = 16;
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

static void drv_audio_callback(aic_audio_ctrl *pcodec, void *arg)
{
    unsigned long event = (unsigned long)arg;
    struct aic_audio *p_snd_dev;
    struct rt_audio_device *audio;

    p_snd_dev = rt_container_of(pcodec, struct aic_audio, codec);
    audio = (struct rt_audio_device *)p_snd_dev;

    switch (event)
    {
    case AUDIO_TX_PERIOD_INT:
        rt_audio_tx_complete(audio);
        aicos_dcache_clean_range((void *)audio_tx_fifo, TX_FIFO_SIZE);
        break;

    default:
        hal_log_err("not support event\n");
        break;
    }
}

/* Get the queued space for playback */
static rt_size_t drv_audio_get_playback_avail(struct rt_audio_device *audio)
{
    rt_uint16_t data_queue_item;
    u32 dma_pos;
    struct aic_audio *p_snd_dev;
    aic_audio_ctrl *pcodec;
    uint32_t cache_in_tx_buf, cache_in_data_queue, cache_in_mempool;

    p_snd_dev = (struct aic_audio *)audio;
    pcodec = &p_snd_dev->codec;

    data_queue_item = rt_data_queue_len(&audio->replay->queue);

    hal_dma_chan_tx_status(pcodec->tx_info.dma_chan, &dma_pos);

    cache_in_data_queue = RT_AUDIO_REPLAY_MP_BLOCK_SIZE * data_queue_item;
    cache_in_mempool = audio->replay->write_index;
    cache_in_tx_buf = TX_FIFO_SIZE -
                      (pcodec->tx_info.buf_info.period_len - dma_pos);

    return cache_in_data_queue + cache_in_mempool + cache_in_tx_buf;
}

rt_err_t drv_audio_pause(struct rt_audio_device *audio, int enable)
{
    struct aic_audio *p_snd_dev;
    aic_audio_ctrl *pcodec;

    p_snd_dev = (struct aic_audio *)audio;
    pcodec = &p_snd_dev->codec;

    if(enable)
        hal_dma_chan_pause(pcodec->tx_info.dma_chan);
    else
        hal_dma_chan_resume(pcodec->tx_info.dma_chan);

    return RT_EOK;
}


struct rt_audio_ops aic_audio_ops =
{
    .getcaps     = drv_audio_getcaps,
    .configure   = drv_audio_configure,
    .init        = drv_audio_init,
    .start       = drv_audio_start,
    .stop        = drv_audio_stop,
    .transmit    = NULL,
    .buffer_info = drv_audio_buffer_info,
    .get_avail   = drv_audio_get_playback_avail,
    .pause       = drv_audio_pause,
};

#ifdef RT_USING_PM
static int aic_audio_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        if (hal_clk_is_enabled(CLK_CODEC))
            hal_clk_disable(CLK_CODEC);
        break;
    default:
        break;
    }

    return 0;
}

static void aic_audio_resume(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        if (!hal_clk_is_enabled(CLK_CODEC))
            hal_clk_enable(CLK_CODEC);
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops aic_audio_pm_ops =
{
    SET_DEVICE_PM_OPS(aic_audio_suspend, aic_audio_resume)
    NULL,
};
#endif

int rt_hw_sound_init(void)
{
    rt_err_t ret = RT_EOK;
    unsigned int group, pin;

    hal_audio_init(&snd_dev.codec);

    snd_dev.audio.ops = &aic_audio_ops;
    snd_dev.pa_name = AIC_AUDIO_PA_ENABLE_GPIO;
    snd_dev.gpio_pa = hal_gpio_name2pin(snd_dev.pa_name);

    group = GPIO_GROUP(snd_dev.gpio_pa);
    pin = GPIO_GROUP_PIN(snd_dev.gpio_pa);
    hal_gpio_direction_output(group, pin);
#ifdef AIC_AUDIO_EN_PIN_HIGH
    hal_gpio_clr_output(group, pin);
#else
    hal_gpio_set_output(group, pin);
#endif

    ret = rt_audio_register(&snd_dev.audio, "sound0",
                            RT_DEVICE_FLAG_WRONLY, &snd_dev);

#ifdef RT_USING_PM
    rt_pm_device_register(&snd_dev.audio.parent, &aic_audio_pm_ops);
#endif

    return ret;
}

INIT_DEVICE_EXPORT(rt_hw_sound_init);
