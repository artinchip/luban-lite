/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include "aic_mac.h"
#include "aic_phy.h"

#define PHY_POLL_TASK_STACK_SIZE        (1024)
#define PHY_POLL_TASK_PRIORITY          (TCPIP_THREAD_PRIO-1)
#define PHY_POLL_TASK_INTERVAL_MS       1000

#define PHY_SETTING(s, d, b)                      \
    {                                             \
        .speed = SPEED_##s, .duplex = DUPLEX_##d, \
        .bit = ETHTOOL_LINK_MODE_##b##_BIT        \
    }

static const struct phy_setting settings[] = {
#if 0
	/* 400G */
	PHY_SETTING( 400000, FULL, 400000baseCR8_Full		),
	PHY_SETTING( 400000, FULL, 400000baseKR8_Full		),
	PHY_SETTING( 400000, FULL, 400000baseLR8_ER8_FR8_Full	),
	PHY_SETTING( 400000, FULL, 400000baseDR8_Full		),
	PHY_SETTING( 400000, FULL, 400000baseSR8_Full		),
	PHY_SETTING( 400000, FULL, 400000baseCR4_Full		),
	PHY_SETTING( 400000, FULL, 400000baseKR4_Full		),
	PHY_SETTING( 400000, FULL, 400000baseLR4_ER4_FR4_Full	),
	PHY_SETTING( 400000, FULL, 400000baseDR4_Full		),
	PHY_SETTING( 400000, FULL, 400000baseSR4_Full		),
	/* 200G */
	PHY_SETTING( 200000, FULL, 200000baseCR4_Full		),
	PHY_SETTING( 200000, FULL, 200000baseKR4_Full		),
	PHY_SETTING( 200000, FULL, 200000baseLR4_ER4_FR4_Full	),
	PHY_SETTING( 200000, FULL, 200000baseDR4_Full		),
	PHY_SETTING( 200000, FULL, 200000baseSR4_Full		),
	PHY_SETTING( 200000, FULL, 200000baseCR2_Full		),
	PHY_SETTING( 200000, FULL, 200000baseKR2_Full		),
	PHY_SETTING( 200000, FULL, 200000baseLR2_ER2_FR2_Full	),
	PHY_SETTING( 200000, FULL, 200000baseDR2_Full		),
	PHY_SETTING( 200000, FULL, 200000baseSR2_Full		),
	/* 100G */
	PHY_SETTING( 100000, FULL, 100000baseCR4_Full		),
	PHY_SETTING( 100000, FULL, 100000baseKR4_Full		),
	PHY_SETTING( 100000, FULL, 100000baseLR4_ER4_Full	),
	PHY_SETTING( 100000, FULL, 100000baseSR4_Full		),
	PHY_SETTING( 100000, FULL, 100000baseCR2_Full		),
	PHY_SETTING( 100000, FULL, 100000baseKR2_Full		),
	PHY_SETTING( 100000, FULL, 100000baseLR2_ER2_FR2_Full	),
	PHY_SETTING( 100000, FULL, 100000baseDR2_Full		),
	PHY_SETTING( 100000, FULL, 100000baseSR2_Full		),
	PHY_SETTING( 100000, FULL, 100000baseCR_Full		),
	PHY_SETTING( 100000, FULL, 100000baseKR_Full		),
	PHY_SETTING( 100000, FULL, 100000baseLR_ER_FR_Full	),
	PHY_SETTING( 100000, FULL, 100000baseDR_Full		),
	PHY_SETTING( 100000, FULL, 100000baseSR_Full		),
	/* 56G */
	PHY_SETTING(  56000, FULL,  56000baseCR4_Full	  	),
	PHY_SETTING(  56000, FULL,  56000baseKR4_Full	  	),
	PHY_SETTING(  56000, FULL,  56000baseLR4_Full	  	),
	PHY_SETTING(  56000, FULL,  56000baseSR4_Full	  	),
	/* 50G */
	PHY_SETTING(  50000, FULL,  50000baseCR2_Full		),
	PHY_SETTING(  50000, FULL,  50000baseKR2_Full		),
	PHY_SETTING(  50000, FULL,  50000baseSR2_Full		),
	PHY_SETTING(  50000, FULL,  50000baseCR_Full		),
	PHY_SETTING(  50000, FULL,  50000baseKR_Full		),
	PHY_SETTING(  50000, FULL,  50000baseLR_ER_FR_Full	),
	PHY_SETTING(  50000, FULL,  50000baseDR_Full		),
	PHY_SETTING(  50000, FULL,  50000baseSR_Full		),
	/* 40G */
	PHY_SETTING(  40000, FULL,  40000baseCR4_Full		),
	PHY_SETTING(  40000, FULL,  40000baseKR4_Full		),
	PHY_SETTING(  40000, FULL,  40000baseLR4_Full		),
	PHY_SETTING(  40000, FULL,  40000baseSR4_Full		),
	/* 25G */
	PHY_SETTING(  25000, FULL,  25000baseCR_Full		),
	PHY_SETTING(  25000, FULL,  25000baseKR_Full		),
	PHY_SETTING(  25000, FULL,  25000baseSR_Full		),
	/* 20G */
	PHY_SETTING(  20000, FULL,  20000baseKR2_Full		),
	PHY_SETTING(  20000, FULL,  20000baseMLD2_Full		),
    /* 10G */
    PHY_SETTING(10000, FULL, 10000baseCR_Full),
    PHY_SETTING(10000, FULL, 10000baseER_Full),
    PHY_SETTING(10000, FULL, 10000baseKR_Full),
    PHY_SETTING(10000, FULL, 10000baseKX4_Full),
    PHY_SETTING(10000, FULL, 10000baseLR_Full),
    PHY_SETTING(10000, FULL, 10000baseLRM_Full),
    PHY_SETTING(10000, FULL, 10000baseR_FEC),
    PHY_SETTING(10000, FULL, 10000baseSR_Full),
    PHY_SETTING(10000, FULL, 10000baseT_Full),
    /* 5G */
    PHY_SETTING(5000, FULL, 5000baseT_Full),
    /* 2.5G */
    PHY_SETTING(2500, FULL, 2500baseT_Full),
    PHY_SETTING(2500, FULL, 2500baseX_Full),
#endif
    /* 1G */
    PHY_SETTING(1000, FULL, 1000baseKX_Full),
    PHY_SETTING(1000, FULL, 1000baseT_Full),
    PHY_SETTING(1000, HALF, 1000baseT_Half),
    PHY_SETTING(1000, FULL, 1000baseT1_Full),
    PHY_SETTING(1000, FULL, 1000baseX_Full),
    /* 100M */
    PHY_SETTING(100, FULL, 100baseT_Full),
    PHY_SETTING(100, FULL, 100baseT1_Full),
    PHY_SETTING(100, HALF, 100baseT_Half),
    PHY_SETTING(100, HALF, 100baseFX_Half),
    PHY_SETTING(100, FULL, 100baseFX_Full),
    /* 10M */
    PHY_SETTING(10, FULL, 10baseT_Full),
    PHY_SETTING(10, HALF, 10baseT_Half),
};

