/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "aic_hal_clk.h"
#include "hal_efuse.h"
#include "hal_syscfg.h"

/* Register of Syscfg */
#ifdef FPGA_BOARD_ARTINCHIP
#define SYSCFG_MMCM2_CTL    0xF0
#define SYSCFG_MMCM2_STA    0xF4
#define SYSCFG_LCD_IO_CFG   0xF8
#define SYSCFG_PHY_CFG      0xFC

#define SYSCFG_MMCM2_CTL_DRP_DIN_SHIFT      16
#define SYSCFG_MMCM2_CTL_DRP_DADDR_SHIFT    9
#define SYSCFG_MMCM2_CTL_DRP_DADDR_MASK     GENMASK(15, 9)
#define SYSCFG_MMCM2_CTL_DRP_DWE            BIT(8)
#define SYSCFG_MMCM2_CTL_DRP_START          BIT(4)
#define SYSCFG_MMCM2_CTL_DRP_RESET          BIT(0)

#define SYSCFG_MMCM2_STA_DRP_DOUT_SHIFT     16
#define SYSCFG_MMCM2_STA_DRP_DOUT_MASK      GENMASK(31, 16)

enum fpga_disp_clk {
    FPGA_DISP_CLK_RGB_SERIAL            = 0,
    FPGA_DISP_CLK_RGB_PARALLEL          = 1,
    FPGA_DISP_CLK_LVDS_SINGLE           = 2,
    FPGA_DISP_CLK_LVDS_DUAL             = 3,
    FPGA_DISP_CLK_MIPI_4LANE_RGB888     = 4,
    FPGA_DISP_CLK_MIPI_4LANE_RGB666     = 5,
    FPGA_DISP_CLK_MIPI_4LANE_RGB565     = 6,
    FPGA_DISP_CLK_MIPI_3LANE_RGB888     = 7,
    FPGA_DISP_CLK_MIPI_2LANE_RGB888     = 8,
    FPGA_DISP_CLK_MIPI_1LANE_RGB888     = 9,
    FPGA_DISP_CLK_I8080_24BIT           = 10,
    FPGA_DISP_CLK_I8080_18BIT           = 11,
    FPGA_DISP_CLK_I8080_16BIT_666_1     = 12,
    FPGA_DISP_CLK_I8080_16BIT_666_2     = 13,
    FPGA_DISP_CLK_I8080_16BIT_565       = 14,
    FPGA_DISP_CLK_I8080_9BIT            = 15,
    FPGA_DISP_CLK_I8080_8BIT_666        = 16,
    FPGA_DISP_CLK_I8080_8BIT_565        = 17,
    FPGA_DISP_CLK_SPI_4LINE_RGB888_OR_RGB666 = 18,
    FPGA_DISP_CLK_SPI_4LINE_RG565       = 19,
    FPGA_DISP_CLK_SPI_3LINE_RGB888_OR_RGB666 = 20,
    FPGA_DISP_CLK_SPI_3LINE_RG565       = 21,
    FPGA_DISP_CLK_SPI_4SDA_RGB888_OR_RGB666  = 22,
    FPGA_DISP_CLK_SPI_4SDA_RGB565       = 23,
};

enum fpga_mmcm_daddr {
    FPGA_MMCM_DADDR_CLKOUT0_CTL0        = 0x8,
    FPGA_MMCM_DADDR_CLKOUT0_CTL1        = 0x9,
    FPGA_MMCM_DADDR_CLKOUT1_CTL0        = 0xA,
    FPGA_MMCM_DADDR_CLKOUT1_CTL1        = 0xB,
    FPGA_MMCM_DADDR_CLKOUT2_CTL0        = 0xC,
    FPGA_MMCM_DADDR_CLKOUT2_CTL1        = 0xD,
    FPGA_MMCM_DADDR_CLKOUT3_CTL0        = 0xE,
    FPGA_MMCM_DADDR_CLKOUT3_CTL1        = 0xF,
    FPGA_MMCM_DADDR_CLKOUT4_CTL0        = 0x10,
    FPGA_MMCM_DADDR_CLKOUT4_CTL1        = 0x11,
    FPGA_MMCM_DADDR_CLKOUT5_CTL0        = 0x06,
    FPGA_MMCM_DADDR_CLKOUT5_CTL1        = 0x07,
    FPGA_MMCM_DADDR_CLKOUT6_CTL0        = 0x12,
    FPGA_MMCM_DADDR_CLKOUT6_CTL1        = 0x13,
    FPGA_MMCM_DADDR_VCO_M_CTL0          = 0x14,
    FPGA_MMCM_DADDR_VCO_M_CTL1          = 0x15,
};

