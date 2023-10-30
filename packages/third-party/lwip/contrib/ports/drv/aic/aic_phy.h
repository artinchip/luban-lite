/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_PHY_H_
#define _ARTINCHIP_AIC_PHY_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u64 supported[1];
    u64 advertising[1];
    u64 lp_advertising[1];

    int speed;
    unsigned autoneg            : 1;
    unsigned duplex             : 1;
    unsigned pause              : 1;
    unsigned asym_pause         : 1;
    unsigned link               : 1;
    unsigned autoneg_complete   : 1;
    unsigned is_gigabit_capable : 1;
} aic_phy_device_t;

struct phy_setting {
    u32 speed;
    u8 duplex;
    u8 bit;
};

/* Link mode bit indices */
enum ethtool_link_mode_bit_indices {
    ETHTOOL_LINK_MODE_10baseT_Half_BIT = 0,
    ETHTOOL_LINK_MODE_10baseT_Full_BIT = 1,
    ETHTOOL_LINK_MODE_100baseT_Half_BIT = 2,
    ETHTOOL_LINK_MODE_100baseT_Full_BIT = 3,
    ETHTOOL_LINK_MODE_1000baseT_Half_BIT = 4,
    ETHTOOL_LINK_MODE_1000baseT_Full_BIT = 5,
    ETHTOOL_LINK_MODE_Autoneg_BIT = 6,
    ETHTOOL_LINK_MODE_TP_BIT = 7,
    ETHTOOL_LINK_MODE_AUI_BIT = 8,
    ETHTOOL_LINK_MODE_MII_BIT = 9,
    ETHTOOL_LINK_MODE_FIBRE_BIT = 10,
    ETHTOOL_LINK_MODE_BNC_BIT = 11,
    ETHTOOL_LINK_MODE_10000baseT_Full_BIT = 12,
    ETHTOOL_LINK_MODE_Pause_BIT = 13,
    ETHTOOL_LINK_MODE_Asym_Pause_BIT = 14,
    ETHTOOL_LINK_MODE_2500baseX_Full_BIT = 15,
    ETHTOOL_LINK_MODE_Backplane_BIT = 16,
    ETHTOOL_LINK_MODE_1000baseKX_Full_BIT = 17,
    ETHTOOL_LINK_MODE_10000baseKX4_Full_BIT = 18,
    ETHTOOL_LINK_MODE_10000baseKR_Full_BIT = 19,
    ETHTOOL_LINK_MODE_10000baseR_FEC_BIT = 20,
    ETHTOOL_LINK_MODE_20000baseMLD2_Full_BIT = 21,
    ETHTOOL_LINK_MODE_20000baseKR2_Full_BIT = 22,
    ETHTOOL_LINK_MODE_40000baseKR4_Full_BIT = 23,
    ETHTOOL_LINK_MODE_40000baseCR4_Full_BIT = 24,
    ETHTOOL_LINK_MODE_40000baseSR4_Full_BIT = 25,
    ETHTOOL_LINK_MODE_40000baseLR4_Full_BIT = 26,
    ETHTOOL_LINK_MODE_56000baseKR4_Full_BIT = 27,
    ETHTOOL_LINK_MODE_56000baseCR4_Full_BIT = 28,
    ETHTOOL_LINK_MODE_56000baseSR4_Full_BIT = 29,
    ETHTOOL_LINK_MODE_56000baseLR4_Full_BIT = 30,
    ETHTOOL_LINK_MODE_25000baseCR_Full_BIT = 31,

    /* Last allowed bit for __ETHTOOL_LINK_MODE_LEGACY_MASK is bit
	 * 31. Please do NOT define any SUPPORTED_* or ADVERTISED_*
	 * macro for bits > 31. The only way to use indices > 31 is to
	 * use the new ETHTOOL_GLINKSETTINGS/ETHTOOL_SLINKSETTINGS API.
	 */

