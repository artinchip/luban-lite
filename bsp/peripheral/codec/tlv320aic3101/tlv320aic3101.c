/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "aic_hal_gpio.h"
#include "tlv320aic3101.h"

#define TLV320_ADDR     0x18

struct tlv320_device
{
    struct rt_i2c_bus_device *i2c;
    uint32_t pin;
};

static struct tlv320_device tlv320_dev = {0};

static void reg_write(uint8_t addr, uint8_t val)
{
    struct rt_i2c_msg msgs[1] = {0};
    uint8_t buff[2] = {0};

    buff[0] = addr;
    buff[1] = val;

    msgs[0].addr  = TLV320_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = buff;
    msgs[0].len   = 2;

    if (rt_i2c_transfer(tlv320_dev.i2c, msgs, 1) != 1)
    {
        rt_kprintf("I2C write data failed, reg = 0x%2x. \n", addr);
        return;
    }
}

int tlv320_init(struct codec *codec)
{
    tlv320_dev.i2c = rt_i2c_bus_device_find(codec->i2c_name);
    if (tlv320_dev.i2c == RT_NULL)
    {
        rt_kprintf("%s bus not found\n", codec->i2c_name);
        return -RT_ERROR;
    }

    //reset pin
    tlv320_dev.pin = codec->pa;
    rt_pin_mode(tlv320_dev.pin, PIN_MODE_OUTPUT);
    rt_pin_write(tlv320_dev.pin, 0);
    rt_thread_delay(1);
    rt_pin_write(tlv320_dev.pin, 1);
    rt_thread_delay(2);

    reg_write(TLV320_PAGE0_PSR, 0x0);
    reg_write(TLV320_PAGE0_SRR, 0x80);
    reg_write(TLV320_PAGE0_CR, 0x1);
    reg_write(TLV320_PAGE0_CGCR, 0x2);
    reg_write(TLV320_PAGE0_LEFTOL, 0x8);
    reg_write(TLV320_PAGE0_RIGHTOL, 0x8);
    reg_write(TLV320_PAGE0_CDPSR, 0xA);  //Left Right data path
    reg_write(TLV320_PAGE0_ASDICRB, 0x00);
    reg_write(TLV320_PAGE0_DACPODCR, 0xC0); //dac POWER ON
    reg_write(TLV320_PAGE0_DACOSCR, 0x50);  //DAC L1/L3 select
    reg_write(TLV320_PAGE0_ASDICRC, 0x1);
    reg_write(TLV320_PAGE0_LDACDVCR, 0x46);
    reg_write(TLV320_PAGE0_RDACDVCR, 0x46);

    //ADC
    reg_write(TLV320_PAGE0_LADCPGCR, 0x0);
    reg_write(TLV320_PAGE0_RADCPGCR, 0x0);
    reg_write(TLV320_PAGE0_MIC1LPLADC, 0x4); //MIC1LP
    reg_write(TLV320_PAGE0_MIC1RPRADC, 0x4); //MIC1RP
    reg_write(TLV320_PAGE0_MICBIAS, 0x40);

    return RT_EOK;
}

struct codec_ops tlv320_ops =
{
    .init = tlv320_init,
};

static struct codec tlv320 =
{
    .name = "tlv320",
    .i2c_name = AIC_I2S_CODEC_TLV320_I2C,
    .addr = TLV320_ADDR,
    .pa_name = AIC_I2S_CODEC_PA_PIN,
    .ops = &tlv320_ops,
};

int rt_hw_tlv320_init(void)
{
    tlv320.pa = hal_gpio_name2pin(tlv320.pa_name);
    codec_register(&tlv320);
    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_tlv320_init);