enum fpga_gmac_clk_t {
    FPGA_GMAC_CLK_25M           = 0,
    FPGA_GMAC_CLK_125M          = 1,
};

#endif

#define SYSCFG_USB0_REXT                0x48
#define SYSCFG_USB1_REXT                0x4C
#define SYSCFG_FLASH_CFG                0x1F0
#define SYSCFG_USB0_CFG                 0x40C
#define SYSCFG_GMAC0_CFG                0x410 /* It's EMAC for syscfg v1.1 */
#ifdef AIC_SYSCFG_DRV_V10
#define SYSCFG_GMAC1_CFG                0x414
#endif

#define SYSCFG_USB_RES_CAL_EN_SHIFT     8
#define SYSCFG_USB_RES_CAL_EN_MASK      BIT(8)
#define SYSCFG_USB_RES_CAL_VAL_SHIFT    0
#define SYSCFG_USB_RES_CAL_VAL_MASK     GENMASK(7, 0)
#define SYSCFG_USB_RES_CAL_VAL_DEF      0x40
#define SYSCFG_USB_RES_CAL_BIAS_DEF     (-0x18)

#define SYSCFG_USB0_HOST_MODE           0
#define SYSCFG_USB0_DEVICE_MODE         1

#ifdef AIC_SYSCFG_DRV_V10
#define SYSCFG_LDO_CFG                      0x20
#define SYSCFG_LDO_CFG_IBIAS_EN_SHIFT       18
#define SYSCFG_LDO_CFG_IBIAS_EN_MASK        GENMASK(18, 17)
#define SYSCFG_LDO_CFG_VAL_MASK             GENMASK(2, 0)
#define SYSCFG_LDO_CFG_REFERENCE_VOLTAGE    28000
#define SYSCFG_LDO_CFG_VOLTAGE_SPACING      500
#endif

#if defined(AIC_SYSCFG_DRV_V11) || defined(AIC_SYSCFG_DRV_V12)
#define SYSCFG_LDO25_CFG                    0x20
#define SYSCFG_LDO25_CFG_IBIAS_EN_SHIFT     16
#define SYSCFG_LDO25_CFG_IBIAS_EN_MASK      GENMASK(17, 16)
#define SYSCFG_LDO25_CFG_VAL_MASK           GENMASK(2, 0)
#define SYSCFG_LDO25_CFG_REFERENCE_VOLTAGE  24000
#define SYSCFG_LDO25_CFG_VOLTAGE_SPACING    1000
#endif

#ifdef AIC_SYSCFG_DRV_V11
#define SYSCFG_LDO18_CFG                 0x24
#define SYSCFG_LDO18_CFG_LDO18_EN_SHIFT  4
#define SYSCFG_LDO18_CFG_LDO18_EN_MASK   BIT(4)
#define SYSCFG_LDO18_CFG_LDO18_VAL_SHIFT 0
#define SYSCFG_LDO18_CFG_LDO18_VAL_MASK  GENMASK(2, 0)
#define LDO18_VAL_DEFAULT 255

enum syscfg_ldo18_cfg_ldo18_en_t {
    LDO18_EN_DISABLE = 0,
    LDO18_EN_ENABLE = 1,
};

#endif


