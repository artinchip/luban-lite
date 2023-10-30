/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: weilin.peng@artinchip.com
 */

#include <aic_core.h>
#include <aic_hal.h>
#include "board.h"

struct aic_pinmux
{
    unsigned char       func;
    unsigned char       bias;
    unsigned char       drive;
    char *              name;
};

struct aic_pinmux aic_pinmux_config[] = {
#ifdef AIC_USING_UART0
    /* uart0 */
    {5, PIN_PULL_DIS, 3, "PA.0"},
    {5, PIN_PULL_DIS, 3, "PA.1"},
#endif
#ifdef AIC_USING_UART1
    /* uart1 */
    {5, PIN_PULL_DIS, 3, "PA.4"},
    {5, PIN_PULL_DIS, 3, "PA.5"},
#endif
#ifdef AIC_USING_UART2
    /* uart2 */
    {5, PIN_PULL_DIS, 3, "PA.8"},
    {5, PIN_PULL_DIS, 3, "PA.9"},
#endif
#ifdef AIC_USING_SDMC0
    {2, PIN_PULL_UP, 3, "PB.0"},
    {2, PIN_PULL_UP, 3, "PB.1"},
    {2, PIN_PULL_UP, 3, "PB.2"},
    {2, PIN_PULL_UP, 3, "PB.3"},
    {2, PIN_PULL_UP, 3, "PB.4"},
    {2, PIN_PULL_UP, 3, "PB.5"},
    {2, PIN_PULL_UP, 3, "PB.6"},
    {2, PIN_PULL_UP, 3, "PB.7"},
    {2, PIN_PULL_UP, 3, "PB.8"},
    {2, PIN_PULL_UP, 3, "PB.9"},
    {2, PIN_PULL_UP, 3, "PB.10"},
    {2, PIN_PULL_UP, 3, "PB.11"},
#endif
#ifdef AIC_USING_SDMC1
    {2, PIN_PULL_UP, 3, "PC.0"},
    {2, PIN_PULL_UP, 3, "PC.1"},
    {2, PIN_PULL_UP, 3, "PC.2"},
    {2, PIN_PULL_UP, 3, "PC.3"},
    {2, PIN_PULL_UP, 3, "PC.4"},
    {2, PIN_PULL_UP, 3, "PC.5"},
    {2, PIN_PULL_UP, 3, "PC.6"},
#endif
#ifdef AIC_USING_SDMC2
    {2, PIN_PULL_UP, 3, "PF.0"},
    {2, PIN_PULL_UP, 3, "PF.1"},
    {2, PIN_PULL_UP, 3, "PF.2"},
    {2, PIN_PULL_UP, 3, "PF.3"},
    {2, PIN_PULL_UP, 3, "PF.4"},
    {2, PIN_PULL_UP, 3, "PF.5"},
#endif
#ifdef AIC_USING_QSPI0
    /* qspi0 */
    {3, PIN_PULL_DIS, 3, "PB.0"},
    {3, PIN_PULL_DIS, 3, "PB.1"},
    {3, PIN_PULL_DIS, 3, "PB.2"},
    {3, PIN_PULL_DIS, 3, "PB.3"},
    {3, PIN_PULL_DIS, 3, "PB.4"},
    {3, PIN_PULL_DIS, 3, "PB.5"},
#endif
#if (defined(AIC_USING_USB0_DEVICE) || defined(AIC_USING_USB0_HOST))
    /* usb0 */
    {2, PIN_PULL_DIS, 3, "PO.0"},
    {2, PIN_PULL_DIS, 3, "PO.1"},
#endif
#ifdef AIC_USING_USB1_HOST
    /* usb1 */
    {2, PIN_PULL_DIS, 3, "PO.2"},
    {2, PIN_PULL_DIS, 3, "PO.3"},
#endif
#ifdef AIC_USING_GMAC0
    /* gmac0 */
    {6, PIN_PULL_DIS, 3, "PE.0"},
    {6, PIN_PULL_DIS, 3, "PE.1"},
    {6, PIN_PULL_DIS, 3, "PE.2"},
    {6, PIN_PULL_DIS, 3, "PE.3"},
    {6, PIN_PULL_DIS, 3, "PE.4"},
    {6, PIN_PULL_DIS, 3, "PE.5"},
    {6, PIN_PULL_DIS, 3, "PE.6"},
    {6, PIN_PULL_DIS, 3, "PE.7"},
    {6, PIN_PULL_DIS, 3, "PE.8"},
    {6, PIN_PULL_DIS, 3, "PE.9"},
    /* phy0 reset gpio */
    {1, PIN_PULL_DIS, 3, "PA.2"},
#endif
#ifdef AIC_USING_GMAC1
    /* gmac1 */
    {6, PIN_PULL_DIS, 3, "PF.0"},
    {6, PIN_PULL_DIS, 3, "PF.1"},
    {6, PIN_PULL_DIS, 3, "PF.2"},
    {6, PIN_PULL_DIS, 3, "PF.3"},
    {6, PIN_PULL_DIS, 3, "PF.4"},
    {6, PIN_PULL_DIS, 3, "PF.5"},
    {6, PIN_PULL_DIS, 3, "PF.6"},
    {6, PIN_PULL_DIS, 3, "PF.7"},
    {6, PIN_PULL_DIS, 3, "PF.8"},
    {6, PIN_PULL_DIS, 3, "PF.9"},
    /* phy1 reset gpio */
    {1, PIN_PULL_DIS, 3, "PA.3"},
#endif
#ifdef AIC_USING_RTP
    {2, PIN_PULL_DIS, 3, "PA.8"},
    {2, PIN_PULL_DIS, 3, "PA.9"},
    {2, PIN_PULL_DIS, 3, "PA.10"},
    {2, PIN_PULL_DIS, 3, "PA.11"},
#endif
};

void aic_board_pinmux_init(void)
{
    uint32_t i = 0;
    long pin = 0;
    unsigned int g;
    unsigned int p;

    for (i=0; i<sizeof(aic_pinmux_config)/sizeof(struct aic_pinmux); i++) {
        pin = hal_gpio_name2pin(aic_pinmux_config[i].name);
        if (pin < 0)
            continue;
        g = GPIO_GROUP(pin);
        p = GPIO_GROUP_PIN(pin);
        hal_gpio_set_func(g, p, aic_pinmux_config[i].func);
        hal_gpio_set_bias_pull(g, p, aic_pinmux_config[i].bias);
        hal_gpio_set_drive_strength(g, p, aic_pinmux_config[i].drive);
    }
}
