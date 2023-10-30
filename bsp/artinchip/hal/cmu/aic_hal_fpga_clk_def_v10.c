/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_clk.h"

#ifdef FPGA_BOARD_ARTINCHIP

const unsigned long fpga_board_rate[] = {
    [CLK_OSC24M] = CLOCK_24M,      [CLK_OSC32K] = CLOCK_32K,
    [CLK_RC1M] = CLOCK_1M,         [CLK_PLL_INT0] = CLOCK_24M,
    [CLK_PLL_INT1] = CLOCK_24M,    [CLK_PLL_FRA0] = CLOCK_24M,
    [CLK_PLL_FRA1] = CLOCK_24M,    [CLK_PLL_FRA2] = CLOCK_24M,
    [CLK_AXI0_SRC1] = CLOCK4_FREQ, [CLK_AHB0_SRC1] = CLOCK_60M,
    [CLK_APB0_SRC1] = CLOCK_30M,   [CLK_APB1_SRC1] = CLOCK_24M,
    [CLK_CPU_SRC1] = CLOCK_100M,   [CLK_AXI0] = CLOCK4_FREQ,
    [CLK_AHB0] = CLOCK_24M,        [CLK_APB0] = CLOCK_24M,
    [CLK_APB1] = CLOCK_24M,        [CLK_CPU] = CLOCK_100M,
    [CLK_DMA] = CLOCK_60M,         [CLK_CE] = CLOCK_72M,
    [CLK_USBD] = CLOCK_60M,        [CLK_USBH0] = CLOCK_60M,
    [CLK_USBH1] = CLOCK_60M,       [CLK_USB_PHY0] = CLOCK_60M,
    [CLK_USB_PHY1] = CLOCK_60M,    [CLK_GMAC0] = CLOCK_50M,
    [CLK_GMAC1] = CLOCK_50M,       [CLK_QSPI0] = CLOCK_24M,
    [CLK_QSPI1] = CLOCK_24M,        [CLK_SDMC0] = CLOCK1_FREQ,
    [CLK_SDMC1] = CLOCK1_FREQ,     [CLK_SDMC2] = CLOCK1_FREQ,
    [CLK_SYSCFG] = CLOCK_24M,      [CLK_RTC] = CLOCK_1M,
    [CLK_I2S0] = CLOCK3_FREQ,      [CLK_I2S1] = CLOCK3_FREQ,
    [CLK_CODEC] = CLOCK3_FREQ,     [CLK_RGB] = CLOCK_100M,
    [CLK_LVDS] = CLOCK_100M,       [CLK_MIPIDSI] = CLOCK_100M,
    [CLK_DE] = CLOCK_72M,          [CLK_GE] = CLOCK_72M,
    [CLK_VE] = CLOCK_72M,          [CLK_WDT] = CLOCK_1M,
    [CLK_SID] = CLOCK_24M,         [CLK_GTC] = CLOCK_24M,
    [CLK_GPIO] = CLOCK_24M,        [CLK_CIR] = CLOCK_24M,
    [CLK_UART0] = CLOCK_60M,       [CLK_UART1] = CLOCK_60M,
    [CLK_UART2] = CLOCK_60M,       [CLK_UART3] = CLOCK_60M,
    [CLK_UART4] = CLOCK_60M,       [CLK_UART5] = CLOCK_60M,
    [CLK_UART6] = CLOCK_60M,       [CLK_UART7] = CLOCK_60M,
    [CLK_I2C0] = CLOCK_24M,        [CLK_I2C1] = CLOCK_24M,
    [CLK_I2C2] = CLOCK_24M,        [CLK_I2C3] = CLOCK_24M,
    [CLK_CAN0] = CLOCK_24M,        [CLK_CAN1] = CLOCK_24M,
    [CLK_PWM] = CLOCK_24M,         [CLK_ADCIM] = CLOCK_24M,
    [CLK_GPAI] = CLOCK_24M,        [CLK_RTP] = CLOCK_24M,
    [CLK_TSEN] = CLOCK_24M,        [CLK_OUT0] = CLOCK_24M,
    [CLK_OUT1] = CLOCK_24M,        [CLK_OUT2] = CLOCK_24M,
    [CLK_OUT3] = CLOCK_24M,
};
#endif