#if defined(AIC_SYSCFG_DRV_V11) || defined(AIC_SYSCFG_DRV_V12)
#define SYSCFG_LDO1X_CFG                 0x28
#define SYSCFG_LDO1X_CFG_LDO1X_VAL_SHIFT 0
#define SYSCFG_LDO1X_CFG_LDO1X_VAL_MASK  GENMASK(2, 0)
#define SYSCFG_LDO1X_CFG_LDO1X_EN_SHIFT  4
#define SYSCFG_LDO1X_CFG_LDO1X_EN_MASK   BIT(4)
#define SYSCFG_LDO1X_CFG_LDO1X_PD_FAST_SHIFT  5
#define SYSCFG_LDO1X_CFG_LDO1X_PD_FAST_MASK   BIT(5)
#define SYSCFG_LDO1X_CFG_LDO1X_SOFT_EN_SHIFT  6
#define SYSCFG_LDO1X_CFG_LDO1X_SOFT_EN_MASK   BIT(6)
#define LDO1X_VAL_DEFAULT 255

enum syscfg_ldo1x_cfg_ldo1x_en_t {
    LDO1X_EN_DISABLE = 0,
    LDO1X_EN_ENABLE = 1,
};
#endif

#define SYSCFG_GMAC_REFCLK_INV          BIT(29)
#define SYSCFG_GMAC_REFDLY_SEL_SHIF
#define SYSCFG_GMAC_REFDLY_SEL_MASK     GENMASK(28, 24)
#ifdef AIC_SYSCFG_DRV_V10
#define SYSCFG_GMAC_RXCLK_INV           BIT(23)
#define SYSCFG_GMAC_RXDLY_SEL_SHIFT     18
#define SYSCFG_GMAC_RXDLY_SEL_MASK      GENMASK(22, 18)
#endif
#define SYSCFG_GMAC_TXCLK_INV           BIT(17)
#define SYSCFG_GMAC_TXDLY_SEL_SHIFT     12
#define SYSCFG_GMAC_TXDLY_SEL_MASK      GENMASK(16, 12)
#define SYSCFG_GMAC_SW_TXCLK_DIV2_SHIFT 8
#define SYSCFG_GMAC_SW_TXCLK_DIV2_MASK  GENMASK(11, 8)
#define SYSCFG_GMAC_SW_TXCLK_DIV1_SHIFT 4
#define SYSCFG_GMAC_SW_TXCLK_DIV1_MASK  GENMASK(7, 4)
#define SYSCFG_GMAC_SW_TXCLK_DIV_EN     BIT(2)
#define SYSCFG_GMAC_RMII_EXTCLK_SEL     BIT(1)
#ifdef AIC_SYSCFG_DRV_V10
#define SYSCFG_GMAC_PHY_RGMII_1000M     BIT(0)
#endif

#define syscfg_readl(reg)           readl(SYSCFG_BASE + reg)
#define syscfg_writel(val, reg)     writel(val, SYSCFG_BASE + reg)

u32 syscfg_read_ldo_cfg(void)
{
    u32 st_voltage = 0;

#ifdef AIC_SYSCFG_DRV_V10
    u32 ldo30_val = syscfg_readl(SYSCFG_LDO_CFG) & SYSCFG_LDO_CFG_VAL_MASK;
    st_voltage = SYSCFG_LDO_CFG_REFERENCE_VOLTAGE + ldo30_val * SYSCFG_LDO_CFG_VOLTAGE_SPACING;
#endif
#if defined(AIC_SYSCFG_DRV_V11) || defined(AIC_SYSCFG_DRV_V12)
    u32 ldo25_val = syscfg_readl(SYSCFG_LDO25_CFG) & SYSCFG_LDO25_CFG_VAL_MASK;
    st_voltage = SYSCFG_LDO25_CFG_REFERENCE_VOLTAGE + ldo25_val * SYSCFG_LDO25_CFG_VOLTAGE_SPACING;
#endif

    return st_voltage;
}

#ifndef AIC_SYSCFG_DRV_V12
void syscfg_usb_phy0_sw_host(s32 sw)
{
    if (sw)
        syscfg_writel(SYSCFG_USB0_HOST_MODE, SYSCFG_USB0_CFG);
    else
        syscfg_writel(SYSCFG_USB0_DEVICE_MODE, SYSCFG_USB0_CFG);
}
#endif

#ifdef FPGA_BOARD_ARTINCHIP