    ETHTOOL_LINK_MODE_25000baseKR_Full_BIT = 32,
    ETHTOOL_LINK_MODE_25000baseSR_Full_BIT = 33,
    ETHTOOL_LINK_MODE_50000baseCR2_Full_BIT = 34,
    ETHTOOL_LINK_MODE_50000baseKR2_Full_BIT = 35,
    ETHTOOL_LINK_MODE_100000baseKR4_Full_BIT = 36,
    ETHTOOL_LINK_MODE_100000baseSR4_Full_BIT = 37,
    ETHTOOL_LINK_MODE_100000baseCR4_Full_BIT = 38,
    ETHTOOL_LINK_MODE_100000baseLR4_ER4_Full_BIT = 39,
    ETHTOOL_LINK_MODE_50000baseSR2_Full_BIT = 40,
    ETHTOOL_LINK_MODE_1000baseX_Full_BIT = 41,
    ETHTOOL_LINK_MODE_10000baseCR_Full_BIT = 42,
    ETHTOOL_LINK_MODE_10000baseSR_Full_BIT = 43,
    ETHTOOL_LINK_MODE_10000baseLR_Full_BIT = 44,
    ETHTOOL_LINK_MODE_10000baseLRM_Full_BIT = 45,
    ETHTOOL_LINK_MODE_10000baseER_Full_BIT = 46,
    ETHTOOL_LINK_MODE_2500baseT_Full_BIT = 47,
    ETHTOOL_LINK_MODE_5000baseT_Full_BIT = 48,
#if 1
    ETHTOOL_LINK_MODE_1000baseT1_Full_BIT = 49,
    ETHTOOL_LINK_MODE_100baseT1_Full_BIT = 50,
    ETHTOOL_LINK_MODE_100baseFX_Half_BIT = 51,
    ETHTOOL_LINK_MODE_100baseFX_Full_BIT = 52,
#else
    ETHTOOL_LINK_MODE_FEC_NONE_BIT = 49,
    ETHTOOL_LINK_MODE_FEC_RS_BIT = 50,
    ETHTOOL_LINK_MODE_FEC_BASER_BIT = 51,
    ETHTOOL_LINK_MODE_50000baseKR_Full_BIT = 52,
    ETHTOOL_LINK_MODE_50000baseSR_Full_BIT = 53,
    ETHTOOL_LINK_MODE_50000baseCR_Full_BIT = 54,
    ETHTOOL_LINK_MODE_50000baseLR_ER_FR_Full_BIT = 55,
    ETHTOOL_LINK_MODE_50000baseDR_Full_BIT = 56,
    ETHTOOL_LINK_MODE_100000baseKR2_Full_BIT = 57,
    ETHTOOL_LINK_MODE_100000baseSR2_Full_BIT = 58,
    ETHTOOL_LINK_MODE_100000baseCR2_Full_BIT = 59,
    ETHTOOL_LINK_MODE_100000baseLR2_ER2_FR2_Full_BIT = 60,
    ETHTOOL_LINK_MODE_100000baseDR2_Full_BIT = 61,
    ETHTOOL_LINK_MODE_200000baseKR4_Full_BIT = 62,
    ETHTOOL_LINK_MODE_200000baseSR4_Full_BIT = 63,
    ETHTOOL_LINK_MODE_200000baseLR4_ER4_FR4_Full_BIT = 64,
    ETHTOOL_LINK_MODE_200000baseDR4_Full_BIT = 65,
    ETHTOOL_LINK_MODE_200000baseCR4_Full_BIT = 66,
    ETHTOOL_LINK_MODE_100baseT1_Full_BIT = 67,
    ETHTOOL_LINK_MODE_1000baseT1_Full_BIT = 68,
    ETHTOOL_LINK_MODE_400000baseKR8_Full_BIT = 69,
    ETHTOOL_LINK_MODE_400000baseSR8_Full_BIT = 70,
    ETHTOOL_LINK_MODE_400000baseLR8_ER8_FR8_Full_BIT = 71,
    ETHTOOL_LINK_MODE_400000baseDR8_Full_BIT = 72,
    ETHTOOL_LINK_MODE_400000baseCR8_Full_BIT = 73,
    ETHTOOL_LINK_MODE_FEC_LLRS_BIT = 74,
    ETHTOOL_LINK_MODE_100000baseKR_Full_BIT = 75,
    ETHTOOL_LINK_MODE_100000baseSR_Full_BIT = 76,
    ETHTOOL_LINK_MODE_100000baseLR_ER_FR_Full_BIT = 77,
    ETHTOOL_LINK_MODE_100000baseCR_Full_BIT = 78,
    ETHTOOL_LINK_MODE_100000baseDR_Full_BIT = 79,
    ETHTOOL_LINK_MODE_200000baseKR2_Full_BIT = 80,
    ETHTOOL_LINK_MODE_200000baseSR2_Full_BIT = 81,
    ETHTOOL_LINK_MODE_200000baseLR2_ER2_FR2_Full_BIT = 82,
    ETHTOOL_LINK_MODE_200000baseDR2_Full_BIT = 83,
    ETHTOOL_LINK_MODE_200000baseCR2_Full_BIT = 84,
    ETHTOOL_LINK_MODE_400000baseKR4_Full_BIT = 85,
    ETHTOOL_LINK_MODE_400000baseSR4_Full_BIT = 86,
    ETHTOOL_LINK_MODE_400000baseLR4_ER4_FR4_Full_BIT = 87,
    ETHTOOL_LINK_MODE_400000baseDR4_Full_BIT = 88,
    ETHTOOL_LINK_MODE_400000baseCR4_Full_BIT = 89,
    ETHTOOL_LINK_MODE_100baseFX_Half_BIT = 90,
    ETHTOOL_LINK_MODE_100baseFX_Full_BIT = 91,
#endif
    /* must be last entry */
    __ETHTOOL_LINK_MODE_MASK_NBITS
};

