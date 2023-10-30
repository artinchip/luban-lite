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

unsigned int wifi_power_pin = 0;

int rtl8733bs_reset(void)
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

int rtl8733bs_power_on(void)
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

int rtl8733bs_power_off(void)
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

static rt_int32_t rtl8733bs_probe(struct rt_mmcsd_card *card)
{
#ifdef RTL8733_WLAN_INTF_SDIO
    return (wifi_sdio_probe(card));
#else
    return 0;
#endif
}

static rt_int32_t rtl8733bs_remove(struct rt_mmcsd_card *card)
{
#ifdef RTL8733_WLAN_INTF_SDIO
    wifi_sdio_remove(card);
#endif

    return 0;
}

struct rt_sdio_device_id rtl8733bs_id = { 1, 0x024c, 0xB733};

struct rt_sdio_driver rtl8733bs_drv = {
    "rtl-8733bs",
    rtl8733bs_probe,
    rtl8733bs_remove,
    &rtl8733bs_id
};

int rtl8733bs_init(void)
{
#ifdef AIC_DEV_RTL8733_WLAN0_PWR_GPIO
    wifi_power_pin = hal_gpio_name2pin(AIC_DEV_RTL8733_WLAN0_PWR_GPIO);
    hal_gpio_direction_output(GPIO_GROUP(wifi_power_pin),
                              GPIO_GROUP_PIN(wifi_power_pin));
#else
    #error Not define rtl8733 power_on gpio pin.
#endif

    rtl8733bs_reset();

    sdio_register_driver(&rtl8733bs_drv);

    return 0;
}


