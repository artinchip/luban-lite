#ifndef __ETHERNETIF_WLAN_H
#define __ETHERNETIF_WLAN_H

struct eth_drv_sg {
    unsigned int	buf;
    unsigned int 	len;
};

#if !LWIP_IPV4
int init_wlan_netif(void);
#else
int init_wlan_netif(ip4_addr_t *ipaddr, ip4_addr_t *mask, ip4_addr_t *gw);
#endif

#endif /* __ETHERNETIF_WLAN_H */