aic_phy_device_t phy_device[MAX_ETH_MAC_PORT];
extern aicmac_config_t mac_config[MAX_ETH_MAC_PORT];
extern aicmac_netif_t aic_netif;

int aicphy_sw_reset(uint32_t port)
{
    uint16_t tmpreg = 0;
    uint32_t timeout = 0;
    int ret = 0;

    /* Get the PHY configuration to update it */
    ret = aicmac_write_phy_reg(port, MII_BMCR, BMCR_RESET);
    if (ret)
        return ret;

    /* Check for the Busy flag */
    do {
        timeout++;
        ret = aicmac_read_phy_reg(port, MII_BMCR, &tmpreg);
        if (ret)
            return ret;
    } while ((tmpreg & BMCR_RESET) && (timeout < (uint32_t)PHY_WRITE_TO));

    if (timeout == PHY_WRITE_TO) {
        pr_err("aicmac software reset timeout.\n");
        ret = ETH_ERROR;
    }
    return ret;
}

int aicphy_set_loopback(uint32_t port, bool en)
{
    uint16_t tmpreg = 0;
    int ret = 0;

    /* Get the PHY configuration to update it */
    ret = aicmac_read_phy_reg(port, MII_BMCR, &tmpreg);
    if (ret)
        return ret;

    if (en)
        tmpreg |= BMCR_LOOPBACK;
    else
        tmpreg &= ~BMCR_LOOPBACK;

    ret = aicmac_write_phy_reg(port, MII_BMCR, tmpreg);
    return ret;
}

