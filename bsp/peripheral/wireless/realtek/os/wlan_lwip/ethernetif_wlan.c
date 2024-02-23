/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "netif/etharp.h"
#include "lwip/tcpip.h"
#include "ethernetif_wlan.h"
#include "autoconf.h"
#include "net_stack_intf.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"

/* Define those to better describe your network interface. */
#define WLAN_NAME0 'W'
#define WLAN_NAME1 'L'

#define WLAN_MTU       1500
#define MAX_ETH_DRV_SG 16
#define MAX_ETH_MSG    (WLAN_MTU + 18)

struct netif xnetif[NET_IF_NUM];

static err_t wlan_low_level_output(struct netif *netif, struct pbuf *p)
{
    /* initialize the one struct eth_drv_sg array */
    struct eth_drv_sg sg_list[MAX_ETH_DRV_SG];
    int sg_len = 0;
    struct pbuf *q;

    if(!rltk_wlan_running(netif_get_idx(netif)))
        return ERR_IF;

    /* packet is stored in one list composed by several pbuf. */
    for (q = p; q != NULL && sg_len < MAX_ETH_DRV_SG; q = q->next) {
        sg_list[sg_len].buf = (unsigned int) q->payload;
        sg_list[sg_len++].len = q->len;
    }

    if (sg_len)
        rltk_wlan_send(netif_get_idx(netif), sg_list, sg_len, p->tot_len);

    return ERR_OK;
}

void ethernetif_recv(struct netif *netif, int total_len)
{
    int errcode;

    struct eth_drv_sg sg_list[MAX_ETH_DRV_SG];
    struct pbuf *p, *q;
    int sg_len = 0;

    /* WIFI chip is running */
    if(!rltk_wlan_running(netif_get_idx(netif)))
        return;

    if ((total_len > MAX_ETH_MSG) || (total_len < 0))
        total_len = MAX_ETH_MSG;

    /* Allocate buffer to store received packet */
    p = pbuf_alloc(PBUF_RAW, total_len, PBUF_POOL);
    if (p == NULL) {
        rt_kprintf("\n\rCannot allocate pbuf to receive packet length %d,l%d.\n", total_len,__LINE__);
        return;
    }

    /* Create scatter list */
    for (q = p; q != NULL && sg_len < MAX_ETH_DRV_SG; q = q->next) {
        sg_list[sg_len].buf = (unsigned int) q->payload;
        sg_list[sg_len++].len = q->len;
    }

    /* Copy received packet to scatter list from wrapper RX skb */
    rltk_wlan_recv(netif_get_idx(netif), sg_list, sg_len);

    /* Pass received packet to the interface */
    errcode = netif->input(p, netif);
    if (ERR_OK != errcode) {
        rt_kprintf("netif->input error.code=%d.\n", errcode);
        pbuf_free(p);
    }
}

extern int realtek_init(void);
err_t wlan_ethernetif_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "wlan";
#endif /* LWIP_NETIF_HOSTNAME */

    netif->name[0] = WLAN_NAME0;
    netif->name[1] = WLAN_NAME1;

    netif->mtu = WLAN_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;

    /* set netif MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    netif->output = etharp_output;
    netif->linkoutput = wlan_low_level_output;

    /* initialize the hardware */
    realtek_init();

    return ERR_OK;
}

#if defined(AIC_USING_REALTEK_WLAN0) && NO_SYS
#error "WLAN should working with OS\n"
#endif

#if !LWIP_IPV4
int init_wlan_netif(void)
#else
int init_wlan_netif(ip4_addr_t *ipaddr, ip4_addr_t *mask, ip4_addr_t *gw)
#endif
{
    struct netif *netif = &xnetif[0];

#ifdef LWIP_IPV4
    netif_add(netif, ipaddr, mask, gw, NULL, wlan_ethernetif_init, tcpip_input);
#else
    netif_add(netif, NULL, wlan_ethernetif_init, tcpip_input);
#endif

#if LWIP_NETIF_LINK_CALLBACK
    netif_set_link_callback(netif, link_callback);
#endif /* LWIP_NETIF_LINK_CALLBACK */

    netif_set_up(netif);

    return 0;
}

#pragma GCC diagnostic pop

