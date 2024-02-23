/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_HAL_QSPI_REGS_H_
#define _AIC_HAL_QSPI_REGS_H_
#include <aic_common.h>
#include <aic_soc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QSPI_REG_VER(base)              (volatile void *)((base) + 0x00UL)
#define QSPI_REG_CFG(base)              (volatile void *)((base) + 0x04UL)
#define QSPI_REG_TCFG(base)             (volatile void *)((base) + 0x08UL)
#define QSPI_REG_ICR(base)              (volatile void *)((base) + 0x10UL)
#define QSPI_REG_ISTS(base)             (volatile void *)((base) + 0x14UL)
#define QSPI_REG_FCTL(base)             (volatile void *)((base) + 0x18UL)
#define QSPI_REG_FSTS(base)             (volatile void *)((base) + 0x1cUL)
#define QSPI_REG_WCC(base)              (volatile void *)((base) + 0x20UL)
#define QSPI_REG_CCFG(base)             (volatile void *)((base) + 0x24UL)
#define QSPI_REG_TBC(base)              (volatile void *)((base) + 0x30UL)
#define QSPI_REG_TWC(base)              (volatile void *)((base) + 0x34UL)
#define QSPI_REG_TMC(base)              (volatile void *)((base) + 0x38UL)
#define QSPI_REG_BMTC(base)             (volatile void *)((base) + 0x40UL)
#define QSPI_REG_BMCLK(base)            (volatile void *)((base) + 0x44UL)
#define QSPI_REG_BMTXD(base)            (volatile void *)((base) + 0x48UL)
#define QSPI_REG_BMRXD(base)            (volatile void *)((base) + 0x4cUL)
#define QSPI_REG_DMC(base)              (volatile void *)((base) + 0x88UL)
#define QSPI_REG_TXD(base)              (volatile void *)((base) + 0x200UL)
#define QSPI_REG_RXD(base)              (volatile void *)((base) + 0x300UL)

#define CFG_BIT_CTRL_EN_OFS             (0)
#define CFG_BIT_CTRL_EN_MSK             (1UL << 0)
#define CFG_BIT_CTRL_EN_VAL(v)          (((v) << CFG_BIT_CTRL_EN_OFS) & CFG_BIT_CTRL_EN_MSK)
#define CFG_BIT_CTRL_MODE_SEL_OFS       (1)
#define CFG_BIT_CTRL_MODE_SEL_MSK       (1UL << 1)
#define CFG_BIT_CTRL_MODE_SEL_VAL(v)    (((v) << CFG_BIT_CTRL_MODE_SEL_OFS) & CFG_BIT_CTRL_MODE_SEL_MSK)
#define CFG_BIT_RXFULL_STOP_OFS         (7)
#define CFG_BIT_RXFULL_STOP_MSK         (1UL << 7)
#define CFG_BIT_RXFULL_STOP_VAL(v)      (((v) << CFG_BIT_RXFULL_STOP_OFS) & CFG_BIT_RXFULL_STOP_MSK)
#define CFG_BIT_CTRL_RST_OFS            (31)
#define CFG_BIT_CTRL_RST_MSK            (1UL << 31)
#define CFG_BIT_CTRL_RST_VAL(v)         (((v) << CFG_BIT_CTRL_RST_OFS) & CFG_BIT_CTRL_RST_MSK)

