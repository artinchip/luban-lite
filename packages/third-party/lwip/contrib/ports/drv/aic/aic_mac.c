/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "lwip/err.h"
#include <string.h>
#include <rtconfig.h>
#include <aic_core.h>
#include "aic_mac.h"
#include "aic_phy.h"
#include <aic_hal.h>

#define USE_FULL_ASSERT
#ifdef USE_FULL_ASSERT
#define assert_param(expr) \
    ((expr) ?              \
         (void)0 :         \
         assert_failed((uint8_t *)__FILE__, (uint8_t *)__func__, __LINE__))

static inline void assert_failed(uint8_t *file, uint8_t *func, uint32_t line)
{
    printf("Wrong parameters value: file %s function %s on line %d\r\n", file,
           func, (unsigned int)line);

    /* Infinite loop */
    while (1) {
    }
}
#else
#define assert_param(expr) ((void)0)
#endif

#if MAX_ETH_MAC_PORT == 2
unsigned long mac_base[MAX_ETH_MAC_PORT] = { GMAC0_BASE, GMAC1_BASE };
unsigned long mac_irq[MAX_ETH_MAC_PORT] = { GMAC0_IRQn, GMAC0_IRQn+1 };
#else
unsigned long mac_base[MAX_ETH_MAC_PORT] = { GMAC0_BASE };
unsigned long mac_irq[MAX_ETH_MAC_PORT] = { GMAC0_IRQn };
#endif

#ifdef CONFIG_MAC_USE_UNCACHE_BUF
CMA_DATA_DEFINE aicmac_dma_desc_ctl_t dctl[MAX_ETH_MAC_PORT];
#else
aicmac_dma_desc_ctl_t dctl[MAX_ETH_MAC_PORT];
#endif

aicmac_config_t mac_config[MAX_ETH_MAC_PORT] = {
    /* mac port 0: 100M/1000M */
    {
        .port = 0,
        #ifdef AIC_DEV_GMAC0_PHYADDR
        .phyaddr = AIC_DEV_GMAC0_PHYADDR,
        #else
        .phyaddr = 1,
        #endif
        #ifdef AIC_DEV_GMAC0_RMII
        .rgmii_bus = 0,
        .max_speed = SPEED_100,
        #else
        .rgmii_bus = 1,
        .max_speed = SPEED_1000,
        #endif
        .duplex = 1,
        .flowctl = 1,
        .autonegotiation = 1,
        #ifdef LPKG_LWIP_USING_TX_HW_CHECKSUM
        .coe_tx = 1,
        #else
        .coe_tx = 0,
        #endif
        #ifdef LPKG_LWIP_USING_RX_HW_CHECKSUM
        .coe_rx = 1,
        #else
        .coe_rx = 0,
        #endif
        .dma_rxpbl = CONFIG_DMA_RX_PBL,
        .dma_txpbl = CONFIG_DMA_TX_PBL,
        .dma_pblx8 = 1,
        .dma_fixed_burst = 1,
        .dma_mixed_burst = 0,
        .dma_aal = 1,
        .dma_sf_mode = 1,
        #ifdef AIC_DEV_GMAC0_PHYRST_GPIO
        .phyrst_gpio_name = AIC_DEV_GMAC0_PHYRST_GPIO,
        #endif
    },

#if MAX_ETH_MAC_PORT == 2
    /* mac port 1: 100M/1000M */
    {
        .port = 1,
        #ifdef AIC_DEV_GMAC1_PHYADDR
        .phyaddr = AIC_DEV_GMAC1_PHYADDR,
        #else
        .phyaddr = 1,
        #endif
        #ifdef AIC_DEV_GMAC1_RMII
        .rgmii_bus = 0,
        .max_speed = SPEED_100,
        #else
        .rgmii_bus = 1,
        .max_speed = SPEED_1000,
        #endif
        .duplex = 1,
        .flowctl = 1,
        .autonegotiation = 1,
        #ifdef LPKG_LWIP_USING_TX_HW_CHECKSUM
        .coe_tx = 1,
        #else
        .coe_tx = 0,
        #endif
        #ifdef LPKG_LWIP_USING_RX_HW_CHECKSUM
        .coe_rx = 1,
        #else
        .coe_rx = 0,
        #endif
        .dma_rxpbl = CONFIG_DMA_RX_PBL,
        .dma_txpbl = CONFIG_DMA_TX_PBL,
        .dma_pblx8 = 1,
        .dma_fixed_burst = 1,
        .dma_mixed_burst = 0,
        .dma_aal = 1,
        .dma_sf_mode = 1,
        #ifdef AIC_DEV_GMAC1_PHYRST_GPIO
        .phyrst_gpio_name = AIC_DEV_GMAC1_PHYRST_GPIO,
        #endif
    },
#endif
};

extern void aicmac_low_level_init(uint32_t port, bool en);

void aicmac_exit(uint32_t port)
{
    /* Check the parameters */
    assert_param(port < MAX_ETH_MAC_PORT);

    /* HW Low-Level Init */
    aicmac_low_level_init(port, DISABLE);
}

