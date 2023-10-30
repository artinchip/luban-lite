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
#include <aic_core.h>
#include "ethernetif.h"
#include "aic_mac.h"
#include "aic_phy.h"

#define NETIF_MTU                       (1500)
#define NETIF_RX_TASK_STACK_SIZE        (1024)
#define NETIF_RX_TASK_PRIORITY          (TCPIP_THREAD_PRIO-1)

/* Define those to better describe your network interface. */
#define IFNAME0 'a'
#define IFNAME1 'i'

#define ETH_EVENT_RX_COMPLETE   0x1
#define ETH_EVENT_TX_COMPLETE   0x2

aicos_mutex_t eth_tx_mutex = NULL;
aicos_event_t eth_rx_event = NULL;

extern aicmac_netif_t aic_netif;
extern unsigned long mac_base[MAX_ETH_MAC_PORT];
extern aicmac_dma_desc_ctl_t dctl[MAX_ETH_MAC_PORT];
extern unsigned long mac_irq[MAX_ETH_MAC_PORT];
extern aicmac_config_t mac_config[MAX_ETH_MAC_PORT];
#if !NO_SYS
static void ethernetif_input_thread(void *pvParameters);
#endif
static int hexchar2int(char c)
{
    if (c >= '0' && c <= '9') return (c - '0');
    if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (c - 'a' + 10);
    return 0;
}

static void hex2bin(unsigned char *bin, char *hex, int binlength)
{
    int i = 0;

    if ((strlen(hex) > binlength*2) || (strlen(hex)%2))
        return;

    for (i = 0; i < strlen(hex); i += 2) {
        bin[i / 2] = (char)((hexchar2int(hex[i]) << 4)
            | hexchar2int(hex[i + 1]));
    }
}

void aicmac_interrupt(void)
//void aicmac_interrupt(void * arg)
{
    pr_debug("%s\n", __func__);

    /* Frame received */
    if (aicmac_get_dma_int_status(aic_netif.port, ETH_DMAINTSTS_RI) == SET) {
        /* Clear pending bit */
        aicmac_clear_dma_int_pending(aic_netif.port, ETH_DMAINTSTS_RI);

#if !NO_SYS
        /* Disable interrupt until task complete handle */
        aicmac_dis_dma_int(aic_netif.port, ETH_DMAINTEN_RIE);
        /* Give the semaphore to wakeup LwIP task */
        aicos_event_send(eth_rx_event, ETH_EVENT_RX_COMPLETE);
#endif
    }

    /* Frame transmit complete */
    #ifdef CONFIG_MAC_ZERO_COPY_TXBUF
    if (aicmac_get_dma_int_status(aic_netif.port, ETH_DMAINTSTS_TI) == SET) {
        /* Disable interrupt until task complete handle */
        aicmac_dis_dma_int(aic_netif.port, ETH_DMAINTEN_TIE);
        /* Clear pending bit */
        aicmac_clear_dma_int_pending(aic_netif.port, ETH_DMAINTSTS_TI);
        /* Give the semaphore to wakeup LwIP task */
        aicos_event_send(eth_rx_event, ETH_EVENT_TX_COMPLETE);
    }
    #endif

    /* Clear the interrupt flags. */
    /* Clear the Eth DMA Rx IT pending bits */
    aicmac_clear_dma_int_pending(aic_netif.port, ETH_DMAINTSTS_NIS);
}