#define TCFG_BIT_CPHA_OFS               (0)
#define TCFG_BIT_CPHA_MSK               (1UL << 0)
#define TCFG_BIT_CPHA_VAL(v)            (((v) << TCFG_BIT_CPHA_OFS) & TCFG_BIT_CPHA_MSK)
#define TCFG_BIT_CPOL_OFS               (1)
#define TCFG_BIT_CPOL_MSK               (1UL << 1)
#define TCFG_BIT_CPOL_VAL(v)            (((v) << TCFG_BIT_CPOL_OFS) & TCFG_BIT_CPOL_MSK)
#define TCFG_BIT_CS_POL_OFS             (2)
#define TCFG_BIT_CS_POL_MSK             (1UL << 2)
#define TCFG_BIT_CS_POL_VAL(v)          (((v) << TCFG_BIT_CS_POL_OFS) & TCFG_BIT_CS_POL_MSK)
#define TCFG_BIT_CS_VALID_CTL_OFS       (3)
#define TCFG_BIT_CS_VALID_CTL_MSK       (1UL << 3)
#define TCFG_BIT_CS_VALID_CTL_VAL(v)    (((v) << TCFG_BIT_CS_VALID_CTL_OFS) & TCFG_BIT_CS_VALID_CTL_MSK)
#define TCFG_BIT_CS_NUM_OFS             (4)
#define TCFG_BIT_CS_NUM_MSK             (0x3 << 4)
#define TCFG_BIT_CS_NUM_VAL(v)          (((v) << TCFG_BIT_CS_NUM_OFS) & TCFG_BIT_CS_NUM_MSK)
#define TCFG_BIT_CS_CTL_SEL_OFS         (6)
#define TCFG_BIT_CS_CTL_SEL_MSK         (1UL << 6)
#define TCFG_BIT_CS_CTL_SEL_VAL(v)      (((v) << TCFG_BIT_CS_CTL_SEL_OFS) & TCFG_BIT_CS_CTL_SEL_MSK)
#define TCFG_BIT_CS_LEVEL_OFS           (7)
#define TCFG_BIT_CS_LEVEL_MSK           (1UL << 7)
#define TCFG_BIT_CS_LEVEL_VAL(v)        (((v) << TCFG_BIT_CS_LEVEL_OFS) & TCFG_BIT_CS_LEVEL_MSK)
#define TCFG_BIT_DINVD_OFS              (8)
#define TCFG_BIT_DINVD_MSK              (1UL << 8)
#define TCFG_BIT_DINVD_VAL(v)           (((v) << TCFG_BIT_DINVD_OFS) & TCFG_BIT_DINVD_MSK)
#define TCFG_BIT_DMY_VAL_OFS            (9)
#define TCFG_BIT_DMY_VAL_MSK            (1UL << 9)
#define TCFG_BIT_DMY_VAL_VAL(v)         (((v) << TCFG_BIT_DMY_VAL_OFS) & TCFG_BIT_DMY_VAL_MSK)
#define TCFG_BIT_HSWM_OFS               (10)
#define TCFG_BIT_HSWM_MSK               (1UL << 10)
#define TCFG_BIT_HSWM_VAL(v)            (((v) << TCFG_BIT_HSWM_OFS) & TCFG_BIT_HSWM_MSK)
#define TCFG_BIT_RXINDLY_EN_OFS         (11)
#define TCFG_BIT_RXINDLY_EN_MSK         (1UL << 11)
#define TCFG_BIT_RXINDLY_EN_VAL(v)      (((v) << TCFG_BIT_RXINDLY_EN_OFS) & TCFG_BIT_RXINDLY_EN_MSK)
#define TCFG_BIT_LSB_EN_OFS             (12)
#define TCFG_BIT_LSB_EN_MSK             (1UL << 12)
#define TCFG_BIT_LSB_EN_VAL(v)          (((v) << TCFG_BIT_LSB_EN_OFS) & TCFG_BIT_LSB_EN_MSK)
#define TCFG_BIT_RXDLY_DIS_OFS          (13)
#define TCFG_BIT_RXDLY_DIS_MSK          (1UL << 13)
#define TCFG_BIT_RXDLY_DIS_VAL(v)       (((v) << TCFG_BIT_RXDLY_DIS_OFS) & TCFG_BIT_RXDLY_DIS_MSK)
#define TCFG_BIT_TXDLY_EN_OFS           (14)
#define TCFG_BIT_TXDLY_EN_MSK           (1UL << 14)
#define TCFG_BIT_TXDLY_EN_VAL(v)        (((v) << TCFG_BIT_TXDLY_EN_OFS) & TCFG_BIT_TXDLY_EN_MSK)
#define TCFG_BIT_START_OFS              (31)
#define TCFG_BIT_START_MSK              (1UL << 31)
#define TCFG_BIT_START_VAL(v)           (((v) << TCFG_BIT_START_OFS) & TCFG_BIT_START_MSK)

#define TCFG_RX_SAMP_DLY_MSK            (TCFG_BIT_RXINDLY_EN_MSK | TCFG_BIT_RXDLY_DIS_MSK)
#define TCFG_RX_SAMP_DLY_NONE           (TCFG_BIT_RXDLY_DIS_MSK)
#define TCFG_RX_SAMP_DLY_HALF           (0)
#define TCFG_RX_SAMP_DLY_ONE            (TCFG_BIT_RXINDLY_EN_MSK)

#define FCTL_BIT_RF_WATERMARK_OFS       (0)
#define FCTL_BIT_RF_WATERMARK_MSK       (0xFFUL << 0)
#define FCTL_BIT_RF_WATERMARK_VAL(v)    (((v) << FCTL_BIT_RF_WATERMARK_OFS) & FCTL_BIT_RF_WATERMARK_MSK)
#define FCTL_BIT_RF_DREQ_EN_OFS         (8)
#define FCTL_BIT_RF_DREQ_EN_MSK         (1UL << 8)
#define FCTL_BIT_RF_DREQ_EN_VAL(v)      (((v) << FCTL_BIT_RF_DREQ_EN_OFS) & FCTL_BIT_RF_DREQ_EN_MSK)
#define FCTL_BIT_RF_RST_OFS             (15)
#define FCTL_BIT_RF_RST_MSK             (1UL << 15)
#define FCTL_BIT_RF_RST_VAL(v)          (((v) << FCTL_BIT_RF_RST_OFS) & FCTL_BIT_RF_RST_MSK)
#define FCTL_BIT_TF_WATERMARK_OFS       (16)
#define FCTL_BIT_TF_WATERMARK_MSK       (0xFFUL << 16)
#define FCTL_BIT_TF_WATERMARK_VAL(v)    (((v) << FCTL_BIT_TF_WATERMARK_OFS) & FCTL_BIT_TF_WATERMARK_MSK)
#define FCTL_BIT_TF_DREQ_EN_OFS         (24)
#define FCTL_BIT_TF_DREQ_EN_MSK         (1UL << 24)
#define FCTL_BIT_TF_DREQ_EN_VAL(v)      (((v) << FCTL_BIT_TF_DREQ_EN_OFS) & FCTL_BIT_TF_DREQ_EN_MSK)
#define FCTL_BIT_TF_RST_OFS             (31)
#define FCTL_BIT_TF_RST_MSK             (1UL << 31)
#define FCTL_BIT_TF_RST_VAL(v)          (((v) << FCTL_BIT_TF_RST_OFS) & FCTL_BIT_TF_RST_MSK)
#define FCTL_BIT_DMA_EN_MSK             (FCTL_BIT_TF_DREQ_EN_MSK | FCTL_BIT_RF_DREQ_EN_MSK)