int aicmac_init(uint32_t port)
{
    uint32_t tmpreg = 0;
    uint32_t ahbclk = 0;
    uint32_t pin = 0;
    uint32_t p = 0;
    uint32_t g = 0;

    /* Check the parameters */
    assert_param(port < MAX_ETH_MAC_PORT);

    memset(dctl, 0, sizeof(dctl));

    /* HW Low-Level Init */
    aicmac_low_level_init(port, ENABLE);

    /* Software reset */
    aicmac_sw_reset(port);
    /* Wait for software reset */
    while (aicmac_get_sw_reset_status(port) == SET) {
    
    }
    
    /* phy reset must after mac reset */
    if (mac_config[port].phyrst_gpio_name) {
        pin = hal_gpio_name2pin(mac_config[port].phyrst_gpio_name);
        g = GPIO_GROUP(pin);
        p = GPIO_GROUP_PIN(pin);

        hal_gpio_direction_output(g, p);

        hal_gpio_clr_output(g, p);
        aicos_mdelay(50);
        hal_gpio_set_output(g, p);
        aicos_mdelay(50);
    }

    aicos_udelay(1000);

    /* MDCIO Internal Clock Select */
    tmpreg = readl(MAC(port, mdioctl));
    tmpreg &= ~(ETH_MDIOCTL_CR_MSK);
    ahbclk = hal_clk_get_freq(CLK_AHB0);
    if ((ahbclk >= 20000000) && (ahbclk < 35000000)) {
        tmpreg |= ETH_MDIOCTL_CR_Div16;
    } else if ((ahbclk >= 35000000) && (ahbclk < 60000000)) {
        tmpreg |= ETH_MDIOCTL_CR_Div26;
    } else if ((ahbclk >= 60000000) && (ahbclk < 100000000)) {
        tmpreg |= ETH_MDIOCTL_CR_Div42;
    } else if ((ahbclk >= 100000000) && (ahbclk < 150000000)) {
        tmpreg |= ETH_MDIOCTL_CR_Div62;
    } else if ((ahbclk >= 150000000) && (ahbclk < 250000000)) {
        tmpreg |= ETH_MDIOCTL_CR_Div102;
    } else {
        /* ((ahbclk >= 250000000)&&(ahbclk <= 300000000)) */
        tmpreg |= ETH_MDIOCTL_CR_Div124;
    }
    writel(tmpreg, MAC(port, mdioctl));

    /*------------------------ MAC Configuration --------------------*/
    /* (1) macconf */
    tmpreg = readl(MAC(port, macconf));
    tmpreg &= ~ETH_MACCONF_SPEED_MSK;
    if (mac_config[port].max_speed == SPEED_100) {
        tmpreg |= ETH_MACCONF_SPEED_100M;
        /* set syscfg RMII */
    } else if (mac_config[port].max_speed == SPEED_1000) {
        tmpreg |= ETH_MACCONF_SPEED_1000M;
        /* set syscfg RGMII */
    }

    if (mac_config[port].duplex)
        tmpreg |= ETH_MACCONF_DM;
    else
        tmpreg &= ~ETH_MACCONF_DM;

    if (mac_config[port].coe_rx)
            tmpreg |= ETH_MACCONF_IPC;

    writel(tmpreg, MAC(port, macconf));

    /* (2) mactxfunc */
    tmpreg = readl(MAC(port, mactxfunc));

    writel(tmpreg, MAC(port, mactxfunc));

    /* (3) macrxfunc */
    tmpreg = readl(MAC(port, macrxfunc));

    writel(tmpreg, MAC(port, macrxfunc));

    /* (4) macfrmflt */

    /* (5) flowctl */
    tmpreg = readl(MAC(port, flowctl));
    tmpreg &= ~ETH_FLOWCTL_PT;
    tmpreg |= (0x4 << ETH_FLOWCTL_PT_SHIFT);
    writel(tmpreg, MAC(port, flowctl));

    /*------------------------ DMA Configuration --------------------*/
    /* (1) dma0conf */
    tmpreg = readl(MAC(port, dma0conf));
    tmpreg &= ~ETH_DMACONF_PR_MSK;
    tmpreg |= ETH_DMACONF_PR_3_1;
    tmpreg |= ETH_DMACONF_ATDS;

    tmpreg |= ETH_DMACONF_USP;
    tmpreg &= ~(ETH_DMACONF_RPBL_MSK | ETH_DMACONF_PBL_MSK);
    tmpreg |= (mac_config[port].dma_rxpbl << ETH_DMACONF_RPBL_SHIFT);
    tmpreg |= (mac_config[port].dma_txpbl << ETH_DMACONF_PBL_SHIFT);

    if (mac_config[port].dma_pblx8)
        tmpreg |= ETH_DMACONF_PBLx8;
    else
        tmpreg &= ~ETH_DMACONF_PBLx8;

    if (mac_config[port].dma_fixed_burst)
        tmpreg |= ETH_DMACONF_FB;
    else
        tmpreg &= ~ETH_DMACONF_FB;

    if (mac_config[port].dma_mixed_burst)
        tmpreg |= ETH_DMACONF_MB;
    else
        tmpreg &= ~ETH_DMACONF_MB;

    if (mac_config[port].dma_aal)
        tmpreg |= ETH_DMACONF_AAL;
    else
        tmpreg &= ~ETH_DMACONF_AAL;

    writel(tmpreg, MAC(port, dma0conf));

    /* (2) rxdma0ctl */
    tmpreg = readl(MAC(port, rxdma0ctl));
    if (mac_config[port].dma_sf_mode) {
        tmpreg |= ETH_RXDMACTL_RSF;
    } else {
        tmpreg &= ~ETH_RXDMACTL_RSF;
        tmpreg &= ~ETH_RXDMACTL_RTC_MSK;
        tmpreg |= (mac_config[port].dma_fifo_rxth << ETH_RXDMACTL_RTC_SHIFT);
    }
    writel(tmpreg, MAC(port, rxdma0ctl));

    /* (3) txdma0ctl */
    tmpreg = readl(MAC(port, txdma0ctl));
    if (mac_config[port].dma_sf_mode) {
        tmpreg |= ETH_TXDMACTL_TSF;
    } else {
        tmpreg &= ~ETH_TXDMACTL_TSF;
        tmpreg &= ~ETH_TXDMACTL_TTC_MSK;
        tmpreg |= (mac_config[port].dma_fifo_txth << ETH_RXDMACTL_TTC_SHIFT);
    }
    writel(tmpreg, MAC(port, txdma0ctl));

#if !NO_SYS
    /* (4) dma0inten */
    tmpreg = readl(MAC(port, dma0inten));
    tmpreg |= (ETH_DMAINTEN_NIE | ETH_DMAINTEN_RIE);
#ifdef CONFIG_MAC_ZERO_COPY_TXBUF
    tmpreg |= ETH_DMAINTEN_TIE;
#endif
    writel(tmpreg, MAC(port, dma0inten));
#endif
    return ETH_SUCCESS;
}

