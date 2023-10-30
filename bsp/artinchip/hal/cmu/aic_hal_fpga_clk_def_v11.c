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
    [CLK_PLL_INT0] = CLOCK_60M,    [CLK_PLL_INT1] = CLOCK_60M,
    [CLK_PLL_FRA0] = CLOCK_60M,    [CLK_PLL_FRA2] = CLOCK_60M,
    [CLK_AXI_AHB_SRC1] = CLOCK1_FREQ, [CLK_APB0_SRC1] = CLOCK_30M,
    [CLK_CPU_SRC1] = CLOCK_60M,    [CLK_AXI0] = CLOCK1_FREQ,
    [CLK_AHB0] = CLOCK1_FREQ,      [CLK_APB0] = CLOCK_30M,
    [CLK_APB1] = CLOCK_24M,        [CLK_CPU] = CLOCK_60M,
    [CLK_DMA] = CLOCK_60M,         [CLK_CE] = CLOCK_36M,
    [CLK_USBD] = CLOCK_60M,        [CLK_USBH0] = CLOCK_60M,
    [CLK_USB_PHY0] = CLOCK_24M,    [CLK_GMAC0] = CLOCK_50M,
    [CLK_XSPI] = CLOCK_24M,        [CLK_QSPI0] = CLOCK1_FREQ,
    [CLK_QSPI1] = CLOCK1_FREQ,     [CLK_QSPI2] = CLOCK1_FREQ,
    [CLK_QSPI3] = CLOCK1_FREQ,     [CLK_SDMC0] = CLOCK1_FREQ,
    [CLK_SDMC1] = CLOCK1_FREQ,     [CLK_PBUS] = CLOCK1_FREQ,
    [CLK_SYSCFG] = CLOCK_24M,      [CLK_RTC] = CLOCK_32K,
    [CLK_I2S0] = CLOCK_AUDIO,      [CLK_GPIO] = CLOCK_24M,
    [CLK_CODEC] = CLOCK_AUDIO,     [CLK_RGB] = CLOCK_100M,
    [CLK_LVDS] = CLOCK_100M,       [CLK_MIPIDSI] = CLOCK_100M,
    [CLK_DE] = CLOCK_36M,          [CLK_GE] = CLOCK_36M,
    [CLK_VE] = CLOCK_36M,          [CLK_WDT] = CLOCK_32K,
    [CLK_SID] = CLOCK_24M,         [CLK_GTC] = CLOCK_24M,
    [CLK_UART0] = CLOCK_24M,       [CLK_UART1] = CLOCK_24M,
    [CLK_UART2] = CLOCK_24M,       [CLK_UART3] = CLOCK_24M,
    [CLK_UART4] = CLOCK_24M,       [CLK_UART5] = CLOCK_24M,
    [CLK_UART6] = CLOCK_24M,       [CLK_UART7] = CLOCK_24M,
    [CLK_I2C0] = CLOCK_24M,        [CLK_I2C1] = CLOCK_24M,
    [CLK_I2C2] = CLOCK_24M,        [CLK_CIR] = CLOCK_24M,
    [CLK_CAN0] = CLOCK_24M,        [CLK_CAN1] = CLOCK_24M,
    [CLK_PWM] = CLOCK_24M,         [CLK_ADCIM] = CLOCK_24M,
    [CLK_GPAI] = CLOCK_24M,        [CLK_RTP] = CLOCK_24M,
    [CLK_TSEN] = CLOCK_24M,        [CLK_OUT0] = CLOCK_24M,
    [CLK_OUT1] = CLOCK_24M,        [CLK_OUT2] = CLOCK_24M,
    [CLK_OUT3] = CLOCK_24M,
};
#endif
