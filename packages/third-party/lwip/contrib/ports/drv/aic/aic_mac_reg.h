/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_MAC_REG_H_
#define _ARTINCHIP_AIC_MAC_REG_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
  * AICMAC Registers Offset
  */

typedef struct
{
    uint32_t macconf;      /* 0x0000 MAC_CONF */
    uint32_t dma0conf;     /* 0x0004 DMA0_CONF */
    uint32_t dma1conf;     /* 0x0008 DMA1_CONF */
    uint32_t dma0intsts;   /* 0x000C DMA0_INT_STS */
    uint32_t dma0inten;    /* 0x0010 DMA0_INT_EN */
    uint32_t dma1intsts;   /* 0x0014 DMA1_INT_STS */
    uint32_t dma1inten;    /* 0x0018 DMA1_INT_EN */
    uint32_t mactxfunc;    /* 0x001C MAC_TX_FUNC */
    uint32_t macrxfunc;    /* 0x0020 MAC_RX_FUNC */
    uint32_t txdma0ctl;    /* 0x0024 TX_DMA0_CTL */
    uint32_t rxdma0ctl;    /* 0x0028 RX_DMA0_CTL */
    uint32_t txdma1ctl;    /* 0x002C TX_DMA1_CTL */
    uint32_t rxdma1ctl;    /* 0x0030 RX_DMA1_CTL */
    uint32_t flowctl;      /* 0x0034 FLOW_CTL */
    uint32_t vlantag;      /* 0x0038 VLAN_TAG */
    uint32_t vlanflt;      /* 0x003C VLAN_FLT */
    uint32_t macfrmflt;    /* 0x0040 MAC_FRM_FLT */
    uint32_t hashtblhigh;  /* 0x0044 HASH_TBL_HIGH */
    uint32_t hashtbllow;   /* 0x0048 HASH_TBL_LOW */
    uint32_t vlanhashtbl;  /* 0x004C VLAN_HASH_TBL */
    uint32_t macaddr0high; /* 0x0050 MAC_ADDR0_HIGH */
    uint32_t macaddr0low;  /* 0x0054 MAC_ADDR0_LOW */
    uint32_t macaddr1high; /* 0x0058 MAC_ADDR1_HIGH */
    uint32_t macaddr1low;  /* 0x005C MAC_ADDR1_LOW */
    uint32_t macaddr2high; /* 0x0060 MAC_ADDR2_HIGH */
    uint32_t macaddr2low;  /* 0x0064 MAC_ADDR2_LOW */
    uint32_t macaddr3high; /* 0x0068 MAC_ADDR3_HIGH */
    uint32_t macaddr3low;  /* 0x006C MAC_ADDR3_LOW */
    uint32_t macaddr4high; /* 0x0070 MAC_ADDR4_HIGH */
    uint32_t macaddr4low;  /* 0x0074 MAC_ADDR4_LOW */
    uint32_t macaddr5high; /* 0x0078 MAC_ADDR5_HIGH */
    uint32_t macaddr5low;  /* 0x007C MAC_ADDR5_LOW */
    uint32_t macaddr6high; /* 0x0080 MAC_ADDR6_HIGH */
    uint32_t macaddr6low;  /* 0x0084 MAC_ADDR6_LOW */
    uint32_t macaddr7high; /* 0x0088 MAC_ADDR7_HIGH */
    uint32_t macaddr7low;  /* 0x008C MAC_ADDR7_LOW */
    uint32_t mdioctl;      /* 0x0090 MDIO_CTL */
    uint32_t mdiodata;     /* 0x0094 MDIO_DATA */
    uint32_t RESERVED0[2];
    uint32_t debugreg; /* 0x00A0 DEBUG_REG */
    uint32_t rgmiists; /* 0x00A4 RGMII_STS */
    uint32_t RESERVED1[2];
    uint32_t txdma0descstart;  /* 0x00B0 TXDMA0_DESC_START */
    uint32_t rxdma0descstart;  /* 0x00B4 RXDMA0_DESC_START */
    uint32_t curtxdma0desc;    /* 0x00B8 CUR_TXDMA0_DESC */
    uint32_t currxdma0desc;    /* 0x00BC CUR_RXDMA0_DESC */
    uint32_t curtxdma0bufaddr; /* 0x00C0 CUR_TXDMA0_BUFADDR */
    uint32_t currxdma0bufaddr; /* 0x00C4 CUR_RXDMA0_BUFADDR */
    uint32_t RESERVED2[2];
    uint32_t txdma1descstart;  /* 0x00D0 TXDMA1_DESC_START */
    uint32_t rxdma1descstart;  /* 0x00D4 RXDMA1_DESC_START */
    uint32_t curtxdma1desc;    /* 0x00D8 CUR_TXDMA1_DESC */
    uint32_t currxdma1desc;    /* 0x00DC CUR_RXDMA1_DESC */
    uint32_t curtxdma1bufaddr; /* 0x00E0 CUR_TXDMA1_BUFADDR */
    uint32_t currxdma1bufaddr; /* 0x00E4 CUR_RXDMA1_BUFADDR */
    uint32_t dma0misfrmbufovf; /* 0x00E8 DMA0_MISFRM_BUFOVF */
    uint32_t dma1misfrmbufovf; /* 0x00EC DMA1_MISFRM_BUFOVF */
    uint32_t RESERVED3[4];
    uint32_t tmstmpctl;       /* 0x0100 TMSTMP_CTL */
    uint32_t subsecincr;      /* 0x0104 SUB_SEC_INCR */
    uint32_t tmsmpaddend;     /* 0x0108 TMSMP_ADDEND */
    uint32_t systimesec;      /* 0x010C SYS_TIME_SEC */
    uint32_t systimenanosec;  /* 0x0110 SYS_TIME_NANO_SEC */
    uint32_t updttimesec;     /* 0x0114 UPDT_TIME_SEC */
    uint32_t updttimenanosec; /* 0x0118 UPDT_TIME_NANO_SEC */
    uint32_t trgttimesec;     /* 0x011C TRGT_TIME_SEC */
    uint32_t trgttimenanosec; /* 0x0120 TRGT_TIME_NANO_SEC */
    uint32_t auxtmsmpsec;     /* 0x0124 AUX_TMSMP_SEC */
    uint32_t auxtmsmpnanosec; /* 0x0128 AUX_TMSMP_NANO_SEC */
    uint32_t systimehisec;    /* 0x012C SYS_TIME_HI_WORD_SEC */
    uint32_t tmsmpsts;        /* 0x0130 TMSMP_STS */
    uint32_t ppsctl;          /* 0x0134 PPS_CTL */
    uint32_t pps0interval;    /* 0x0138 PPS0_INTERVAL */
    uint32_t pps0width;       /* 0x013C PPS0_WIDTH */
    uint32_t avmacctl;        /* 0x0140 AV_MAC_CTL */
    uint32_t ch1slotctlsts;   /* 0x0144 CH1_SLOT_CTL_STS */
    uint32_t ch1cbsctl;       /* 0x0148 CH1_CBS_CTL */
    uint32_t ch1cbssts;       /* 0x014C CH1_CBS_STS */
    uint32_t ch1idleslopcred; /* 0x0150 CH1_IDLE_SLOP_CRED */
    uint32_t ch1sendslopcred; /* 0x0154 CH1_SEND_SLOP_CRED */
    uint32_t ch1hicred;       /* 0x0158 CH1_HI_CRED */
    uint32_t ch1locred;       /* 0x015C CH1_LO_CRED */
    uint32_t RESERVED4[0x3A7];
    uint32_t version; /* 0x0FFC VERSION */
} aicmac_reg_t;