static s32 syscfg_fpga_drp_wr(u8 addr, u16 data)
{
    syscfg_writel(SYSCFG_MMCM2_CTL_DRP_RESET, SYSCFG_MMCM2_CTL);
    aicos_udelay(20);

    syscfg_writel((data << SYSCFG_MMCM2_CTL_DRP_DIN_SHIFT)
                  | (addr << SYSCFG_MMCM2_CTL_DRP_DADDR_SHIFT)
                  | SYSCFG_MMCM2_CTL_DRP_DWE | SYSCFG_MMCM2_CTL_DRP_START
                  | SYSCFG_MMCM2_CTL_DRP_RESET, SYSCFG_MMCM2_CTL);

    while (syscfg_readl(SYSCFG_MMCM2_CTL) & SYSCFG_MMCM2_CTL_DRP_START) {
    }

    aicos_udelay(20);

    syscfg_writel(syscfg_readl(SYSCFG_MMCM2_CTL) & ~SYSCFG_MMCM2_CTL_DRP_RESET,
                  SYSCFG_MMCM2_CTL);
    return 0;
}

static u16 syscfg_fpga_drp_rd(u16 addr)
{
    u32 val = syscfg_readl(SYSCFG_MMCM2_CTL);

    val &= ~SYSCFG_MMCM2_CTL_DRP_DWE;
    val &= ~SYSCFG_MMCM2_CTL_DRP_DADDR_MASK;
    val |= (addr << SYSCFG_MMCM2_CTL_DRP_DADDR_SHIFT)
            | SYSCFG_MMCM2_CTL_DRP_START;

    syscfg_writel(val, SYSCFG_MMCM2_CTL);
    while (syscfg_readl(SYSCFG_MMCM2_CTL) & SYSCFG_MMCM2_CTL_DRP_START) {
    }

    return syscfg_readl(SYSCFG_MMCM2_STA) >> SYSCFG_MMCM2_STA_DRP_DOUT_SHIFT;
}

static s32 syscfg_fpga_cfg_vco(u32 freq)
{
    u8  cntr;
    u16 data;

    cntr = freq / 2;
    if ((freq % 2) == 0)
        data = 1 << 12 | cntr << 6 | cntr;
    else
        data = 1 << 12 | cntr << 6 | (cntr + 1);

    if (cntr > 0) {
        syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_VCO_M_CTL0, data);
        if (syscfg_fpga_drp_rd(FPGA_MMCM_DADDR_VCO_M_CTL0) != data)
            return -1;
    } else {
        syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_VCO_M_CTL1, 0x40);
    }

    return 0;
}

s32 syscfg_fpga_de_clk_sel_by_div(u8 sclk, u8 pixclk)
{
    u8  cntr;
    u16 data;

    cntr = sclk / 2;
    data = (1 << 12) | (cntr << 6) | cntr;

    if (cntr > 0) {
        syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_CLKOUT2_CTL0, data);
        if (syscfg_fpga_drp_rd(FPGA_MMCM_DADDR_CLKOUT2_CTL0) != data)
            return -1;
    } else {
        syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_CLKOUT2_CTL1, 0x40);
    }

    cntr = pixclk / 2;
    data = (1 << 12) | (cntr << 6) | cntr;

    if (cntr > 0) {
        syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_CLKOUT3_CTL0, data);
        if (syscfg_fpga_drp_rd(FPGA_MMCM_DADDR_CLKOUT3_CTL0) != data)
            return -1;
    } else {
        syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_CLKOUT3_CTL1, 0x40);
    }

    return 0;
}

static s32 syscfg_fpga_de_clk_sel(enum fpga_disp_clk type)
{
#if !defined(CONFIG_AIC_DISP_V12) && !defined(CONFIG_AIC_DISP_V11)
    u8 sclk_div[] = { 10,  8,  4,  4,  4,  4,  4,  4,  4,  4,  3,  3,   8,
        3,  6,   6,   3,   6,  1,   2,   1,  2,  4,  6};
    u8 pixclk_div[] = {120, 32, 28, 14, 24, 18, 16, 32, 48, 96, 30, 30, 120,
        60, 60, 120,  90, 120, 96, 128, 108, 72, 96, 96};
#else
    u8 sclk_div[] = {20,  16,  4,  4,  4,  4,  4,  4,  4,  4,  3,  3,   8,
        3,  6,   6,   4,   6,  1,   2,   1,  2,  4,  6};
    u8 pixclk_div[] = {120, 32, 28, 14, 24, 18, 16, 32, 48, 96, 30, 30, 120,
        60, 60, 120, 120, 120, 96, 128, 108, 72, 96, 96};
#endif

    if (type > 17)
        syscfg_fpga_cfg_vco(3);

    if (type == FPGA_DISP_CLK_MIPI_2LANE_RGB888)
        syscfg_fpga_cfg_vco(12);

    return syscfg_fpga_de_clk_sel_by_div(sclk_div[type], pixclk_div[type]);
}