static void low_level_init(struct netif *netif)
{
    aicmac_netif_t *aic_netif = (aicmac_netif_t *)netif;

    /* set netif MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* set netif MAC hardware address */
    netif->hwaddr[0] = CONFIG_PORT_MAC_ADDR_LOW & 0xFF;
    netif->hwaddr[1] = (CONFIG_PORT_MAC_ADDR_LOW >> 8) & 0xFF;
    netif->hwaddr[2] = (CONFIG_PORT_MAC_ADDR_LOW >> 16) & 0xFF;
    netif->hwaddr[3] = (CONFIG_PORT_MAC_ADDR_LOW >> 24) & 0xFF;
    netif->hwaddr[4] = CONFIG_PORT_MAC_ADDR_HIGH & 0xFF;
    netif->hwaddr[5] = (CONFIG_PORT_MAC_ADDR_HIGH >> 8) & 0xFF;

#ifdef AIC_DEV_GMAC0_MACADDR
    if (aic_netif->port == 0){
        hex2bin(netif->hwaddr, AIC_DEV_GMAC0_MACADDR, 6);
    }
#endif

#ifdef AIC_DEV_GMAC1_MACADDR
    if (aic_netif->port == 1){
        hex2bin(netif->hwaddr, AIC_DEV_GMAC1_MACADDR, 6);
    }
#endif

    /* set netif maximum transfer unit */
    netif->mtu = NETIF_MTU;

    /* Accept broadcast address and ARP traffic */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;

    /* create binary semaphore used for informing ethernetif of frame reception */
    eth_tx_mutex = aicos_mutex_create();
    eth_rx_event = aicos_event_create();

    /* Mac init */
    aicmac_init(aic_netif->port);

    /* initialize MAC address 0 in ethernet MAC */
    aicmac_set_mac_addr(aic_netif->port, 0, netif->hwaddr);

    /* Initialize Tx Descriptors list: Chain Mode */
    aicmac_dma_tx_desc_init(aic_netif->port);
    /* Initialize Rx Descriptors list: Chain Mode  */
    aicmac_dma_rx_desc_init(aic_netif->port);

#if NO_SYS
    /* Disable all interrupt */
    aicmac_set_dma_tx_desc_int(aic_netif->port, DISABLE);
    aicmac_set_dma_rx_desc_int(aic_netif->port, DISABLE);
#else
    /* Enable Ethernet Rx interrrupt */
    aicmac_set_dma_rx_desc_int(aic_netif->port, ENABLE);

    aicos_request_irq(mac_irq[aic_netif->port],
                      (irq_handler_t)aicmac_interrupt, 0, NULL, NULL);

    /* create the task that handles the ETH_MAC */
    aicos_thread_create("eth_rx", NETIF_RX_TASK_STACK_SIZE,
                        NETIF_RX_TASK_PRIORITY, ethernetif_input_thread, NULL);
#endif
    /* Enable MAC and DMA transmission and reception */
    //aicmac_start(aic_netif->port);
    /* set phy linkup */
    //netif_set_link_up(netif);

    /* Phy init */
    aicphy_init(aic_netif->port);
}

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    aicmac_netif_t *aic_netif = (aicmac_netif_t *)netif;
    struct pbuf *q;
    aicmac_dma_desc_t *pdesc;
    int ret = ERR_OK;
#ifndef CONFIG_MAC_ZERO_COPY_TXBUF
    u8 *buffer;
    uint16_t framelength = 0;
    uint32_t bufferoffset = 0;
    uint32_t byteslefttocopy = 0;
    uint32_t payloadoffset = 0;
#else
    uint32_t p_cnt = 0;
    uint32_t p_type = 0;
    uint32_t empty_desc_cnt = 0;
    uint32_t index;
    uint32_t tmpreg = 0;
    uint32_t i = 0;
#endif

    pr_debug("%s\n", __func__);

    if ((netif == NULL) || (p == NULL)){
        pr_err("%s invalid parameter.\n", __func__);
        return ERR_MEM;
    }

    aicos_mutex_take(eth_tx_mutex, AICOS_WAIT_FOREVER);

    pdesc = dctl[aic_netif->port].tx_desc_p;
    /* before read: invalid cache */
    aicmac_dcache_invalid((uintptr_t)pdesc, sizeof(aicmac_dma_desc_t));

#ifndef CONFIG_MAC_ZERO_COPY_TXBUF
    buffer = (u8 *)(unsigned long)(pdesc->buff1_addr);
    bufferoffset = 0;

    for (q = p; q != NULL; q = q->next) {
        if ((pdesc->control & ETH_DMATxDesc_OWN) != (u32)RESET) {
            pr_err("%s no enough desc for transmit.(len = %u)\n", __func__, q->len);
            ret = ERR_MEM;
            goto error;
        }

        /* Get bytes in current lwIP buffer  */
        byteslefttocopy = q->len;
        payloadoffset = 0;

        /* Check if the length of data to copy is bigger than Tx buffer size*/
        while ((byteslefttocopy + bufferoffset) > ETH_TX_BUF_SIZE) {
            /* Copy data to Tx buffer*/
            memcpy((u8_t *)((u8_t *)buffer + bufferoffset),
                   (u8_t *)((u8_t *)q->payload + payloadoffset),
                   (ETH_TX_BUF_SIZE - bufferoffset));
            /* after write: flush cache */
            aicmac_dcache_clean((uintptr_t)((u8_t *)buffer + bufferoffset),
                                (ETH_TX_BUF_SIZE - bufferoffset));

            /* Point to next descriptor */
            pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
            /* before read: invalid cache */
            aicmac_dcache_invalid((uintptr_t)pdesc, sizeof(aicmac_dma_desc_t));

            /* Check if the buffer is available */
            if ((pdesc->control & ETH_DMATxDesc_OWN) != (u32)RESET) {
                pr_err("%s no enough desc for transmit.(len = %u)\n", __func__, q->len);
                ret = ERR_MEM;
                goto error;
            }

            buffer = (u8 *)(unsigned long)(pdesc->buff1_addr);

            byteslefttocopy = byteslefttocopy - (ETH_TX_BUF_SIZE - bufferoffset);
            payloadoffset = payloadoffset + (ETH_TX_BUF_SIZE - bufferoffset);
            framelength = framelength + (ETH_TX_BUF_SIZE - bufferoffset);
            bufferoffset = 0;
        }

        /* Copy the remaining bytes */
        memcpy((u8_t *)((u8_t *)buffer + bufferoffset),
               (u8_t *)((u8_t *)q->payload + payloadoffset), byteslefttocopy);
        /* after write: flush cache */
        aicmac_dcache_clean((uintptr_t)((u8_t *)buffer + bufferoffset),
                            byteslefttocopy);
        bufferoffset = bufferoffset + byteslefttocopy;
        framelength = framelength + byteslefttocopy;
    }

    /* Prepare transmit descriptors to give to DMA*/
    aicmac_submit_tx_frame(aic_netif->port, framelength);