/*----------------------------------------------------------------------------*/
/* AICMAC Registers bits definitions                                          */
/*----------------------------------------------------------------------------*/

/* 0x0000 macconf: Bit definition for MAC Config Reg0 */
#define ETH_MACCONF_LUD           ((uint32_t)0x00000100) /* 0x0000.8: Link Up or Down */
#define ETH_MACCONF_IPC           ((uint32_t)0x00000080) /* 0x0000.7: IP Checksum offload */
#define ETH_MACCONF_PRELEN_MSK    ((uint32_t)0x00000060) /* 0x0000.5-6: Preamble Length for Transmit frames */
#define ETH_MACCONF_PRELEN_7BYTES ((uint32_t)0x00000000)
#define ETH_MACCONF_PRELEN_5BYTES ((uint32_t)0x00000020)
#define ETH_MACCONF_PRELEN_3BYTES ((uint32_t)0x00000040)
#define ETH_MACCONF_DM            ((uint32_t)0x00000010) /* 0x0000.4: Duplex mode */
#define ETH_MACCONF_LM            ((uint32_t)0x00000008) /* 0x0000.3: loopback mode */
#define ETH_MACCONF_SPEED_MSK     ((uint32_t)0x00000006) /* 0x0000.1-2: Ethernet line speed */
#define ETH_MACCONF_SPEED_10M     ((uint32_t)0x00000004)
#define ETH_MACCONF_SPEED_100M    ((uint32_t)0x00000006)
#define ETH_MACCONF_SPEED_1000M   ((uint32_t)0x00000000)
#define ETH_MACCONF_SWR           ((uint32_t)0x00000001) /* 0x0000.0: Software Reset */

/* 0x001C mactxfunc: Bit definition for MAC Config Reg1 */
#define ETH_MACTXFUNC_SARC_MSK      ((uint32_t)0x00007000) /* 0x001C.12-14: Source Address Insertion or Replacement Control */
#define ETH_MACTXFUNC_SARC_INSERT0  ((uint32_t)0x00002000)
#define ETH_MACTXFUNC_SARC_REPLACE0 ((uint32_t)0x00003000)
#define ETH_MACTXFUNC_SARC_INSERT1  ((uint32_t)0x00006000)
#define ETH_MACTXFUNC_SARC_REPLACE1 ((uint32_t)0x00007000)
#define ETH_MACTXFUNC_DC            ((uint32_t)0x00000400) /* 0x001C.10: Defferal check */
#define ETH_MACTXFUNC_JD            ((uint32_t)0x00000200) /* 0x001C.9: Jabber disable */
#define ETH_MACTXFUNC_BE            ((uint32_t)0x00000100) /* 0x001C.8: Frame Burst Enable */
#define ETH_MACTXFUNC_IFG_MSK       ((uint32_t)0x000000E0) /* 0x001C.5-7: Minimum IFG between frames during transmission */
#define ETH_MACTXFUNC_IFG_96Bit     ((uint32_t)0x00000000) /* Minimum IFG between frames during transmission is 96Bit */
#define ETH_MACTXFUNC_IFG_88Bit     ((uint32_t)0x00000020) /* Minimum IFG between frames during transmission is 88Bit */
#define ETH_MACTXFUNC_IFG_80Bit     ((uint32_t)0x00000040) /* Minimum IFG between frames during transmission is 80Bit */
#define ETH_MACTXFUNC_IFG_72Bit     ((uint32_t)0x00000060) /* Minimum IFG between frames during transmission is 72Bit */
#define ETH_MACTXFUNC_IFG_64Bit     ((uint32_t)0x00000080) /* Minimum IFG between frames during transmission is 64Bit */
#define ETH_MACTXFUNC_IFG_56Bit     ((uint32_t)0x000000A0) /* Minimum IFG between frames during transmission is 56Bit */
#define ETH_MACTXFUNC_IFG_48Bit     ((uint32_t)0x000000C0) /* Minimum IFG between frames during transmission is 48Bit */
#define ETH_MACTXFUNC_IFG_40Bit     ((uint32_t)0x000000E0) /* Minimum IFG between frames during transmission is 40Bit */
#define ETH_MACTXFUNC_DCRS          ((uint32_t)0x00000010) /* 0x001C.4: Disable Carrier Sense During Transmission */
#define ETH_MACTXFUNC_DR            ((uint32_t)0x00000008) /* 0x001C.3: Disable Retry */
#define ETH_MACTXFUNC_BL_MSK        ((uint32_t)0x00000006) /* 0x001C.1-2: Back-off limit: random integer number (r) of slot time delays before rescheduling
                                                                    a transmission attempt during retries after a collision: 0 =< r <2^k */
#define ETH_MACTXFUNC_BL_10         ((uint32_t)0x00000000) /* k = min (n, 10) */
#define ETH_MACTXFUNC_BL_8          ((uint32_t)0x00000002) /* k = min (n, 8) */
#define ETH_MACTXFUNC_BL_4          ((uint32_t)0x00000004) /* k = min (n, 4) */
#define ETH_MACTXFUNC_BL_1          ((uint32_t)0x00000006) /* k = min (n, 1) */
#define ETH_MACTXFUNC_TE            ((uint32_t)0x00000001) /* 0x001C.0: Transmitter enable */

/* 0x0020 macrxfunc: Bit definition for MAC Config Reg2 */
#define ETH_MACRXFUNC_PWE     ((uint32_t)0x00400000) /* 0x0020.22: Programmable Watchdog Enable */
#define ETH_MACRXFUNC_WTO_MSK ((uint32_t)0x003FFF00) /* 0x0020.8-21: Watchdog Timeout */
#define ETH_MACRXFUNC_WD      ((uint32_t)0x00000040) /* 0x0020.6: Watchdog disable */
#define ETH_MACRXFUNC_TWOKPE  ((uint32_t)0x00000020) /* 0x0020.5: IEEE 802.3as Support for 2K Packets */
#define ETH_MACRXFUNC_JE      ((uint32_t)0x00000010) /* 0x0020.4: Jumbo Frame Enable */
#define ETH_MACRXFUNC_DO      ((uint32_t)0x00000008) /* 0x0020.3: Disable Receive Own */
#define ETH_MACRXFUNC_CST     ((uint32_t)0x00000004) /* 0x0020.2: CRC Stripping for Type Frames */
#define ETH_MACRXFUNC_APC     ((uint32_t)0x00000002) /* 0x0020.1: Automatic Pad or CRC Stripping */
#define ETH_MACRXFUNC_RE      ((uint32_t)0x00000001) /* 0x0020.0: Receiver enable */