static inline void linkmode_mod_bit(int nr, u64 *addr, int set)
{
    if (set)
        *addr |= (1ULL << nr);
    else
        *addr &= ~(1ULL << nr);
}

static inline int linkmode_test_bit(int nr, u64 *addr)
{
    return (*addr & (1ULL << nr));
}

static inline int test_bit(int nr, u64 *addr)
{
    return (*addr & (1ULL << nr));
}

static inline u16 linkmode_adv_to_mii_adv_t(u64 *advertising)
{
    u16 result = 0;

    if (linkmode_test_bit(ETHTOOL_LINK_MODE_10baseT_Half_BIT, advertising))
        result |= ADVERTISE_10HALF;
    if (linkmode_test_bit(ETHTOOL_LINK_MODE_10baseT_Full_BIT, advertising))
        result |= ADVERTISE_10FULL;
    if (linkmode_test_bit(ETHTOOL_LINK_MODE_100baseT_Half_BIT, advertising))
        result |= ADVERTISE_100HALF;
    if (linkmode_test_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT, advertising))
        result |= ADVERTISE_100FULL;
    if (linkmode_test_bit(ETHTOOL_LINK_MODE_Pause_BIT, advertising))
        result |= ADVERTISE_PAUSE_CAP;
    if (linkmode_test_bit(ETHTOOL_LINK_MODE_Asym_Pause_BIT, advertising))
        result |= ADVERTISE_PAUSE_ASYM;

    return result;
}

static inline u16 linkmode_adv_to_mii_ctrl1000_t(u64 *advertising)
{
    u16 result = 0;

    if (linkmode_test_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT, advertising))
        result |= ADVERTISE_1000HALF;
    if (linkmode_test_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT, advertising))
        result |= ADVERTISE_1000FULL;

    return result;
}

static inline void mii_stat1000_mod_linkmode_lpa_t(u64 *advertising, u16 lpa)
{
    linkmode_mod_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT, advertising,
                     lpa & LPA_1000HALF);

    linkmode_mod_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT, advertising,
                     lpa & LPA_1000FULL);
}

static inline void mii_adv_mod_linkmode_adv_t(u64 *advertising, u16 adv)
{
    linkmode_mod_bit(ETHTOOL_LINK_MODE_10baseT_Half_BIT, advertising,
                     adv & ADVERTISE_10HALF);

    linkmode_mod_bit(ETHTOOL_LINK_MODE_10baseT_Full_BIT, advertising,
                     adv & ADVERTISE_10FULL);

    linkmode_mod_bit(ETHTOOL_LINK_MODE_100baseT_Half_BIT, advertising,
                     adv & ADVERTISE_100HALF);

    linkmode_mod_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT, advertising,
                     adv & ADVERTISE_100FULL);

    linkmode_mod_bit(ETHTOOL_LINK_MODE_Pause_BIT, advertising,
                     adv & ADVERTISE_PAUSE_CAP);

    linkmode_mod_bit(ETHTOOL_LINK_MODE_Asym_Pause_BIT, advertising,
                     adv & ADVERTISE_PAUSE_ASYM);
}

static inline void mii_lpa_mod_linkmode_lpa_t(u64 *lp_advertising, u16 lpa)
{
    mii_adv_mod_linkmode_adv_t(lp_advertising, lpa);

    linkmode_mod_bit(ETHTOOL_LINK_MODE_Autoneg_BIT, lp_advertising,
                     lpa & LPA_LPACK);
}

