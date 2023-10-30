/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ARTINCHIP_AIC_CLK_ID_H__
#define __ARTINCHIP_AIC_CLK_ID_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Fixed rate clock */
#define CLK_DUMMY  0
#define CLK_OSC24M 1
#define CLK_OSC32K 2
#define CLK_RC1M   3
/* PLL clock */
#define CLK_PLL_INT0 4
#define CLK_PLL_INT1 5
#define CLK_PLL_FRA0 6
#define CLK_PLL_FRA1 7
#define CLK_PLL_FRA2 8
/* fixed factor clock */
#define CLK_AXI0_SRC1 9
#define CLK_AHB0_SRC1 10
#define CLK_APB0_SRC1 11
#define CLK_APB1_SRC1 12
#define CLK_CPU_SRC1  13
#define CLK_DM_SRC1   14
/* system clock */
#define CLK_AXI0 15
#define CLK_AHB0 16
#define CLK_APB0 17
#define CLK_APB1 18
#define CLK_CPU  19
/* Peripheral clock */
#define CLK_DMA      20
#define CLK_CE       21
#define CLK_USBD     22
#define CLK_USBH0    23
#define CLK_USBH1    24
#define CLK_USB_PHY0 25
#define CLK_USB_PHY1 26
#define CLK_GMAC0    27
#define CLK_GMAC1    28
#define CLK_QSPI0     29
#define CLK_QSPI1     30
#define CLK_QSPI2     31
#define CLK_QSPI3     32
#define CLK_SDMC0    33
#define CLK_SDMC1    34
#define CLK_SDMC2    35
#define CLK_SYSCFG   36
#define CLK_RTC      37
#define CLK_SPIENC   38
#define CLK_I2S0     39
#define CLK_I2S1     40
#define CLK_CODEC    41
#define CLK_RGB      42
#define CLK_LVDS     43
#define CLK_MIPIDSI  44
#define CLK_DE       45
#define CLK_GE       46
#define CLK_VE       47
#define CLK_WDT      48
#define CLK_SID      49
#define CLK_GTC      50
#define CLK_GPIO     51
#define CLK_UART0    52
#define CLK_UART1    53
#define CLK_UART2    54
#define CLK_UART3    55
#define CLK_UART4    56
#define CLK_UART5    57
#define CLK_UART6    58
#define CLK_UART7    59
#define CLK_I2C0     60
#define CLK_I2C1     61
#define CLK_I2C2     62
#define CLK_I2C3     63
#define CLK_CAN0     64
#define CLK_CAN1     65
#define CLK_PWM      66
#define CLK_ADCIM    67
#define CLK_GPAI     68
#define CLK_RTP      69
#define CLK_TSEN     70
#define CLK_CIR      71
#define CLK_DVP      72
#define CLK_PBUS     73
#define CLK_MTOP     74
#define CLK_DM       75
#define CLK_PWMCS    76
#define CLK_PSADC    77
/* Display clock */
#define CLK_PIX  78
#define CLK_SCLK 79
/* Output clock */
#define CLK_OUT0    80
#define CLK_OUT1    81
#define CLK_OUT2    82
#define CLK_OUT3    83
#define AIC_CLK_END 84


/* frequence */

#define CLOCK1_FREQ 48000000
#define CLOCK2_FREQ 100000000
#define CLOCK3_FREQ 24576000
#define CLOCK4_FREQ 62500000
#define CLOCK5_FREQ 48000000
#define CLOCK6_FREQ 60000000
#define CLOCK_120M  120000000
#define CLOCK_100M  100000000
#define CLOCK_72M   72000000
#define CLOCK_60M   60000000
#define CLOCK_50M   50000000
#define CLOCK_30M   30000000
#define CLOCK_24M   24000000
#define CLOCK_12M   12000000
#define CLOCK_4M    4000000
#define CLOCK_1M    1000000
#define CLOCK_32K   32768

/*--- ArtInChip CMU register offsets ---*/