/* 0x0040 macfrmflt: Bit definition for Ethernet MAC Frame Filter Register */
#define ETH_MACFRMFLT_RA                          ((uint32_t)0x80000000) /* Receive all */
#define ETH_MACFRMFLT_DNTU                        ((uint32_t)0x02000000) /* Drop non-TCP/UDP over IP Frames */
#define ETH_MACFRMFLT_IPFE                        ((uint32_t)0x01000000) /* Layer 3 and Layer 4 Filter Enable */
#define ETH_MACFRMFLT_VTFE                        ((uint32_t)0x80010000) /* VLAN Tag Filter Enable */
#define ETH_MACFRMFLT_HPF                         ((uint32_t)0x00000400) /* Hash or perfect filter */
#define ETH_MACFRMFLT_SAF                         ((uint32_t)0x00000200) /* Source address filter enable */
#define ETH_MACFRMFLT_SAIF                        ((uint32_t)0x00000100) /* SA inverse filtering */
#define ETH_MACFRMFLT_PCF_MSK                     ((uint32_t)0x000000C0) /* Pass control frames: 3 cases */
#define ETH_MACFRMFLT_PCF_BlockAll                ((uint32_t)0x00000040) /* MAC filters all control frames from reaching the application */
#define ETH_MACFRMFLT_PCF_ForwardAll              ((uint32_t)0x00000080) /* MAC forwards all control frames to application even if they fail the Address Filter */
#define ETH_MACFRMFLT_PCF_ForwardPassedAddrFilter ((uint32_t)0x000000C0) /* MAC forwards control frames that pass the Address Filter. */
#define ETH_MACFRMFLT_DBF                         ((uint32_t)0x00000020) /* Disable Broadcast Frames */
#define ETH_MACFRMFLT_PM                          ((uint32_t)0x00000010) /* Pass all mutlicast */
#define ETH_MACFRMFLT_DAIF                        ((uint32_t)0x00000008) /* DA Inverse filtering */
#define ETH_MACFRMFLT_HMC                         ((uint32_t)0x00000004) /* Hash multicast */
#define ETH_MACFRMFLT_HUC                         ((uint32_t)0x00000002) /* Hash unicast */
#define ETH_MACFRMFLT_PR                          ((uint32_t)0x00000001) /* Promiscuous mode */

/* 0x0044 hashtblhigh: Bit definition for Ethernet MAC Hash Table High Register */
#define ETH_HASHTBLHIGH_HTH ((uint32_t)0xFFFFFFFF) /* Hash table high */

/* 0x0048 hashtbllow: Bit definition for Ethernet MAC Hash Table Low Register */
#define ETH_HASHTBLLOW_HTL ((uint32_t)0xFFFFFFFF) /* Hash table low */

/* 0x0090 mdioctl: Bit definition for Ethernet MAC MII Address Register */
#define ETH_MDIOCTL_PA        ((uint32_t)0x0000F800) /* Physical layer address */
#define ETH_MDIOCTL_MR        ((uint32_t)0x000007C0) /* MII register in the selected PHY */
#define ETH_MDIOCTL_CR_MSK    ((uint32_t)0x0000003C) /* CR clock range: 6 cases */
#define ETH_MDIOCTL_CR_Div42  ((uint32_t)0x00000000) /* HCLK:60-100 MHz; MDC clock= HCLK/42 */
#define ETH_MDIOCTL_CR_Div62  ((uint32_t)0x00000004) /* HCLK:100-150 MHz; MDC clock= HCLK/62 */
#define ETH_MDIOCTL_CR_Div16  ((uint32_t)0x00000008) /* HCLK:20-35 MHz; MDC clock= HCLK/16 */
#define ETH_MDIOCTL_CR_Div26  ((uint32_t)0x0000000C) /* HCLK:35-60 MHz; MDC clock= HCLK/26 */
#define ETH_MDIOCTL_CR_Div102 ((uint32_t)0x00000010) /* HCLK:150-250 MHz; MDC clock= HCLK/102 */
#define ETH_MDIOCTL_CR_Div124 ((uint32_t)0x00000014) /* HCLK:250–300 MHz; MDC clock= HCLK/124 */
#define ETH_MDIOCTL_MW        ((uint32_t)0x00000002) /* MII write */
#define ETH_MDIOCTL_MB        ((uint32_t)0x00000001) /* MII busy */

/* 0x0094 mdiodata: Bit definition for Ethernet MAC MII Data Register */
#define ETH_MDIODATA_MD ((uint32_t)0x0000FFFF) /* MII data: read/write data from/to PHY */

/* 0x0034 flowctl: Bit definition for Ethernet MAC Flow Control Register */
#define ETH_FLOWCTL_PT           ((uint32_t)0xFFFF0000) /* Pause time */
#define ETH_FLOWCTL_PT_SHIFT     16
#define ETH_FLOWCTL_DZPQ         ((uint32_t)0x00000080) /* Disable Zero-Quanta Pause */
#define ETH_FLOWCTL_PLT_MSK      ((uint32_t)0x00000030) /* Pause low threshold: 4 cases */
#define ETH_FLOWCTL_PLT_Minus4   ((uint32_t)0x00000000) /* Pause time minus 4 slot times */
#define ETH_FLOWCTL_PLT_Minus28  ((uint32_t)0x00000010) /* Pause time minus 28 slot times */
#define ETH_FLOWCTL_PLT_Minus144 ((uint32_t)0x00000020) /* Pause time minus 144 slot times */
#define ETH_FLOWCTL_PLT_Minus256 ((uint32_t)0x00000030) /* Pause time minus 256 slot times */
#define ETH_FLOWCTL_UP           ((uint32_t)0x00000008) /* Unicast pause frame detect */
#define ETH_FLOWCTL_RFE          ((uint32_t)0x00000004) /* Receive flow control enable */
#define ETH_FLOWCTL_TFE          ((uint32_t)0x00000002) /* Transmit flow control enable */
#define ETH_FLOWCTL_FCBBPA       ((uint32_t)0x00000001) /* Flow control busy/backpressure activate */

/* 0x003C vlanflt: Bit definition for Ethernet MAC VLAN Tag Register */
#define ETH_VLANFLT_VTHM ((uint32_t)0x00080000) /* VLAN Tag Hash Table Match Enable */
#define ETH_VLANFLT_ESVL ((uint32_t)0x00040000) /* Enable S-VLAN */
#define ETH_VLANFLT_VTIM ((uint32_t)0x00020000) /* VLAN Tag Inverse Match Enable */
#define ETH_VLANFLT_ETV  ((uint32_t)0x00010000) /* Enable 12-Bit VLAN Tag Comparison */
#define ETH_VLANFLT_VL   ((uint32_t)0x0000FFFF) /* VLAN tag identifier (for receive frames) */

/* 0x0050 macaddr0high: Bit definition for Ethernet MAC Address0 High Register */
#define ETH_MACA0HR_MACA0H ((uint32_t)0x0000FFFF) /* MAC address0 high */

/* 0x0054 macaddr0low: Bit definition for Ethernet MAC Address0 Low Register */
#define ETH_MACA0LR_MACA0L ((uint32_t)0xFFFFFFFF) /* MAC address0 low */