#else
    /* Count number of pbufs in a chain */
    q = p;
    while (q != NULL) {
        if (q->len > ETH_DMATxDesc_TBS1){
            pr_err("%s too large pbuf.(len = %d)\n", __func__, q->len);
            ret = ERR_MEM;
            goto error;
        }
        p_cnt++;
        q = q->next;
    }

    /* Scan empty descriptor for DMA tx */
    while (((pdesc->control & ETH_DMATxDesc_OWN) == (uint32_t)RESET) &&
           (empty_desc_cnt < ETH_RXBUFNB)) {

        empty_desc_cnt++;
        if (empty_desc_cnt >= p_cnt)
            break;

        /* Point to next descriptor */
        pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
        if (pdesc == dctl[aic_netif->port].tx_desc_unconfirm_p){
            pr_info("%s don't overwrite unconfirm area.\n", __func__);
            break;
        }

        /* before read: invalid cache */
        aicmac_dcache_invalid((uintptr_t)pdesc, sizeof(aicmac_dma_desc_t));
    }

    if (p_cnt > empty_desc_cnt){
        pr_err("%s no enough desc for transmit pbuf.(pbuf_cnt = %d, empty_desc = %d)\n",
                __func__, p_cnt, empty_desc_cnt);
        ret = ERR_MEM;
        goto error;
    }

    pbuf_ref(p);
    q = p;
    p_type = p->type_internal;
    for(i=0; i<p_cnt; i++){
        index = pdesc->reserved1;
        if (index >= ETH_RXBUFNB){
            pr_err("%s get dma desc index err.\n", __func__);
            pbuf_free(p);
            ret = ERR_MEM;
            goto error;
        }

        if (i == (p_cnt-1)){
            dctl[aic_netif->port].tx_buff[index] = p;
        }else{
            dctl[aic_netif->port].tx_buff[index] = NULL;
        }

        /* flush data cache */
        if (p_type == PBUF_POOL){
            aicmac_dcache_clean((uintptr_t)q->payload, q->len);
        }else{
            aicos_dcache_clean_range((unsigned long *)q->payload, q->len);
        }

        /* Set Buffer1 address pointer */
        pdesc->buff1_addr =
            (uint32_t)(unsigned long)(q->payload);
        /* Set frame size */
        pdesc->buff_size = (q->len & ETH_DMATxDesc_TBS1);
        /* after write: flush cache */
        aicmac_dcache_clean((uintptr_t)&pdesc->buff_size, 2*sizeof(uint32_t));

        /*set LAST and FIRST segment */
        tmpreg =  ETH_DMATxDesc_TCH;
        if (i == 0)
            tmpreg |= ETH_DMATxDesc_FS;
        if (i == (p_cnt-1))
            tmpreg |= ETH_DMATxDesc_LS | ETH_DMATxDesc_IC;
        /* TCP/IP Tx Checksum Insertion */
        if (mac_config[aic_netif->port].coe_tx)
            tmpreg |= ETH_DMATxDesc_CIC_TCPUDPICMP_Full;
        /* Set Own bit of the Tx descriptor Status: gives the buffer back to ETHERNET DMA */
        tmpreg |= ETH_DMATxDesc_OWN;
        pdesc->control = tmpreg;
        /* after write: flush cache */
        aicmac_dcache_clean((uintptr_t)&pdesc->control, sizeof(uint32_t));

        /* Point to next descriptor */
        pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
        q = q->next;
    }

    dctl[aic_netif->port].tx_desc_p = pdesc;

    /* Resume DMA transmission */
    aicmac_resume_dma_tx(aic_netif->port);