#define FSTS_BIT_RF_CNT_OFS             (0)
#define FSTS_BIT_RF_CNT_MSK             (0xFF)
#define FSTS_BIT_RF_CNT_VAL(v)          (((v) << FSTS_BIT_RF_CNT_OFS) & FSTS_BIT_RF_CNT_MSK)
#define FSTS_BIT_RF_RBUF_CNT_OFS        (12)
#define FSTS_BIT_RF_RBUF_CNT_MSK        (0xF << 12)
#define FSTS_BIT_RF_RBUF_CNT_VAL(v)     (((v) << FSTS_BIT_RF_RBUF_CNT_OFS) & FSTS_BIT_RF_RBUF_CNT_MSK)
#define FSTS_BIT_RF_RBUF_STS_OFS        (15)
#define FSTS_BIT_RF_RBUF_STS_MSK        (1UL << 15)
#define FSTS_BIT_RF_RBUF_STS_VAL(v)     (((v) << FSTS_BIT_RF_RBUF_STS_OFS) & FSTS_BIT_RF_RBUF_STS_MSK)
#define FSTS_BIT_TF_CNT_OFS             (16)
#define FSTS_BIT_TF_CNT_MSK             (0xFF << 16)
#define FSTS_BIT_TF_CNT_VAL(v)          (((v) << FSTS_BIT_TF_CNT_OFS) & FSTS_BIT_TF_CNT_MSK)
#define FSTS_BIT_TF_WBUF_CNT_OFS        (28)
#define FSTS_BIT_TF_WBUF_CNT_MSK        (0xF << 28)
#define FSTS_BIT_TF_WBUF_CNT_VAL(v)     (((v) << FSTS_BIT_TF_WBUF_CNT_OFS) & FSTS_BIT_TF_WBUF_CNT_MSK)
#define FSTS_BIT_TF_WBUF_STS_OFS        (31)
#define FSTS_BIT_TF_WBUF_STS_MSK        (1UL << 31)
#define FSTS_BIT_TF_WBUF_STS_VAL(v)     (((v) << FSTS_BIT_TF_WBUF_STS_OFS) & FSTS_BIT_TF_WBUF_STS_MSK)

#define ICR_BIT_RF_RDY_INTE             (1UL << 0)
#define ICR_BIT_RF_EMP_INTE             (1UL << 1)
#define ICR_BIT_RF_FUL_INTE             (1UL << 2)
#define ICR_BIT_TF_RDY_INTE             (1UL << 4)
#define ICR_BIT_TF_EMP_INTE             (1UL << 5)
#define ICR_BIT_TF_FUL_INTE             (1UL << 6)
#define ICR_BIT_RF_OVF_INTE             (1UL << 8)
#define ICR_BIT_RF_UDR_INTE             (1UL << 9)
#define ICR_BIT_TF_OVF_INTE             (1UL << 10)
#define ICR_BIT_TF_UDR_INTE             (1UL << 11)
#define ICR_BIT_TDONE_INTE              (1UL << 12)
#define ICR_BIT_ERRS                    (ICR_BIT_TF_OVF_INTE | ICR_BIT_RF_UDR_INTE | ICR_BIT_RF_OVF_INTE)
#define ICR_BIT_ALL_MSK                 (0x77 | (0x3f << 8))
#define ICR_BIT_DMA_MSK                 (ICR_BIT_ERRS | ICR_BIT_TDONE_INTE)
#define ICR_BIT_CPU_MSK \
    (ICR_BIT_ERRS | ICR_BIT_RF_RDY_INTE | ICR_BIT_TF_RDY_INTE | ICR_BIT_TDONE_INTE)

#define ISTS_BIT_RF_RDY                 (1UL << 0)
#define ISTS_BIT_RF_EMP                 (1UL << 1)
#define ISTS_BIT_RF_FUL                 (1UL << 2)
#define ISTS_BIT_TF_RDY                 (1UL << 4)
#define ISTS_BIT_TF_EMP                 (1UL << 5)
#define ISTS_BIT_TF_FUL                 (1UL << 6)
#define ISTS_BIT_RF_OVF                 (1UL << 8)
#define ISTS_BIT_RF_UDR                 (1UL << 9)
#define ISTS_BIT_TF_OVF                 (1UL << 10)
#define ISTS_BIT_TF_UDR                 (1UL << 11)
#define ISTS_BIT_TDONE                  (1UL << 12)
#define ISTS_BIT_ERRS                   (ISTS_BIT_TF_OVF | ISTS_BIT_RF_UDR | ISTS_BIT_RF_OVF)
#define ISTS_BIT_ALL_MSK                (0x77 | (0x3f << 8))
#define ISTS_BIT_DMA_MSK                (ISTS_BIT_ERRS | ISTS_BIT_TDONE)

#define WCC_BIT_WCC_OFS                 (0)
#define WCC_BIT_WCC_MSK                 (0xFFFF)
#define WCC_BIT_WCC_VAL(v)              (((v) << WCC_BIT_WCC_OFS) & WCC_BIT_WCC_MSK)
#define WCC_BIT_SMWC_OFS                (16)
#define WCC_BIT_SMWC_MSK                (0xF << 16)
#define WCC_BIT_SMWC_VAL(v)             (((v) << WCC_BIT_SMWC_OFS) & WCC_BIT_SMWC_MSK)