/* Phy speed */
#define SPEED_10      10
#define SPEED_100     100
#define SPEED_1000    1000
#define SPEED_2500    2500
#define SPEED_5000    5000
#define SPEED_10000   10000
#define SPEED_14000   14000
#define SPEED_20000   20000
#define SPEED_25000   25000
#define SPEED_40000   40000
#define SPEED_50000   50000
#define SPEED_56000   56000
#define SPEED_100000  100000
#define SPEED_200000  200000
#define SPEED_400000  400000
#define SPEED_UNKNOWN -1

/* Duplex, half or full. */
#define DUPLEX_HALF    0x00
#define DUPLEX_FULL    0x01
#define DUPLEX_UNKNOWN 0xff

/* Generic MII registers. */
#define MII_BMCR        0x00 /* Basic mode control register */
#define MII_BMSR        0x01 /* Basic mode status register  */
#define MII_PHYSID1     0x02 /* PHYS ID 1                   */
#define MII_PHYSID2     0x03 /* PHYS ID 2                   */
#define MII_ADVERTISE   0x04 /* Advertisement control reg   */
#define MII_LPA         0x05 /* Link partner ability reg    */
#define MII_EXPANSION   0x06 /* Expansion register          */
#define MII_CTRL1000    0x09 /* 1000BASE-T control          */
#define MII_STAT1000    0x0a /* 1000BASE-T status           */
#define MII_MMD_CTRL    0x0d /* MMD Access Control Register */
#define MII_MMD_DATA    0x0e /* MMD Access Data Register */
#define MII_ESTATUS     0x0f /* Extended Status             */
#define MII_DCOUNTER    0x12 /* Disconnect counter          */
#define MII_FCSCOUNTER  0x13 /* False carrier counter       */
#define MII_NWAYTEST    0x14 /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER 0x15 /* Receive error counter       */
#define MII_SREVISION   0x16 /* Silicon revision            */
#define MII_RESV1       0x17 /* Reserved...                 */
#define MII_LBRERROR    0x18 /* Lpback, rx, bypass error    */
#define MII_PHYADDR     0x19 /* PHY address                 */
#define MII_RESV2       0x1a /* Reserved...                 */
#define MII_TPISTATUS   0x1b /* TPI status for 10mbps       */
#define MII_NCONFIG     0x1c /* Network interface config    */

/* Basic mode control register. */
#define BMCR_RESV      0x003f /* Unused...                   */
#define BMCR_SPEED1000 0x0040 /* MSB of Speed (1000)         */
#define BMCR_CTST      0x0080 /* Collision test              */
#define BMCR_FULLDPLX  0x0100 /* Full duplex                 */
#define BMCR_ANRESTART 0x0200 /* Auto negotiation restart    */
#define BMCR_ISOLATE   0x0400 /* Isolate data paths from MII */
#define BMCR_PDOWN     0x0800 /* Enable low power state      */
#define BMCR_ANENABLE  0x1000 /* Enable auto negotiation     */
#define BMCR_SPEED100  0x2000 /* Select 100Mbps              */
#define BMCR_LOOPBACK  0x4000 /* TXD loopback bits           */
#define BMCR_RESET     0x8000 /* Reset to default state      */
#define BMCR_SPEED10   0x0000 /* Select 10Mbps               */