void syscfg_fpga_lcd_io_set(u32 val)
{
    syscfg_writel(val, SYSCFG_LCD_IO_CFG);
}

#ifndef AIC_SYSCFG_DRV_V12
static void syscfg_fpga_gmac_clk_sel(u32 id)
{
    u8 fpga_mmcm2_div_gmac_clk[] = {40,  8};
    u8  cntr;
    u16 data;

    cntr = fpga_mmcm2_div_gmac_clk[id] / 2;
    data = 1 << 12 | cntr << 6 | cntr;

    syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_CLKOUT0_CTL0, data);
    if (syscfg_fpga_drp_rd(FPGA_MMCM_DADDR_CLKOUT0_CTL0) != data)
        return;

    syscfg_fpga_drp_wr(FPGA_MMCM_DADDR_CLKOUT1_CTL0, data);
    if (syscfg_fpga_drp_rd(FPGA_MMCM_DADDR_CLKOUT1_CTL0) != data)
        return;
}
#endif

#endif

#ifndef AIC_SYSCFG_DRV_V12

static s32 syscfg_usb_init(void)
{
#if defined(AIC_USING_USB0) || defined(AIC_USING_USB1)
    u32 cfg_reg = 0;
    s32 cfg = 0;
#endif

#ifdef AIC_USING_USB0
    cfg_reg =  SYSCFG_USB0_REXT;
    cfg = syscfg_readl(cfg_reg);
    cfg &= SYSCFG_USB_RES_CAL_VAL_MASK;
    cfg += SYSCFG_USB_RES_CAL_BIAS_DEF;
    cfg &= SYSCFG_USB_RES_CAL_VAL_MASK;
    cfg |= (1 << SYSCFG_USB_RES_CAL_EN_SHIFT);
    syscfg_writel(cfg, cfg_reg);
#endif

#ifdef AIC_USING_USB1
    cfg_reg =  SYSCFG_USB1_REXT;
    cfg &= SYSCFG_USB_RES_CAL_VAL_MASK;
    cfg += SYSCFG_USB_RES_CAL_BIAS_DEF;
    cfg &= SYSCFG_USB_RES_CAL_VAL_MASK;
    cfg |= (1 << SYSCFG_USB_RES_CAL_EN_SHIFT);
    syscfg_writel(cfg, cfg_reg);
#endif
    return 0;
}

static inline s32 phy_interface_mode_is_rgmii(phy_interface_t mode)
{
    return mode >= PHY_INTERFACE_MODE_RGMII &&
        mode <= PHY_INTERFACE_MODE_RGMII_TXID;
}