#define CCFG_BIT_CKDIV2_OFS             (0)
#define CCFG_BIT_CKDIV2_MSK             (0xFF)
#define CCFG_BIT_CKDIV2_VAL(v)          (((v) << CCFG_BIT_CKDIV2_OFS) & CCFG_BIT_CKDIV2_MSK)
#define CCFG_BIT_CKDIV1_OFS             (8)
#define CCFG_BIT_CKDIV1_MSK             (0xF << 8)
#define CCFG_BIT_CKDIV1_VAL(v)          (((v) << CCFG_BIT_CKDIV1_OFS) & CCFG_BIT_CKDIV1_MSK)
#define CCFG_BIT_CKDIV_SEL_OFS          (12)
#define CCFG_BIT_CKDIV_SEL_MSK          (1UL << 12)
#define CCFG_BIT_CKDIV_SEL_VAL(v)       (((v) << CCFG_BIT_CKDIV_SEL_OFS) & CCFG_BIT_CKDIV_SEL_MSK)

#define TBC_BIT_TB_CNT_OFS              (0)
#define TBC_BIT_TB_CNT_MSK              (0xFFFFFF)
#define TBC_BIT_TB_CNT_VAL(v)           (((v) << TBC_BIT_TB_CNT_OFS) & TBC_BIT_TB_CNT_MSK)

#define TWC_BIT_TXD_CNT_OFS             (0)
#define TWC_BIT_TXD_CNT_MSK             (0xFFFFFF)
#define TWC_BIT_TXD_CNT_VAL(v)          (((v) << TWC_BIT_TXD_CNT_OFS) & TWC_BIT_TXD_CNT_MSK)

#define TMC_BIT_STXD_CNT_OFS            (0)
#define TMC_BIT_STXD_CNT_MSK            (0xFFFFFF)
#define TMC_BIT_STXD_CNT_VAL(v)         (((v) << TMC_BIT_STXD_CNT_OFS) & TMC_BIT_STXD_CNT_MSK)
#define TMC_BIT_DMY_CNT_OFS             (24)
#define TMC_BIT_DMY_CNT_MSK             (0xF << 24)
#define TMC_BIT_DMY_CNT_VAL(v)          (((v) << TMC_BIT_DMY_CNT_OFS) & TMC_BIT_DMY_CNT_MSK)
#define TMC_BIT_DUAL_EN_OFS             (28)
#define TMC_BIT_DUAL_EN_MSK             (1UL << 28)
#define TMC_BIT_DUAL_EN_VAL(v)          (((v) << TMC_BIT_DUAL_EN_OFS) & TMC_BIT_DUAL_EN_MSK)
#define TMC_BIT_QUAD_EN_OFS             (29)
#define TMC_BIT_QUAD_EN_MSK             (1UL << 29)
#define TMC_BIT_QUAD_EN_VAL(v)          (((v) << TMC_BIT_QUAD_EN_OFS) & TMC_BIT_QUAD_EN_MSK)

#define BMTC_BIT_WM_OFS		(0)
#define BMTC_BIT_WM_MSK		GENMASK(1, 0)
#define BMTC_BIT_WM_BYTE		(0x0)
#define BMTC_BIT_WM_BIT_3WIRE	(0x2)
#define BMTC_BIT_WM_BIT_STD	(0x3)
#define BMTC_BIT_SS_SEL_OFS	(2)
#define BMTC_BIT_SS_SEL_MSK	GENMASK(3, 2)
#define BMTC_BIT_SPOL_OFS	(5)
#define BMTC_BIT_SPOL_MSK	BIT(5)
#define BMTC_BIT_SS_OWNER_OFS	(6)
#define BMTC_BIT_SS_OWNER_MSK	BIT(6)
#define BMTC_BIT_SS_LEVEL_OFS	(7)
#define BMTC_BIT_SS_LEVEL_MSK	BIT(7)
#define BMTC_BIT_TX_BIT_LEN_OFS	(8)
#define BMTC_BIT_TX_BIT_LEN_MSK	GENMASK(13, 8)
#define BMTC_BIT_RX_BIT_LEN_OFS	(16)
#define BMTC_BIT_RX_BIT_LEN_MSK	GENMASK(21, 16)
#define BMTC_BIT_XFER_COMP_OFS	(25)
#define BMTC_BIT_XFER_COMP_MSK	BIT(25)
#define BMTC_BIT_SAMP_MODE_OFS	(30)
#define BMTC_BIT_SAMP_MODE_MSK	BIT(30)
#define BMTC_BIT_XFER_EN_OFS	(31)
#define BMTC_BIT_XFER_EN_MSK	BIT(31)

#define QSPI_MAX_XFER_SIZE              (0xFFFFFF)
#define QSPI_FIFO_DEPTH                 64
#define QSPI_INVALID_BASE               (0xFFFFFFFF)

static inline u32 qspi_hw_index_to_base(u32 idx)
{
    switch (idx) {
        case 0:
            return QSPI0_BASE;
        case 1:
            return QSPI1_BASE;
        case 2:
            return QSPI2_BASE;
        case 3:
            return QSPI3_BASE;
        default:
            return 0;
    }
    return 0;
}

static inline u32 qspi_hw_base_to_index(u32 base)
{
    switch (base) {
        case QSPI0_BASE:
            return 0;
        case QSPI1_BASE:
            return 1;
        case QSPI2_BASE:
            return 2;
        case QSPI3_BASE:
            return 3;
        default:
            return QSPI_INVALID_BASE;
    }
    return QSPI_INVALID_BASE;
}

static inline void qspi_hw_init_default(u32 base)
{
    u32 val;

    val = CFG_BIT_CTRL_EN_MSK | CFG_BIT_CTRL_MODE_SEL_MSK |
          CFG_BIT_RXFULL_STOP_MSK | CFG_BIT_CTRL_RST_MSK;
    writel(val, QSPI_REG_CFG(base));
}