/* 0x0058 macaddr1high: Bit definition for Ethernet MAC Address1 High Register */
#define ETH_MACA1HR_AE             ((uint32_t)0x80000000) /* Address enable */
#define ETH_MACA1HR_SA             ((uint32_t)0x40000000) /* Source address */
#define ETH_MACA1HR_MBC_SHIFT      24
#define ETH_MACA1HR_MBC_MMSK       ((uint32_t)0x3F000000) /* Mask byte control: bits to mask for comparison of the MAC Address bytes */
#define ETH_MACA1HR_MBC_HBits15_8  ((uint32_t)0x20000000) /* Mask MAC Address high reg bits [15:8] */
#define ETH_MACA1HR_MBC_HBits7_0   ((uint32_t)0x10000000) /* Mask MAC Address high reg bits [7:0] */
#define ETH_MACA1HR_MBC_LBits31_24 ((uint32_t)0x08000000) /* Mask MAC Address low reg bits [31:24] */
#define ETH_MACA1HR_MBC_LBits23_16 ((uint32_t)0x04000000) /* Mask MAC Address low reg bits [23:16] */
#define ETH_MACA1HR_MBC_LBits15_8  ((uint32_t)0x02000000) /* Mask MAC Address low reg bits [15:8] */
#define ETH_MACA1HR_MBC_LBits7_0   ((uint32_t)0x01000000) /* Mask MAC Address low reg bits [7:0] */
#define ETH_MACA1HR_MACA1H         ((uint32_t)0x0000FFFF) /* MAC address1 high */

/* 0x005C macaddr1low: Bit definition for Ethernet MAC Address1 Low Register */
#define ETH_MACA1LR_MACA1L ((uint32_t)0xFFFFFFFF) /* MAC address1 low */

#define ETH_MACADDR_MAX_INDEX 8

/* 0x0004 dma0conf: Bit definition for Ethernet DMA Bus Mode Register */
#define ETH_DMACONF_RIB               ((uint32_t)0x80000000) /* 0x0004.31: Rebuild INCRx Burst */
#define ETH_DMACONF_PRWG_MSK          ((uint32_t)0x18000000) /* 0x0004.27-28: Channel Priority Weights */
#define ETH_DMACONF_PRWG_PRIO1        ((uint32_t)0x00000000) /* priority weight is 1 */
#define ETH_DMACONF_PRWG_PRIO2        ((uint32_t)0x08000000) /* priority weight is 2 */
#define ETH_DMACONF_PRWG_PRIO3        ((uint32_t)0x10000000) /* priority weight is 3 */
#define ETH_DMACONF_PRWG_PRIO4        ((uint32_t)0x18000000) /* priority weight is 4 */
#define ETH_DMACONF_AAL               ((uint32_t)0x04000000) /* 0x0004.26: Address-Aligned beats */
#define ETH_DMACONF_USP               ((uint32_t)0x02000000) /* 0x0004.25:Use separate PBL */
#define ETH_DMACONF_RPBL_SHIFT        19
#define ETH_DMACONF_RPBL_MSK          ((uint32_t)0x01F80000) /* 0x0004.19-24: RxDMA PBL */
#define ETH_DMACONF_RPBL_1Beat        ((uint32_t)0x00080000) /* maximum number of beats to be transferred in one RxDMA transaction is 1 */
#define ETH_DMACONF_RPBL_2Beat        ((uint32_t)0x00100000) /* maximum number of beats to be transferred in one RxDMA transaction is 2 */
#define ETH_DMACONF_RPBL_4Beat        ((uint32_t)0x00200000) /* maximum number of beats to be transferred in one RxDMA transaction is 4 */
#define ETH_DMACONF_RPBL_8Beat        ((uint32_t)0x00400000) /* maximum number of beats to be transferred in one RxDMA transaction is 8 */
#define ETH_DMACONF_RPBL_16Beat       ((uint32_t)0x00800000) /* maximum number of beats to be transferred in one RxDMA transaction is 16 */
#define ETH_DMACONF_RPBL_32Beat       ((uint32_t)0x01000000) /* maximum number of beats to be transferred in one RxDMA transaction is 32 */
#define ETH_DMACONF_MB                ((uint32_t)0x00040000) /* 0x0004.18: Mixed Burst */
#define ETH_DMACONF_FB                ((uint32_t)0x00020000) /* 0x0004.17: Fixed Burst */
#define ETH_DMACONF_PBLx8             ((uint32_t)0x00010000) /* 0x0004.16: PBLx8 Mode */
#define ETH_DMACONF_PBL_SHIFT         10
#define ETH_DMACONF_PBL_MSK           ((uint32_t)0x0001FC00) /* 0x0004.10-15: Programmable burst length */
#define ETH_DMACONF_PBL_1Beat         ((uint32_t)0x00000400) /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 1 */
#define ETH_DMACONF_PBL_2Beat         ((uint32_t)0x00000800) /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 2 */
#define ETH_DMACONF_PBL_4Beat         ((uint32_t)0x00001000) /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 4 */
#define ETH_DMACONF_PBL_8Beat         ((uint32_t)0x00002000) /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 8 */
#define ETH_DMACONF_PBL_16Beat        ((uint32_t)0x00004000) /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 16 */
#define ETH_DMACONF_PBL_32Beat        ((uint32_t)0x00008000) /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 32 */
#define ETH_DMACONF_PBL_8xPBL_8Beat   ((uint32_t)0x00010400) /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 4 */
#define ETH_DMACONF_PBL_8xPBL_16Beat  ((uint32_t)0x00010800) /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 8 */
#define ETH_DMACONF_PBL_8xPBL_32Beat  ((uint32_t)0x00011000) /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 16 */
#define ETH_DMACONF_PBL_8xPBL_64Beat  ((uint32_t)0x00012000) /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 32 */
#define ETH_DMACONF_PBL_8xPBL_128Beat ((uint32_t)0x00014000) /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 64 */
#define ETH_DMACONF_PBL_8xPBL_256Beat ((uint32_t)0x00018000) /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 128 */
#define ETH_DMACONF_ATDS              ((uint32_t)0x00000200) /* 0x0004.9: Alternate Descriptor Size */
#define ETH_DMACONF_DSL_MSK           ((uint32_t)0x000001F0) /* 0x0004.4-8: Descriptor Skip Length */
#define ETH_DMACONF_TXPR              ((uint32_t)0x80000008) /* 0x0004.3: Transmit Priority */
#define ETH_DMACONF_PR_MSK            ((uint32_t)0x00000006) /* 0x0004.1-2: Priority Ratio */
#define ETH_DMACONF_PR_1_1            ((uint32_t)0x00000000) /* Rx Tx priority ratio is 1:1 */
#define ETH_DMACONF_PR_2_1            ((uint32_t)0x00000002) /* Rx Tx priority ratio is 2:1 */
#define ETH_DMACONF_PR_3_1            ((uint32_t)0x00000004) /* Rx Tx priority ratio is 3:1 */
#define ETH_DMACONF_PR_4_1            ((uint32_t)0x00000006) /* Rx Tx priority ratio is 4:1 */
#define ETH_DMACONF_DA                ((uint32_t)0x00000001) /* 0x0004.0: DMA arbitration scheme */