void aicmac_start(uint32_t port)
{
    /* Check the parameters */
    assert_param(port < MAX_ETH_MAC_PORT);

    /* Enable transmit state machine of the MAC for transmission on the MII */
    aicmac_set_mac_tx(port, ENABLE);

    /* Enable receive state machine of the MAC for reception from the MII */
    aicmac_set_mac_rx(port, ENABLE);

    /* Flush Transmit FIFO */
    aicmac_flush_tx_fifo(port);

    /* Start DMA transmission */
    aicmac_set_dma_tx(port, ENABLE);

    /* Start DMA reception */
    aicmac_set_dma_rx(port, ENABLE);
}

void aicmac_stop(uint32_t port)
{
    /* Check the parameters */
    assert_param(port < MAX_ETH_MAC_PORT);

    /* Stop DMA transmission */
    aicmac_set_dma_tx(port, DISABLE);

    /* Stop DMA reception */
    aicmac_set_dma_rx(port, DISABLE);

    /* Disable receive state machine of the MAC for reception from the MII */
    aicmac_set_mac_rx(port, DISABLE);

    /* Flush Transmit FIFO */
    aicmac_flush_tx_fifo(port);

    /* Disable transmit state machine of the MAC for transmission on the MII */
    aicmac_set_mac_tx(port, DISABLE);
}

void aicmac_set_mac_speed(uint32_t port, int speed)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, macconf));
    tmpreg &= ~ETH_MACCONF_SPEED_MSK;
    if (speed == SPEED_1000) {
        tmpreg |= ETH_MACCONF_SPEED_1000M;
    } else if (speed == SPEED_100) {
        tmpreg |= ETH_MACCONF_SPEED_100M;
    } else if (speed == SPEED_10) {
        tmpreg |= ETH_MACCONF_SPEED_10M;
    }
    writel(tmpreg, MAC(port, macconf));
}

void aicmac_set_mac_duplex(uint32_t port, bool state)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, macconf));
    if (state)
        tmpreg |= ETH_MACCONF_DM;
    else
        tmpreg &= ~ETH_MACCONF_DM;
    writel(tmpreg, MAC(port, macconf));
}

void aicmac_set_mac_pause(uint32_t port, bool state)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, flowctl));
    if (state)
        tmpreg |= (ETH_FLOWCTL_RFE | ETH_FLOWCTL_TFE);
    else
        tmpreg &= ~(ETH_FLOWCTL_RFE | ETH_FLOWCTL_TFE);
    writel(tmpreg, MAC(port, flowctl));
}

void aicmac_set_mac_tx(uint32_t port, bool state)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, mactxfunc));
    if (state)
        tmpreg |= ETH_MACTXFUNC_TE;
    else
        tmpreg &= ~ETH_MACTXFUNC_TE;
    writel(tmpreg, MAC(port, mactxfunc));
}

void aicmac_set_mac_rx(uint32_t port, bool state)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, macrxfunc));
    if (state)
        tmpreg |= ETH_MACRXFUNC_RE;
    else
        tmpreg &= ~ETH_MACRXFUNC_RE;
    writel(tmpreg, MAC(port, macrxfunc));
}

void aicmac_set_mac_addr(uint32_t port, uint32_t index, uint8_t *addr)
{
    uint32_t tmpreg = 0;

    /* Check the parameters */
    assert_param(index < ETH_MACADDR_MAX_INDEX);

    tmpreg = ((uint32_t)addr[5] << 8) | (uint32_t)addr[4];
    writel(tmpreg, MAC(port, macaddr0high) + (index * 8));
    tmpreg = ((uint32_t)addr[3] << 24) | ((uint32_t)addr[2] << 16) |
             ((uint32_t)addr[1] << 8) | addr[0];
    writel(tmpreg, MAC(port, macaddr0low) + (index * 8));
}

void aicmac_get_mac_addr(uint32_t port, uint32_t index, uint8_t *addr)
{
    uint32_t tmpreg = 0;

    /* Check the parameters */
    assert_param(index < ETH_MACADDR_MAX_INDEX);

    tmpreg = readl(MAC(port, macaddr0high) + (index * 8));
    addr[5] = ((tmpreg >> 8) & (uint8_t)0xFF);
    addr[4] = (tmpreg & (uint8_t)0xFF);
    tmpreg = readl(MAC(port, macaddr0low) + (index * 8));
    addr[3] = ((tmpreg >> 24) & (uint8_t)0xFF);
    addr[2] = ((tmpreg >> 16) & (uint8_t)0xFF);
    addr[1] = ((tmpreg >> 8) & (uint8_t)0xFF);
    addr[0] = (tmpreg & (uint8_t)0xFF);
}

void aicmac_set_mac_addr_mode(uint32_t port, uint32_t index, bool en, bool sa,
                              uint32_t mask)
{
    uint32_t tmpreg = 0;

    /* Check the parameters */
    assert_param((index < ETH_MACADDR_MAX_INDEX) && (index > 0));

    tmpreg = readl(MAC(port, macaddr0high) + (index * 8));

    if (en)
        tmpreg |= ETH_MACA1HR_AE;
    else
        tmpreg &= ~ETH_MACA1HR_AE;

    if (sa)
        tmpreg |= ETH_MACA1HR_SA;
    else
        tmpreg &= ~ETH_MACA1HR_SA;

    tmpreg &= ~ETH_MACA1HR_MBC_MMSK;
    tmpreg |= ((mask << ETH_MACA1HR_MBC_SHIFT) & ETH_MACA1HR_MBC_MMSK);

    writel(tmpreg, MAC(port, macaddr0high) + (index * 8));
}