#define QSPI_CTRL_MODE_SLAVE 0
#define QSPI_CTRL_MODE_MASTER 1
static inline void qspi_hw_set_ctrl_mode(u32 base, u32 mode)
{
    u32 val = readl(QSPI_REG_CFG(base));
    val &= ~(CFG_BIT_CTRL_MODE_SEL_MSK);
    val |= CFG_BIT_CTRL_MODE_SEL_VAL(mode);
    writel(val, QSPI_REG_CFG(base));
}

static inline void qspi_hw_set_tx_delay_mode(u32 base, u32 en)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    val &= ~(TCFG_BIT_TXDLY_EN_MSK);
    val |= TCFG_BIT_TXDLY_EN_VAL(en);
    writel(val, QSPI_REG_TCFG(base));
}

static inline u32 qspi_hw_freq_to_delay_mode(u32 freq)
{
    if (freq <= 24000000)
        return TCFG_RX_SAMP_DLY_NONE;
    else if (freq <= 60000000)
        return TCFG_RX_SAMP_DLY_HALF;
    return TCFG_RX_SAMP_DLY_ONE;
}

static inline void qspi_hw_set_rx_delay_mode(u32 base, u32 mode)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    val &= ~(TCFG_RX_SAMP_DLY_MSK);
    val |= mode;
    writel(val, QSPI_REG_TCFG(base));
}

static inline void qspi_hw_cs_init(u32 base, u32 pol, u32 level,
                                   u32 soft_ctrl)
{
    u32 val;

    val = readl(QSPI_REG_TCFG(base));
    val &= ~(TCFG_BIT_CS_POL_MSK | TCFG_BIT_CS_LEVEL_MSK |
             TCFG_BIT_CS_CTL_SEL_MSK);

    val |= ((pol << TCFG_BIT_CS_POL_OFS) & TCFG_BIT_CS_POL_MSK);
    val |= ((level << TCFG_BIT_CS_LEVEL_OFS) & TCFG_BIT_CS_LEVEL_MSK);
    val |= ((soft_ctrl << TCFG_BIT_CS_CTL_SEL_OFS) & TCFG_BIT_CS_CTL_SEL_MSK);

    writel(val, QSPI_REG_TCFG(base));
}

static inline void qspi_hw_select_cs_num(u32 base, u32 num)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    val &= ~(TCFG_BIT_CS_NUM_MSK);
    val |= TCFG_BIT_CS_NUM_VAL(num);
    writel(val, QSPI_REG_TCFG(base));
}

static inline u32 qspi_hw_get_cur_cs_num(u32 base)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    return ((val & TCFG_BIT_CS_NUM_MSK) >> TCFG_BIT_CS_NUM_OFS);
}

#define QSPI_CS_POL_VALID_HIGH 0
#define QSPI_CS_POL_VALID_LOW  1
static inline void qspi_hw_set_cs_polarity(u32 base, u32 pol)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    val &= ~(TCFG_BIT_CS_POL_MSK);
    val |= TCFG_BIT_CS_POL_VAL(pol);
    writel(val, QSPI_REG_TCFG(base));
}

static inline u32 qspi_hw_get_cs_polarity(u32 base)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    return ((val & TCFG_BIT_CS_POL_MSK) >> TCFG_BIT_CS_POL_OFS);
}

static inline void qspi_hw_set_cpha(u32 base, u32 cpha)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    val &= ~(TCFG_BIT_CPHA_MSK);
    val |= TCFG_BIT_CPHA_VAL(cpha);
    writel(val, QSPI_REG_TCFG(base));
}

static inline u32 qspi_hw_get_cpha(u32 base)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    return ((val & TCFG_BIT_CPHA_MSK) >> TCFG_BIT_CPHA_OFS);
}

static inline void qspi_hw_set_cpol(u32 base, u32 cpol)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    val &= ~(TCFG_BIT_CPOL_MSK);
    val |= TCFG_BIT_CPOL_VAL(cpol);
    writel(val, QSPI_REG_TCFG(base));
}

static inline u32 qspi_hw_get_cpol(u32 base)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    return ((val & TCFG_BIT_CPOL_MSK) >> TCFG_BIT_CPOL_OFS);
}

static inline void qspi_hw_set_lsb_en(u32 base, u32 en)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    val &= ~(TCFG_BIT_LSB_EN_MSK);
    val |= TCFG_BIT_LSB_EN_VAL(en);
    writel(val, QSPI_REG_TCFG(base));
}

static inline u32 qspi_hw_get_lsb_en(u32 base)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    return ((val & TCFG_BIT_LSB_EN_MSK) >> TCFG_BIT_LSB_EN_OFS);
}

#define QSPI_CS_LEVEL_LOW  0
#define QSPI_CS_LEVEL_HIGH 1
static inline void qspi_hw_set_cs_level(u32 base, u32 level)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    val &= ~(TCFG_BIT_CS_LEVEL_MSK);
    val |= TCFG_BIT_CS_LEVEL_VAL(level);
    writel(val, QSPI_REG_TCFG(base));
}

#define QSPI_CS_CTL_BY_HW 0
#define QSPI_CS_CTL_BY_SW 1
static inline void qspi_hw_set_cs_owner(u32 base, u32 soft)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    val &= ~(TCFG_BIT_CS_CTL_SEL_MSK);
    val |= TCFG_BIT_CS_CTL_SEL_VAL(soft);
    writel(val, QSPI_REG_TCFG(base));
}