int phy_modify(uint32_t port, u32 regnum, u16 mask, u16 set)
{
    uint16_t val = 0;
    uint16_t new = 0;
    int err = 0;

    err = aicmac_read_phy_reg(port, regnum, &val);
    if (err)
        return ETH_ERROR;

    new = (val & ~mask) | set;
    if (new == val)
        return ETH_SUCCESS;

    err = aicmac_write_phy_reg(port, regnum, val);
    if (err)
        return ETH_ERROR;

    return ETH_SUCCESS;
}

int aicphy_read_abilities(uint32_t port)
{
    aic_phy_device_t *phydev = &phy_device[port];
    u16 val = 0;
    int err = 0;

    /*
	linkmode_set_bit_array(phy_basic_ports_array,
			       ARRAY_SIZE(phy_basic_ports_array),
			       phydev->supported);
    */
    //linkmode_mod_bit(ETHTOOL_LINK_MODE_Autoneg_BIT, phydev->supported, 1);
    linkmode_mod_bit(ETHTOOL_LINK_MODE_TP_BIT, phydev->supported, 1);
    linkmode_mod_bit(ETHTOOL_LINK_MODE_MII_BIT, phydev->supported, 1);
    linkmode_mod_bit(ETHTOOL_LINK_MODE_Pause_BIT, phydev->supported, 1);

    err = aicmac_read_phy_reg(port, MII_BMSR, &val);
    if (err)
        return ETH_ERROR;

    linkmode_mod_bit(ETHTOOL_LINK_MODE_Autoneg_BIT, phydev->supported,
                     val & BMSR_ANEGCAPABLE);

    linkmode_mod_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT, phydev->supported,
                     val & BMSR_100FULL);
    linkmode_mod_bit(ETHTOOL_LINK_MODE_100baseT_Half_BIT, phydev->supported,
                     val & BMSR_100HALF);
    linkmode_mod_bit(ETHTOOL_LINK_MODE_10baseT_Full_BIT, phydev->supported,
                     val & BMSR_10FULL);
    linkmode_mod_bit(ETHTOOL_LINK_MODE_10baseT_Half_BIT, phydev->supported,
                     val & BMSR_10HALF);

    if (val & BMSR_ESTATEN) {
        err = aicmac_read_phy_reg(port, MII_ESTATUS, &val);
        if (err)
            return ETH_ERROR;

        linkmode_mod_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
                         phydev->supported, val & ESTATUS_1000_TFULL);
        linkmode_mod_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT,
                         phydev->supported, val & ESTATUS_1000_THALF);
        linkmode_mod_bit(ETHTOOL_LINK_MODE_1000baseX_Full_BIT,
                         phydev->supported, val & ESTATUS_1000_XFULL);
    }

    /* Check phy abilities */
    if (!linkmode_test_bit(ETHTOOL_LINK_MODE_Autoneg_BIT, phydev->supported))
        phydev->autoneg = 0;

    if (linkmode_test_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT,
                          phydev->supported))
        phydev->is_gigabit_capable = 1;
    if (linkmode_test_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
                          phydev->supported))
        phydev->is_gigabit_capable = 1;

    return ETH_SUCCESS;
}

int aicphy_config_advert(uint32_t port)
{
    aic_phy_device_t *phydev = &phy_device[port];
    int err = 0;
    u16 bmsr = 0;
    u16 adv = 0;

    /* Only allow advertising what this PHY supports */
    /* linkmode_and(phydev->advertising, phydev->advertising,
		     phydev->supported);*/
    phydev->advertising[0] = phydev->supported[0];

    adv = linkmode_adv_to_mii_adv_t(phydev->advertising);

    /* Setup standard advertisement */
    err = phy_modify(port, MII_ADVERTISE,
                     ADVERTISE_ALL | ADVERTISE_100BASE4 | ADVERTISE_PAUSE_CAP |
                         ADVERTISE_PAUSE_ASYM,
                     adv);
    if (err)
        return err;

    err = aicmac_read_phy_reg(port, MII_BMSR, &bmsr);
    if (err)
        return ETH_ERROR;

    /* Per 802.3-2008, Section 22.2.4.2.16 Extended status all
	 * 1000Mbits/sec capable PHYs shall have the BMSR_ESTATEN bit set to a
	 * logical 1.
	 */
    if (!(bmsr & BMSR_ESTATEN))
        return ETH_SUCCESS;

    adv = linkmode_adv_to_mii_ctrl1000_t(phydev->advertising);

    err = phy_modify(port, MII_CTRL1000,
                     ADVERTISE_1000FULL | ADVERTISE_1000HALF, adv);
    if (err)
        return err;

    return ETH_SUCCESS;
}