#if defined(AIC_USING_GMAC0) || defined(AIC_USING_GMAC1)
static s32 syscfg_gmac_init(u32 ch)
{
    #ifdef AIC_SYSCFG_DRV_V10
    u32 cfg_reg = ch ? SYSCFG_GMAC1_CFG : SYSCFG_GMAC0_CFG;
    #else
    u32 cfg_reg =  SYSCFG_GMAC0_CFG;
    #endif
    s32 cfg;

    cfg = syscfg_readl(cfg_reg);

    if (ch == 0) {
        #ifdef AIC_SYSCFG_DRV_V10
        #ifdef AIC_DEV_GMAC0_RGMII
        cfg |= SYSCFG_GMAC_PHY_RGMII_1000M;
        #else
        cfg &= ~SYSCFG_GMAC_PHY_RGMII_1000M;
        #endif
        #endif
        #ifdef AIC_DEV_GMAC0_PHY_EXTCLK
        cfg |= SYSCFG_GMAC_RMII_EXTCLK_SEL;
        #endif
        #if AIC_DEV_GMAC0_TXDELAY
        cfg |= (AIC_DEV_GMAC0_TXDELAY << SYSCFG_GMAC_TXDLY_SEL_SHIFT);
        #endif
        #if AIC_DEV_GMAC0_RXDELAY
        cfg |= (AIC_DEV_GMAC0_RXDELAY << SYSCFG_GMAC_RXDLY_SEL_SHIFT);
        #endif
    } else if (ch == 1) {
        #ifdef AIC_SYSCFG_DRV_V10
        #ifdef AIC_DEV_GMAC1_RGMII
        cfg |= SYSCFG_GMAC_PHY_RGMII_1000M;
        #else
        cfg &= ~SYSCFG_GMAC_PHY_RGMII_1000M;
        #endif
        #endif
        #ifdef AIC_DEV_GMAC1_PHY_EXTCLK
        cfg |= SYSCFG_GMAC_RMII_EXTCLK_SEL;
        #endif
        #if AIC_DEV_GMAC1_TXDELAY
        cfg |= (AIC_DEV_GMAC1_TXDELAY << SYSCFG_GMAC_TXDLY_SEL_SHIFT);
        #endif
        #if AIC_DEV_GMAC1_RXDELAY
        cfg |= (AIC_DEV_GMAC1_RXDELAY << SYSCFG_GMAC_RXDLY_SEL_SHIFT);
        #endif
    }

    syscfg_writel(cfg, cfg_reg);

    return 0;
}
#endif
#endif

static void syscfg_sip_flash_init(void)
{
#if defined(AIC_SYSCFG_SIP_FLASH_ENABLE) && defined(AIC_SID_DRV_V11)
    u32 val, map, ctrl_id;
    u32 iomap_efuse_wid = 9;

    /* 1. Read eFuse to set SiP flash IO mapping */
    hal_efuse_read(iomap_efuse_wid, &val);
    map = (val >> 24) & 0xFF;

    /* 2. Set the SiP flash's access Controller */
    ctrl_id = 2 + AIC_SIP_FLASH_ACCESS_QSPI_ID;
    val = map << 8 | (ctrl_id & 0x3);
    syscfg_writel(val, SYSCFG_FLASH_CFG);
#endif
}

#if defined(AIC_SYSCFG_DRV_V11) && defined(AIC_XSPI_DRV)
/* ldo25 BIT16, BIT17 ctrl the XSPI */
static void syscfg_ldo25_xspi_init(void)
{
    u32 val;
    val = syscfg_readl(SYSCFG_LDO25_CFG);

    // clear bit16, bit17
    val &= ~(SYSCFG_LDO25_CFG_IBIAS_EN_MASK);
    // set bit16, bit17
    val |= SYSCFG_LDO25_CFG_IBIAS_EN_MASK;

    syscfg_writel(val, SYSCFG_LDO25_CFG);
}
#endif

#if defined(AIC_SYSCFG_DRV_V11) || defined(AIC_SYSCFG_DRV_V12)
#define LDO1X_DISABLE_BIT4_6_VAL_STEP1   0x30
#define LDO1X_DISABLE_BIT4_6_VAL_STEP2   0x70
#define LDO1X_DISABLE_BIT4_6_VAL_STEP3   0x60
#define LDO1X_DISABLE_BIT4_6_VAL_STEP4   0x40
static void syscfg_ldo1x_init(u8 status, u8 v_level)
{
    u32 val = 0;
    if (status == LDO1X_EN_ENABLE) {
        /*SD suggest: set the SYSCFG_LDO1X_CFG_LDO1X_PD_FAST_SHIFT to 0, other bit not use.*/
        val |= (v_level & SYSCFG_LDO1X_CFG_LDO1X_VAL_MASK);
        syscfg_writel(val, SYSCFG_LDO1X_CFG);
    } else {
        val = syscfg_readl(SYSCFG_LDO1X_CFG);
        v_level = val & SYSCFG_LDO1X_CFG_LDO1X_VAL_MASK;

        /*BIT4, BIT5, BIT6, DISABLE*/
        val = 0;
        val |= (LDO1X_DISABLE_BIT4_6_VAL_STEP1 | v_level);
        syscfg_writel(val, SYSCFG_LDO1X_CFG);

        val = 0;
        val |= (LDO1X_DISABLE_BIT4_6_VAL_STEP2 | v_level);
        syscfg_writel(val, SYSCFG_LDO1X_CFG);

        val = 0;
        val |= (LDO1X_DISABLE_BIT4_6_VAL_STEP3 | v_level);
        syscfg_writel(val, SYSCFG_LDO1X_CFG);

        val = 0;
        val |= (LDO1X_DISABLE_BIT4_6_VAL_STEP4 | v_level);
        syscfg_writel(val, SYSCFG_LDO1X_CFG);
    }
    aicos_udelay(10);
}
#endif

