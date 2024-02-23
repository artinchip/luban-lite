/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lwip/opt.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "lwip/ip4.h"
#include "netif_startup.h"
#include <aic_core.h>

#ifdef AIC_USING_REALTEK_WLAN0
#include "ethernetif_wlan.h"
#endif

void ifconfig(void)
{
    struct netif *netif;

    for (netif = netif_list; netif != NULL; netif = netif->next) {
        printf("%s:\n",netif->name);
        printf("    IPv4 Address   : %s\n", ip4addr_ntoa(&netif->ip_addr));
        printf("    Default Gateway: %s\n", ip4addr_ntoa(&netif->gw));
        printf("    Subnet mask    : %s\n", ip4addr_ntoa(&netif->netmask));
        printf("    MAC addr       : %02x:%02x:%02x:%02x:%02x:%02x\n",
                netif->hwaddr[0],
                netif->hwaddr[1],
                netif->hwaddr[2],
                netif->hwaddr[3],
                netif->hwaddr[4],
                netif->hwaddr[5]);
    }
}

#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(ifconfig, list all net information);
#endif

#if defined(AIC_USING_GMAC0) | defined(AIC_USING_GMAC1)
#include "ethernetif.h"
#include "aic_mac.h"

aicmac_netif_t aic_netif;

#if LWIP_IPV4
void init_gmac_netif(const ip4_addr_t *ipaddr, const ip4_addr_t *netmask, const ip4_addr_t *gw)
#else
void init_gmac_netif(void)
#endif
{
    /* Select aic mac port num */
    aic_netif.port = 0;
    /* ethernet init */
#ifdef AIC_USING_GMAC0
#if LWIP_IPV4

#if NO_SYS
    netif_add(&aic_netif.netif, ipaddr, netmask, gw, NULL, ethernetif_init, netif_input);
#else /* NO_SYS */
    netif_add(&aic_netif.netif, ipaddr, netmask, gw, NULL, ethernetif_init, tcpip_input);
#endif /* !NO_SYS */

#else /* LWIP_IPV4 */

#if NO_SYS
    netif_add(&aic_netif.netif, NULL, ethernetif_init, netif_input);
#else
    netif_add(&aic_netif.netif, NULL, ethernetif_init, tcpip_input);
#endif /* !NO_SYS */
#endif /* !LWIP_IPV4 */
    netif_set_up(&aic_netif.netif);
#endif /* AIC_USING_GMAC0 */
}
#endif /* defined(AIC_USING_GMAC0) | defined(AIC_USING_GMAC1) */

void netif_startup(void)
{
    struct netif *netif;
#if LWIP_IPV4
    ip4_addr_t ipaddr, netmask, gw;

    ip4_addr_set_zero(&gw);
    ip4_addr_set_zero(&ipaddr);
    ip4_addr_set_zero(&netmask);
#endif

#ifdef AIC_USING_GMAC0
#if LWIP_IPV4
#if !LWIP_DHCP
    ip4addr_aton(AIC_DEV_GMAC0_GW, &gw);
    ip4addr_aton(AIC_DEV_GMAC0_IPADDR, &ipaddr);
    ip4addr_aton(AIC_DEV_GMAC0_NETMASK, &netmask);
#endif
    init_gmac_netif(&ipaddr, &netmask, &gw);
#else
    init_gmac_netif();
#endif /* LWIP_IPV4 */
#endif /* WLAN */

#ifdef AIC_USING_REALTEK_WLAN0
#if LWIP_IPV4
#if !LWIP_DHCP
    ip4addr_aton(AIC_DEV_REALTEK_WLAN0_GW, &gw);
    ip4addr_aton(AIC_DEV_REALTEK_WLAN0_IPADDR, &ipaddr);
    ip4addr_aton(AIC_DEV_REALTEK_WLAN0_NETMASK, &netmask);
#endif /* !LWIP_DHCP */
    /* wlan init */
    init_wlan_netif(&ipaddr, &netmask, &gw);
#else
    init_wlan_netif();
#endif /* LWIP_IPV4 */
#endif /* AIC_USING_REALTEK_WLAN0 */

    if (netif_list == NULL) {
        printf("LwIP startup error!, Make sure you have enabled gmac or wlan\n");
        return;
    }

    /* the first netif as the default netif */
    for (netif = netif_list; netif->next != NULL; netif = netif->next) {

    }

    ifconfig();

#if LWIP_IGMP
    ip4_set_default_multicast_netif(netif);
#endif
    netif_set_default(netif);
}

#if NO_SYS
extern void aic_phy_poll(void);
void netif_baremetal_poll(void)
{
    struct netif *netif;

    aic_phy_poll();

    for (netif = netif_list; netif != NULL; netif = netif->next) {
        if (netif_is_link_up(netif)) {
            ethernetif_input_poll();
        }
    }
}
#endif