#endif

error:
    /* Give semaphore and exit */
    aicos_mutex_give(eth_tx_mutex);

    return ret;
}

static struct pbuf * low_level_input(struct netif *netif, bool poll)
{
    aicmac_netif_t *aic_netif = (aicmac_netif_t *)netif;
    struct pbuf *p = NULL;
    struct pbuf *q = NULL;
    u32_t len;
    aicmac_frame_t frame = { 0, 0, 0, 0};
    aicmac_dma_desc_t *pdesc = NULL;
#ifndef CONFIG_MAC_ZERO_COPY_RXBUF
    u8 *buffer;
    uint32_t bufferoffset = 0;
    uint32_t payloadoffset = 0;
    uint32_t byteslefttocopy = 0;
#else
    uint32_t index;
#endif

    pr_debug("%s\n", __func__);

    /* get received frame */
    if (poll){
        if (!aicmac_check_rx_frame_poll(aic_netif->port)){
            return NULL;
        }
        frame = aicmac_get_rx_frame_poll(aic_netif->port);
    } else {
        frame = aicmac_get_rx_frame_interrupt(aic_netif->port);
    }

    if ((NULL == frame.descriptor) || (NULL == frame.last_desc))
        return NULL;

#ifndef CONFIG_MAC_ZERO_COPY_RXBUF
    /* Obtain the size of the packet and put it into the "len" variable. */
    len = frame.length;
    buffer = (u8 *)(unsigned long)frame.buffer;

    if (len > 0) {
        /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
        p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    }

    if (p != NULL) {
        pdesc = frame.descriptor;
        bufferoffset = 0;
        for (q = p; q != NULL; q = q->next) {
            byteslefttocopy = q->len;
            payloadoffset = 0;

            /* Check if the length of bytes to copy in current pbuf is bigger than Rx buffer size*/
            while ((byteslefttocopy + bufferoffset) > ETH_RX_BUF_SIZE) {
                /* before read: invalid cache */
                aicmac_dcache_invalid((uintptr_t)((u8_t *)buffer + bufferoffset),
                                      (ETH_RX_BUF_SIZE - bufferoffset));
                /* Copy data to pbuf*/
                memcpy((u8_t *)((u8_t *)q->payload + payloadoffset),
                       (u8_t *)((u8_t *)buffer + bufferoffset),
                       (ETH_RX_BUF_SIZE - bufferoffset));

                /* Point to next descriptor */
                pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
                buffer = (unsigned char *)(unsigned long)(pdesc->buff1_addr);

                byteslefttocopy = byteslefttocopy - (ETH_RX_BUF_SIZE - bufferoffset);
                payloadoffset = payloadoffset + (ETH_RX_BUF_SIZE - bufferoffset);
                bufferoffset = 0;
            }

            /* before read: invalid cache */
            aicmac_dcache_invalid((uintptr_t)((u8_t *)buffer + bufferoffset),
                                  byteslefttocopy);
            /* Copy remaining data in pbuf */
            memcpy((u8_t *)((u8_t *)q->payload + payloadoffset),
                   (u8_t *)((u8_t *)buffer + bufferoffset), byteslefttocopy);
            bufferoffset += byteslefttocopy;
        }
    }

    /* Point to next descriptor */
    dctl[aic_netif->port].rx_desc_received_p = (aicmac_dma_desc_t *)(unsigned long)(frame.last_desc->buff2_addr);
    dctl[aic_netif->port].rx_frame_info.seg_cnt = 0;
#else
    len = frame.length;
    pdesc = frame.descriptor;

    do{
        index = pdesc->reserved1;
        if (index >= ETH_RXBUFNB){
            pr_err("%s get dma desc index err.\n", __func__);
            return NULL;
        }

        /* first pbuf */
        if (p == NULL)
            p = dctl[aic_netif->port].rx_buff[index];

        /* pre pbuf */
        if (q != NULL)
            q->next = dctl[aic_netif->port].rx_buff[index];

        /* current pbuf */
        q = dctl[aic_netif->port].rx_buff[index];

        q->tot_len = len;
        if (len > AICMAC_PBUF_SIZE){
            q->len = AICMAC_PBUF_SIZE;
        }else{
            q->len = len;
        }
        len -= q->len;
        q->next = NULL;
        aicmac_dcache_invalid((uintptr_t)q->payload, q->len);

        /* Point to next descriptor */
        pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
        dctl[aic_netif->port].rx_desc_received_p = pdesc;
        dctl[aic_netif->port].rx_frame_info.seg_cnt = 0;
    }while(len);
#endif