#define QSPI_RECV_ALL_INPUT_DATA 0
#define QSPI_DROP_INVALID_DATA   1
static inline void qspi_hw_drop_invalid_data(u32 base, bool drop)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    val &= ~(TCFG_BIT_DINVD_MSK);
    val |= TCFG_BIT_DINVD_VAL(drop);
    writel(val, QSPI_REG_TCFG(base));
}

static inline void qspi_hw_reset_fifo(u32 base)
{
    u32 val = readl(QSPI_REG_FCTL(base));
    val |= (FCTL_BIT_RF_RST_MSK | FCTL_BIT_TF_RST_MSK);
    writel(val, QSPI_REG_FCTL(base));
}

static inline void qspi_hw_reset_rx_fifo(u32 base)
{
    u32 val = readl(QSPI_REG_FCTL(base));
    val |= (FCTL_BIT_RF_RST_MSK);
    writel(val, QSPI_REG_FCTL(base));
}

static inline void qspi_hw_reset_tx_fifo(u32 base)
{
    u32 val = readl(QSPI_REG_FCTL(base));
    val |= (FCTL_BIT_TF_RST_MSK);
    writel(val, QSPI_REG_FCTL(base));
}

#define QSPI_RX_WATERMARK 32
#define QSPI_TX_WATERMARK 32
static inline void qspi_hw_set_fifo_watermark(u32 base, u32 tx_wm, u32 rx_wm)
{
    u32 val = readl(QSPI_REG_FCTL(base));
    val &= ~(FCTL_BIT_RF_WATERMARK_MSK | FCTL_BIT_TF_WATERMARK_MSK);
    val |= FCTL_BIT_RF_WATERMARK_VAL(rx_wm);
    val |= FCTL_BIT_TF_WATERMARK_VAL(tx_wm);
    writel(val, QSPI_REG_FCTL(base));
}

static inline void qspi_hw_interrupt_enable(u32 base, u32 mask)
{
    u32 val = readl(QSPI_REG_ICR(base));
    val |= mask;
    writel(val, QSPI_REG_ICR(base));
}

static inline void qspi_hw_interrupt_disable(u32 base, u32 mask)
{
    u32 val = readl(QSPI_REG_ICR(base));
    val &= ~mask;
    writel(val, QSPI_REG_ICR(base));
}

static inline void qspi_hw_rx_dma_enable(u32 base)
{
    u32 val = readl(QSPI_REG_FCTL(base));
    val |= FCTL_BIT_RF_DREQ_EN_MSK;
    writel(val, QSPI_REG_FCTL(base));
}

static inline void qspi_hw_rx_dma_disable(u32 base)
{
    u32 val = readl(QSPI_REG_FCTL(base));
    val &= ~(FCTL_BIT_RF_DREQ_EN_MSK);
    writel(val, QSPI_REG_FCTL(base));
}

static inline bool qspi_hw_rx_dma_is_enabled(u32 base)
{
    u32 val = readl(QSPI_REG_FCTL(base));
    return ((val & FCTL_BIT_RF_DREQ_EN_MSK) != 0);
}

static inline void qspi_hw_tx_dma_enable(u32 base)
{
    u32 val = readl(QSPI_REG_FCTL(base));
    val |= FCTL_BIT_TF_DREQ_EN_MSK;
    writel(val, QSPI_REG_FCTL(base));
}

static inline void qspi_hw_tx_dma_disable(u32 base)
{
    u32 val = readl(QSPI_REG_FCTL(base));
    val &= ~(FCTL_BIT_TF_DREQ_EN_MSK);
    writel(val, QSPI_REG_FCTL(base));
}

static inline bool qspi_hw_tx_dma_is_enabled(u32 base)
{
    u32 val = readl(QSPI_REG_FCTL(base));
    return ((val & FCTL_BIT_TF_DREQ_EN_MSK) != 0);
}

static inline void qspi_hw_get_interrupt_status(u32 base, u32 *sts)
{
    *sts = readl(QSPI_REG_ISTS(base));
}

static inline void qspi_hw_clear_interrupt_status(u32 base, u32 msk)
{
    writel(msk, QSPI_REG_ISTS(base));
}

#define QSPI_CLK_DIVIDER_1 0
#define QSPI_CLK_DIVIDER_2 1
static inline void qspi_hw_set_clk_div(u32 base, u32 div_sel, u32 div)
{
    u32 val = readl(QSPI_REG_CCFG(base));
    if (div_sel == QSPI_CLK_DIVIDER_2) {
        val &= ~(CCFG_BIT_CKDIV_SEL_MSK);
        val &= ~(CCFG_BIT_CKDIV2_MSK);
        val |= CCFG_BIT_CKDIV_SEL_VAL(QSPI_CLK_DIVIDER_2);
        val |= CCFG_BIT_CKDIV2_VAL(div);
    } else {
        val &= ~(CCFG_BIT_CKDIV_SEL_MSK);
        val &= ~(CCFG_BIT_CKDIV1_MSK);
        val |= CCFG_BIT_CKDIV_SEL_VAL(QSPI_CLK_DIVIDER_1);
        val |= CCFG_BIT_CKDIV1_VAL(div);
    }
    writel(val, QSPI_REG_CCFG(base));
}

#define QSPI_BUS_WIDTH_QUAD   4
#define QSPI_BUS_WIDTH_DUAL   2
#define QSPI_BUS_WIDTH_SINGLE 1
static inline void qspi_hw_set_bus_width(u32 base, u32 bw)
{
    u32 val = readl(QSPI_REG_TMC(base));
    val &= ~(TMC_BIT_QUAD_EN_MSK | TMC_BIT_DUAL_EN_MSK);
    switch (bw) {
        case QSPI_BUS_WIDTH_QUAD:
            val |= TMC_BIT_QUAD_EN_VAL(1);
            break;
        case QSPI_BUS_WIDTH_DUAL:
            val |= TMC_BIT_DUAL_EN_VAL(1);
            break;
        case QSPI_BUS_WIDTH_SINGLE:
        default:
            break;
    }
    writel(val, QSPI_REG_TMC(base));
}