#define PLL_INT0_GEN_REG (0x0000)
#define PLL_INT1_GEN_REG (0x0004)
#define PLL_FRA0_GEN_REG (0x0020)
#define PLL_FRA1_GEN_REG (0x0024)
#define PLL_FRA2_GEN_REG (0x0028)
#define PLL_INT0_CFG_REG (0x0040)
#define PLL_INT1_CFG_REG (0x0044)
#define PLL_FRA0_CFG_REG (0x0060)
#define PLL_FRA1_CFG_REG (0x0064)
#define PLL_FRA2_CFG_REG (0x0068)
#define PLL_FRA0_SDM_REG (0x0080)
#define PLL_FRA1_SDM_REG (0x0084)
#define PLL_FRA2_SDM_REG (0x0088)
#define PLL_COM_REG      (0x00A0)
#define PLL_IN_REG       (0x00A4)
#define CLK_OUT0_REG     (0x00E0)
#define CLK_OUT1_REG     (0x00E4)
#define CLK_OUT2_REG     (0x00E8)
#define CLK_OUT3_REG     (0x00EC)
#define CLK_AXI0_REG     (0x0100)
#define CLK_AHB0_REG     (0x0110)
#define CLK_APB0_REG     (0x0120)
#define CLK_APB1_REG     (0x0124)
#define CLK_CPU_REG      (0x0200)
#define CLK_DM_REG       (0x0204)
#define CLK_WDT_REG      (0x020C)
#define CLK_DDR_REG      (0x0210)
#define CLK_DISP_REG     (0x0220)
#define CLK_DMA_REG      (0x0410)
#define CLK_CE_REG       (0x0418)
#define CLK_USBD_REG     (0x041C)
#define CLK_USBH0_REG    (0x0420)
#define CLK_USBH1_REG    (0x0424)
#define CLK_USB_PHY0_REG (0x0430)
#define CLK_USB_PHY1_REG (0x0434)
#define CLK_GMAC0_REG    (0x0440)
#define CLK_GMAC1_REG    (0x0444)
#define CLK_QSPI0_REG    (0x0460)
#define CLK_QSPI1_REG    (0x0464)
#define CLK_QSPI2_REG    (0x0468)
#define CLK_QSPI3_REG    (0x046C)
#define CLK_SDMC0_REG    (0x0470)
#define CLK_SDMC1_REG    (0x0474)
#define CLK_SDMC2_REG    (0x0478)
#define CLK_PBUS_REG     (0x04A0)
#define CLK_SYSCFG_REG   (0x0800)
#define CLK_SPIENC_REG   (0x0810)
#define CLK_PWMCS_REG    (0x0814)
#define CLK_PSADC_REG    (0x0818)
#define CLK_MTOP_REG     (0x081C)
#define CLK_I2S0_REG     (0x0820)
#define CLK_I2S1_REG     (0x0824)
#define CLK_CODEC_REG    (0x0830)
#define CLK_GPIO_REG     (0x083C)
#define CLK_UART0_REG    (0x0840)
#define CLK_UART1_REG    (0x0844)
#define CLK_UART2_REG    (0x0848)
#define CLK_UART3_REG    (0x084C)
#define CLK_UART4_REG    (0x0850)
#define CLK_UART5_REG    (0x0854)
#define CLK_UART6_REG    (0x0858)
#define CLK_UART7_REG    (0x085C)
#define CLK_RGB_REG      (0x0880)
#define CLK_LVDS_REG     (0x0884)
#define CLK_MIPID_REG    (0x0888)
#define CLK_DVP_REG      (0x0890)
#define CLK_DE_REG       (0x08C0)
#define CLK_GE_REG       (0x08C4)
#define CLK_VE_REG       (0x08C8)
#define CLK_SID_REG      (0x0904)
#define CLK_RTC_REG      (0x0908)
#define CLK_GTC_REG      (0x090C)
#define CLK_I2C0_REG     (0x0960)
#define CLK_I2C1_REG     (0x0964)
#define CLK_I2C2_REG     (0x0968)
#define CLK_I2C3_REG     (0x096C)
#define CLK_CAN0_REG     (0x0980)
#define CLK_CAN1_REG     (0x0984)
#define CLK_PWM_REG      (0x0990)
#define CLK_ADCIM_REG    (0x09A0)
#define CLK_GPAI_REG     (0x09A4)
#define CLK_RTP_REG      (0x09A8)
#define CLK_TSEN_REG     (0x09AC)
#define CLK_CIR_REG      (0x09B0)
#define CLK_VER_REG      (0x0FFC)

/* PLL_xxx_GEN register fields */

#define PLL_LOCK_BIT            (17)
#define PLL_EN_BIT              (16)
#define PLL_FACTORN_BIT         (8)
#define PLL_FACTORN_MASK        (0xff)
#define PLL_FACTORN_MIN         (14)
#define PLL_FACTORN_MAX         (199)

#define PLL_FACTORM_BIT         (4)
#define PLL_FACTORM_MASK        (0x3)
#define PLL_FACTORM_MIN         (0)
#define PLL_FACTORM_MAX         (3)
#define PLL_FACTORM_EN_BIT      (19)

#define PLL_FACTORP_BIT         (0)
#define PLL_FACTORP_MASK        (0x1)
#define PLL_FACTORP_MIN         (0)
#define PLL_FACTORP_MAX         (1)

#define PLL_DITHER_EN_BIT       (24)
#define PLL_FRAC_EN_BIT         (20)
#define PLL_FRAC_DIV_BIT        (0)
#define PLL_FRAC_DIV_MASK       (0x1ffff)

#define PLL_OUT_MUX             (20)
#define PLL_OUT_SYS             (18)

#define PLL_SDM_AMP_BIT         (0)
#define PLL_SDM_FREQ_BIT        (17)
#define PLL_SDM_STEP_BIT        (20)
#define PLL_SDM_MODE_BIT        (29)
#define PLL_SDM_EN_BIT          (31)

#define PLL_VCO_MIN             (768000000)
#define PLL_VCO_MAX             (1560000000)
#define PLL_SDM_AMP_MAX         (0x20000)
#define PLL_SDM_SPREAD_PPM      (10000)
#define PLL_SDM_SPREAD_FREQ     (33000)

/* BUS CLX_xxx register fields */

#define BUS_CLK_SEL (8)

/* APB0 CLX_xxx register fields */

#define MOD_RSTN   (13)
#define MOD_BUS_EN (12)
#define MOD_CLK_EN (8)

#ifdef __cplusplus
}
#endif

#endif /* __ARTINCHIP_AIC_CLK_ID_H__ */