#if defined(AIC_SYSCFG_DRV_V11)
#define LDO18_ENABLE_BIT4_VAL_STEP1   0x0
#define LDO18_ENABLE_BIT4_VAL_STEP2   0x10
[[maybe_unused]] static void syscfg_ldo18_init(u8 status, u8 v_level)
{
    u32 val = 0;
    if (status == LDO18_EN_ENABLE) {
        /*SD suggest*/
        val |= (LDO18_ENABLE_BIT4_VAL_STEP1 | (v_level & SYSCFG_LDO18_CFG_LDO18_VAL_MASK));
        syscfg_writel(val, SYSCFG_LDO18_CFG);
        aicos_udelay(1);

        val = 0;
        val |= (LDO18_ENABLE_BIT4_VAL_STEP2 | (v_level & SYSCFG_LDO18_CFG_LDO18_VAL_MASK));
        syscfg_writel(val, SYSCFG_LDO18_CFG);
        aicos_udelay(10);
    } else {
        (void)v_level;
        val = syscfg_readl(SYSCFG_LDO18_CFG);
        val &= ~SYSCFG_LDO18_CFG_LDO18_EN_MASK;
        syscfg_writel(val, SYSCFG_LDO18_CFG);
    }
}
#endif

s32 hal_syscfg_probe(void)
{
    s32 ret = 0;

    ret = hal_clk_enable(CLK_SYSCFG);
    if (ret < 0)
        return -1;

    ret = hal_clk_enable_deassertrst(CLK_SYSCFG);
    if (ret < 0)
        return -1;

    syscfg_sip_flash_init();
#ifndef AIC_SYSCFG_DRV_V12
    syscfg_usb_init();
#ifdef AIC_USING_GMAC0
    syscfg_gmac_init(0);
#endif
#ifdef AIC_USING_GMAC1
    syscfg_gmac_init(1);
#endif
#endif

#ifdef AIC_SYSCFG_DRV_V11
#ifdef AIC_XSPI_DRV
    syscfg_ldo25_xspi_init();
#endif

#ifdef AIC_SYSCFG_LDO1X_ENABLE
    syscfg_ldo1x_init(LDO1X_EN_ENABLE, AIC_SYSCFG_LDO1X_VOL_VAL);
#else
    // disable ldo1x, the broom code maybe enable it.
    syscfg_ldo1x_init(LDO1X_EN_DISABLE, LDO1X_VAL_DEFAULT);
#endif
#endif

#ifdef AIC_SYSCFG_DRV_V12
#ifdef AIC_SYSCFG_LDO1X_ENABLE
    syscfg_ldo1x_init(LDO1X_EN_ENABLE, AIC_SYSCFG_LDO1X_VOL_VAL);
#else
    // disable ldo1x, the broom code maybe enable it.
    syscfg_ldo1x_init(LDO1X_EN_DISABLE, LDO1X_VAL_DEFAULT);
#endif
#endif

#ifdef FPGA_BOARD_ARTINCHIP
    syscfg_fpga_de_clk_sel(FPGA_DISP_CLK_RGB_PARALLEL);
#ifndef AIC_SYSCFG_DRV_V12
    //If use GMAC, set to 125M
    syscfg_fpga_gmac_clk_sel(FPGA_GMAC_CLK_25M);
#endif

#endif

    return 0;
}
