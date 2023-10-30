/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_MAC_H_
#define _ARTINCHIP_AIC_MAC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/netif.h"
#include <aic_core.h>
#include "aic_mac_reg.h"

#define MAX_ETH_MAC_PORT            AIC_GMAC_DEV_NUM
#define MAC(port, member)           ((unsigned long)&(((aicmac_reg_t *)mac_base[port])->member))

#define ENABLE                      true
#define DISABLE                     false
#define SET                         true
#define RESET                       false
#define IS_FUNCTIONAL_STATE(STATE)  (((STATE) == DISABLE) || ((STATE) == ENABLE))
/* ETHERNET errors */
#define ETH_ERROR                   (-1)
#define ETH_SUCCESS                 (0)

typedef enum {
    MODE_RMII = 0,
    MODE_RGMII = 1,
} aicmac_mii_mode_t;

typedef struct {
    uint32_t autonegotiation : 1;
    uint32_t duplex          : 1;
    uint32_t flowctl         : 1;
    uint32_t mac_loopback    : 1;
    uint32_t phy_loopback    : 1;
    uint32_t coe_rx          : 1;           /* TCP/IP Rx Checksum Offload Engine */
    uint32_t coe_tx          : 1;           /* TCP/IP Tx Checksum Insertion */
    uint32_t dma_rxpbl       : 6;
    uint32_t dma_txpbl       : 6;
    uint32_t dma_pblx8       : 1;
    uint32_t dma_fixed_burst : 1;
    uint32_t dma_mixed_burst : 1;
    uint32_t dma_aal         : 1;
    uint32_t dma_sf_mode     : 1;
    uint32_t dma_fifo_rxth   : 2;
    uint32_t dma_fifo_txth   : 3;
    uint32_t rgmii_bus       : 1;
    uint16_t phyaddr;
    uint32_t port;
    uint32_t max_speed;
    char * phyrst_gpio_name;
} aicmac_config_t;

typedef struct {
    struct netif netif;
    uint32_t port;
}aicmac_netif_t;

/* default config */
#ifndef CONFIG_HW_TX_IP_CHECK
#define CONFIG_HW_TX_IP_CHECK 0
#endif

#ifndef CONFIG_HW_RX_IP_CHECK
#define CONFIG_HW_RX_IP_CHECK 0
#endif

#ifndef CONFIG_DMA_RX_PBL
#define CONFIG_DMA_RX_PBL DEFAULT_DMA_PB
#endif

#ifndef CONFIG_DMA_TX_PBL
#define CONFIG_DMA_TX_PBL DEFAULT_DMA_PB
#endif

#ifndef CONFIG_USE_MAC_PORT
#define CONFIG_USE_MAC_PORT 0
#endif

#ifndef CONFIG_PORT_MAC_ADDR_LOW
#define CONFIG_PORT_MAC_ADDR_LOW 0x33221100
#endif

#ifndef CONFIG_PORT_MAC_ADDR_HIGH
#define CONFIG_PORT_MAC_ADDR_HIGH 0x5544
#endif

#ifndef CONFIG_PORT_IP_ADDR0
#define CONFIG_PORT_IP_ADDR0 192
#endif
#ifndef CONFIG_PORT_IP_ADDR1
#define CONFIG_PORT_IP_ADDR1 168
#endif
#ifndef CONFIG_PORT_IP_ADDR2
#define CONFIG_PORT_IP_ADDR2 1
#endif
#ifndef CONFIG_PORT_IP_ADDR3
#define CONFIG_PORT_IP_ADDR3 200
#endif

#ifndef CONFIG_PORT_NET_MASK0
#define CONFIG_PORT_NET_MASK0 255
#endif
#ifndef CONFIG_PORT_NET_MASK1
#define CONFIG_PORT_NET_MASK1 255
#endif
#ifndef CONFIG_PORT_NET_MASK2
#define CONFIG_PORT_NET_MASK2 255
#endif
#ifndef CONFIG_PORT_NET_MASK3
#define CONFIG_PORT_NET_MASK3 0
#endif