void aicmac_en_dma_int(uint32_t port, uint32_t interrupt)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, dma0inten));
    tmpreg |= interrupt;
    writel(tmpreg, MAC(port, dma0inten));
}

void aicmac_dis_dma_int(uint32_t port, uint32_t interrupt)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, dma0inten));
    tmpreg &= ~interrupt;
    writel(tmpreg, MAC(port, dma0inten));
}

void aicmac_resume_dma_rx(uint32_t port)
{
    uint32_t tmpreg = 0;

#if 0
    /* When Rx Buffer unavailable flag is set: clear it and resume receive */
    tmpreg = readl(MAC(port, dma0intsts));
    if (tmpreg & ETH_DMAINTSTS_RU) {
        tmpreg |= ETH_DMAINTSTS_RU;
        writel(tmpreg, MAC(port, dma0intsts));

        tmpreg = readl(MAC(port, rxdma0ctl));
        tmpreg |= ETH_RXDMACTL_RPD;
        writel(tmpreg, MAC(port, rxdma0ctl));
    }
#else
    tmpreg = readl(MAC(port, rxdma0ctl));
    tmpreg |= ETH_RXDMACTL_RPD;
    writel(tmpreg, MAC(port, rxdma0ctl));
#endif

}

void aicmac_resume_dma_tx(uint32_t port)
{
    uint32_t tmpreg = 0;

#if 0
    tmpreg = readl(MAC(port, dma0intsts));
    if (tmpreg & ETH_DMAINTSTS_TU) {
        tmpreg |= ETH_DMAINTSTS_TU;
        writel(tmpreg, MAC(port, dma0intsts));

        tmpreg = readl(MAC(port, txdma0ctl));
        tmpreg |= ETH_TXDMACTL_TPD;
        writel(tmpreg, MAC(port, txdma0ctl));
    }
#else
    tmpreg = readl(MAC(port, txdma0ctl));
    tmpreg |= ETH_TXDMACTL_TPD;
    writel(tmpreg, MAC(port, txdma0ctl));
#endif


}

void aicmac_dma_rx_desc_init(uint32_t port)
{
    uint32_t i = 0;
    aicmac_dma_desc_t *rxdesc_tbl = &dctl[port].rx_desc_tbl[0];
    uint32_t count = ETH_RXBUFNB;
    aicmac_dma_desc_t *pdesc;
    #ifndef CONFIG_MAC_ZERO_COPY_RXBUF
    uint8_t *buff = &dctl[port].rx_buff[0][0];
    #else
    struct pbuf *p = NULL;
    #endif

    memset(rxdesc_tbl, 0, sizeof(aicmac_dma_desc_t) * count);
    for (i = 0; i < count; i++) {
        pdesc = rxdesc_tbl + i;

        /* Set Own bit of the Rx descriptor Status */
        pdesc->control = ETH_DMARxDesc_OWN;

        #ifndef CONFIG_MAC_ZERO_COPY_RXBUF
        /* Set Buffer1 size and Second Address Chained bit */
        pdesc->buff_size =
            ETH_DMARxDesc_RCH | (ETH_RX_BUF_SIZE & ETH_DMARxDesc_RBS1);
        /* Set Buffer1 address pointer */
        pdesc->buff1_addr =
            (uint32_t)(unsigned long)(&buff[i * ETH_RX_BUF_SIZE]);
        pdesc->reserved1 = i;
        #else
        /* get a pbuf from PBUF_POOL */
        p = pbuf_alloc(PBUF_RAW, 1, PBUF_POOL);
        if (p == NULL) {
            pr_err("%s pbuf_alloc fail!\n", __func__);
            return;
        }
        p->tot_len = AICMAC_PBUF_SIZE;
        p->len = AICMAC_PBUF_SIZE;
        dctl[port].rx_buff[i] = p;
        /* Set Buffer1 size and Second Address Chained bit */
        pdesc->buff_size =
            ETH_DMARxDesc_RCH | (AICMAC_PBUF_SIZE & ETH_DMARxDesc_RBS1);
        /* Set Buffer1 address pointer */
        pdesc->buff1_addr = (uint32_t)(unsigned long)(p->payload);
        pdesc->reserved1 = i;
        aicmac_dcache_clean((uintptr_t)p, sizeof(struct pbuf));
        #endif

        /* Initialize the next descriptor with the Next Descriptor Polling Enable */
        if (i < (count - 1)) {
            /* Set next descriptor address register with next descriptor base address */
            pdesc->buff2_addr =
                (uint32_t)(unsigned long)(rxdesc_tbl + i + 1);
        } else {
            /* For last descriptor, set next descriptor address register equal to the first descriptor base address */
            pdesc->buff2_addr = (uint32_t)(unsigned long)rxdesc_tbl;
        }
    }
    /* after write: flush cache */
    aicmac_dcache_clean((uintptr_t)rxdesc_tbl, sizeof(aicmac_dma_desc_t) * count);

    writel((uint32_t)(unsigned long)rxdesc_tbl, MAC(port, rxdma0descstart));

    dctl[port].rx_desc_p = rxdesc_tbl;
    dctl[port].rx_desc_received_p = rxdesc_tbl;
    dctl[port].rx_desc_unrelease_p = rxdesc_tbl;
    dctl[port].rx_frame_info_p = &dctl[port].rx_frame_info;
}

