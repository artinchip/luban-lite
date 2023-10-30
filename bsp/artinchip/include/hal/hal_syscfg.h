/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_SYSCFG_H_
#define _ARTINCHIP_HAL_SYSCFG_H_

#include "aic_common.h"

typedef enum {
    PHY_INTERFACE_MODE_NA,
    PHY_INTERFACE_MODE_INTERNAL,
    PHY_INTERFACE_MODE_MII,
    PHY_INTERFACE_MODE_GMII,
    PHY_INTERFACE_MODE_SGMII,
    PHY_INTERFACE_MODE_TBI,
    PHY_INTERFACE_MODE_REVMII,
    PHY_INTERFACE_MODE_RMII,
    PHY_INTERFACE_MODE_RGMII,
    PHY_INTERFACE_MODE_RGMII_ID,
    PHY_INTERFACE_MODE_RGMII_RXID,
    PHY_INTERFACE_MODE_RGMII_TXID,
    PHY_INTERFACE_MODE_RTBI,
    PHY_INTERFACE_MODE_SMII,
    PHY_INTERFACE_MODE_XGMII,
    PHY_INTERFACE_MODE_XLGMII,
    PHY_INTERFACE_MODE_MOCA,
    PHY_INTERFACE_MODE_QSGMII,
    PHY_INTERFACE_MODE_TRGMII,
    PHY_INTERFACE_MODE_1000BASEX,
    PHY_INTERFACE_MODE_2500BASEX,
    PHY_INTERFACE_MODE_RXAUI,
    PHY_INTERFACE_MODE_XAUI,
    /* 10GBASE-R, XFI, SFI - single lane 10G Serdes */
    PHY_INTERFACE_MODE_10GBASER,
    PHY_INTERFACE_MODE_USXGMII,
    /* 10GBASE-KR - with Clause 73 AN */
    PHY_INTERFACE_MODE_10GKR,
    PHY_INTERFACE_MODE_MAX,
} phy_interface_t;

// #define SYSCFG_GMAC_USE_EXTCLK  1
// #define SYSCFG_GMAC_TX_DELAY    0xC
// #define SYSCFG_GMAC_RX_DELAY    0xC

#define SYSCFG_GMAC0_PHY_MODE   PHY_INTERFACE_MODE_RMII
// #define SYSCFG_GMAC1_PHY_MODE   PHY_INTERFACE_MODE_RGMII

void syscfg_usb_phy0_sw_host(s32 sw);

s32 syscfg_fpga_de_clk_sel_by_div(u8 sclk, u8 pixclk);
void syscfg_fpga_lcd_io_set(u32 val);

s32 hal_syscfg_probe(void);

#endif