#define DEFAULT_DMA_PB 8

/* 0x00B4 rxdma0descstart: Bit definition for Ethernet DMA Receive Descriptor List Address Register */
#define ETHT_RXDMADESCSTART_STL ((uint32_t)0xFFFFFFFF) /* Start of receive list */

/* 0x00B0 txdma0descstart: Bit definition for Ethernet DMA Transmit Descriptor List Address Register */
#define ETHT_TXDMADESCSTART_STL ((uint32_t)0xFFFFFFFF) /* Start of transmit list */

/* 0x000C dma0intsts: Bit definition for Interrupt Status Register */
#define ETH_DMAINTSTS_TSIS            ((uint32_t)0x20000000) /* 0x000C.29: Timestamp Interrupt Status */
#define ETH_DMAINTSTS_RGSMIIIS        ((uint32_t)0x04000000) /* 0x000C.26: RGMII or SMII Interrupt Status */
#define ETH_DMAINTSTS_EB_MSK          ((uint32_t)0x03800000) /* Error bits status */
#define ETH_DMAINTSTS_EB_DescAccess   ((uint32_t)0x02000000) /* Error bits 0-data buffer, 1-desc. access */
#define ETH_DMAINTSTS_EB_ReadTransf   ((uint32_t)0x01000000) /* Error bits 0-write trnsf, 1-read transfr */
#define ETH_DMAINTSTS_EB_DataTransfTx ((uint32_t)0x00800000) /* Error bits 0-Rx DMA, 1-Tx DMA */
#define ETH_DMAINTSTS_TS_SHIFT        20
#define ETH_DMAINTSTS_TS_MSK          ((uint32_t)0x00700000) /* Transmit process state */
#define ETH_DMAINTSTS_TS_Stopped      ((uint32_t)0x00000000) /* Stopped - Reset or Stop Tx Command issued  */
#define ETH_DMAINTSTS_TS_Fetching     ((uint32_t)0x00100000) /* Running - fetching the Tx descriptor */
#define ETH_DMAINTSTS_TS_Waiting      ((uint32_t)0x00200000) /* Running - waiting for status */
#define ETH_DMAINTSTS_TS_Reading      ((uint32_t)0x00300000) /* Running - reading the data from host memory */
#define ETH_DMAINTSTS_TS_WriteTS      ((uint32_t)0x00400000) /* Running - TIME_STAMP write state */
#define ETH_DMAINTSTS_TS_Suspended    ((uint32_t)0x00600000) /* Suspended - Tx Descriptor unavailabe */
#define ETH_DMAINTSTS_TS_Closing      ((uint32_t)0x00700000) /* Running - Closing Transmit Descriptor */
#define ETH_DMAINTSTS_RS_SHIFT        17
#define ETH_DMAINTSTS_RS_MSK          ((uint32_t)0x000E0000) /* Receive process state */
#define ETH_DMAINTSTS_RS_Stopped      ((uint32_t)0x00000000) /* Stopped - Reset or Stop Rx Command issued */
#define ETH_DMAINTSTS_RS_Fetching     ((uint32_t)0x00020000) /* Running - fetching the Rx descriptor */
#define ETH_DMAINTSTS_RS_Waiting      ((uint32_t)0x00060000) /* Running - waiting for packet */
#define ETH_DMAINTSTS_RS_Suspended    ((uint32_t)0x00080000) /* Suspended - Rx Descriptor unavailable */
#define ETH_DMAINTSTS_RS_Closing      ((uint32_t)0x000A0000) /* Running - closing descriptor */
#define ETH_DMAINTSTS_RS_WriteTS      ((uint32_t)0x000C0000) /* TIME_STAMP write state */
#define ETH_DMAINTSTS_RS_Queuing      ((uint32_t)0x000E0000) /* Running - queuing the recieve frame into host memory */
#define ETH_DMAINTSTS_NIS             ((uint32_t)0x00010000) /* Normal interrupt summary */
#define ETH_DMAINTSTS_AIS             ((uint32_t)0x00008000) /* Abnormal interrupt summary */
#define ETH_DMAINTSTS_ERI             ((uint32_t)0x00004000) /* Early receive status */
#define ETH_DMAINTSTS_FBI             ((uint32_t)0x00002000) /* Fatal bus error status */
#define ETH_DMAINTSTS_ETI             ((uint32_t)0x00000400) /* Early transmit status */
#define ETH_DMAINTSTS_RWT             ((uint32_t)0x00000200) /* Receive watchdog timeout status */
#define ETH_DMAINTSTS_RPS             ((uint32_t)0x00000100) /* Receive process stopped status */
#define ETH_DMAINTSTS_RU              ((uint32_t)0x00000080) /* Receive buffer unavailable status */
#define ETH_DMAINTSTS_RI              ((uint32_t)0x00000040) /* Receive status */
#define ETH_DMAINTSTS_UNF             ((uint32_t)0x00000020) /* Transmit underflow status */
#define ETH_DMAINTSTS_OVF             ((uint32_t)0x00000010) /* Receive overflow status */
#define ETH_DMAINTSTS_TJT             ((uint32_t)0x00000008) /* Transmit jabber timeout status */
#define ETH_DMAINTSTS_TU              ((uint32_t)0x00000004) /* Transmit buffer unavailable status */
#define ETH_DMAINTSTS_TPS             ((uint32_t)0x00000002) /* Transmit process stopped status */
#define ETH_DMAINTSTS_TI              ((uint32_t)0x00000001) /* Transmit status */

/* 0x0010 dma0inten: Bit definition for Interrupt Mask Register */
#define ETH_DMAINTEN_TSIM     ((uint32_t)0x20000000) /* 0x0010.29: Timestamp Interrupt Mask */
#define ETH_DMAINTEN_RGSMIIIM ((uint32_t)0x04000000) /* 0x0010.26: RGMII or SMII Interrupt Mask */
#define ETH_DMAINTEN_NIE      ((uint32_t)0x00010000) /* Normal interrupt summary enable */
#define ETH_DMAINTEN_AIE      ((uint32_t)0x00008000) /* Abnormal interrupt summary enable */
#define ETH_DMAINTEN_ERE      ((uint32_t)0x00004000) /* Early receive interrupt enable */
#define ETH_DMAINTEN_FBE      ((uint32_t)0x00002000) /* Fatal bus error interrupt enable */
#define ETH_DMAINTEN_ETE      ((uint32_t)0x00000400) /* Early transmit interrupt enable */
#define ETH_DMAINTEN_RWE      ((uint32_t)0x00000200) /* Receive watchdog timeout interrupt enable */
#define ETH_DMAINTEN_RSE      ((uint32_t)0x00000100) /* Receive process stopped interrupt enable */
#define ETH_DMAINTEN_RUE      ((uint32_t)0x00000080) /* Receive buffer unavailable interrupt enable */
#define ETH_DMAINTEN_RIE      ((uint32_t)0x00000040) /* Receive interrupt enable */
#define ETH_DMAINTEN_UNE      ((uint32_t)0x00000020) /* Transmit Underflow interrupt enable */
#define ETH_DMAINTEN_OVE      ((uint32_t)0x00000010) /* Receive Overflow interrupt enable */
#define ETH_DMAINTEN_TJE      ((uint32_t)0x00000008) /* Transmit jabber timeout interrupt enable */
#define ETH_DMAINTEN_TUE      ((uint32_t)0x00000004) /* Transmit buffer unavailable interrupt enable */
#define ETH_DMAINTEN_TSE      ((uint32_t)0x00000002) /* Transmit process stopped interrupt enable */
#define ETH_DMAINTEN_TIE      ((uint32_t)0x00000001) /* Transmit interrupt enable */