#ifndef CONFIG_PORT_GW_IP_ADDR0
#define CONFIG_PORT_GW_IP_ADDR0 192
#endif
#ifndef CONFIG_PORT_GW_IP_ADDR1
#define CONFIG_PORT_GW_IP_ADDR1 168
#endif
#ifndef CONFIG_PORT_GW_IP_ADDR2
#define CONFIG_PORT_GW_IP_ADDR2 1
#endif
#ifndef CONFIG_PORT_GW_IP_ADDR3
#define CONFIG_PORT_GW_IP_ADDR3 1
#endif

/*----------------------------------------------------------------------------*/
/* ETH Frames defines                                                         */
/*----------------------------------------------------------------------------*/

#define ETH_MAX_PACKET_SIZE 1524    /* ETH_HEADER + ETH_EXTRA + VLAN_TAG + MAX_ETH_PAYLOAD + ETH_CRC */
#define ETH_HEADER          14      /* 6 byte Dest addr, 6 byte Src addr, 2 byte length/type */
#define ETH_CRC             4       /* Ethernet CRC */
#define ETH_EXTRA           2       /* Extra bytes in some cases */
#define VLAN_TAG            4       /* optional 802.1q VLAN Tag */
#define MIN_ETH_PAYLOAD     46      /* Minimum Ethernet payload size */
#define MAX_ETH_PAYLOAD     1500    /* Maximum Ethernet payload size */
#define JUMBO_FRAME_PAYLOAD 9000    /* Jumbo frame payload size */

#ifndef ETH_RX_BUF_SIZE
#define ETH_RX_BUF_SIZE ETH_MAX_PACKET_SIZE
#endif

/* 5 Ethernet driver receive buffers are used (in a chained linked list)*/
#ifndef ETH_RXBUFNB
#define ETH_RXBUFNB 16 /*  4 Rx buffers of size ETH_RX_BUF_SIZE */
#endif

#ifndef ETH_TX_BUF_SIZE
#define ETH_TX_BUF_SIZE ETH_MAX_PACKET_SIZE
#endif

/* 5 ethernet driver transmit buffers are used (in a chained linked list)*/
#ifndef ETH_TXBUFNB
#define ETH_TXBUFNB 16 /* 4  Tx buffers of size ETH_TX_BUF_SIZE */
#endif

#define AICMAC_PBUF_SIZE    LWIP_MEM_ALIGN_SIZE(PBUF_POOL_BUFSIZE)

/*----------------------------------------------------------------------------*/
/* DMA descriptors types                                                      */
/*----------------------------------------------------------------------------*/

typedef struct {
    uint32_t length;
    uint32_t buffer;
    aicmac_dma_desc_t *descriptor;
    aicmac_dma_desc_t *last_desc;
} aicmac_frame_t;

typedef struct {
    aicmac_dma_desc_t *first_desc; /* First Segment Rx Desc */
    aicmac_dma_desc_t *last_desc;  /* Last Segment Rx Desc */
    uint32_t seg_cnt;              /* Segment count */
} aicmac_rx_frame_info_t;

typedef struct {
    aicmac_dma_desc_t rx_desc_tbl[ETH_RXBUFNB] __attribute__((aligned(CACHE_LINE_SIZE)));     /* Ethernet Rx DMA Descriptor */
    aicmac_dma_desc_t tx_desc_tbl[ETH_TXBUFNB] __attribute__((aligned(CACHE_LINE_SIZE)));     /* Ethernet Tx DMA Descriptor */
#ifdef CONFIG_MAC_ZERO_COPY_RXBUF
    struct pbuf * rx_buff[ETH_RXBUFNB];
#else
    uint8_t rx_buff[ETH_RXBUFNB][__ALIGN_MASK(ETH_RX_BUF_SIZE, CACHE_LINE_SIZE)] __attribute__((aligned(CACHE_LINE_SIZE))); /* Ethernet Receive Buffer */
#endif
#ifdef CONFIG_MAC_ZERO_COPY_TXBUF
    struct pbuf * tx_buff[ETH_RXBUFNB];
#else
    uint8_t tx_buff[ETH_TXBUFNB][__ALIGN_MASK(ETH_RX_BUF_SIZE, CACHE_LINE_SIZE)] __attribute__((aligned(CACHE_LINE_SIZE))); /* Ethernet Transmit Buffer */
#endif

    /* Global pointers on Tx and Rx descriptor used to track transmit and receive descriptors */
    aicmac_dma_desc_t *tx_desc_p;
    aicmac_dma_desc_t *tx_desc_unconfirm_p;
    aicmac_dma_desc_t *rx_desc_p;
    aicmac_dma_desc_t *rx_desc_received_p;
    aicmac_dma_desc_t *rx_desc_unrelease_p;

    /* Structure used to hold the last received packet descriptors info */
    aicmac_rx_frame_info_t rx_frame_info;
    aicmac_rx_frame_info_t *rx_frame_info_p;

    /* flag */
    uint8_t rx_buf_underrun;
} aicmac_dma_desc_ctl_t;