    aicmac_release_rx_frame(aic_netif->port);
    return p;
}
#if !NO_SYS
void ethernetif_input_thread(void *pvParameters)
{
    struct pbuf *p;
    uint32_t event = 0;
    int ret = 0;

    for (;;) {
        ret = aicos_event_recv(eth_rx_event, ETH_EVENT_RX_COMPLETE|ETH_EVENT_TX_COMPLETE, &event, AICOS_WAIT_FOREVER);
        if (ret < 0)
            continue;

        if (event & ETH_EVENT_RX_COMPLETE) {
        TRY_GET_NEXT_FRAME:
            p = low_level_input(&aic_netif.netif, false);
            if (p != NULL) {
                if (ERR_OK != aic_netif.netif.input(p, &aic_netif.netif)) {
                    pbuf_free(p);
                } else {
                    goto TRY_GET_NEXT_FRAME;
                }
            }

            #ifdef CONFIG_MAC_ZERO_COPY_TXBUF
            while (dctl[aic_netif.port].rx_buf_underrun){
                aicos_msleep(100);
                pr_info("%s try resume rx underrun!\n", __func__);
                aicmac_release_rx_frame(aic_netif.port);
            }
            #endif

            /* Enable interrupt again */
            aicmac_en_dma_int(aic_netif.port, ETH_DMAINTEN_RIE);
        }

        if (event & ETH_EVENT_TX_COMPLETE) {
            aicmac_confirm_tx_frame(aic_netif.port);

            /* Enable interrupt again */
            aicmac_en_dma_int(aic_netif.port, ETH_DMAINTEN_TIE);
        }
    }
}
#endif

#if LWIP_IGMP
static err_t
ethernetif_igmp_mac_filter_fun(struct netif *netif,
                               const ip4_addr_t *group,
                               enum netif_mac_filter_action action)
{
    static uint8_t macaddr_used_mask = 0x01;    /* MAC_ADDR0 always used for Unicast */
    uint32_t port = 0;    /* default */
    uint8_t macaddr[6];
    uint8_t reg_macaddr[6];
    uint32_t grp_ip = group->addr & 0xFFFFFE00;
    uint32_t i,j;

    /* Assemble multicast MAC address */
    macaddr[0] = 0x01;
    macaddr[1] = 0x00;
    macaddr[2] = 0x5E;
    macaddr[3] = (uint8_t)(grp_ip >> 8);
    macaddr[4] = (uint8_t)(grp_ip >> 16);
    macaddr[5] = (uint8_t)(grp_ip >> 24);

    /* MAC_ADDR0 used for Unicast */
    for (i = 1; i < ETH_MACADDR_MAX_INDEX; i++) {
        aicmac_get_mac_addr(port, i, reg_macaddr);
        if (0 == memcmp(&reg_macaddr[0], &macaddr[0], 6)) {
            break;
        }
    }

    if (action == NETIF_ADD_MAC_FILTER) {
        if (i == ETH_MACADDR_MAX_INDEX)
        {
            for (j = 1; j < ETH_MACADDR_MAX_INDEX; j++) {
                if (((macaddr_used_mask >> j) & 0x01) == 0) {
                    macaddr_used_mask |= (1 << j);
                    break;
                }
            }
            if (j == ETH_MACADDR_MAX_INDEX)
                return ERR_MEM;
        }
        else
            return ERR_USE;

        aicmac_set_mac_addr(port, j, macaddr);
        aicmac_set_mac_addr_mode(port, j, true, false, 0);
    }
    else {
        if (i < ETH_MACADDR_MAX_INDEX)
            macaddr_used_mask &= ~(1 << i);
        else
            return ERR_MEM;

        aicmac_set_mac_addr_mode(port, i, false, false, 0);

        /* Avoid setting MAC filtering error next time */
        for (j = 0; j < 6; j++)
            reg_macaddr[j] = 0xFF;
        aicmac_set_mac_addr(port, i, reg_macaddr);
    }

    return ERR_OK;
}
#endif

void ethernetif_input_poll(void)
{
    struct pbuf *p;

    p = low_level_input(&aic_netif.netif, true);
    if (p == NULL)
        return;

    if (ERR_OK != aic_netif.netif.input(p, &aic_netif.netif)) {
        pbuf_free(p);
    }
}

err_t ethernetif_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;

    netif->output = etharp_output;
    netif->linkoutput = low_level_output;
#if LWIP_IGMP
    netif_set_igmp_mac_filter(netif, ethernetif_igmp_mac_filter_fun);
#endif
    /* initialize the hardware */
    low_level_init(netif);

    return ERR_OK;
}