void aicmac_dma_tx_desc_init(uint32_t port)
{
    uint32_t i = 0;
    aicmac_dma_desc_t *txdesc_tbl = &dctl[port].tx_desc_tbl[0];
    #ifndef CONFIG_MAC_ZERO_COPY_TXBUF
    uint8_t *buff = &dctl[port].tx_buff[0][0];
    #endif
    uint32_t count = ETH_TXBUFNB;
    aicmac_dma_desc_t *pdesc;

    memset(txdesc_tbl, 0, sizeof(aicmac_dma_desc_t) * count);
    for (i = 0; i < count; i++) {
        pdesc = txdesc_tbl + i;

        /* Set Second Address Chained bit */
        pdesc->control = ETH_DMATxDesc_TCH;

        #ifndef CONFIG_MAC_ZERO_COPY_TXBUF
        /* Set Buffer1 address pointer */
        pdesc->buff1_addr =
            (uint32_t)(unsigned long)(&buff[i * ETH_TX_BUF_SIZE]);
        #else
        pdesc->reserved1 = i;
        dctl[port].tx_buff[i] = NULL;
        #endif

        /* Initialize the next descriptor with the Next Descriptor Polling Enable */
        if (i < (count - 1)) {
            /* Set next descriptor address register with next descriptor base address */
            pdesc->buff2_addr =
                (uint32_t)(unsigned long)(txdesc_tbl + i + 1);
        } else {
            /* For last descriptor, set next descriptor address register equal to the first descriptor base address */
            pdesc->buff2_addr = (uint32_t)(unsigned long)txdesc_tbl;
        }
    }
    /* after write: flush cache */
    aicmac_dcache_clean((uintptr_t)txdesc_tbl, sizeof(aicmac_dma_desc_t) * count);

    writel((uint32_t)(unsigned long)txdesc_tbl, MAC(port, txdma0descstart));

    dctl[port].tx_desc_p = txdesc_tbl;
    #ifdef CONFIG_MAC_ZERO_COPY_TXBUF
    dctl[port].tx_desc_unconfirm_p = txdesc_tbl;
    #endif
}