int aicphy_restart_aneg(uint32_t port)
{
    /* Don't isolate the PHY if we're negotiating */
    return phy_modify(port, MII_BMCR, BMCR_ISOLATE,
                      BMCR_ANENABLE | BMCR_ANRESTART);
}

int aicphy_setup_forced(uint32_t port)
{
    aicmac_config_t *config = &mac_config[port];
    u16 ctl = 0;

    if (SPEED_1000 == config->max_speed)
        ctl |= BMCR_SPEED1000;
    else if (SPEED_100 == config->max_speed)
        ctl |= BMCR_SPEED100;

    if (config->duplex)
        ctl |= BMCR_FULLDPLX;

    return phy_modify(port, MII_BMCR,
                      ~(BMCR_LOOPBACK | BMCR_ISOLATE | BMCR_PDOWN), ctl);
}

int aicphy_config_aneg(uint32_t port)
{
    aic_phy_device_t *phydev = &phy_device[port];
    int err;

    if (!phydev->autoneg) {
        return aicphy_setup_forced(port);
    } else {
        err = aicphy_config_advert(port);
        if (err)
            return err;
        err = aicphy_restart_aneg(port);
        if (err)
            return err;
    }

    return ETH_SUCCESS;
}

int aicphy_read_lpa(uint32_t port)
{
    aic_phy_device_t *phydev = &phy_device[port];
    u16 lpa, lpagb, adv;
    int err = 0;

    if (phydev->autoneg) {
        if (!phydev->autoneg_complete) {
            mii_stat1000_mod_linkmode_lpa_t(phydev->lp_advertising, 0);
            mii_lpa_mod_linkmode_lpa_t(phydev->lp_advertising, 0);
            return 0;
        }

        if (phydev->is_gigabit_capable) {
            err = aicmac_read_phy_reg(port, MII_STAT1000, &lpagb);
            if (err)
                return ETH_ERROR;

            if (lpagb & LPA_1000MSFAIL) {
                err = aicmac_read_phy_reg(port, MII_CTRL1000, &adv);
                if (err)
                    return ETH_ERROR;

                if (adv & CTL1000_ENABLE_MASTER)
                    pr_err(
                        "Master/Slave resolution failed, maybe conflicting manual settings?\n");
                else
                    pr_err("Master/Slave resolution failed\n");
                return ETH_ERROR;
            }

            mii_stat1000_mod_linkmode_lpa_t(phydev->lp_advertising, lpagb);
        }

        err = aicmac_read_phy_reg(port, MII_LPA, &lpa);
        if (err)
            return ETH_ERROR;

        mii_lpa_mod_linkmode_lpa_t(phydev->lp_advertising, lpa);
    } else {
        phydev->lp_advertising[0] = 0;
    }

    return ETH_SUCCESS;
}

void aicphy_resolve_aneg_pause(uint32_t port)
{
    aic_phy_device_t *phydev = &phy_device[port];

    if (phydev->duplex) {
        if(linkmode_test_bit(ETHTOOL_LINK_MODE_Pause_BIT,
                                          phydev->lp_advertising))
            phydev->pause = 1;
        if (linkmode_test_bit(ETHTOOL_LINK_MODE_Asym_Pause_BIT,
                                               phydev->lp_advertising))
           phydev->asym_pause = 1;
    }
}

