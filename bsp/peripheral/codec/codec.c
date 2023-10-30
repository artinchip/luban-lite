/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */
#include "codec.h"

struct codec *register_codec = NULL;

int codec_init(struct codec *codec)
{
    CHECK_PARAM(codec, -1);

    if (codec->ops->init)
        return codec->ops->init(codec);

    return 0;
}

int codec_start(struct codec *codec, i2s_stream_t stream)
{
    CHECK_PARAM(codec, -1);

    if (codec->ops->start)
        return codec->ops->start(codec, stream);

    return 0;
}

int codec_stop(struct codec *codec, i2s_stream_t stream)
{
    CHECK_PARAM(codec, -1);

    if (codec->ops->stop)
        return codec->ops->stop(codec, stream);

    return 0;
}

int codec_set_protocol(struct codec *codec, i2s_format_t *format)
{
    CHECK_PARAM(codec, -1);

    if (codec->ops->set_protocol)
        return codec->ops->set_protocol(codec, format);

    return 0;
}

int codec_set_polarity(struct codec *codec, i2s_format_t *format)
{
    CHECK_PARAM(codec, -1);

    if (codec->ops->set_polarity)
        return codec->ops->set_polarity(codec, format);

    return 0;
}

int codec_set_channel(struct codec *codec, i2s_format_t *format)
{
    CHECK_PARAM(codec, -1);

    if (codec->ops->set_channel)
        return codec->ops->set_channel(codec, format);

    return 0;
}

int codec_set_sample_width(struct codec *codec, i2s_format_t *format)
{
    CHECK_PARAM(codec, -1);

    if (codec->ops->set_sample_width)
        return codec->ops->set_sample_width(codec, format);

    return 0;
}

int codec_set_sample_rate(struct codec *codec, i2s_format_t *format)
{
    CHECK_PARAM(codec, -1);

    if (codec->ops->set_sample_rate)
        return codec->ops->set_sample_rate(codec, format);

    return 0;
}

int codec_set_sclk(struct codec *codec, i2s_format_t *format)
{
    CHECK_PARAM(codec, -1);

    if (codec->ops->set_sclk)
        return codec->ops->set_sclk(codec, format);

    return 0;
}

int codec_set_mclk(struct codec *codec, i2s_format_t *format)
{
    CHECK_PARAM(codec, -1);

    if (codec->ops->set_mclk)
        return codec->ops->set_mclk(codec, format);

    return 0;
}

void codec_set_volume(struct codec *codec, uint8_t volume)
{
    CHECK_PARAM_RET(codec);

    if (codec->ops->set_volume)
        codec->ops->set_volume(codec, volume);
}

uint8_t codec_get_volume(struct codec *codec)
{
    CHECK_PARAM(codec, 0);

    if (codec->ops->get_volume)
        return codec->ops->get_volume(codec);

    return 0;
}

void codec_pa_power(struct codec *codec, uint8_t enable)
{
    CHECK_PARAM_RET(codec);

    if (codec->ops->pa_power)
        codec->ops->pa_power(codec, enable);
}

void codec_dump_reg(struct codec *codec)
{
    CHECK_PARAM_RET(codec);

    if (codec->ops->dump_reg)
        codec->ops->dump_reg(codec);
}

void codec_register(struct codec *codec)
{
    CHECK_PARAM_RET(codec);

    register_codec = codec;
}

void codec_unregister(void)
{
    register_codec = NULL;
}