uint32_t aicmac_check_rx_frame_poll(uint32_t port)
{
    aicmac_dma_desc_t *pdesc = dctl[port].rx_desc_p;
    aicmac_rx_frame_info_t *pinfo = dctl[port].rx_frame_info_p;

    /* before read: invalid cache */
    aicmac_dcache_invalid((uintptr_t)pdesc, sizeof(aicmac_dma_desc_t));

    /* check if last segment */
    if (((pdesc->control & ETH_DMARxDesc_OWN) == (uint32_t)RESET)
        && ((pdesc->control & ETH_DMARxDesc_LS) != (uint32_t)RESET)) {

        if ((pdesc->control & ETH_DMARxDesc_FS) != (uint32_t)RESET)
            pinfo->seg_cnt = 1;
        else
            pinfo->seg_cnt++;

        if (pinfo->seg_cnt == 1) {
            pinfo->first_desc = pdesc;
        }
        pinfo->last_desc = pdesc;
        return 1;

        /* check if first segment */
    } else if (((pdesc->control & ETH_DMARxDesc_OWN) == (uint32_t)RESET)
               && ((pdesc->control & ETH_DMARxDesc_FS) != (uint32_t)RESET)
               && ((pdesc->control & ETH_DMARxDesc_LS) == (uint32_t)RESET)) {
        pinfo->first_desc = pdesc;
        pinfo->last_desc = NULL;
        pinfo->seg_cnt = 1;
        dctl[port].rx_desc_p =
            (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);

        /* check if intermediate segment */
    } else if (((pdesc->control & ETH_DMARxDesc_OWN) == (uint32_t)RESET) &&
               ((pdesc->control & ETH_DMARxDesc_FS) == (uint32_t)RESET) &&
               ((pdesc->control & ETH_DMARxDesc_LS) == (uint32_t)RESET)) {
        (pinfo->seg_cnt)++;
        dctl[port].rx_desc_p =
            (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
    }
    return 0;
}

aicmac_frame_t aicmac_get_rx_frame_poll(uint32_t port)
{
    aicmac_dma_desc_t *pdesc = dctl[port].rx_desc_p;
    aicmac_rx_frame_info_t *pinfo = dctl[port].rx_frame_info_p;
    uint32_t framelength = 0;
    aicmac_frame_t frame = { 0, 0, 0 };

    /* Get the Frame Length of the received packet: substruct 4 bytes of the CRC */
    framelength = ((pdesc->control & ETH_DMARxDesc_FL) >>
                   ETH_DMARxDesc_FL_SHIFT) - 4;
    frame.length = framelength;

    /* Get the address of the first frame descriptor and the buffer start address */
    frame.descriptor = pinfo->first_desc;
    frame.buffer = (pinfo->first_desc)->buff1_addr;
    frame.last_desc = pinfo->last_desc;

    /* Update the ETHERNET DMA global Rx descriptor with next Rx descriptor */
    /* Chained Mode */
    /* Selects the next DMA Rx descriptor list for next buffer to read */
    dctl[port].rx_desc_p =
        (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);

    /* Return Frame */
    return (frame);
}

aicmac_frame_t aicmac_get_rx_frame_interrupt(uint32_t port)
{
    aicmac_dma_desc_t *pdesc = dctl[port].rx_desc_p;
    aicmac_rx_frame_info_t *pinfo = dctl[port].rx_frame_info_p;
    aicmac_frame_t frame = { 0, 0, 0, 0 };
    uint32_t descriptor_scan_counter = 0;

    /* before read: invalid cache */
    aicmac_dcache_invalid((uintptr_t)pdesc, sizeof(aicmac_dma_desc_t));

    /* scan descriptors owned by CPU */
    while (((pdesc->control & ETH_DMARxDesc_OWN) == (uint32_t)RESET) &&
           (descriptor_scan_counter < ETH_RXBUFNB)) {
        /* Just by security */
        descriptor_scan_counter++;

        /* check if first segment in frame */
        if (((pdesc->control & ETH_DMARxDesc_FS) != (uint32_t)RESET) &&
            ((pdesc->control & ETH_DMARxDesc_LS) == (uint32_t)RESET)) {
            pinfo->first_desc = pdesc;
            pinfo->seg_cnt = 1;
            pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
            dctl[port].rx_desc_p = pdesc;

            /* check if intermediate segment */
        } else if (((pdesc->control & ETH_DMARxDesc_LS) == (uint32_t)RESET) &&
                   ((pdesc->control & ETH_DMARxDesc_FS) == (uint32_t)RESET)) {
            (pinfo->seg_cnt)++;
            pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
            dctl[port].rx_desc_p = pdesc;

            /* should be last segment */
        } else {
            /* last segment */
            pinfo->last_desc = pdesc;
            (pinfo->seg_cnt)++;

            /* first segment is last segment */
            if ((pinfo->seg_cnt) == 1)
                pinfo->first_desc = pdesc;

            /* Get the Frame Length of the received packet: substruct 4 bytes of the CRC */
            frame.length = ((pdesc->control & ETH_DMARxDesc_FL) >>
                            ETH_DMARxDesc_FL_SHIFT) -
                           4;

            /* Get the address of the buffer start address */
            /* Check if more than one segment in the frame */
            if (pinfo->seg_cnt > 1)
                frame.buffer = (pinfo->first_desc)->buff1_addr;
            else
                frame.buffer = pdesc->buff1_addr;

            frame.descriptor = pinfo->first_desc;
            frame.last_desc = pinfo->last_desc;

            /* Update the ETHERNET DMA global Rx descriptor with next Rx descriptor */
            pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
            dctl[port].rx_desc_p = pdesc;

            /* Return Frame */
            return (frame);
        }

        /* before read: invalid cache */
        aicmac_dcache_invalid((uintptr_t)pdesc, sizeof(aicmac_dma_desc_t));
    }
    return (frame);
}

void aicmac_release_rx_frame(uint32_t port)
{
    aicmac_dma_desc_t *pdesc = dctl[port].rx_desc_unrelease_p;
    aicmac_dma_desc_t *pdesc_end = dctl[port].rx_desc_received_p;
    #ifdef CONFIG_MAC_ZERO_COPY_RXBUF
    uint32_t index = 0;
    struct pbuf *p = NULL;
    #endif

    /* Set Own bit in Rx descriptors: gives the buffers back to DMA */
    while(pdesc != pdesc_end) {
        #ifdef CONFIG_MAC_ZERO_COPY_RXBUF
        index = pdesc->reserved1;
        if (index >= ETH_RXBUFNB) {
            pr_err("%s get dma desc index err.\n", __func__);
            return;
        }
        /* get a pbuf from PBUF_POOL */
        p = pbuf_alloc(PBUF_RAW, 1, PBUF_POOL);
        if (p == NULL) {
            pr_info("%s rx pbuf_alloc fail!\n", __func__);
            dctl[port].rx_buf_underrun = 1;
            return;
        }
        p->tot_len = AICMAC_PBUF_SIZE;
        p->len = AICMAC_PBUF_SIZE;
        dctl[port].rx_buff[index] = p;
        /* Set Buffer1 size and Second Address Chained bit */
        pdesc->buff_size =
            ETH_DMARxDesc_RCH | (AICMAC_PBUF_SIZE & ETH_DMARxDesc_RBS1);
        /* Set Buffer1 address pointer */
        pdesc->buff1_addr =
            (uint32_t)(unsigned long)(p->payload);

        /* after write: flush cache */
        aicmac_dcache_clean((uintptr_t)&pdesc->buff_size, 2*sizeof(uint32_t));
        aicmac_dcache_clean((uintptr_t)p, sizeof(struct pbuf));
        #endif

        pdesc->control = ETH_DMARxDesc_OWN;
        /* after write: flush cache */
        aicmac_dcache_clean((uintptr_t)&pdesc->control, sizeof(uint32_t));

        /* Point to next descriptor */
        pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
        dctl[port].rx_desc_unrelease_p = pdesc;
    }

    #ifdef CONFIG_MAC_ZERO_COPY_RXBUF
    if (dctl[port].rx_buf_underrun) {
        pr_info("%s rx pbuf underrun resume!\n", __func__);
        dctl[port].rx_buf_underrun = 0;
    }
    #endif

    /* Resume DMA receive */
    aicmac_resume_dma_rx(port);
}

int aicmac_submit_tx_frame(uint32_t port, u16 frame_len)
{
    uint32_t buf_count = 0, size = 0, i = 0;
    uint32_t tmpreg = 0;
    aicmac_dma_desc_t *pdesc = dctl[port].tx_desc_p;

    /* Check if the descriptor is owned by the ETHERNET DMA (when set) or CPU (when reset) */
    if ((pdesc->control & ETH_DMATxDesc_OWN) != (u32)RESET) {
        /* Return ERROR: OWN bit set */
        return ETH_ERROR;
    }

    if (frame_len > ETH_TX_BUF_SIZE) {
        buf_count = frame_len / ETH_TX_BUF_SIZE;
        if (frame_len % ETH_TX_BUF_SIZE)
            buf_count++;
    } else {
        buf_count = 1;
    }

    if (buf_count == 1) {
        /* Set frame size */
        pdesc->buff_size = (frame_len & ETH_DMATxDesc_TBS1);
        /* after write: flush cache */
        aicmac_dcache_clean((uintptr_t)&pdesc->buff_size, sizeof(uint32_t));
        /*set LAST and FIRST segment */
        //tmpreg = pdesc->control;
        tmpreg =  ETH_DMATxDesc_TCH;
        tmpreg |= ETH_DMATxDesc_FS | ETH_DMATxDesc_LS;
        /* TCP/IP Tx Checksum Insertion */
        if (mac_config[port].coe_tx)
            tmpreg |= ETH_DMATxDesc_CIC_TCPUDPICMP_Full;
        /* Set Own bit of the Tx descriptor Status: gives the buffer back to ETHERNET DMA */
        tmpreg |= ETH_DMATxDesc_OWN;
        pdesc->control = tmpreg;
        /* after write: flush cache */
        aicmac_dcache_clean((uintptr_t)&pdesc->control, sizeof(uint32_t));
        /* update */
        pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
    } else {
        for (i = 0; i < buf_count; i++) {
            /* Clear FIRST and LAST segment bits */
            pdesc->control &= ~(ETH_DMATxDesc_FS | ETH_DMATxDesc_LS);

            if (i == 0) {
                /* Setting the first segment bit */
                pdesc->control |= ETH_DMATxDesc_FS;
            }

            /* Program size */
            pdesc->buff_size = (ETH_TX_BUF_SIZE & ETH_DMATxDesc_TBS1);

            if (i == (buf_count - 1)) {
                /* Setting the last segment bit */
                pdesc->control |= ETH_DMATxDesc_LS;
                size = frame_len - (buf_count - 1) * ETH_TX_BUF_SIZE;
                pdesc->buff_size = (size & ETH_DMATxDesc_TBS1);
            }
            /* after write: flush cache */
            aicmac_dcache_clean((uintptr_t)&pdesc->buff_size, sizeof(uint32_t));

            /* TCP/IP Tx Checksum Insertion */
            if (mac_config[port].coe_tx)
                tmpreg |= ETH_DMATxDesc_CIC_TCPUDPICMP_Full;

            /* Set Own bit of the Tx descriptor Status: gives the buffer back to ETHERNET DMA */
            pdesc->control |= ETH_DMATxDesc_OWN;

            /* after write: flush cache */
            aicmac_dcache_clean((uintptr_t)&pdesc->control, sizeof(uint32_t));
            /* update */
            pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
        }
    }

    dctl[port].tx_desc_p = pdesc;

    /* Resume DMA transmission */
    aicmac_resume_dma_tx(port);

    /* Return SUCCESS */
    return ETH_SUCCESS;
}

void aicmac_confirm_tx_frame(uint32_t port)
{
#ifdef CONFIG_MAC_ZERO_COPY_TXBUF
    aicmac_dma_desc_t *pdesc = dctl[port].tx_desc_unconfirm_p;
    aicmac_dma_desc_t *pdesc_end = dctl[port].tx_desc_p;
    struct pbuf *p;
    uint32_t index;

    if (pdesc == pdesc_end)
        return;

    /* before read: invalid cache */
    aicmac_dcache_invalid((uintptr_t)pdesc, sizeof(aicmac_dma_desc_t));

    /* scan descriptors owned by CPU */
    while ((pdesc != pdesc_end) &&
            ((pdesc->control & ETH_DMARxDesc_OWN) == (uint32_t)RESET)) {

        index = pdesc->reserved1;
        if (index >= ETH_RXBUFNB) {
            pr_err("%s get dma desc index err.\n", __func__);
            return;
        }

        p = dctl[port].tx_buff[index];
        if (p != NULL) {
            pbuf_free(p);
            dctl[port].tx_buff[index] = NULL;
        }

        /* next pointer */
        pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
        /* before read: invalid cache */
        aicmac_dcache_invalid((uintptr_t)pdesc, sizeof(aicmac_dma_desc_t));
    }

    dctl[port].tx_desc_unconfirm_p = pdesc;

    /* Resume DMA transmission */
    aicmac_resume_dma_tx(port);
#endif
}

void aicmac_set_dma_tx_desc_int(uint32_t port, bool en)
{
    aicmac_dma_desc_t *prxdesc;
    uint32_t count = ETH_TXBUFNB;
    uint32_t i;

    for (i = 0; i < count; i++) {
        prxdesc = &dctl[port].tx_desc_tbl[i];
        if (en) {
            /* Enable the DMA Tx Desc Transmit interrupt */
            prxdesc->control |= ETH_DMATxDesc_IC;
        } else {
            /* Disable the DMA Tx Desc Transmit interrupt */
            prxdesc->control &= (~(uint32_t)ETH_DMATxDesc_IC);
        }
    }

    /* after write: flush cache */
    aicmac_dcache_clean((uintptr_t)prxdesc, sizeof(aicmac_dma_desc_t) * count);
}

void aicmac_set_dma_rx_desc_int(uint32_t port, bool en)
{
    aicmac_dma_desc_t *prxdesc;
    uint32_t count = ETH_RXBUFNB;
    uint32_t i;

    for (i = 0; i < count; i++) {
        prxdesc = &dctl[port].rx_desc_tbl[i];
        if (en) {
            /* Enable the DMA Rx Desc receive interrupt */
            prxdesc->buff_size &= (~(uint32_t)ETH_DMARxDesc_DIC);
        } else {
            /* Disable the DMA Rx Desc receive interrupt */
            prxdesc->buff_size |= ETH_DMARxDesc_DIC;
        }
    }

    /* after write: flush cache */
    aicmac_dcache_clean((uintptr_t)prxdesc, sizeof(aicmac_dma_desc_t) * count);
}

void aicmac_sw_reset(uint32_t port)
{
    uint32_t tmpreg = 0;
    uint32_t timeout = 0;

    tmpreg = readl(MAC(port, macconf));
    tmpreg |= ETH_MACCONF_SWR;
    writel(tmpreg, MAC(port, macconf));

    /* Check for the Busy flag */
    do {
        timeout++;
        tmpreg = readl(MAC(port, macconf));
    } while ((tmpreg & ETH_MACCONF_SWR) && (timeout < (uint32_t)PHY_WRITE_TO));

    if (timeout == PHY_WRITE_TO) {
        pr_err("aicmac software reset timeout.\n");
    }
}

bool aicmac_get_sw_reset_status(uint32_t port)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, macconf));
    if (tmpreg & ETH_MACCONF_SWR)
        return SET;
    else
        return RESET;
}

