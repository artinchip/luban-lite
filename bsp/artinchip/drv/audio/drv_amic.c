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

#define RX_AMIC_FIFO_SIZE (4096)
static rt_uint8_t amic_rx_fifo[RX_AMIC_FIFO_SIZE] __attribute__((aligned(64)));

struct aic_amic
{
    struct rt_audio_device audio;
    aic_audio_ctrl codec;
    rt_uint8_t volume;
};

static struct aic_amic amic_dev;
static void drv_amic_callback(aic_audio_ctrl *pcodec, void *arg);

rt_err_t drv_amic_init(struct rt_audio_device *audio)
{
    struct aic_amic *p_amic_dev;
    aic_audio_ctrl *pcodec;

    p_amic_dev = (struct aic_amic *)audio;
    pcodec = &p_amic_dev->codec;

    pcodec->amic_info.buf_info.buf = (void *)amic_rx_fifo;
    pcodec->amic_info.buf_info.buf_len = RX_AMIC_FIFO_SIZE;
    pcodec->amic_info.buf_info.period_len = RX_AMIC_FIFO_SIZE / 2;

    hal_audio_attach_callback(pcodec, drv_amic_callback, NULL);

    return RT_EOK;
}

rt_err_t drv_amic_start(struct rt_audio_device *audio, int stream)
{
    if (stream == AUDIO_STREAM_RECORD)
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

rt_err_t drv_amic_stop(struct rt_audio_device *audio, int stream)
{
    struct aic_amic *p_amic_dev;
    aic_audio_ctrl *pcodec;

    p_amic_dev = (struct aic_amic *)audio;
    pcodec = &p_amic_dev->codec;

    if (stream == AUDIO_STREAM_RECORD)
    {
        hal_audio_amic_stop(pcodec);
    }
    else
    {
        hal_log_err("stream error\n");
        return -RT_EINVAL;
    }

    return RT_EOK;
}

rt_err_t drv_amic_configure(struct rt_audio_device *audio,
                            struct rt_audio_caps *caps)
{
    rt_err_t ret = RT_EOK;
    struct aic_amic *p_amic_dev;
    aic_audio_ctrl *pcodec;
    rt_uint32_t volume, reg_volume;

    p_amic_dev = (struct aic_amic *)audio;
    pcodec = &p_amic_dev->codec;

    switch (caps->main_type)
    {
    case AUDIO_TYPE_MIXER:
    {
        switch (caps->sub_type)
        {
        case AUDIO_MIXER_VOLUME:
            volume = caps->udata.value;
            /* The miximum value of volume in register is 255,
             * but in rtt audio framework, the miximum volume
             * is 100, so must convert user volume to register volume.
             **/
            reg_volume = volume * 255 / 100;

            hal_audio_set_amic_volume(pcodec, reg_volume);
            p_amic_dev->volume = volume;
            break;

        default:
            ret = -RT_ERROR;
            break;
        }

        break;
    }

    case AUDIO_TYPE_INPUT:
    {
        switch (caps->sub_type)
        {
        case AUDIO_DSP_PARAM:
            /* AudioCodec only support 16 bits, amic only support mono */
            hal_audio_set_amic_channel(pcodec);
            hal_audio_set_samplerate(pcodec, caps->udata.config.samplerate);
            pcodec->config.samplerate = caps->udata.config.samplerate;
            pcodec->config.channel = 1;
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
            hal_audio_set_amic_channel(pcodec);
            pcodec->config.channel = caps->udata.config.channels;
            LOG_D("set channel: %d\n", caps->udata.config.channels);
            break;

        case AUDIO_DSP_SAMPLEBITS:
            LOG_D("AudioCodec only support 16 sample bits\n");
            break;

        default:
            ret = -RT_ERROR;
        }

        hal_audio_amic_start(pcodec);

        break;
    }

    default:
        break;
    }

    return ret;
}

static rt_err_t drv_amic_getcaps(struct rt_audio_device *audio,
                                 struct rt_audio_caps *caps)
{
    rt_err_t ret = RT_EOK;
    struct aic_amic *p_amic_dev;
    aic_audio_ctrl *pcodec;

    p_amic_dev = (struct aic_amic *)audio;
    pcodec = &p_amic_dev->codec;

    switch (caps->main_type)
    {
    case AUDIO_TYPE_QUERY:
    {
        switch (caps->sub_type)
        {
        case AUDIO_TYPE_QUERY:
            caps->udata.mask = AUDIO_TYPE_INPUT | AUDIO_TYPE_MIXER;
            break;

        default:
            ret = -RT_ERROR;
            break;
        }

        break;
    }

    case AUDIO_TYPE_INPUT:
    {
        switch (caps->sub_type)
        {
        case AUDIO_DSP_PARAM:
            caps->udata.config.samplerate = pcodec->config.samplerate;
            caps->udata.config.channels = 1;
            caps->udata.config.samplebits = 16;
            break;

        case AUDIO_DSP_SAMPLERATE:
            caps->udata.config.samplerate = pcodec->config.samplerate;
            break;

        case AUDIO_DSP_CHANNELS:
            caps->udata.config.channels = 1;
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
            caps->udata.value = p_amic_dev->volume;
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

static void drv_amic_callback(aic_audio_ctrl *pcodec, void *arg)
{
    unsigned long event = (unsigned long)arg;
    struct aic_amic *p_amic_dev;
    struct rt_audio_device *audio;
    uint32_t period_len = 0;

    p_amic_dev = rt_container_of(pcodec, struct aic_amic, codec);
    audio = (struct rt_audio_device *)p_amic_dev;

    switch (event)
    {
    case AUDIO_RX_AMIC_PERIOD_INT:
        period_len = pcodec->dmic_info.buf_info.period_len;
        if (!p_amic_dev->index){
            rt_audio_rx_done(audio, &amic_rx_fifo[0], period_len);
            p_amic_dev->index = 1;
        } else {
            rt_audio_rx_done(audio, &amic_rx_fifo[period_len], period_len);
            p_amic_dev->index = 0;
        }
        break;

    default:
        hal_log_err("not support event\n");
        break;
    }
}

struct rt_audio_ops aic_amic_ops =
{
    .getcaps     = drv_amic_getcaps,
    .configure   = drv_amic_configure,
    .init        = drv_amic_init,
    .start       = drv_amic_start,
    .stop        = drv_amic_stop,
    .transmit    = NULL,
    .buffer_info = NULL,
};

#ifdef RT_USING_PM
static int aic_amic_suspend(const struct rt_device *device, rt_uint8_t mode)
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

static void aic_amic_resume(const struct rt_device *device, rt_uint8_t mode)
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

static struct rt_device_pm_ops aic_amic_pm_ops =
{
    SET_DEVICE_PM_OPS(aic_amic_suspend, aic_amic_resume)
    NULL,
};
#endif

int rt_hw_amic_init(void)
{
    rt_err_t ret = RT_EOK;

    hal_audio_init(&amic_dev.codec);

    amic_dev.audio.ops = &aic_amic_ops;

    ret = rt_audio_register(&amic_dev.audio, "amic0",
                            RT_DEVICE_FLAG_RDONLY, &amic_dev);

#ifdef RT_USING_PM
    rt_pm_device_register(&amic_dev.audio.parent, &aic_amic_pm_ops);
#endif

    return ret;
}

INIT_DEVICE_EXPORT(rt_hw_amic_init);
