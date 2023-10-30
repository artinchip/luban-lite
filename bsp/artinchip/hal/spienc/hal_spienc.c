/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Hao Xiong <hao.xiong@artinchip.com>
 */

#include <aic_core.h>
#include <aic_hal.h>
#include <hal_spienc.h>

#define SPIE_REG_CTL   0x00
#define SPIE_REG_ICR   0x04
#define SPIE_REG_ISR   0x08
#define SPIE_REG_KCNT  0x0C
#define SPIE_REG_OCNT  0x10
#define SPIE_REG_ADDR  0x14
#define SPIE_REG_TWEAK 0x18
#define SPIE_REG_CPOS  0x1C
#define SPIE_REG_CLEN  0x20

#define SPIE_START_OFF   0
#define SPIE_SPI_SEL_OFF 12

#define SPIE_START_MSK   (0x1 << SPIE_START_OFF)
#define SPIE_SPI_SEL_MSK (0x3 << SPIE_SPI_SEL_OFF)

#define SPIE_INTR_KEY_GEN_MSK     (1 << 0)
#define SPIE_INTR_ENC_DEC_FIN_MSK (1 << 1)
#define SPIE_INTR_ALL_EMP_MSK     (1 << 2)
#define SPIE_INTR_HALF_EMP_MSK    (1 << 3)
#define SPIE_INTR_KEY_UDF_MSK     (1 << 4)
#define SPIE_INTR_KEY_OVF_MSK     (1 << 5)
#define SPIE_INTR_ALL_MSK         (0x3F)

#define SPI_CTLR_0     0
#define SPI_CTLR_1     1
#define SPI_CTLR_INVAL 0xFF

static int hal_spienc_attach_bus(u8 bus)
{
    u32 val;

    val = readl(SPI_ENC_BASE + SPIE_REG_CTL);
    val &= ~SPIE_SPI_SEL_MSK;

    switch (bus) {
        case SPI_CTLR_0:
            val |= (1 << SPIE_SPI_SEL_OFF);
            writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
            break;
        case SPI_CTLR_1:
            val |= (2 << SPIE_SPI_SEL_OFF);
            writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
            break;
        case SPI_CTLR_INVAL:
            val |= (0 << SPIE_SPI_SEL_OFF);
            writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
            break;
        default:
            val |= (0 << SPIE_SPI_SEL_OFF);
            writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
            hal_log_err("Wrong SPI Controller ID In DTS\n");
            return -EINVAL;
    }

    return 0;
}

int hal_spienc_init(void)
{
    int ret = 0;

    ret = hal_clk_enable(CLK_SPIENC);
    if (ret < 0) {
        hal_log_err("Failed to enable SID clk.\n");
        return -EFAULT;
    }

    ret = hal_clk_enable_deassertrst(CLK_SPIENC);
    if (ret < 0) {
        hal_log_err("Failed to reset SID deassert.\n");
        return -EFAULT;
    }
    /* Enable Interrupt */
    writel(SPIE_INTR_ALL_MSK, (SPI_ENC_BASE + SPIE_REG_ICR));

    return 0;
}

void hal_spienc_set_cfg(u32 spi_bus, u32 addr, u32 cpos, u32 clen)
{
    u32 tweak = 0;

    if (spi_bus == 0) {
#if defined(AIC_SPIENC_QSPI0)
        tweak = AIC_SPIENC_QSPI0_TWEAK;
#endif
    } else if (spi_bus == 1){
#if defined(AIC_SPIENC_QSPI1)
        tweak = AIC_SPIENC_QSPI1_TWEAK;
#endif
    } else {
        tweak = 0;
        hal_log_warn("not define spi %d tweak, default(%d).\n", spi_bus, tweak);
    }

    hal_spienc_attach_bus(spi_bus);
    writel(addr, (SPI_ENC_BASE + SPIE_REG_ADDR));
    writel(cpos, (SPI_ENC_BASE + SPIE_REG_CPOS));
    writel(clen, (SPI_ENC_BASE + SPIE_REG_CLEN));
    writel(tweak, (SPI_ENC_BASE + SPIE_REG_TWEAK));
}

void hal_spienc_start(void)
{
    u32 val;

    writel(SPIE_INTR_ALL_MSK, (SPI_ENC_BASE + SPIE_REG_ISR));
    val = readl((SPI_ENC_BASE + SPIE_REG_CTL));
    val |= SPIE_START_MSK;
    writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
}

void hal_spienc_stop(void)
{
    u32 val;

    val = readl((SPI_ENC_BASE + SPIE_REG_CTL));
    val &= ~SPIE_START_MSK;
    writel(val, (SPI_ENC_BASE + SPIE_REG_CTL));
}

int hal_spienc_check_empty(void)
{
    u32 val;

    val = readl((SPI_ENC_BASE + SPIE_REG_ISR));
    writel(val, (SPI_ENC_BASE + SPIE_REG_ISR));
    if (val & SPIE_INTR_ALL_EMP_MSK)
        return 1;

    return 0;
}

