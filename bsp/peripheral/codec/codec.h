/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */
#ifndef __CODEC_H__
#define __CODEC_H__

#include <aic_core.h>
#include "hal_i2s_format.h"

extern struct codec *register_codec;
struct codec;

struct codec_ops {
    int     (*init)(struct codec *codec);
    int     (*start)(struct codec *codec, i2s_stream_t stream);
    int     (*stop)(struct codec *codec, i2s_stream_t stream);
    int     (*set_protocol)(struct codec *codec, i2s_format_t *format);
    int     (*set_polarity)(struct codec *codec, i2s_format_t *format);
    int     (*set_channel)(struct codec *codec, i2s_format_t *format);
    int     (*set_sample_width)(struct codec *codec, i2s_format_t *format);
    int     (*set_sample_rate)(struct codec *codec, i2s_format_t *format);
    int     (*set_sclk)(struct codec *codec, i2s_format_t *format);
    int     (*set_mclk)(struct codec *codec, i2s_format_t *format);
    void    (*set_volume)(struct codec *codec, uint8_t volume);
    uint8_t (*get_volume)(struct codec *codec);
    void    (*pa_power)(struct codec *codec, uint8_t enable);
    void    (*dump_reg)(struct codec *codec);
};

struct codec {
    char *name;
    char *i2c_name;
    char *i2s_name;
    uint16_t addr; //codec device addr
    uint16_t pa;   //pa pin
    char *pa_name;
    struct codec_ops *ops;
};

int codec_init(struct codec *codec);
int codec_start(struct codec *codec, i2s_stream_t stream);
int codec_stop(struct codec *codec, i2s_stream_t stream);

int codec_set_protocol(struct codec *codec, i2s_format_t *format);
int codec_set_polarity(struct codec *codec, i2s_format_t *format);
int codec_set_channel(struct codec *codec, i2s_format_t *format);
int codec_set_sample_width(struct codec *codec, i2s_format_t *format);
int codec_set_sample_rate(struct codec *codec, i2s_format_t *format);
int codec_set_sclk(struct codec *codec, i2s_format_t *format);
int codec_set_mclk(struct codec *codec, i2s_format_t *format);

void codec_set_volume(struct codec *codec, uint8_t volume);
uint8_t codec_get_volume(struct codec *codec);
void codec_pa_power(struct codec *codec, uint8_t enable);
void codec_dump_reg(struct codec *codec);

void codec_register(struct codec *codec);
void codec_unregister(void);

#endif