/* 0x0024 txdma0ctl: Bit definition for Ethernet DMA Register 0 */
#define ETH_TXDMACTL_TPD          ((uint32_t)0x00000080) /* 0x0024.7: Transmit Poll Demand Register */
#define ETH_TXDMACTL_OSF          ((uint32_t)0x00000040) /* 0x0024.6: operate on second frame */
#define ETH_TXDMACTL_TSF          ((uint32_t)0x00000020) /* 0x0024.5: Transmit store and forward */
#define ETH_TXDMACTL_FTF          ((uint32_t)0x00000010) /* 0x0024.4: Flush transmit FIFO */
#define ETH_RXDMACTL_TTC_SHIFT    1
#define ETH_TXDMACTL_TTC_MSK      ((uint32_t)0x0000000E) /* 0x0024.1-3: Transmit threshold control */
#define ETH_TXDMACTL_TTC_64Bytes  ((uint32_t)0x00000000) /* threshold level of the MTL Transmit FIFO is 64 Bytes */
#define ETH_TXDMACTL_TTC_128Bytes ((uint32_t)0x00000002) /* threshold level of the MTL Transmit FIFO is 128 Bytes */
#define ETH_TXDMACTL_TTC_192Bytes ((uint32_t)0x00000004) /* threshold level of the MTL Transmit FIFO is 192 Bytes */
#define ETH_TXDMACTL_TTC_256Bytes ((uint32_t)0x00000006) /* threshold level of the MTL Transmit FIFO is 256 Bytes */
#define ETH_TXDMACTL_TTC_40Bytes  ((uint32_t)0x00000008) /* threshold level of the MTL Transmit FIFO is 40 Bytes */
#define ETH_TXDMACTL_TTC_32Bytes  ((uint32_t)0x0000000A) /* threshold level of the MTL Transmit FIFO is 32 Bytes */
#define ETH_TXDMACTL_TTC_24Bytes  ((uint32_t)0x0000000C) /* threshold level of the MTL Transmit FIFO is 24 Bytes */
#define ETH_TXDMACTL_TTC_16Bytes  ((uint32_t)0x0000000E) /* threshold level of the MTL Transmit FIFO is 16 Bytes */
#define ETH_TXDMACTL_ST           ((uint32_t)0x00000001) /* 0x0024.0: Start/stop transmission command */

/* 0x0028 rxdma0ctl: Bit definition for Ethernet DMA Register 1 */
#define ETH_RXDMACTL_RPD          ((uint32_t)0x00000200) /* 0x0028.9: Receive Poll Demand */
#define ETH_RXDMACTL_DT           ((uint32_t)0x00000100) /* 0x0028.8: Disable Dropping of TCP/IP checksum error frames */
#define ETH_RXDMACTL_DFF          ((uint32_t)0x00000080) /* 0x0028.7: Disable flushing of received frames */
#define ETH_RXDMACTL_FEF          ((uint32_t)0x00000040) /* 0x0028.6: Forward error frames */
#define ETH_RXDMACTL_FUF          ((uint32_t)0x00000020) /* 0x0028.5: Forward undersized good frames */
#define ETH_RXDMACTL_RSF          ((uint32_t)0x00000008) /* 0x0028.3: Receive store and forward */
#define ETH_RXDMACTL_RTC_SHIFT    1
#define ETH_RXDMACTL_RTC_MSK      ((uint32_t)0x00000006) /* 0x0028.1-2: receive threshold control */
#define ETH_RXDMACTL_RTC_64Bytes  ((uint32_t)0x00000000) /* threshold level of the MTL Receive FIFO is 64 Bytes */
#define ETH_RXDMACTL_RTC_32Bytes  ((uint32_t)0x00000002) /* threshold level of the MTL Receive FIFO is 32 Bytes */
#define ETH_RXDMACTL_RTC_96Bytes  ((uint32_t)0x00000004) /* threshold level of the MTL Receive FIFO is 96 Bytes */
#define ETH_RXDMACTL_RTC_128Bytes ((uint32_t)0x00000006) /* threshold level of the MTL Receive FIFO is 128 Bytes */
#define ETH_RXDMACTL_SR           ((uint32_t)0x00000001) /* 0x0028.0: Start/stop receive */

/* 0x00E8 dma0misfrmbufovf: Bit definition for Ethernet DMA Missed Frame and Buffer Overflow Counter Register */
#define ETH_DMAMFBOCR_OVFCNTOVF     ((uint32_t)0x10000000) /* Overflow bit for FIFO overflow counter */
#define ETH_DMAMFBOCR_OVFFRMCNT_MSK ((uint32_t)0x0FFE0000) /* Number of frames missed by the application */
#define ETH_DMAMFBOCR_MISCNTOVF     ((uint32_t)0x00010000) /* Overflow bit for missed frame counter */
#define ETH_DMAMFBOCR_MISFRMCNT_MSK ((uint32_t)0x0000FFFF) /* Number of frames missed by the controller */

/*0x00B8 curtxdma0desc:  Bit definition for Ethernet DMA Current Host Transmit Descriptor Register */
#define ETH_DMACHTDR_HTDAP ((uint32_t)0xFFFFFFFF) /* Host transmit descriptor address pointer */

/* 0x00BC currxdma0desc: Bit definition for Ethernet DMA Current Host Receive Descriptor Register */
#define ETH_DMACHRDR_HRDAP ((uint32_t)0xFFFFFFFF) /* Host receive descriptor address pointer */

/* 0x00C0 curtxdma0bufaddr: Bit definition for Ethernet DMA Current Host Transmit Buffer Address Register */
#define ETH_DMACHTBAR_HTBAP ((uint32_t)0xFFFFFFFF) /* Host transmit buffer address pointer */

/* 0x00C4 currxdma0bufaddr: Bit definition for Ethernet DMA Current Host Receive Buffer Address Register */
#define ETH_DMACHRBAR_HRBAP ((uint32_t)0xFFFFFFFF) /* Host receive buffer address pointer */

/*----------------------------------------------------------------------------*/
/* AICMAC DMA descriptors registers bits definition                           */
/*----------------------------------------------------------------------------*/

/**
  * AICMAC DMA Descriptors data structure definition
  */