bool aicmac_get_dma_int_status(uint32_t port, uint32_t flag)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, dma0intsts));
    if (tmpreg & flag)
        return SET;
    else
        return RESET;
}

void aicmac_clear_dma_int_pending(uint32_t port, uint32_t flag)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, dma0intsts));
    tmpreg |= flag;
    writel(tmpreg, MAC(port, dma0intsts));
}

uint32_t aicmac_get_dma_tx_state(uint32_t port)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, dma0intsts));
    tmpreg = (tmpreg & ETH_DMAINTSTS_TS_MSK) >> ETH_DMAINTSTS_TS_SHIFT;
    return tmpreg;
}

uint32_t aicmac_get_dma_rx_state(uint32_t port)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, dma0intsts));
    tmpreg = (tmpreg & ETH_DMAINTSTS_RS_MSK) >> ETH_DMAINTSTS_RS_SHIFT;
    return tmpreg;
}

void aicmac_flush_tx_fifo(uint32_t port)
{
    uint32_t tmpreg = 0;
    uint32_t timeout = 0;

    tmpreg = readl(MAC(port, txdma0ctl));
    tmpreg |= ETH_TXDMACTL_FTF;
    writel(tmpreg, MAC(port, txdma0ctl));

    /* Check for the Busy flag */
    do {
        timeout++;
        tmpreg = readl(MAC(port, txdma0ctl));
    } while ((tmpreg & ETH_TXDMACTL_FTF) && (timeout < (uint32_t)PHY_WRITE_TO));
}