static inline u32 qspi_hw_get_bus_width(u32 base)
{
    u32 val = readl(QSPI_REG_TMC(base));
    if (val & TMC_BIT_QUAD_EN_MSK)
        return QSPI_BUS_WIDTH_QUAD;
    else if (val & TMC_BIT_DUAL_EN_MSK)
        return QSPI_BUS_WIDTH_DUAL;
    else
        return QSPI_BUS_WIDTH_SINGLE;
}

static inline void qspi_hw_set_dummy_value(u32 base, u8 dummy)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    val &= ~(TCFG_BIT_DMY_VAL_MSK);
    val |= TCFG_BIT_DMY_VAL_VAL(dummy);
    writel(val, QSPI_REG_TCFG(base));
}

static inline u32 qspi_hw_get_tx_fifo_cnt(u32 base)
{
    u32 val = readl(QSPI_REG_FSTS(base));
    return ((val & FSTS_BIT_TF_CNT_MSK) >> FSTS_BIT_TF_CNT_OFS);
}

static inline void qspi_hw_write_fifo(u32 base, u8 *data, u32 len)
{
    while (len) {
        writeb(*data++, QSPI_REG_TXD(base));
        len--;
    }
}

static inline u32 qspi_hw_get_rx_fifo_cnt(u32 base)
{
    u32 val = readl(QSPI_REG_FSTS(base));
    return ((val & FSTS_BIT_RF_CNT_MSK) >> FSTS_BIT_RF_CNT_OFS);
}

static inline void qspi_hw_read_fifo(u32 base, u8 *data, u32 len)
{
    while (len) {
        *data++ = readb(QSPI_REG_RXD(base));
        len--;
    }
}

/*
 * txlen: All tx data byte length in this transfer
 * tx_1line_cnt: How much tx bytes using 1 line to send out in this transfer
 * rxlen: All rx data byte length in this transfer
 * dmy_cnt: How much dummy bytes need hardware to generate and send out
 */
static inline void qspi_hw_set_transfer_cnt(u32 base, u32 txlen, u32 tx_1line_cnt, u32 rxlen,
                                            u32 hw_dmy_cnt)
{
    u32 val = readl(QSPI_REG_TBC(base));

    /* Total transfer byte count */
    val &= ~(TBC_BIT_TB_CNT_MSK);
    val |= TBC_BIT_TB_CNT_VAL(txlen + rxlen + hw_dmy_cnt);
    writel(val, QSPI_REG_TBC(base));

    /* Total write data length */
    val = readl(QSPI_REG_TWC(base));
    val &= ~(TWC_BIT_TXD_CNT_MSK);
    val |= TWC_BIT_TXD_CNT_VAL(txlen);
    writel(val, QSPI_REG_TWC(base));

    /*
     * Special configuration:
     * # Data length need to send in single mode
     *   - If the transfer is already single mode, the tx_1line_cnt should be
     *     the same with total send data length
     *   - If the transfer is DUAL/QUAD mode, the tx_1line_cnt specifies the length
     *     that need to send out in single mode, usually for cmd,addr,...
     * # Dummy count for DUAL/QUAD mode
     *   - DMY_CNT specifies how many dummy burst should be sent before receive
     *     data in DUAL/QUAD mode
     */
    val = readl(QSPI_REG_TMC(base));
    val &= ~(TMC_BIT_STXD_CNT_MSK);
    val |= TMC_BIT_STXD_CNT_VAL(tx_1line_cnt);
    val &= ~(TMC_BIT_DMY_CNT_MSK);
    val |= TMC_BIT_DMY_CNT_VAL(hw_dmy_cnt);
    writel(val, QSPI_REG_TMC(base));
}

static inline void qspi_hw_start_transfer(u32 base)
{
    u32 val = readl(QSPI_REG_TCFG(base));
    val |= TCFG_BIT_START_VAL(1UL);
    writel(val, QSPI_REG_TCFG(base));
}

static inline bool qspi_hw_check_transfer_done(u32 base)
{
    u32 val = readl(QSPI_REG_ISTS(base));
    return ((val & ISTS_BIT_TDONE) != 0);
}

static inline u32 qspi_hw_bit_mode_set_cs_num(u32 chipselect, u32 base)
{
    u32 val;

    if (chipselect != 0)
        return -EINVAL;

    val = readl(QSPI_REG_BMTC(base));
    val &= ~BMTC_BIT_SS_SEL_MSK;
    val |= (chipselect << BMTC_BIT_SS_SEL_OFS);
    writel(val, QSPI_REG_BMTC(base));

    return 0;
}

static inline void qspi_hw_bit_mode_set_clk(u32 spiclk, u32 mclk, u32 base)
{
    u32 div;

    /*
	 * mclk: module source clock
	 * spiclk: expected spi working clock
	 */

    if (spiclk == 0)
        spiclk = 1;
    div = mclk / (2 * spiclk) - 1;

    writel(div, QSPI_REG_BMCLK(base));
}

static inline void qspi_hw_bit_mode_set_cs_level(u32 base, bool level)
{
    u32 reg_val = readl(QSPI_REG_BMTC(base));

    if (level)
        reg_val |= BMTC_BIT_SS_LEVEL_MSK;
    else
        reg_val &= ~BMTC_BIT_SS_LEVEL_MSK;

    writel(reg_val, QSPI_REG_BMTC(base));
}

