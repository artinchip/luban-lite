/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "aic_hal_gpio.h"
#include "es8388.h"

/* ES8388 address */
#define ES8388_ADDR     0x10  /*0x11:CE=1;0x10:CE=0*/
#define ARG_UNUSED(x)   ((void)x)

struct es8388_sample_width
{
    uint8_t val;
    uint8_t width;
};

static const struct es8388_sample_width sample_width[] =
{
    { .val = 0, .width = 24 },
    { .val = 1, .width = 20 },
    { .val = 2, .width = 18 },
    { .val = 3, .width = 16 },
    { .val = 4, .width = 32 },
};

struct es8388_device
{
    struct rt_i2c_bus_device *i2c;
    uint32_t pin;
};

static struct es8388_device es_dev = {0};

static uint8_t reg_read(uint8_t addr)
{
    struct rt_i2c_msg msg[2] = {0};
    uint8_t val = 0xff;

    msg[0].addr  = ES8388_ADDR;
    msg[0].flags = RT_I2C_WR;
    msg[0].len   = 1;
    msg[0].buf   = &addr;

    msg[1].addr  = ES8388_ADDR;
    msg[1].flags = RT_I2C_RD;
    msg[1].len   = 1;
    msg[1].buf   = &val;

    if (rt_i2c_transfer(es_dev.i2c, msg, 2) != 2)
    {
        rt_kprintf("I2C read data failed, reg = 0x%02x. \n", addr);
        return 0xff;
    }

    return val;
}

static void reg_write(uint8_t addr, uint8_t val)
{
    struct rt_i2c_msg msgs[1] = {0};
    uint8_t buff[2] = {0};

    buff[0] = addr;
    buff[1] = val;

    msgs[0].addr  = ES8388_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = buff;
    msgs[0].len   = 2;

    if (rt_i2c_transfer(es_dev.i2c, msgs, 1) != 1)
    {
        rt_kprintf("I2C write data failed, reg = 0x%2x. \n", addr);
        return;
    }
}

void es8388_set_voice_mute(uint8_t enable)
{
    uint8_t reg = 0;

    reg = reg_read(ES8388_DACCONTROL3);
    reg = reg & 0xFB;
    reg_write(ES8388_DACCONTROL3, reg | (((int)enable) << 2));
}

int es8388_init(struct codec *codec)
{
    es_dev.i2c = rt_i2c_bus_device_find(codec->i2c_name);
    if (es_dev.i2c == RT_NULL)
    {
        rt_kprintf("%s bus not found\n", codec->i2c_name);
        return -RT_ERROR;
    }

    es_dev.pin = codec->pa;
    rt_pin_mode(es_dev.pin, PIN_MODE_OUTPUT);

    reg_write(ES8388_CONTROL2, 0x58);
    reg_write(ES8388_CONTROL2, 0x50);
    reg_write(ES8388_CHIPPOWER, 0xF3);
    reg_write(ES8388_CHIPPOWER, 0xF0);
    reg_write(ES8388_DACCONTROL21, 0x80);
    reg_write(ES8388_CONTROL1, 0x36);

    reg_write(ES8388_MASTERMODE, 0x00);  //slave mode

    return RT_EOK;
}

int es8388_start(struct codec *codec, i2s_stream_t stream)
{
    ARG_UNUSED(codec);

    if (!stream)
    {
        //Playback
        reg_write(ES8388_DACPOWER, 0x00);
        reg_write(ES8388_CHIPLOPOW1, 0x00);
        reg_write(ES8388_CHIPLOPOW2, 0xC3);
        /* LRDAC default volume: 0db */
        reg_write(ES8388_DACCONTROL4, 0x00);
        reg_write(ES8388_DACCONTROL5, 0x00);
        /* Mixer setting for LDAC to LOUT & RDAC to ROUT */
        reg_write(ES8388_DACCONTROL17, 0xB8);
        reg_write(ES8388_DACCONTROL20, 0xB8);
        /* startup FSM and DLL */
        reg_write(ES8388_CHIPPOWER, 0xAA);
        rt_thread_delay(500);
        reg_write(ES8388_DACCONTROL24, 0x1C);
        reg_write(ES8388_DACCONTROL25, 0x1C);
        reg_write(ES8388_DACCONTROL26, 0x1C);
        reg_write(ES8388_DACCONTROL27, 0x1C);
        /* Enable LOUT&ROUT */
        reg_write(ES8388_DACPOWER, 0x3C);
        reg_write(ES8388_ADCPOWER, 0xFF);
    }
    else
    {
        //Record
        reg_write(ES8388_DACPOWER, 0xC0);
        reg_write(ES8388_CHIPLOPOW1, 0x00);
        reg_write(ES8388_CHIPLOPOW2, 0xC3);
        /* set differential input mode */
        reg_write(ES8388_ADCCONTROL2, 0xF0);
        reg_write(ES8388_ADCCONTROL3, 0x02);
        /* default ADC volume: 0db */
        reg_write(ES8388_ADCCONTROL8, 0x00);
        reg_write(ES8388_ADCCONTROL9, 0x00);
        /* MIC PGA =24DB */
        reg_write(ES8388_ADCCONTROL1, 0x88);
        /* MIC ALC setting */
        reg_write(ES8388_ADCCONTROL10, 0xE2);
        reg_write(ES8388_ADCCONTROL11, 0xC0);
        reg_write(ES8388_ADCCONTROL12, 0x12);
        reg_write(ES8388_ADCCONTROL13, 0x06);
        reg_write(ES8388_ADCCONTROL14, 0xC3);
        /* startup FSM and DLL */
        reg_write(ES8388_CHIPPOWER, 0x55);
        reg_write(ES8388_ADCPOWER, 0x09);
    }

    return 0;
}

