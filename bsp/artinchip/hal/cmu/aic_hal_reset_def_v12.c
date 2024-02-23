/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_clk.h"
#include "aic_hal_reset.h"

const struct aic_reset_signal aic_reset_signals[RESET_NUMBER] = {
    [RESET_DMA]         = { CLK_DMA_REG, BIT(13) },
    [RESET_DCE]         = { CLK_DCE_REG, BIT(13) },
    [RESET_XSPI]        = { CLK_XSPI_REG, BIT(13) },
    [RESET_QSPI0]       = { CLK_QSPI0_REG, BIT(13) },
    [RESET_QSPI1]       = { CLK_QSPI1_REG, BIT(13) },
    [RESET_SDMMC0]      = { CLK_SDMC0_REG, BIT(13) },
    [RESET_SDMMC1]      = { CLK_SDMC1_REG, BIT(13) },
    [RESET_SYSCFG]      = { CLK_SYSCFG_REG, BIT(13) },
    [RESET_SPIENC]      = { CLK_SPIENC_REG, BIT(13) },
    [RESET_CODEC]       = { CLK_CODEC_REG, BIT(13) },
    [RESET_RGB]         = { CLK_RGB_REG, BIT(13) },
    [RESET_DE]          = { CLK_DE_REG, BIT(13) },
    [RESET_GE]          = { CLK_GE_REG, BIT(13) },
    [RESET_VE]          = { CLK_VE_REG, BIT(13) },
    [RESET_WDT]         = { CLK_WDT_REG, BIT(13) },
    [RESET_SID]         = { CLK_SID_REG, BIT(13) },
    [RESET_GTC]         = { CLK_GTC_REG, BIT(13) },
    [RESET_GPIO]        = { CLK_GPIO_REG, BIT(13) },
    [RESET_UART0]       = { CLK_UART0_REG, BIT(13) },
    [RESET_UART1]       = { CLK_UART1_REG, BIT(13) },
    [RESET_UART2]       = { CLK_UART2_REG, BIT(13) },
    [RESET_UART3]       = { CLK_UART3_REG, BIT(13) },
    [RESET_I2C0]        = { CLK_I2C0_REG, BIT(13) },
    [RESET_I2C1]        = { CLK_I2C1_REG, BIT(13) },
    [RESET_CAN0]        = { CLK_CAN0_REG, BIT(13) },
    [RESET_CAN1]        = { CLK_CAN1_REG, BIT(13) },
    [RESET_PWM]         = { CLK_PWM_REG, BIT(13) },
    [RESET_ADCIM]       = { CLK_ADCIM_REG, BIT(13) },
    [RESET_GPAI]        = { CLK_GPAI_REG, BIT(13) },
    [RESET_RTP]         = { CLK_RTP_REG, BIT(13) },
    [RESET_TSEN]        = { CLK_TSEN_REG, BIT(13) },
    [RESET_MDI]         = { CLK_MDI_REG, BIT(13) },
    [RESET_CIR]         = { CLK_CIR_REG, BIT(13) },
    [RESET_MTOP]        = { CLK_MTOP_REG, BIT(13) },
};
