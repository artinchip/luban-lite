#ifndef __HAL_I2S_SOUND_H__
#define __HAL_I2S_SOUND_H__

#include <aic_common.h>

typedef enum {
    I2S_MODE_MASTER,
    I2S_MODE_SLAVE,
} i2s_mode_t;

typedef enum {
    I2S_PROTOCOL_I2S,
    I2S_PROTOCOL_LEFT_J,
    I2S_PROTOCOL_RIGHT_J,
    I2S_PCM_SHORT,
    I2S_PCM_LONG,
} i2s_protocol_t;

typedef enum {
    I2S_LEFT_POLARITY_LOW,
    I2S_LEFT_POLARITY_HIGH,
} i2s_polarity_t;

typedef enum {
    I2S_SAMPLE_RATE_8000              = 8000U,
    I2S_SAMPLE_RATE_11025             = 11025U,
    I2S_SAMPLE_RATE_12000             = 12000U,
    I2S_SAMPLE_RATE_16000             = 16000U,
    I2S_SAMPLE_RATE_22050             = 22050U,
    I2S_SAMPLE_RATE_24000             = 24000U,
    I2S_SAMPLE_RATE_32000             = 32000U,
    I2S_SAMPLE_RATE_44100             = 44100U,
    I2S_SAMPLE_RATE_48000             = 48000U,
    I2S_SAMPLE_RATE_96000             = 96000U,
    I2S_SAMPLE_RATE_192000            = 192000U,
    I2S_SAMPLE_RATE_256000            = 256000U,
} i2s_sample_rate_t;

typedef enum {
    I2S_SAMPLE_WIDTH_8BIT = 8U,
    I2S_SAMPLE_WIDTH_12BIT = 12U,
    I2S_SAMPLE_WIDTH_16BIT = 16U,
    I2S_SAMPLE_WIDTH_20BIT = 20U,
    I2S_SAMPLE_WIDTH_24BIT = 24U,
    I2S_SAMPLE_WIDTH_28BIT = 28U,
    I2S_SAMPLE_WIDTH_32BIT = 32U,
} i2s_sample_width_t;

typedef enum {
    I2S_LEFT_CHANNEL,
    I2S_RIGHT_CHANNEL,
    I2S_LEFT_RIGHT_CHANNEL,
} i2s_sound_channel_t;

typedef enum {
    I2S_STREAM_PLAYBACK     = 0,
    I2S_STREAM_RECORD       = 1,
} i2s_stream_t;

typedef struct {
    i2s_mode_t              mode;
    i2s_protocol_t          protocol;
    i2s_polarity_t          polarity;
    i2s_sample_rate_t       rate;
    i2s_sample_width_t      width;
    i2s_sound_channel_t     channel;
    uint32_t                sclk_nfs;
    uint32_t                mclk_nfs;
    i2s_stream_t            stream;
} i2s_format_t;

#endif