typedef struct {
    uint32_t control;        /* Status */
    uint32_t buff_size;      /* Control and Buffer1, Buffer2 lengths */
    uint32_t buff1_addr;     /* Buffer1 address pointer */
    uint32_t buff2_addr;     /* Buffer2 or next descriptor address pointer */
    uint32_t ext_stat;       /* Extended status for PTP receive descriptor */
    uint32_t reserved1;      /* Reserved */
    uint32_t timestamp_low;  /* Time Stamp Low value */
    uint32_t timestamp_high; /* Time Stamp High value */
#if ((CACHE_LINE_SIZE == 64) && !defined(CONFIG_MAC_USE_UNCACHE_BUF))
    uint32_t reserved2[8];
#endif
} aicmac_dma_desc_t;

/**
  * DMA Tx Desciptor
  */

/* Bit definition of TDES0 register: DMA Tx descriptor status register */
#define ETH_DMATxDesc_OWN                    ((uint32_t)0x80000000) /* OWN bit: descriptor is owned by DMA engine */
#define ETH_DMATxDesc_IC                     ((uint32_t)0x40000000) /* Interrupt on Completion */
#define ETH_DMATxDesc_LS                     ((uint32_t)0x20000000) /* Last Segment */
#define ETH_DMATxDesc_FS                     ((uint32_t)0x10000000) /* First Segment */
#define ETH_DMATxDesc_DC                     ((uint32_t)0x08000000) /* Disable CRC */
#define ETH_DMATxDesc_DP                     ((uint32_t)0x04000000) /* Disable Padding */
#define ETH_DMATxDesc_TTSE                   ((uint32_t)0x02000000) /* Transmit Time Stamp Enable */
#define ETH_DMATxDesc_CRCR                   ((uint32_t)0x01000000) /* CRC Replacement Control */
#define ETH_DMATxDesc_CIC                    ((uint32_t)0x00C00000) /* Checksum Insertion Control: 4 cases */
#define ETH_DMATxDesc_CIC_ByPass             ((uint32_t)0x00000000) /* Do Nothing: Checksum Engine is bypassed */
#define ETH_DMATxDesc_CIC_IPV4Header         ((uint32_t)0x00400000) /* IPV4 header Checksum Insertion */
#define ETH_DMATxDesc_CIC_TCPUDPICMP_Segment ((uint32_t)0x00800000) /* TCP/UDP/ICMP Checksum Insertion calculated over segment only */
#define ETH_DMATxDesc_CIC_TCPUDPICMP_Full    ((uint32_t)0x00C00000) /* TCP/UDP/ICMP Checksum Insertion fully calculated */
#define ETH_DMATxDesc_TER                    ((uint32_t)0x00200000) /* Transmit End of Ring */
#define ETH_DMATxDesc_TCH                    ((uint32_t)0x00100000) /* Second Address Chained */
#define ETH_DMATxDesc_VLIC                   ((uint32_t)0x000C0000) /* VLAN Insertion Control: 4 cases */
#define ETH_DMATxDesc_VLIC_ByPass            ((uint32_t)0x00000000) /* Do not add a VLAN tag */
#define ETH_DMATxDesc_VLIC_RmVLAN            ((uint32_t)0x00040000) /* Remove the VLAN tag from the frames before transmission */
#define ETH_DMATxDesc_VLIC_InsertVLAN        ((uint32_t)0x00080000) /* Insert a VLAN tag with the tag value programmed in Register */
#define ETH_DMATxDesc_VLIC_ReplaceVLAN       ((uint32_t)0x000C0000) /* Replace the VLAN tag in frames with the Tag value programmed in Register */
#define ETH_DMATxDesc_TTSS                   ((uint32_t)0x00020000) /* Tx Time Stamp Status */
#define ETH_DMATxDesc_IHE                    ((uint32_t)0x00010000) /* IP Header Error */
#define ETH_DMATxDesc_ES                     ((uint32_t)0x00008000) /* Error summary: OR of the following bits: UE || ED || EC || LCO || NC || LCA || FF || JT */
#define ETH_DMATxDesc_JT                     ((uint32_t)0x00004000) /* Jabber Timeout */
#define ETH_DMATxDesc_FF                     ((uint32_t)0x00002000) /* Frame Flushed: DMA/MTL flushed the frame due to SW flush */
#define ETH_DMATxDesc_PCE                    ((uint32_t)0x00001000) /* Payload Checksum Error */
#define ETH_DMATxDesc_LCA                    ((uint32_t)0x00000800) /* Loss of Carrier: carrier lost during transmission */
#define ETH_DMATxDesc_NC                     ((uint32_t)0x00000400) /* No Carrier: no carrier signal from the transceiver */
#define ETH_DMATxDesc_LCO                    ((uint32_t)0x00000200) /* Late Collision: transmission aborted due to collision */
#define ETH_DMATxDesc_EC                     ((uint32_t)0x00000100) /* Excessive Collision: transmission aborted after 16 collisions */
#define ETH_DMATxDesc_VF                     ((uint32_t)0x00000080) /* VLAN Frame */
#define ETH_DMATxDesc_CC                     ((uint32_t)0x00000078) /* Collision Count */
#define ETH_DMATxDesc_ED                     ((uint32_t)0x00000004) /* Excessive Deferral */
#define ETH_DMATxDesc_UF                     ((uint32_t)0x00000002) /* Underflow Error: late data arrival from the memory */
#define ETH_DMATxDesc_DB                     ((uint32_t)0x00000001) /* Deferred Bit */

/* Bit definition of TDES1 register */
#define ETH_DMATxDesc_SAIC           ((uint32_t)0xE0000000) /* SA Insertion Control */
#define ETH_DMATxDesc_SAIC_MACADDR0  ((uint32_t)0x00000000) /* SA use MAC Address Register 0 */
#define ETH_DMATxDesc_SAIC_MACADDR1  ((uint32_t)0x80000000) /* SA use MAC Address Register 1 */
#define ETH_DMATxDesc_SAIC_ByPass    ((uint32_t)0x00000000) /* Do not include the source address */
#define ETH_DMATxDesc_SAIC_InsertSA  ((uint32_t)0x20000000) /* Include or insert the source address */
#define ETH_DMATxDesc_SAIC_ReplaceSA ((uint32_t)0x40000000) /* Replace the source address */
#define ETH_DMATxDesc_SAIC_Reserved  ((uint32_t)0x60000000) /* Reserved */
#define ETH_DMATxDesc_TBS2           ((uint32_t)0x1FFF0000) /* Transmit Buffer2 Size */
#define ETH_DMATxDesc_TBS1           ((uint32_t)0x00001FFF) /* Transmit Buffer1 Size */

/* Bit definition of TDES2 register */
#define ETH_DMATxDesc_B1AP ((uint32_t)0xFFFFFFFF) /* Buffer1 Address Pointer */

/* Bit definition of TDES3 register */
#define ETH_DMATxDesc_B2AP ((uint32_t)0xFFFFFFFF) /* Buffer2 Address Pointer */

/* Bit definition of TDES6 register */
#define ETH_DMAPTPTxDesc_TTSL ((uint32_t)0xFFFFFFFF) /* Transmit Time Stamp Low */

/* Bit definition of TDES7 register */
#define ETH_DMAPTPTxDesc_TTSH ((uint32_t)0xFFFFFFFF) /* Transmit Time Stamp High */