/* Basic mode status register. */
#define BMSR_ERCAP        0x0001 /* Ext-reg capability          */
#define BMSR_JCD          0x0002 /* Jabber detected             */
#define BMSR_LSTATUS      0x0004 /* Link status                 */
#define BMSR_ANEGCAPABLE  0x0008 /* Able to do auto-negotiation */
#define BMSR_RFAULT       0x0010 /* Remote fault detected       */
#define BMSR_ANEGCOMPLETE 0x0020 /* Auto-negotiation complete   */
#define BMSR_RESV         0x00c0 /* Unused...                   */
#define BMSR_ESTATEN      0x0100 /* Extended Status in R15      */
#define BMSR_100HALF2     0x0200 /* Can do 100BASE-T2 HDX       */
#define BMSR_100FULL2     0x0400 /* Can do 100BASE-T2 FDX       */
#define BMSR_10HALF       0x0800 /* Can do 10mbps, half-duplex  */
#define BMSR_10FULL       0x1000 /* Can do 10mbps, full-duplex  */
#define BMSR_100HALF      0x2000 /* Can do 100mbps, half-duplex */
#define BMSR_100FULL      0x4000 /* Can do 100mbps, full-duplex */
#define BMSR_100BASE4     0x8000 /* Can do 100mbps, 4k packets  */

/* Advertisement control register. */
#define ADVERTISE_SLCT          0x001f /* Selector bits               */
#define ADVERTISE_CSMA          0x0001 /* Only selector supported     */
#define ADVERTISE_10HALF        0x0020 /* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL     0x0020 /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL        0x0040 /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF     0x0040 /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF       0x0080 /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE    0x0080 /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL       0x0100 /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM 0x0100 /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4      0x0200 /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP     0x0400 /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800 /* Try for asymetric pause     */
#define ADVERTISE_RESV          0x1000 /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000 /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000 /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000 /* Next page bit               */

#define ADVERTISE_FULL (ADVERTISE_100FULL | ADVERTISE_10FULL | ADVERTISE_CSMA)
#define ADVERTISE_ALL                                          \
    (ADVERTISE_10HALF | ADVERTISE_10FULL | ADVERTISE_100HALF | \
     ADVERTISE_100FULL)

/* Link partner ability register. */
#define LPA_SLCT            0x001f /* Same as advertise selector  */
#define LPA_10HALF          0x0020 /* Can do 10mbps half-duplex   */
#define LPA_1000XFULL       0x0020 /* Can do 1000BASE-X full-duplex */
#define LPA_10FULL          0x0040 /* Can do 10mbps full-duplex   */
#define LPA_1000XHALF       0x0040 /* Can do 1000BASE-X half-duplex */
#define LPA_100HALF         0x0080 /* Can do 100mbps half-duplex  */
#define LPA_1000XPAUSE      0x0080 /* Can do 1000BASE-X pause     */
#define LPA_100FULL         0x0100 /* Can do 100mbps full-duplex  */
#define LPA_1000XPAUSE_ASYM 0x0100 /* Can do 1000BASE-X pause asym*/
#define LPA_100BASE4        0x0200 /* Can do 100mbps 4k packets   */
#define LPA_PAUSE_CAP       0x0400 /* Can pause                   */
#define LPA_PAUSE_ASYM      0x0800 /* Can pause asymetrically     */
#define LPA_RESV            0x1000 /* Unused...                   */
#define LPA_RFAULT          0x2000 /* Link partner faulted        */
#define LPA_LPACK           0x4000 /* Link partner acked us       */
#define LPA_NPAGE           0x8000 /* Next page bit               */

#define LPA_DUPLEX (LPA_10FULL | LPA_100FULL)
#define LPA_100    (LPA_100FULL | LPA_100HALF | LPA_100BASE4)