static inline void qspi_hw_bit_mode_set_cs_pol(u32 base, bool high_active)
{
    u32 reg_val = readl(QSPI_REG_BMTC(base));

    if (high_active)
        reg_val &= ~BMTC_BIT_SPOL_MSK;
    else
        reg_val |= BMTC_BIT_SPOL_MSK;

    writel(reg_val, QSPI_REG_BMTC(base));
}

static inline u32 qspi_hw_bit_mode_get_cs_pol(u32 base)
{
    u32 val = readl(QSPI_REG_BMTC(base));
    return ((val & BMTC_BIT_SPOL_MSK) >> BMTC_BIT_SPOL_OFS);
}

static inline void qspi_hw_bit_mode_set_ss_owner(u32 base, bool soft_ctrl)
{
    u32 val = readl(QSPI_REG_BMTC(base));

    if (soft_ctrl)
        val |= BMTC_BIT_SS_OWNER_MSK;
    else
        val &= ~BMTC_BIT_SS_OWNER_MSK;

    writel(val, QSPI_REG_BMTC(base));
}

static inline bool qspi_hw_bit_mode_xfer_done(u32 base)
{
    u32 val;

    val = readl(QSPI_REG_BMTC(base));
    if (val & BMTC_BIT_XFER_EN_MSK)
        return false;
    if ((val & BMTC_BIT_XFER_COMP_MSK) == 0)
        return false;
    return true;
}

static inline bool qspi_hw_bit_mode_rxsts_clear(u32 base)
{
    u32 val;

    val = readl(QSPI_REG_BMTC(base));
    val &= ~(BMTC_BIT_RX_BIT_LEN_MSK);
    val |= BMTC_BIT_XFER_COMP_MSK;
    writel(val, QSPI_REG_BMTC(base));
    aic_udelay(1);
    val = readl(QSPI_REG_BMTC(base));
    if (val & BMTC_BIT_XFER_COMP_MSK)
        return false;
    return true;
}

static inline bool qspi_hw_bit_mode_txsts_clear(u32 base)
{
    u32 val;

    val = readl(QSPI_REG_BMTC(base));
    val &= ~(BMTC_BIT_TX_BIT_LEN_MSK);
    val |= BMTC_BIT_XFER_COMP_MSK;
    writel(val, QSPI_REG_BMTC(base));
    aic_udelay(1);
    val = readl(QSPI_REG_BMTC(base));
    if (val & BMTC_BIT_XFER_COMP_MSK)
        return false;
    return true;
}

static inline void qspi_hw_set_work_mode(u32 base, int mode)
{
    u32 val;

    val = readl(QSPI_REG_BMTC(base));
    val &= ~(BMTC_BIT_WM_MSK);
    val |= (mode & BMTC_BIT_WM_MSK);
    writel(val, QSPI_REG_BMTC(base));
}

static inline int qspi_hw_bit_mode_read(u32 base, u8 *rx_buf, u32 rx_bits_len)
{
    int dolen, i;
    u32 val, rxbits;
    u8 *p;

    val = readl(QSPI_REG_BMTC(base));
    val |= BMTC_BIT_XFER_COMP_MSK;
    writel(val, QSPI_REG_BMTC(base));

    p = rx_buf;
    dolen = (rx_bits_len + 7) / 8;
    rxbits = 0;
    if (dolen > 4) {
        dolen = 4;
        rx_bits_len = 32;
    }

    /* Configre rx length and start transfer */
    val = readl(QSPI_REG_BMTC(base));
    val |= rx_bits_len << BMTC_BIT_RX_BIT_LEN_OFS; //receive bits length
    val |= BMTC_BIT_XFER_EN_MSK;
    writel(val, QSPI_REG_BMTC(base));

    while (!qspi_hw_bit_mode_xfer_done(base))
        continue;
    while (!qspi_hw_bit_mode_rxsts_clear(base))
        continue;

    /* Read rx bits */
    rxbits = readl(QSPI_REG_BMRXD(base));
    for (i = 0; i < dolen; i++)
        p[i] = (rxbits >> (i * 8)) & 0xFF;

    return rx_bits_len;
}

static inline int qspi_hw_bit_mode_write(u32 base, const u8 *tx_buf, u32 tx_bits_len)
{
    int dolen, i;
    u32 val, txbits;
    const u8 *p;

    val = readl(QSPI_REG_BMTC(base));
    val |= BMTC_BIT_XFER_COMP_MSK;
    writel(val, QSPI_REG_BMTC(base));

    p = tx_buf;
    dolen = (tx_bits_len + 7) / 8;
    txbits = 0;
    if (dolen > 4) {
        dolen = 4;
        tx_bits_len = 32;
    }

    /* Prepare and write tx bits */
    for (i = 0; i < dolen; i++)
        txbits |= p[i] << (i * 8);
    writel(txbits, QSPI_REG_BMTXD(base));

    /* Configure tx length and start transfer */
    val = readl(QSPI_REG_BMTC(base));
    val |= tx_bits_len << BMTC_BIT_TX_BIT_LEN_OFS; //send bits length
    val |= BMTC_BIT_XFER_EN_MSK;
    writel(val, QSPI_REG_BMTC(base));

    while (!qspi_hw_bit_mode_xfer_done(base))
        continue;
    while (!qspi_hw_bit_mode_txsts_clear(base))
        continue;

    return tx_bits_len;
}

#ifdef __cplusplus
}
#endif

#endif