/*----------------------------------------------------------------------------*/
/* Description of common PHY registers                                        */
/*----------------------------------------------------------------------------*/

/* PHY_Read_write_Timeouts */
#define PHY_READ_TO  ((uint32_t)0x0004FFFF)
#define PHY_WRITE_TO ((uint32_t)0x0004FFFF)

/*----------------------------------------------------------------------------*/
/* Functions Declaration                                                      */
/*----------------------------------------------------------------------------*/

void aicmac_sw_reset(uint32_t port);
bool aicmac_get_sw_reset_status(uint32_t port);
int aicmac_write_phy_reg(uint32_t port, uint32_t addr, uint16_t val);
int aicmac_read_phy_reg(uint32_t port, uint32_t addr, uint16_t *val);
void aicmac_set_mac_tx(uint32_t port, bool state);
void aicmac_set_mac_rx(uint32_t port, bool state);
void aicmac_set_mac_speed(uint32_t port, int speed);
void aicmac_set_mac_duplex(uint32_t port, bool state);
void aicmac_set_mac_pause(uint32_t port, bool state);
void aicmac_flush_tx_fifo(uint32_t port);
void aicmac_set_dma_tx(uint32_t port, bool state);
void aicmac_set_dma_rx(uint32_t port, bool state);
bool aicmac_get_dma_int_status(uint32_t port, uint32_t flag);
void aicmac_clear_dma_int_pending(uint32_t port, uint32_t flag);
void aicmac_set_mac_addr(uint32_t port, uint32_t index, uint8_t *addr);
void aicmac_dma_tx_desc_init(uint32_t port);
void aicmac_dma_rx_desc_init(uint32_t port);
void aicmac_resume_dma_tx(uint32_t port);
void aicmac_resume_dma_rx(uint32_t port);
void aicmac_set_dma_rx_desc_int(uint32_t port, bool en);
void aicmac_set_dma_tx_desc_int(uint32_t port, bool en);
void aicmac_start(uint32_t port);
void aicmac_stop(uint32_t port);
void aicmac_confirm_tx_frame(uint32_t port);
int aicmac_submit_tx_frame(uint32_t port, u16 FrameLength);
aicmac_frame_t aicmac_get_rx_frame_interrupt(uint32_t port);
aicmac_frame_t aicmac_get_rx_frame_poll(uint32_t port);
void aicmac_release_rx_frame(uint32_t port);
uint32_t aicmac_check_rx_frame_poll(uint32_t port);
int aicmac_init(uint32_t port);
void aicmac_en_dma_int(uint32_t port, uint32_t interrupt);
void aicmac_dis_dma_int(uint32_t port, uint32_t interrupt);
void aicmac_dcache_clean(uintptr_t addr, uint32_t len);
void aicmac_dcache_invalid(uintptr_t addr, uint32_t len);
void aicmac_dcache_clean_invalid(uintptr_t addr, uint32_t len);

void aicmac_set_mac_addr(uint32_t port, uint32_t index, uint8_t *addr);
void aicmac_get_mac_addr(uint32_t port, uint32_t index, uint8_t *addr);
void aicmac_set_mac_addr_mode(uint32_t port, uint32_t index, bool en, bool sa,
                              uint32_t mask);

#ifdef __cplusplus
}
#endif

#endif