/* Expansion register for auto-negotiation. */
#define EXPANSION_NWAY        0x0001 /* Can do N-way auto-nego      */
#define EXPANSION_LCWP        0x0002 /* Got new RX page code word   */
#define EXPANSION_ENABLENPAGE 0x0004 /* This enables npage words    */
#define EXPANSION_NPCAPABLE   0x0008 /* Link partner supports npage */
#define EXPANSION_MFAULTS     0x0010 /* Multiple faults detected    */
#define EXPANSION_RESV        0xffe0 /* Unused...                   */

#define ESTATUS_1000_XFULL 0x8000 /* Can do 1000BaseX Full       */
#define ESTATUS_1000_XHALF 0x4000 /* Can do 1000BaseX Half       */
#define ESTATUS_1000_TFULL 0x2000 /* Can do 1000BT Full          */
#define ESTATUS_1000_THALF 0x1000 /* Can do 1000BT Half          */

/* N-way test register. */
#define NWAYTEST_RESV1    0x00ff /* Unused...                   */
#define NWAYTEST_LOOPBACK 0x0100 /* Enable loopback for N-way   */
#define NWAYTEST_RESV2    0xfe00 /* Unused...                   */

/* MAC and PHY tx_config_Reg[15:0] for SGMII in-band auto-negotiation.*/
#define ADVERTISE_SGMII        0x0001 /* MAC can do SGMII            */
#define LPA_SGMII              0x0001 /* PHY can do SGMII            */
#define LPA_SGMII_SPD_MASK     0x0c00 /* SGMII speed mask            */
#define LPA_SGMII_FULL_DUPLEX  0x1000 /* SGMII full duplex           */
#define LPA_SGMII_DPX_SPD_MASK 0x1C00 /* SGMII duplex and speed bits */
#define LPA_SGMII_10           0x0000 /* 10Mbps                      */
#define LPA_SGMII_10HALF       0x0000 /* Can do 10mbps half-duplex   */
#define LPA_SGMII_10FULL       0x1000 /* Can do 10mbps full-duplex   */
#define LPA_SGMII_100          0x0400 /* 100Mbps                     */
#define LPA_SGMII_100HALF      0x0400 /* Can do 100mbps half-duplex  */
#define LPA_SGMII_100FULL      0x1400 /* Can do 100mbps full-duplex  */
#define LPA_SGMII_1000         0x0800 /* 1000Mbps                    */
#define LPA_SGMII_1000HALF     0x0800 /* Can do 1000mbps half-duplex */
#define LPA_SGMII_1000FULL     0x1800 /* Can do 1000mbps full-duplex */
#define LPA_SGMII_LINK         0x8000 /* PHY link with copper-side partner */

/* 1000BASE-T Control register */
#define ADVERTISE_1000FULL    0x0200 /* Advertise 1000BASE-T full duplex */
#define ADVERTISE_1000HALF    0x0100 /* Advertise 1000BASE-T half duplex */
#define CTL1000_PREFER_MASTER 0x0400 /* prefer to operate as master */
#define CTL1000_AS_MASTER     0x0800
#define CTL1000_ENABLE_MASTER 0x1000

/* 1000BASE-T Status register */
#define LPA_1000MSFAIL    0x8000 /* Master/Slave resolution failure */
#define LPA_1000MSRES     0x4000 /* Master/Slave resolution status */
#define LPA_1000LOCALRXOK 0x2000 /* Link partner local receiver status */
#define LPA_1000REMRXOK   0x1000 /* Link partner remote receiver status */
#define LPA_1000FULL      0x0800 /* Link partner 1000BASE-T full duplex */
#define LPA_1000HALF      0x0400 /* Link partner 1000BASE-T half duplex */

/* Flow control flags */
#define FLOW_CTRL_TX 0x01
#define FLOW_CTRL_RX 0x02

/* MMD Access Control register fields */
#define MII_MMD_CTRL_DEVAD_MASK 0x1f /* Mask MMD DEVAD*/
#define MII_MMD_CTRL_ADDR       0x0000 /* Address */
#define MII_MMD_CTRL_NOINCR     0x4000 /* no post increment */
#define MII_MMD_CTRL_INCR_RDWT  0x8000 /* post increment on reads & writes */
#define MII_MMD_CTRL_INCR_ON_WT 0xC000 /* post increment on writes only */

int aicphy_init(uint32_t port);
void aic_phy_poll(void);
#ifdef __cplusplus
}
#endif

#endif