/**
  * DMA Rx Desciptor
  */

/* Bit definition of RDES0 register: DMA Rx descriptor status register */
#define ETH_DMARxDesc_OWN     ((uint32_t)0x80000000) /* OWN bit: descriptor is owned by DMA engine  */
#define ETH_DMARxDesc_AFM     ((uint32_t)0x40000000) /* DA Filter Fail for the rx frame  */
#define ETH_DMARxDesc_FL      ((uint32_t)0x3FFF0000) /* Receive descriptor frame length  */
#define ETH_DMARxDesc_FL_SHIFT 16
#define ETH_DMARxDesc_ES      ((uint32_t)0x00008000) /* Error summary: OR of the following bits: DE || OE || IPC || LC || RWT || RE || CE */
#define ETH_DMARxDesc_DE      ((uint32_t)0x00004000) /* Descriptor error: no more descriptors for receive frame  */
#define ETH_DMARxDesc_SAF     ((uint32_t)0x00002000) /* SA Filter Fail for the received frame */
#define ETH_DMARxDesc_LE      ((uint32_t)0x00001000) /* Frame size not matching with length field */
#define ETH_DMARxDesc_OE      ((uint32_t)0x00000800) /* Overflow Error: Frame was damaged due to buffer overflow */
#define ETH_DMARxDesc_VLAN    ((uint32_t)0x00000400) /* VLAN Tag: received frame is a VLAN frame */
#define ETH_DMARxDesc_FS      ((uint32_t)0x00000200) /* First descriptor of the frame  */
#define ETH_DMARxDesc_LS      ((uint32_t)0x00000100) /* Last descriptor of the frame  */
#define ETH_DMARxDesc_IPV4HCE ((uint32_t)0x00000080) /* IPC Checksum Error: Rx Ipv4 header checksum error   */
#define ETH_DMARxDesc_LC      ((uint32_t)0x00000040) /* Late collision occurred during reception   */
#define ETH_DMARxDesc_FT      ((uint32_t)0x00000020) /* Frame type - Ethernet, otherwise 802.3    */
#define ETH_DMARxDesc_RWT     ((uint32_t)0x00000010) /* Receive Watchdog Timeout: watchdog timer expired during reception    */
#define ETH_DMARxDesc_RE      ((uint32_t)0x00000008) /* Receive error: error reported by MII interface  */
#define ETH_DMARxDesc_DBE     ((uint32_t)0x00000004) /* Dribble bit error: frame contains non int multiple of 8 bits  */
#define ETH_DMARxDesc_CE      ((uint32_t)0x00000002) /* CRC error */
#define ETH_DMARxDesc_MAMPCE  ((uint32_t)0x00000001) /* Rx MAC Address/Payload Checksum Error: Rx MAC address matched/ Rx Payload Checksum Error */

/* Bit definition of RDES1 register */
#define ETH_DMARxDesc_DIC  ((uint32_t)0x80000000) /* Disable Interrupt on Completion */
#define ETH_DMARxDesc_RBS2 ((uint32_t)0x1FFF0000) /* Receive Buffer2 Size */
#define ETH_DMARxDesc_RER  ((uint32_t)0x00008000) /* Receive End of Ring */
#define ETH_DMARxDesc_RCH  ((uint32_t)0x00004000) /* Second Address Chained */
#define ETH_DMARxDesc_RBS1 ((uint32_t)0x00001FFF) /* Receive Buffer1 Size */

/* Bit definition of RDES2 register */
#define ETH_DMARxDesc_B1AP ((uint32_t)0xFFFFFFFF) /* Buffer1 Address Pointer */

/* Bit definition of RDES3 register */
#define ETH_DMARxDesc_B2AP ((uint32_t)0xFFFFFFFF) /* Buffer2 Address Pointer */

/* Bit definition of RDES4 register */
#define ETH_DMAPTPRxDesc_PTPV                            ((uint32_t)0x00002000) /* PTP Version */
#define ETH_DMAPTPRxDesc_PTPFT                           ((uint32_t)0x00001000) /* PTP Frame Type */
#define ETH_DMAPTPRxDesc_PTPMT                           ((uint32_t)0x00000F00) /* PTP Message Type */
#define ETH_DMAPTPRxDesc_PTPMT_Sync                      ((uint32_t)0x00000100) /* SYNC message (all clock types) */
#define ETH_DMAPTPRxDesc_PTPMT_FollowUp                  ((uint32_t)0x00000200) /* FollowUp message (all clock types) */
#define ETH_DMAPTPRxDesc_PTPMT_DelayReq                  ((uint32_t)0x00000300) /* DelayReq message (all clock types) */
#define ETH_DMAPTPRxDesc_PTPMT_DelayResp                 ((uint32_t)0x00000400) /* DelayResp message (all clock types) */
#define ETH_DMAPTPRxDesc_PTPMT_PdelayReq_Announce        ((uint32_t)0x00000500) /* PdelayReq message (peer-to-peer transparent clock) or Announce message (Ordinary or Boundary clock) */
#define ETH_DMAPTPRxDesc_PTPMT_PdelayResp_Manag          ((uint32_t)0x00000600) /* PdelayResp message (peer-to-peer transparent clock) or Management message (Ordinary or Boundary clock)  */
#define ETH_DMAPTPRxDesc_PTPMT_PdelayRespFollowUp_Signal ((uint32_t)0x00000700) /* PdelayRespFollowUp message (peer-to-peer transparent clock) or Signaling message (Ordinary or Boundary clock) */
#define ETH_DMAPTPRxDesc_IPV6PR                          ((uint32_t)0x00000080) /* IPv6 Packet Received */
#define ETH_DMAPTPRxDesc_IPV4PR                          ((uint32_t)0x00000040) /* IPv4 Packet Received */
#define ETH_DMAPTPRxDesc_IPCB                            ((uint32_t)0x00000020) /* IP Checksum Bypassed */
#define ETH_DMAPTPRxDesc_IPPE                            ((uint32_t)0x00000010) /* IP Payload Error */
#define ETH_DMAPTPRxDesc_IPHE                            ((uint32_t)0x00000008) /* IP Header Error */
#define ETH_DMAPTPRxDesc_IPPT                            ((uint32_t)0x00000007) /* IP Payload Type */
#define ETH_DMAPTPRxDesc_IPPT_UDP                        ((uint32_t)0x00000001) /* UDP payload encapsulated in the IP datagram */
#define ETH_DMAPTPRxDesc_IPPT_TCP                        ((uint32_t)0x00000002) /* TCP payload encapsulated in the IP datagram */
#define ETH_DMAPTPRxDesc_IPPT_ICMP                       ((uint32_t)0x00000003) /* ICMP payload encapsulated in the IP datagram */

/* Bit definition of RDES6 register */
#define ETH_DMAPTPRxDesc_RTSL ((uint32_t)0xFFFFFFFF) /* Receive Time Stamp Low */

/* Bit definition of RDES7 register */
#define ETH_DMAPTPRxDesc_RTSH ((uint32_t)0xFFFFFFFF) /* Receive Time Stamp High */

#ifdef __cplusplus
}
#endif

#endif