void aicphy_resolve_aneg_linkmode(uint32_t port)
{
    aic_phy_device_t *phydev = &phy_device[port];
    u64 common = 0;
    int i;
#ifdef PHY_DEBUG
    uint16_t tmpreg = 0;

    pr_info("%s:\n", __func__);
    for (i = 0; i < 0x20; i++) {
        aicmac_read_phy_reg(port, i, &tmpreg);
        pr_info("0x%x: 0x%x\n", i, tmpreg);
    }
#endif

    /* local advertising AND link partner advertising */
    phydev->advertising[0] = phydev->supported[0];
    common = phydev->advertising[0] & phydev->lp_advertising[0];

    pr_debug("common = 0x%llx, supported = 0x%llx, advertising = 0x%llx, lp_advertising = 0x%llx.\n",
            common, phydev->supported[0], phydev->advertising[0], phydev->lp_advertising[0]);

    for (i = 0; i < ARRAY_SIZE(settings); i++)
        if (test_bit(settings[i].bit, &common)) {
            phydev->speed = settings[i].speed;
            phydev->duplex = settings[i].duplex;
            pr_debug("setting %d: speed %dM, duplex %d.\n", i, phydev->speed, phydev->duplex);
            break;
        }

    aicphy_resolve_aneg_pause(port);
}

int aicphy_read_status_fixed(uint32_t port)
{
    aic_phy_device_t *phydev = &phy_device[port];
    u16 bmcr = 0;
    int err;

    err = aicmac_read_phy_reg(port, MII_BMCR, &bmcr);
    if (err)
        return ETH_ERROR;

    if (bmcr & BMCR_FULLDPLX)
        phydev->duplex = DUPLEX_FULL;
    else
        phydev->duplex = DUPLEX_HALF;

    if (bmcr & BMCR_SPEED1000)
        phydev->speed = SPEED_1000;
    else if (bmcr & BMCR_SPEED100)
        phydev->speed = SPEED_100;
    else
        phydev->speed = SPEED_10;

    return 0;
}

int aicphy_update_link(uint32_t port)
{
    aic_phy_device_t *phydev = &phy_device[port];
    u16 status = 0;
    u16 bmcr = 0;
    int err;

    err = aicmac_read_phy_reg(port, MII_BMCR, &bmcr);
    if (err)
        return ETH_ERROR;

    /* Autoneg is being started, therefore disregard BMSR value and
	 * report link as down.
	 */
    if (bmcr & BMCR_ANRESTART)
        goto done;

    /* Read link and autonegotiation status */
    err = aicmac_read_phy_reg(port, MII_BMSR, &status);
    if (err)
        return ETH_ERROR;
done:
    phydev->link = status & BMSR_LSTATUS ? 1 : 0;
    phydev->autoneg_complete = status & BMSR_ANEGCOMPLETE ? 1 : 0;

    /* Consider the case that autoneg was started and "aneg complete"
	 * bit has been reset, but "link up" bit not yet.
	 */
    if (phydev->autoneg && !phydev->autoneg_complete)
        phydev->link = 0;

    return ETH_SUCCESS;
}

int aicphy_read_status(uint32_t port)
{
    aic_phy_device_t *phydev = &phy_device[port];
    int old_link = phydev->link;
    int err;

    /* Update the link, but return if there was an error */
    err = aicphy_update_link(port);
    if (err)
        return ETH_ERROR;

    /* why bother the PHY if nothing can have changed */
    if (!(phydev->link) || (old_link == phydev->link))
        return ETH_SUCCESS;

    phydev->speed = 0;
    phydev->duplex = 0;
    phydev->pause = 0;
    phydev->asym_pause = 0;

    err = aicphy_read_lpa(port);
    if (err)
        return err;

    if (phydev->autoneg && phydev->autoneg_complete) {
        aicphy_resolve_aneg_linkmode(port);
    } else if (!phydev->autoneg) {
        err = aicphy_read_status_fixed(port);
        if (err)
            return err;
    }

    return ETH_SUCCESS;
}

