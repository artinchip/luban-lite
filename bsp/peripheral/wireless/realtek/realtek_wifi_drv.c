/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <drivers/pin.h>
#include <drivers/sdio.h>
#include <drivers/mmcsd_card.h>
#include <aic_core.h>
#include <aic_drv.h>
#include "card.h"
#include "wifi_io.h"
#include "rtconfig.h"

unsigned int wifi_power_pin = 0;

int realtek_reset(void)
{
    unsigned int g;
    unsigned int p;

    /* power on pin */
    g = GPIO_GROUP(wifi_power_pin);
    p = GPIO_GROUP_PIN(wifi_power_pin);

    /* reset */
    hal_gpio_set_value(g, p, 0);
    aicos_msleep(10);
    hal_gpio_set_value(g, p, 1);
    aicos_msleep(10);

    return 0;
}

int realtek_power_on(void)
{
    unsigned int g;
    unsigned int p;

    /* power on pin */
    g = GPIO_GROUP(wifi_power_pin);
    p = GPIO_GROUP_PIN(wifi_power_pin);

    /* power on */
    hal_gpio_set_value(g, p, 1);
    aicos_msleep(10);

    return 0;
}

int realtek_power_off(void)
{
    unsigned int g;
    unsigned int p;

    /* power on pin */
    g = GPIO_GROUP(wifi_power_pin);
    p = GPIO_GROUP_PIN(wifi_power_pin);

    /* power off */
    hal_gpio_set_value(g, p, 0);
    aicos_msleep(10);

    return 0;
}

static rt_int32_t realtek_probe(struct rt_mmcsd_card *card)
{
#ifdef REALTEK_WLAN_INTF_SDIO
    return (wifi_sdio_probe(card));
#else
    return 0;
#endif
}

static rt_int32_t realtek_remove(struct rt_mmcsd_card *card)
{
#ifdef REALTEK_WLAN_INTF_SDIO
    wifi_sdio_remove(card);
#endif

    return 0;
}

struct rt_sdio_device_id realtex_id[]= {
#if defined(AIC_USING_RTL8733_WLAN0)
    { 1, 0x024c, 0xB733},
#elif defined(AIC_USING_RTL8189_WLAN0)
    { 1, 0x024c, 0xf179},
#endif
};

struct rt_sdio_driver realtek_drv = {
    "realtek-wifi",
    realtek_probe,
    realtek_remove,
    realtex_id,
};

int realtek_init(void)
{
    wifi_power_pin = hal_gpio_name2pin(AIC_DEV_REALTEK_WLAN0_PWR_GPIO);
    if (wifi_power_pin > 0)
        hal_gpio_direction_output(GPIO_GROUP(wifi_power_pin),
                                  GPIO_GROUP_PIN(wifi_power_pin));

    realtek_reset();

    printf("wifi device id == 0x%x\n", realtek_drv.id->product);
    sdio_register_driver(&realtek_drv);

    return 0;
}