bool aicmac_get_flush_tx_fifo_status(uint32_t port)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, txdma0ctl));
    if (tmpreg & ETH_TXDMACTL_FTF)
        return SET;
    else
        return RESET;
}

void aicmac_set_dma_tx(uint32_t port, bool state)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, txdma0ctl));
    if (state)
        tmpreg |= ETH_TXDMACTL_ST;
    else
        tmpreg &= ~ETH_TXDMACTL_ST;
    writel(tmpreg, MAC(port, txdma0ctl));
}

void aicmac_set_dma_rx(uint32_t port, bool state)
{
    uint32_t tmpreg = 0;

    tmpreg = readl(MAC(port, rxdma0ctl));
    if (state)
        tmpreg |= ETH_RXDMACTL_SR;
    else
        tmpreg &= ~ETH_RXDMACTL_SR;
    writel(tmpreg, MAC(port, rxdma0ctl));
}

int aicmac_read_phy_reg(uint32_t port, uint32_t addr, uint16_t *val)
{
    uint32_t tmpreg = 0;
    uint32_t timeout = 0;

    tmpreg = readl(MAC(port, mdioctl));
    tmpreg &= ETH_MDIOCTL_CR_MSK;
    tmpreg |= (((uint32_t)mac_config[port].phyaddr << 11) &
               ETH_MDIOCTL_PA); /* Set the PHY device address */
    tmpreg |= (((uint32_t)addr << 6) &
               ETH_MDIOCTL_MR); /* Set the PHY register address */
    tmpreg &= ~ETH_MDIOCTL_MW;  /* Set the read mode */
    tmpreg |= ETH_MDIOCTL_MB;   /* Set the MII Busy bit */
    writel(tmpreg, MAC(port, mdioctl));

    /* Check for the Busy flag */
    do {
        timeout++;
        tmpreg = readl(MAC(port, mdioctl));
    } while ((tmpreg & ETH_MDIOCTL_MB) && (timeout < (uint32_t)PHY_WRITE_TO));

    if (timeout == PHY_WRITE_TO) {
        pr_err("aicmac port%d read phy %d reg %d timeout.\n", (int)port,
               (int)mac_config[port].phyaddr, (int)addr);
        return ETH_ERROR;
    }

    tmpreg = readl(MAC(port, mdiodata));
    *val = tmpreg & 0xFFFF;
    return ETH_SUCCESS;
}

int aicmac_write_phy_reg(uint32_t port, uint32_t addr, uint16_t val)
{
    uint32_t tmpreg = 0;
    uint32_t timeout = 0;

    tmpreg = readl(MAC(port, mdioctl));
    tmpreg &= ETH_MDIOCTL_CR_MSK;
    tmpreg |= (((uint32_t)mac_config[port].phyaddr << 11) &
               ETH_MDIOCTL_PA); /* Set the PHY device address */
    tmpreg |= (((uint32_t)addr << 6) &
               ETH_MDIOCTL_MR); /* Set the PHY register address */
    tmpreg |= ETH_MDIOCTL_MW;   /* Set the write mode */
    tmpreg |= ETH_MDIOCTL_MB;   /* Set the MII Busy bit */
    writel(val, MAC(port, mdiodata));
    writel(tmpreg, MAC(port, mdioctl));

    /* Check for the Busy flag */
    do {
        timeout++;
        tmpreg = readl(MAC(port, mdioctl));
    } while ((tmpreg & ETH_MDIOCTL_MB) && (timeout < (uint32_t)PHY_WRITE_TO));

    if (timeout == PHY_WRITE_TO) {
        pr_err("aicmac port%d write phy %d reg %d timeout.\n", (int)port,
               (int)mac_config[port].phyaddr, (int)addr);
        return ETH_ERROR;
    }

    return ETH_SUCCESS;
}