void aic_phy_poll(void)
{
    uint32_t port = aic_netif.port;
    struct netif *netif = &aic_netif.netif;
    aic_phy_device_t *phydev = &phy_device[port];
    static int old_link = 0;
    int err;

	/* Pool phy satus */
	err = aicphy_read_status(port);
	if (err){
		pr_err("%s fail.\n", __func__);
		return ;
	}

	/* Phy link status not change */
	if (old_link == phydev->link)
		return ;
	old_link = phydev->link;

	/* Phy link status change: DOWN -> UP */
	if (phydev->link){
		pr_info(" Port %d link UP! %s mode: speed %dM, %s duplex, flow control %s.\n",
				(int)port,
				(phydev->autoneg ? "autoneg" : "fixed"),
				phydev->speed,
				(phydev->duplex ? "full" : "half"),
				(phydev->pause ? "on" : "off"));

		/* Config mac base on autoneg result */
		if (phydev->autoneg){
			aicmac_set_mac_speed(port, phydev->speed);
			aicmac_set_mac_duplex(port, phydev->duplex);
			aicmac_set_mac_pause(port, phydev->pause);
		}

		/* Enable MAC and DMA transmission and reception */
		aicmac_start(port);

		/* Netif set phy linkup */
		netif_set_link_up(netif);

	/* Phy link status change: UP -> DOWN */
	} else {
		pr_info(" Port %d link DOWN!\n", (int)port);

		/* Disable MAC and DMA transmission and reception */
		aicmac_stop(port);

		/* Netif set phy linkdown */
		netif_set_link_down(netif);
	}
}

void aicphy_poll_thread(void *pvParameters)
{
    uint32_t port = aic_netif.port;
    struct netif *netif = &aic_netif.netif;
    aic_phy_device_t *phydev = &phy_device[port];
    int old_link = 0;
    int err;

    while(1){
        /* Sleep */
        aicos_msleep(PHY_POLL_TASK_INTERVAL_MS);

        /* Pool phy satus */
        err = aicphy_read_status(port);
        if (err){
            pr_err("%s fail.\n", __func__);
            continue;
        }

        /* Phy link status not change */
        if (old_link == phydev->link)
            continue;
        old_link = phydev->link;

        /* Phy link status change: DOWN -> UP */
        if (phydev->link){
            pr_info(" Port %d link UP! %s mode: speed %dM, %s duplex, flow control %s.\n",
                    (int)port,
                    (phydev->autoneg ? "autoneg" : "fixed"),
                    phydev->speed,
                    (phydev->duplex ? "full" : "half"),
                    (phydev->pause ? "on" : "off"));

            /* Config mac base on autoneg result */
            if (phydev->autoneg){
                aicmac_set_mac_speed(port, phydev->speed);
                aicmac_set_mac_duplex(port, phydev->duplex);
                aicmac_set_mac_pause(port, phydev->pause);
            }

            /* Enable MAC and DMA transmission and reception */
            aicmac_start(port);

            /* Netif set phy linkup */
            netif_set_link_up(netif);

        /* Phy link status change: UP -> DOWN */
        } else {
            pr_info(" Port %d link DOWN!\n", (int)port);

            /* Disable MAC and DMA transmission and reception */
            aicmac_stop(port);

            /* Netif set phy linkdown */
            netif_set_link_down(netif);
        }
    }
}

int aicphy_init(uint32_t port)
{
    aicmac_config_t *config = &mac_config[port];
    aic_phy_device_t *phydev = &phy_device[port];
    phydev->autoneg = config->autonegotiation;
#ifdef PHY_DEBUG
    uint16_t tmpreg = 0;
    int i = 0;

    pr_info("%s:\n", __func__);
    for (i = 0; i < 0x20; i++) {
        aicmac_read_phy_reg(port, i, &tmpreg);
        pr_info("0x%x: 0x%x\n", i, tmpreg);
    }
#endif
    //aicphy_sw_reset(port);

    aicphy_read_abilities(port);

    aicphy_config_aneg(port);

#if !NO_SYS
    /* create the task that handles the ETH_MAC */
    aicos_thread_create("eth_phy_poll", PHY_POLL_TASK_STACK_SIZE,
                        PHY_POLL_TASK_PRIORITY, aicphy_poll_thread, NULL);
#endif
    return ETH_SUCCESS;
}