int es8388_set_protocol(struct codec *codec, i2s_format_t *format)
{
    uint8_t reg_val;

    if (!format->stream)    //Playback
    {
        reg_val = reg_read(ES8388_DACCONTROL1);
    }
    else    //Record
    {
        reg_val = reg_read(ES8388_ADCCONTROL4);
    }

    reg_val &= ~(3 << 0);

    if (format->protocol == I2S_PCM_LONG)
        reg_val |= 3;
    else
        reg_val |= format->protocol;

    if (format->protocol == I2S_PCM_LONG)
    {
        reg_val &= ~(1 << 5);
    }
    else if (format->protocol == I2S_PCM_SHORT)
    {
        reg_val |= (1 << 5);
    }

    if (!format->stream)    //Playback
    {
        reg_write(ES8388_DACCONTROL1, reg_val);
    }
    else    //Record
    {
        reg_write(ES8388_ADCCONTROL4, reg_val);
    }

    return 0;
}

int es8388_set_polarity(struct codec *codec, i2s_format_t *format)
{
    uint8_t reg_val;

    if (!format->stream)    //Playback
    {
        reg_val = reg_read(ES8388_DACCONTROL1);
    }
    else    //Record
    {
        reg_val = reg_read(ES8388_ADCCONTROL4);
    }

    reg_val &= ~(1 << 5);

    reg_val |= (format->polarity << 5);

    if (!format->stream)    //Playback
    {
        reg_write(ES8388_DACCONTROL1, reg_val);
    }
    else    //Record
    {
        reg_write(ES8388_ADCCONTROL4, reg_val);
    }

    return 0;
}

int es8388_set_sample_width(struct codec *codec, i2s_format_t *format)
{
    uint8_t reg_val = 0, i;

    /* sample width */
    for (i = 0; i < ARRAY_SIZE(sample_width); i++)
    {
        if (sample_width[i].width == format->width)
            break;
    }

    if (i == ARRAY_SIZE(sample_width))
    {
        // hal_log_err("es8388 not support sample width\n");
        return -1;
    }

    if (!format->stream)    //Playback
    {
        reg_val = reg_read(ES8388_DACCONTROL1);
        reg_val |= (sample_width[i].val << 2);
        reg_write(ES8388_DACCONTROL1, reg_val);
    }
    else    //Record
    {
        reg_val = reg_read(ES8388_ADCCONTROL4);
        reg_val |= (sample_width[i].val << 2);
        reg_write(ES8388_ADCCONTROL4, reg_val);
    }

    return 0;
}

void es8388_volume_set(struct codec *codec, uint8_t volume)
{
    ARG_UNUSED(codec);

    if (volume > 100)
        volume = 100;
    volume /= 3;

    reg_write(ES8388_DACCONTROL24, volume);
    reg_write(ES8388_DACCONTROL25, volume);
    reg_write(ES8388_DACCONTROL26, volume);
    reg_write(ES8388_DACCONTROL27, volume);
}

uint8_t es8388_volume_get(struct codec *codec)
{
    ARG_UNUSED(codec);

    uint8_t volume;

    volume = reg_read(ES8388_DACCONTROL24);
    if (volume == 0xff)
    {
        volume = 0;
    }
    else
    {
        volume *= 3;
        if (volume == 99)
            volume = 100;
    }

    return volume;
}

void es8388_pa_power(struct codec *codec, uint8_t enable)
{
    if (enable)
    {
        rt_pin_write(es_dev.pin, PIN_HIGH);
    }
    else
    {
        rt_pin_write(es_dev.pin, PIN_LOW);
    }
}

void es8388_dump_reg(struct codec *codec)
{
    int i;
    uint8_t reg_val = 0;

    for (i = 0; i < ES8388_REG_MAX; i++)
    {
        reg_val = reg_read(i);
        rt_kprintf("0x%02x: 0X%02x\n", i, reg_val);
    }
}

struct codec_ops es8388_ops =
{
    .init = es8388_init,
    .start = es8388_start,
    .stop = NULL,
    .set_protocol = es8388_set_protocol,
    .set_polarity = es8388_set_polarity,
    .set_channel = NULL,
    .set_sample_width = es8388_set_sample_width,
    .set_sample_rate = NULL,
    .set_sclk = NULL,
    .set_mclk = NULL,
    .set_volume = es8388_volume_set,
    .get_volume = es8388_volume_get,
    .pa_power = es8388_pa_power,
    .dump_reg = es8388_dump_reg,
};

static struct codec es8388 =
{
    .name = "es8388",
    .i2c_name = AIC_I2S_CODEC_ES8388_I2C,
    .addr = ES8388_ADDR,
    .pa_name = AIC_I2S_CODEC_PA_PIN,
    .ops = &es8388_ops,
};

int rt_hw_es8388_init(void)
{
    es8388.pa = hal_gpio_name2pin(es8388.pa_name);
    codec_register(&es8388);
    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_es8388_init);
