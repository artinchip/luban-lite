/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#include <rtconfig.h>

/*
   -------------- NO SYS --------------
*/
#ifdef KERNEL_BAREMETAL
#define NO_SYS                          1
#else
#define NO_SYS                          0
#endif
#define NO_SYS_NO_TIMERS                0
#define LWIP_TIMERS                     1
#define LWIP_TIMERS_CUSTOM              0
#define LWIP_MPU_COMPATIBLE             0

/*
   ----------- Core locking -----------
*/
#define LWIP_TCPIP_CORE_LOCKING         1
#define LWIP_TCPIP_CORE_LOCKING_INPUT   0
#define SYS_LIGHTWEIGHT_PROT            1

/*
   ---------- Memory options ----------
*/
/* If MEM_LIBC_MALLOC and MEMP_MEM_MALLOC are defined to 1,
MEM_SIZE and MEMP_NUM_XXX will be invalid */
#define MEM_LIBC_MALLOC                 0
#define MEMP_MEM_MALLOC                 0
#define MEMP_MEM_INIT                   0
#define MEM_ALIGNMENT                   4

#define MEMP_OVERFLOW_CHECK             0
#define MEMP_SANITY_CHECK               0
#define MEM_OVERFLOW_CHECK              0
#define MEM_SANITY_CHECK                0
#define MEM_USE_POOLS                   0
#define MEM_USE_POOLS_TRY_BIGGER_POOL   0
#define MEMP_USE_CUSTOM_POOLS           0
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 0
/*---------- Internal Memory Pool Sizes ----------*/
//no meaning if MEMP_MEM_MALLOC = 1
#define MEMP_NUM_PBUF                   16

/* the size of lwip dynamic memory(mem_alloc()), used for transmit Ethernet packet  */
#ifdef LPKG_LWIP_MEM_SIZE
#define MEM_SIZE                        LPKG_LWIP_MEM_SIZE
#endif

/* the number of UDP protocol control blocks. One per active RAW "connection". */
#ifdef LPKG_LWIP_RAW_PCB_NUM
#define MEMP_NUM_RAW_PCB                LPKG_LWIP_RAW_PCB_NUM
#endif
/* the number of UDP protocol control blocks. One per active UDP "connection". */
#ifdef LPKG_LWIP_UDP_PCB_NUM
#define MEMP_NUM_UDP_PCB                LPKG_LWIP_UDP_PCB_NUM
#endif
/* the number of simulatenously active TCP connections. */
#ifdef LPKG_LWIP_TCP_PCB_NUM
#define MEMP_NUM_TCP_PCB                LPKG_LWIP_TCP_PCB_NUM
#endif
#define MEMP_NUM_TCP_PCB_LISTEN         8
/* the number of simultaneously queued TCP */
#ifdef LPKG_LWIP_TCP_SEG_NUM
#define MEMP_NUM_TCP_SEG                LPKG_LWIP_TCP_SEG_NUM
#else
#define MEMP_NUM_TCP_SEG                TCP_SND_QUEUELEN
#endif
#define MEMP_NUM_ALTCP_PCB              MEMP_NUM_TCP_PCB
#define MEMP_NUM_REASSDATA              5
#define MEMP_NUM_FRAG_PBUF              15
#define MEMP_NUM_ARP_QUEUE              30
#define MEMP_NUM_IGMP_GROUP             8
#define MEMP_NUM_SYS_TIMEOUT            (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 2)
#define MEMP_NUM_NETBUF                 2
#ifdef LPKG_MEMP_NUM_NETCONN
#define MEMP_NUM_NETCONN                LPKG_MEMP_NUM_NETCONN
#endif
#define MEMP_NUM_SELECT_CB              4
#define MEMP_NUM_TCPIP_MSG_API          8
#define MEMP_NUM_TCPIP_MSG_INPKT        8
#define MEMP_NUM_NETDB                  1
#define MEMP_NUM_LOCALHOSTLIST          1
#ifdef LPKG_LWIP_PBUF_NUM
#define PBUF_POOL_SIZE                  LPKG_LWIP_PBUF_NUM
#endif
#define MEMP_NUM_API_MSG                MEMP_NUM_TCPIP_MSG_API
#define MEMP_NUM_DNS_API_MSG            MEMP_NUM_TCPIP_MSG_API
#define MEMP_NUM_SOCKET_SETGETSOCKOPT_DATA MEMP_NUM_TCPIP_MSG_API
#define MEMP_NUM_NETIFAPI_MSG           MEMP_NUM_TCPIP_MSG_API

/*
   ---------- ARP options ----------
*/
#define LWIP_ARP                        1
#define ARP_TABLE_SIZE                  10
#define ARP_MAXAGE                      300
#define ARP_QUEUEING                    0
#define ARP_QUEUE_LEN                   3
#define ETHARP_SUPPORT_VLAN             0
#define LWIP_ETHERNET                   LWIP_ARP
#ifdef LPKG_LWIP_ETH_PAD_SIZE
#define ETH_PAD_SIZE                    LPKG_LWIP_ETH_PAD_SIZE
#endif
#define ETHARP_SUPPORT_STATIC_ENTRIES   0
#define ETHARP_TABLE_MATCH_NETIF        !LWIP_SINGLE_NETIF

/*
   ---------- IP options ----------
*/
#define LWIP_IPV4                       1
#define IP_FORWARD                      0
#ifdef LPKG_LWIP_REASSEMBLY_FRAG
#define IP_REASSEMBLY                   1
#define IP_FRAG                         1
#define IP_REASS_MAX_PBUFS              10
#define MEMP_NUM_REASSDATA              10
#else
#define IP_REASSEMBLY                   0
#define IP_FRAG                         0
#endif
#define IP_OPTIONS_ALLOWED              1
#define IP_REASS_MAXAGE                 15
#define IP_REASS_MAX_PBUFS              10
#define IP_DEFAULT_TTL                  255
#define IP_SOF_BROADCAST                LPKG_IP_SOF_BROADCAST
#define IP_SOF_BROADCAST_RECV           LPKG_IP_SOF_BROADCAST_RECV
#define IP_FORWARD_ALLOW_TX_ON_RX_NETIF 0

/*
   ---------- ICMP options ----------
*/
#ifdef LPKG_LWIP_ICMP
#define LWIP_ICMP                       1
#else
#define LWIP_ICMP                       0
#endif
#define ICMP_TTL                        (IP_DEFAULT_TTL)
#define LWIP_BROADCAST_PING             0
#define LWIP_MULTICAST_PING             0

/*
   ---------- RAW options ----------
*/
#ifdef LPKG_LWIP_RAW
#define LWIP_RAW                        1
#else
#define LWIP_RAW                        0
#endif
#define RAW_TTL                         (IP_DEFAULT_TTL)

/*
   ---------- DHCP options ----------
*/
#ifdef LPKG_LWIP_DHCP
#define LWIP_DHCP                       1
#else
#define LWIP_DHCP                       0
#endif
#define DHCP_DOES_ARP_CHECK             ((LWIP_DHCP) && (LWIP_ARP))
#define LWIP_DHCP_CHECK_LINK_UP         0
#define LWIP_DHCP_BOOTP_FILE            0
#define LWIP_DHCP_GET_NTP_SRV           0
#define LWIP_DHCP_MAX_NTP_SERVERS       1
#define LWIP_DHCP_MAX_DNS_SERVERS       DNS_MAX_SERVERS

/*
   ---------- AUTOIP options ----------
*/
#define LWIP_AUTOIP                     0
#define LWIP_DHCP_AUTOIP_COOP           0
#define LWIP_DHCP_AUTOIP_COOP_TRIES     9
#define LWIP_MIB2_CALLBACKS             0
#define LWIP_MULTICAST_TX_OPTIONS       ((LWIP_IGMP || LWIP_IPV6_MLD) && (LWIP_UDP || LWIP_RAW))

/*
   ---------- IGMP options ----------
*/
#ifdef LPKG_LWIP_IGMP
#define LWIP_IGMP                       1
#else
#define LWIP_IGMP                       0
#endif

/*
   ---------- DNS options -----------
*/
#ifdef LPKG_LWIP_DNS
#define LWIP_DNS                        1
#else
#define LWIP_DNS                        0
#endif
#define DNS_TABLE_SIZE                  4
#define DNS_MAX_NAME_LENGTH             256
#define DNS_MAX_SERVERS                 2
#define DNS_MAX_RETRIES                 4
#define DNS_DOES_NAME_CHECK             1
#define LWIP_DNS_SECURE (LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING | LWIP_DNS_SECURE_RAND_SRC_PORT)
#define DNS_LOCAL_HOSTLIST              0
#define DNS_LOCAL_HOSTLIST_IS_DYNAMIC   0
#define LWIP_DNS_SUPPORT_MDNS_QUERIES   0

/*
   ---------- UDP options ----------
*/
#ifdef LPKG_LWIP_UDP
#define LWIP_UDP                        1
#else
#define LWIP_UDP                        0
#endif
#define LWIP_UDPLITE                    0
#define UDP_TTL                         (IP_DEFAULT_TTL)
#define LWIP_NETBUF_RECVINFO            0

/*
   ---------- TCP options ----------
*/
#ifdef LPKG_LWIP_TCP
#define LWIP_TCP                        1
#else
#define LWIP_TCP                        0
#endif
#define TCP_TTL                         (IP_DEFAULT_TTL)
#ifdef LPKG_LWIP_TCP_WND
#define TCP_WND                         LPKG_LWIP_TCP_WND
#else
#define TCP_WND                         (TCP_MSS * 4)
#endif
#define TCP_MAXRTX                      12
#define TCP_SYNMAXRTX                   6
#define TCP_QUEUE_OOSEQ                 (LWIP_TCP)
#define LWIP_TCP_SACK_OUT               0
#define LWIP_TCP_MAX_SACK_NUM           4
#define TCP_MSS                         1460
#define TCP_CALCULATE_EFF_SEND_MSS      1
#ifdef LPKG_LWIP_TCP_SND_BUF
#define TCP_SND_BUF                     LPKG_LWIP_TCP_SND_BUF
#else
#define TCP_SND_BUF                     (TCP_MSS * 2)
#endif
#define TCP_SND_QUEUELEN                ((4 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))
#define TCP_SNDLOWAT                    LWIP_MIN(LWIP_MAX(((TCP_SND_BUF)/2), (2 * TCP_MSS) + 1), (TCP_SND_BUF) - 1)
#define TCP_SNDQUEUELOWAT               LWIP_MAX(((TCP_SND_QUEUELEN)/2), 5)
#define TCP_OOSEQ_MAX_BYTES             0
#define TCP_OOSEQ_BYTES_LIMIT(pcb)      TCP_OOSEQ_MAX_BYTES
#define TCP_OOSEQ_MAX_PBUFS             0
#define TCP_OOSEQ_PBUFS_LIMIT(pcb)      TCP_OOSEQ_MAX_PBUFS
#define TCP_LISTEN_BACKLOG              0
#define TCP_DEFAULT_LISTEN_BACKLOG      0xff
#define TCP_OVERSIZE                    TCP_MSS
#define LWIP_TCP_TIMESTAMPS             0
#define TCP_WND_UPDATE_THRESHOLD        LWIP_MIN((TCP_WND / 4), (TCP_MSS * 4))
#define LWIP_EVENT_API                  0
#define LWIP_CALLBACK_API               1
#define LWIP_WND_SCALE                  0
#define TCP_RCV_SCALE                   0
#define LWIP_TCP_PCB_NUM_EXT_ARGS       0
#define LWIP_ALTCP                      0
#define LWIP_ALTCP_TLS                  0

/*
   ---------- Pbuf options ----------
*/
#define PBUF_LINK_HLEN                  (14 + ETH_PAD_SIZE)
#define PBUF_LINK_ENCAPSULATION_HLEN    0
#ifdef LPKG_LWIP_PBUF_POOL_BUFSIZE
#define PBUF_POOL_BUFSIZE               LPKG_LWIP_PBUF_POOL_BUFSIZE
#else
#define PBUF_POOL_BUFSIZE               LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_ENCAPSULATION_HLEN+PBUF_LINK_HLEN)
#endif
#define LWIP_PBUF_REF_T                 u8_t

/*
   ---------- Thread options ----------
*/
/*#define TCPIP_THREAD_NAME               "tcpip_thread"
#define TCPIP_THREAD_STACKSIZE          0
#define TCPIP_THREAD_PRIO               1
#define TCPIP_MBOX_SIZE                 0
#define LWIP_TCPIP_THREAD_ALIVE()
#define SLIPIF_THREAD_NAME              "slipif_loop"
#define SLIPIF_THREAD_STACKSIZE         0
#define SLIPIF_THREAD_PRIO              1
#define DEFAULT_THREAD_NAME             "lwIP"
#define DEFAULT_THREAD_STACKSIZE        0
#define DEFAULT_THREAD_PRIO             1
#define DEFAULT_RAW_RECVMBOX_SIZE       0
#define DEFAULT_UDP_RECVMBOX_SIZE       0
#define DEFAULT_TCP_RECVMBOX_SIZE       0
#define DEFAULT_ACCEPTMBOX_SIZE         0*/
#define DEFAULT_ACCEPTMBOX_SIZE         16
#define DEFAULT_RAW_RECVMBOX_SIZE       64
#define DEFAULT_UDP_RECVMBOX_SIZE       64
#define DEFAULT_TCP_RECVMBOX_SIZE       64
#ifdef LPKG_LWIP_TCPTHREAD_PRIORITY
#define TCPIP_MBOX_SIZE                 LPKG_LWIP_TCPTHREAD_MBOX_SIZE
#define TCPIP_THREAD_PRIO               LPKG_LWIP_TCPTHREAD_PRIORITY
#define TCPIP_THREAD_STACKSIZE          LPKG_LWIP_TCPTHREAD_STACKSIZE
#else
#define TCPIP_MBOX_SIZE                 8
#define TCPIP_THREAD_PRIO               128
#define TCPIP_THREAD_STACKSIZE          4096
#endif

/*
   ---------- Sequential layer options ----------
*/
#define LWIP_NETCONN                    (NO_SYS==0)
#define LWIP_TCPIP_TIMEOUT              0
#define LWIP_NETCONN_SEM_PER_THREAD     0
#define LWIP_NETCONN_FULLDUPLEX         0

/*
   ---------- Socket options ----------
*/
#define LWIP_SOCKET                     (NO_SYS==0)
/*
 * LWIP_COMPAT_SOCKETS==1: Enable BSD-style sockets functions names.
 * (only used if you use sockets.c)
 */
#ifdef SAL_USING_POSIX
#define LWIP_COMPAT_SOCKETS             0
#else
#ifndef LWIP_COMPAT_SOCKETS
#define LWIP_COMPAT_SOCKETS             1
#endif
#endif
/**
 * LWIP_POSIX_SOCKETS_IO_NAMES==1: Enable POSIX-style sockets functions names.
 * Disable this option if you use a POSIX operating system that uses the same
 * names (read, write & close). (only used if you use sockets.c)
 */
#ifndef LWIP_POSIX_SOCKETS_IO_NAMES
#define LWIP_POSIX_SOCKETS_IO_NAMES     0
#endif
/**
 * LWIP_TCP_KEEPALIVE==1: Enable TCP_KEEPIDLE, TCP_KEEPINTVL and TCP_KEEPCNT
 * options processing. Note that TCP_KEEPIDLE and TCP_KEEPINTVL have to be set
 * in seconds. (does not require sockets.c, and will affect tcp.c)
 */
#ifndef LWIP_TCP_KEEPALIVE
#define LWIP_TCP_KEEPALIVE              1
#endif
#define LWIP_SOCKET_OFFSET              3
/**
 * LWIP_SO_SNDTIMEO==1: Enable send timeout for sockets/netconns and
 * SO_SNDTIMEO processing.
 */
#ifndef LWIP_SO_SNDTIMEO
#define LWIP_SO_SNDTIMEO                1
#endif
/**
 * LWIP_SO_RCVTIMEO==1: Enable receive timeout for sockets/netconns and
 * SO_RCVTIMEO processing.
 */
#ifndef LWIP_SO_RCVTIMEO
#define LWIP_SO_RCVTIMEO                1
#endif
/**
 * LWIP_SO_RCVBUF==1: Enable SO_RCVBUF processing.
 */
#ifndef LWIP_SO_RCVBUF
#define LWIP_SO_RCVBUF                  1
#endif
#define LWIP_SO_SNDRCVTIMEO_NONSTANDARD 0
#define LWIP_SO_LINGER                  0
/**
 * If LWIP_SO_RCVBUF is used, this is the default value for recv_bufsize.
 */
#ifndef RECV_BUFSIZE_DEFAULT
#define RECV_BUFSIZE_DEFAULT            8192
#endif
#define LWIP_TCP_CLOSE_TIMEOUT_MS_DEFAULT 20000
/**
 * SO_REUSE==1: Enable SO_REUSEADDR option.
 */
#ifndef SO_REUSE
#define SO_REUSE                        0
#endif
#define SO_REUSE_RXTOALL                0
#define LWIP_FIONREAD_LINUXMODE         0
#define LWIP_SOCKET_SELECT              1
#define LWIP_SOCKET_POLL                1

/*
   ---------- Statistics options ----------
*/
#ifdef LPKG_LWIP_STATS
#define LWIP_STATS                      1
#define LWIP_STATS_DISPLAY              1
#else
#define LWIP_STATS                      0
#endif
#if LWIP_STATS
#define LINK_STATS                      1
#define IP_STATS                        1
#define ICMP_STATS                      1
#define IGMP_STATS                      1
#define IPFRAG_STATS                    1
#define UDP_STATS                       1
#define TCP_STATS                       1
#define MEM_STATS                       1
#define MEMP_STATS                      1
#define PBUF_STATS                      1
#define SYS_STATS                       1
#define MIB2_STATS                      1
#endif /* LWIP_STATS */
#define IP6_STATS                       0
#define ICMP6_STATS                     0
#define IP6_FRAG_STATS                  0
#define MLD6_STATS                      0
#define ND6_STATS                       0

/*
   ---------- Checksum options ----------
*/
#define LWIP_CHECKSUM_CTRL_PER_NETIF    0
#ifdef LPKG_LWIP_USING_TX_HW_CHECKSUM
#define CHECKSUM_GEN_IP                 0
#define CHECKSUM_GEN_UDP                0
#define CHECKSUM_GEN_TCP                0
#define CHECKSUM_GEN_ICMP               0
#define CHECKSUM_GEN_ICMP6              0
#else
#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_GEN_ICMP6              1
#endif

#ifdef LPKG_LWIP_USING_RX_HW_CHECKSUM
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_CHECK_UDP              0
#define CHECKSUM_CHECK_TCP              0
#define CHECKSUM_CHECK_ICMP             0
#define CHECKSUM_CHECK_ICMP6            0
#else
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_CHECK_UDP              0
#define CHECKSUM_CHECK_TCP              0
#define CHECKSUM_CHECK_ICMP             0
#define CHECKSUM_CHECK_ICMP6            0
#endif
#define LWIP_CHECKSUM_ON_COPY           0

/*
   ---------- IPv6 options ---------------
*/
#ifdef LPKG_USING_LWIP_IPV6
#define LWIP_IPV6                   1
#else
#define LWIP_IPV6                   0
#endif /* LPKG_USING_LWIP_IPV6 */
#define IPV6_REASS_MAXAGE               60
#define LWIP_IPV6_SCOPES                (LWIP_IPV6 && !LWIP_SINGLE_NETIF)
#define LWIP_IPV6_SCOPES_DEBUG          0
#define LWIP_IPV6_NUM_ADDRESSES         3
#define LWIP_IPV6_FORWARD               0
#define LWIP_IPV6_FRAG                  1
#define LWIP_IPV6_REASS                 (LWIP_IPV6)
#define LWIP_IPV6_SEND_ROUTER_SOLICIT   1
#define LWIP_IPV6_AUTOCONFIG            (LWIP_IPV6)
#define LWIP_IPV6_ADDRESS_LIFETIMES     (LWIP_IPV6_AUTOCONFIG)
#define LWIP_IPV6_DUP_DETECT_ATTEMPTS   1
#define LWIP_ICMP6                      (LWIP_IPV6)
#define LWIP_ICMP6_DATASIZE             8
#define LWIP_ICMP6_HL                   255
#define LWIP_IPV6_MLD                   (LWIP_IPV6)
#define MEMP_NUM_MLD6_GROUP             4
#define LWIP_ND6_QUEUEING               (LWIP_IPV6)
#define MEMP_NUM_ND6_QUEUE              20
#define LWIP_ND6_NUM_NEIGHBORS          10
#define LWIP_ND6_NUM_DESTINATIONS       10
#define LWIP_ND6_NUM_PREFIXES           5
#define LWIP_ND6_NUM_ROUTERS            3
#define LWIP_ND6_MAX_MULTICAST_SOLICIT  3
#define LWIP_ND6_MAX_UNICAST_SOLICIT    3
#define LWIP_ND6_MAX_ANYCAST_DELAY_TIME 1000
#define LWIP_ND6_MAX_NEIGHBOR_ADVERTISEMENT  3
#define LWIP_ND6_REACHABLE_TIME         30000
#define LWIP_ND6_RETRANS_TIMER          1000
#define LWIP_ND6_DELAY_FIRST_PROBE_TIME 5000
#define LWIP_ND6_ALLOW_RA_UPDATES       1
#define LWIP_ND6_TCP_REACHABILITY_HINTS 1
#define LWIP_ND6_RDNSS_MAX_DNS_SERVERS  0
#define LWIP_IPV6_DHCP6                 0
#define LWIP_IPV6_DHCP6_STATEFUL        0
#define LWIP_IPV6_DHCP6_STATELESS       LWIP_IPV6_DHCP6
#define LWIP_DHCP6_GET_NTP_SRV          0
#define LWIP_DHCP6_MAX_NTP_SERVERS      1
#define LWIP_DHCP6_MAX_DNS_SERVERS      DNS_MAX_SERVERS

/* TODO: check hooks */
/*
   ---------- Debugging options ----------
*/
#ifdef LPKG_LWIP_DEBUG
#define LWIP_DEBUG                      1
#endif

/* ---------- Debug options ---------- */
#ifdef LWIP_DEBUG
#ifdef LPKG_LWIP_SYS_DEBUG
#define SYS_DEBUG                       LWIP_DBG_ON
#else
#define SYS_DEBUG                       LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_ETHARP_DEBUG
#define ETHARP_DEBUG                    LWIP_DBG_ON
#else
#define ETHARP_DEBUG                    LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_PPP_DEBUG
#define PPP_DEBUG                       LWIP_DBG_ON
#else
#define PPP_DEBUG                       LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_MEM_DEBUG
#define MEM_DEBUG                       LWIP_DBG_ON
#else
#define MEM_DEBUG                       LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_MEMP_DEBUG
#define MEMP_DEBUG                      LWIP_DBG_ON
#else
#define MEMP_DEBUG                      LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_PBUF_DEBUG
#define PBUF_DEBUG                      LWIP_DBG_ON
#else
#define PBUF_DEBUG                      LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_API_LIB_DEBUG
#define API_LIB_DEBUG                   LWIP_DBG_ON
#else
#define API_LIB_DEBUG                   LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_API_MSG_DEBUG
#define API_MSG_DEBUG                   LWIP_DBG_ON
#else
#define API_MSG_DEBUG                   LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_TCPIP_DEBUG
#define TCPIP_DEBUG                     LWIP_DBG_ON
#else
#define TCPIP_DEBUG                     LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_NETIF_DEBUG
#define NETIF_DEBUG                     LWIP_DBG_ON
#else
#define NETIF_DEBUG                     LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_SOCKETS_DEBUG
#define SOCKETS_DEBUG                   LWIP_DBG_ON
#else
#define SOCKETS_DEBUG                   LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_DNS_DEBUG
#define DNS_DEBUG                       LWIP_DBG_ON
#else
#define DNS_DEBUG                       LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_AUTOIP_DEBUG
#define AUTOIP_DEBUG                    LWIP_DBG_ON
#else
#define AUTOIP_DEBUG                    LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_DHCP_DEBUG
#define DHCP_DEBUG                      LWIP_DBG_ON
#else
#define DHCP_DEBUG                      LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_IP_DEBUG
#define IP_DEBUG                        LWIP_DBG_ON
#else
#define IP_DEBUG                        LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_IP_REASS_DEBUG
#define IP_REASS_DEBUG                  LWIP_DBG_ON
#else
#define IP_REASS_DEBUG                  LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_ICMP_DEBUG
#define ICMP_DEBUG                      LWIP_DBG_ON
#else
#define ICMP_DEBUG                      LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_IGMP_DEBUG
#define IGMP_DEBUG                      LWIP_DBG_ON
#else
#define IGMP_DEBUG                      LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_UDP_DEBUG
#define UDP_DEBUG                       LWIP_DBG_ON
#else
#define UDP_DEBUG                       LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_TCP_DEBUG
#define TCP_DEBUG                       LWIP_DBG_ON
#else
#define TCP_DEBUG                       LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_TCP_INPUT_DEBUG
#define TCP_INPUT_DEBUG                 LWIP_DBG_ON
#else
#define TCP_INPUT_DEBUG                 LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_TCP_OUTPUT_DEBUG
#define TCP_OUTPUT_DEBUG                LWIP_DBG_ON
#else
#define TCP_OUTPUT_DEBUG                LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_TCP_RTO_DEBUG
#define TCP_RTO_DEBUG                   LWIP_DBG_ON
#else
#define TCP_RTO_DEBUG                   LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_TCP_CWND_DEBUG
#define TCP_CWND_DEBUG                  LWIP_DBG_ON
#else
#define TCP_CWND_DEBUG                  LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_TCP_WND_DEBUG
#define TCP_WND_DEBUG                   LWIP_DBG_ON
#else
#define TCP_WND_DEBUG                   LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_TCP_FR_DEBUG
#define TCP_FR_DEBUG                    LWIP_DBG_ON
#else
#define TCP_FR_DEBUG                    LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_TCP_QLEN_DEBUG
#define TCP_QLEN_DEBUG                  LWIP_DBG_ON
#else
#define TCP_QLEN_DEBUG                  LWIP_DBG_OFF
#endif
#ifdef LPKG_LWIP_TCP_RST_DEBUG
#define TCP_RST_DEBUG                   LWIP_DBG_ON
#else
#define TCP_RST_DEBUG                   LWIP_DBG_OFF
#endif
#endif /* LWIP_DEBUG */
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_ALL
#define LWIP_DBG_TYPES_ON               (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE|LWIP_DBG_FRESH|LWIP_DBG_HALT)
#define IP6_DEBUG                       LWIP_DBG_OFF
#define DHCP6_DEBUG                     LWIP_DBG_OFF
#define LWIP_TESTMODE                   0
#define LWIP_PERF                       0



#if 0


#define LWIP_TIMEVAL_PRIVATE    1
#define LWIP_NO_UNISTD_H        0
#define LWIP_NO_STDDEF_H        0
#define LWIP_NO_STDINT_H        0
#define LWIP_NO_INTTYPES_H      0
#define LWIP_NO_LIMITS_H        0
#define LWIP_NO_CTYPE_H         0
#define LWIP_SOCKET_SELECT      1
#define LWIP_SOCKET_POLL        1

#ifndef SSIZE_MAX
#define SSIZE_MAX INT_MAX
#endif

/* some errno not defined in newlib */
#ifndef ENSRNOTFOUND
#define ENSRNOTFOUND 163  /* Domain name not found */
#endif

/* ---------- Basic Configuration ---------- */
#define LWIP_IPV4                   1

#ifdef LPKG_USING_LWIP_IPV6
#define LWIP_IPV6                   1
#else
#define LWIP_IPV6                   0
#endif /* LPKG_USING_LWIP_IPV6 */

#define NO_SYS                      0
#define SYS_LIGHTWEIGHT_PROT        1
#define LWIP_SOCKET                 1
#define LWIP_NETCONN                1

#ifdef LPKG_LWIP_IGMP
#define LWIP_IGMP                   1
#else
#define LWIP_IGMP                   0
#endif

#ifdef LPKG_LWIP_ICMP
#define LWIP_ICMP                   1
#else
#define LWIP_ICMP                   0
#endif

#ifdef LPKG_LWIP_SNMP
#define LWIP_SNMP                   1
#else
#define LWIP_SNMP                   0
#endif

#ifdef LPKG_LWIP_DNS
#define LWIP_DNS                    1
#else
#define LWIP_DNS                    0
#endif

#define LWIP_HAVE_LOOPIF            0

#define LWIP_PLATFORM_BYTESWAP      0

/* #define LPKG_LWIP_DEBUG */

#ifdef LPKG_LWIP_DEBUG
#define LWIP_DEBUG
#endif

/* ---------- Debug options ---------- */
#ifdef LWIP_DEBUG
#ifdef LPKG_LWIP_SYS_DEBUG
#define SYS_DEBUG                   LWIP_DBG_ON
#else
#define SYS_DEBUG                   LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_ETHARP_DEBUG
#define ETHARP_DEBUG                LWIP_DBG_ON
#else
#define ETHARP_DEBUG                LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_PPP_DEBUG
#define PPP_DEBUG                   LWIP_DBG_ON
#else
#define PPP_DEBUG                   LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_MEM_DEBUG
#define MEM_DEBUG                   LWIP_DBG_ON
#else
#define MEM_DEBUG                   LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_MEMP_DEBUG
#define MEMP_DEBUG                  LWIP_DBG_ON
#else
#define MEMP_DEBUG                  LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_PBUF_DEBUG
#define PBUF_DEBUG                  LWIP_DBG_ON
#else
#define PBUF_DEBUG                  LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_API_LIB_DEBUG
#define API_LIB_DEBUG               LWIP_DBG_ON
#else
#define API_LIB_DEBUG               LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_API_MSG_DEBUG
#define API_MSG_DEBUG               LWIP_DBG_ON
#else
#define API_MSG_DEBUG               LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_TCPIP_DEBUG
#define TCPIP_DEBUG                 LWIP_DBG_ON
#else
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_NETIF_DEBUG
#define NETIF_DEBUG                 LWIP_DBG_ON
#else
#define NETIF_DEBUG                 LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_SOCKETS_DEBUG
#define SOCKETS_DEBUG               LWIP_DBG_ON
#else
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_DNS_DEBUG
#define DNS_DEBUG                   LWIP_DBG_ON
#else
#define DNS_DEBUG                   LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_AUTOIP_DEBUG
#define AUTOIP_DEBUG                LWIP_DBG_ON
#else
#define AUTOIP_DEBUG                LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_DHCP_DEBUG
#define DHCP_DEBUG                  LWIP_DBG_ON
#else
#define DHCP_DEBUG                  LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_IP_DEBUG
#define IP_DEBUG                    LWIP_DBG_ON
#else
#define IP_DEBUG                    LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_IP_REASS_DEBUG
#define IP_REASS_DEBUG              LWIP_DBG_ON
#else
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_ICMP_DEBUG
#define ICMP_DEBUG                  LWIP_DBG_ON
#else
#define ICMP_DEBUG                  LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_IGMP_DEBUG
#define IGMP_DEBUG                  LWIP_DBG_ON
#else
#define IGMP_DEBUG                  LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_UDP_DEBUG
#define UDP_DEBUG                   LWIP_DBG_ON
#else
#define UDP_DEBUG                   LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_TCP_DEBUG
#define TCP_DEBUG                   LWIP_DBG_ON
#else
#define TCP_DEBUG                   LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_TCP_INPUT_DEBUG
#define TCP_INPUT_DEBUG             LWIP_DBG_ON
#else
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_TCP_OUTPUT_DEBUG
#define TCP_OUTPUT_DEBUG            LWIP_DBG_ON
#else
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_TCP_RTO_DEBUG
#define TCP_RTO_DEBUG               LWIP_DBG_ON
#else
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_TCP_CWND_DEBUG
#define TCP_CWND_DEBUG              LWIP_DBG_ON
#else
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_TCP_WND_DEBUG
#define TCP_WND_DEBUG               LWIP_DBG_ON
#else
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_TCP_FR_DEBUG
#define TCP_FR_DEBUG                LWIP_DBG_ON
#else
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_TCP_QLEN_DEBUG
#define TCP_QLEN_DEBUG              LWIP_DBG_ON
#else
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#endif

#ifdef LPKG_LWIP_TCP_RST_DEBUG
#define TCP_RST_DEBUG               LWIP_DBG_ON
#else
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#endif

#endif /* LWIP_DEBUG */

#define LWIP_DBG_TYPES_ON           (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE|LWIP_DBG_FRESH|LWIP_DBG_HALT)

#ifdef LPKG_LWIP_MEM_ALIGNMENT
#define MEM_ALIGNMENT LPKG_LWIP_MEM_ALIGNMENT
#else
#define MEM_ALIGNMENT               4
#endif

#define MEMP_OVERFLOW_CHECK         1
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 1

//#define MEM_LIBC_MALLOC             1
//#define MEM_USE_POOLS               1
//#define MEMP_USE_CUSTOM_POOLS       1
//#define MEM_SIZE                    (1024*64)

#define MEMP_MEM_MALLOC             0

/* MEMP_NUM_PBUF: the number of memp struct pbufs. If the application
   sends a lot of data out of ROM (or other static memory), this
   should be set high. */
#define MEMP_NUM_PBUF               32 //16

/* the number of struct netconns */
#ifdef LPKG_MEMP_NUM_NETCONN
#define MEMP_NUM_NETCONN            LPKG_MEMP_NUM_NETCONN
#endif

/* the number of UDP protocol control blocks. One per active RAW "connection". */
#ifdef LPKG_LWIP_RAW_PCB_NUM
#define MEMP_NUM_RAW_PCB            LPKG_LWIP_RAW_PCB_NUM
#endif

/* the number of UDP protocol control blocks. One per active UDP "connection". */
#ifdef LPKG_LWIP_UDP_PCB_NUM
#define MEMP_NUM_UDP_PCB            LPKG_LWIP_UDP_PCB_NUM
#endif

/* the number of simulatenously active TCP connections. */
#ifdef LPKG_LWIP_TCP_PCB_NUM
#define MEMP_NUM_TCP_PCB            LPKG_LWIP_TCP_PCB_NUM
#endif

/* the number of simultaneously queued TCP */
#ifdef LPKG_LWIP_TCP_SEG_NUM
#define MEMP_NUM_TCP_SEG            LPKG_LWIP_TCP_SEG_NUM
#else
#define MEMP_NUM_TCP_SEG            TCP_SND_QUEUELEN
#endif

/*
 * You can re-define following setting in rtcofnig.h to overwrite the default
 * setting in the lwip opts.h
 */
/* MEMP_NUM_NETBUF: the number of struct netbufs. */
// #define MEMP_NUM_NETBUF             2
/* MEMP_NUM_NETCONN: the number of struct netconns. */
// #define MEMP_NUM_NETCONN            4

/* MEMP_NUM_TCPIP_MSG_*: the number of struct tcpip_msg, which is used
   for sequential API communication and incoming packets. Used in
   src/api/tcpip.c. */
// #define MEMP_NUM_TCPIP_MSG_API      16
// #define MEMP_NUM_TCPIP_MSG_INPKT    16

/* ---------- Pbuf options ---------- */
/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */
#ifdef LPKG_LWIP_PBUF_NUM
#define PBUF_POOL_SIZE               LPKG_LWIP_PBUF_NUM
#endif

/* PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. */
#ifdef LPKG_LWIP_PBUF_POOL_BUFSIZE
#define PBUF_POOL_BUFSIZE            LPKG_LWIP_PBUF_POOL_BUFSIZE
#endif

/* PBUF_LINK_HLEN: the number of bytes that should be allocated for a
   link level header. */
#define PBUF_LINK_HLEN              16

#ifdef LPKG_LWIP_ETH_PAD_SIZE
#define ETH_PAD_SIZE                LPKG_LWIP_ETH_PAD_SIZE
#endif

#ifdef LWIP_USING_NAT
#define IP_NAT                      1
#else
#define IP_NAT                      0
#endif

/* ---------- TCP options ---------- */
#ifdef LPKG_LWIP_TCP
#define LWIP_TCP                    1
#else
#define LWIP_TCP                    0
#endif

#define TCP_TTL                     255

/* Controls if TCP should queue segments that arrive out of
   order. Define to 0 if your device is low on memory. */
#define TCP_QUEUE_OOSEQ             1

/* TCP Maximum segment size. */
#define TCP_MSS                     1460

/* TCP sender buffer space (bytes). */
#ifdef LPKG_LWIP_TCP_SND_BUF
#define TCP_SND_BUF                 LPKG_LWIP_TCP_SND_BUF
#else
#define TCP_SND_BUF                 (TCP_MSS * 2)
#endif

/* TCP sender buffer space (pbufs). This must be at least = 2 *
   TCP_SND_BUF/TCP_MSS for things to work. */
#define TCP_SND_QUEUELEN            (4 * TCP_SND_BUF/TCP_MSS)

/* TCP writable space (bytes). This must be less than or equal
   to TCP_SND_BUF. It is the amount of space which must be
   available in the tcp snd_buf for select to return writable */
#define TCP_SNDLOWAT                (TCP_SND_BUF/2)
#define TCP_SNDQUEUELOWAT           TCP_SND_QUEUELEN/2

/* TCP receive window. */
#ifdef LPKG_LWIP_TCP_WND
#define TCP_WND                     LPKG_LWIP_TCP_WND
#else
#define TCP_WND                     (TCP_MSS * 2)
#endif

/* Maximum number of retransmissions of data segments. */
#define TCP_MAXRTX                  12

/* Maximum number of retransmissions of SYN segments. */
#define TCP_SYNMAXRTX               4

/* tcpip thread options */
#ifdef LPKG_LWIP_TCPTHREAD_PRIORITY
#define TCPIP_MBOX_SIZE             LPKG_LWIP_TCPTHREAD_MBOX_SIZE
#define TCPIP_THREAD_PRIO           LPKG_LWIP_TCPTHREAD_PRIORITY
#define TCPIP_THREAD_STACKSIZE      LPKG_LWIP_TCPTHREAD_STACKSIZE
#else
#define TCPIP_MBOX_SIZE             8
#define TCPIP_THREAD_PRIO           128
#define TCPIP_THREAD_STACKSIZE      4096
#endif
#define TCPIP_THREAD_NAME           "tcpip"
#define DEFAULT_TCP_RECVMBOX_SIZE   10

/* ---------- ARP options ---------- */
#define LWIP_ARP                    1
#define ARP_TABLE_SIZE              10
#define ARP_QUEUEING                1

/* ---------- Checksum options ---------- */
#ifdef LPKG_LWIP_USING_HW_CHECKSUM
#define CHECKSUM_GEN_IP                 0
#define CHECKSUM_GEN_UDP                0
#define CHECKSUM_GEN_TCP                0
#define CHECKSUM_GEN_ICMP               0
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_CHECK_UDP              0
#define CHECKSUM_CHECK_TCP              0
#define CHECKSUM_CHECK_ICMP             0
#endif

/* ---------- IP options ---------- */
/* Define IP_FORWARD to 1 if you wish to have the ability to forward
   IP packets across network interfaces. If you are going to run lwIP
   on a device with only one network interface, define this to 0. */
#define IP_FORWARD                  0

/* IP reassembly and segmentation.These are orthogonal even
 * if they both deal with IP fragments */
#ifdef LPKG_LWIP_REASSEMBLY_FRAG
#define IP_REASSEMBLY               1
#define IP_FRAG                     1
#define IP_REASS_MAX_PBUFS          10
#define MEMP_NUM_REASSDATA          10
#else
#define IP_REASSEMBLY               0
#define IP_FRAG                     0
#endif

/* ---------- ICMP options ---------- */
#define ICMP_TTL                    255

/* ---------- DHCP options ---------- */
/* Define LWIP_DHCP to 1 if you want DHCP configuration of
   interfaces. */
#ifdef LPKG_LWIP_DHCP
#define LWIP_DHCP                   1
#else
#define LWIP_DHCP                   0
#endif

/* 1 if you want to do an ARP check on the offered address
   (recommended). */
#define DHCP_DOES_ARP_CHECK         (LWIP_DHCP)

/* ---------- AUTOIP options ------- */
#define LWIP_AUTOIP                 0
#define LWIP_DHCP_AUTOIP_COOP       (LWIP_DHCP && LWIP_AUTOIP)

/* ---------- UDP options ---------- */
#ifdef LPKG_LWIP_UDP
#define LWIP_UDP                    1
#else
#define LWIP_UDP                    0
#endif

#define LWIP_UDPLITE                0
#define UDP_TTL                     255
#define DEFAULT_UDP_RECVMBOX_SIZE   1

/* ---------- RAW options ---------- */
#ifdef LPKG_LWIP_RAW
#define LWIP_RAW                    1
#else
#define LWIP_RAW                    0
#endif

#define DEFAULT_RAW_RECVMBOX_SIZE   1
#define DEFAULT_ACCEPTMBOX_SIZE     10

/* ---------- Statistics options ---------- */
#ifdef LPKG_LWIP_STATS
#define LWIP_STATS                  1
#define LWIP_STATS_DISPLAY          1
#else
#define LWIP_STATS                  0
#endif

#if LWIP_STATS
#define LINK_STATS                  1
#define IP_STATS                    1
#define ICMP_STATS                  1
#define IGMP_STATS                  1
#define IPFRAG_STATS                1
#define UDP_STATS                   1
#define TCP_STATS                   1
#define MEM_STATS                   1
#define MEMP_STATS                  1
#define PBUF_STATS                  1
#define SYS_STATS                   1
#define MIB2_STATS                  1
#endif /* LWIP_STATS */

/* ---------- PPP options ---------- */
#ifdef LPKG_LWIP_PPP
#define PPP_SUPPORT                 1      /* Set > 0 for PPP */
#else
#define PPP_SUPPORT                 0      /* Set > 0 for PPP */
#endif

#if PPP_SUPPORT
#define NUM_PPP                     1      /* Max PPP sessions. */

/* Select modules to enable.  Ideally these would be set in the makefile but
 * we're limited by the command line length so you need to modify the settings
 * in this file.
 */
#ifdef LPKG_LWIP_PPPOE
#define PPPOE_SUPPORT               1
#else
#define PPPOE_SUPPORT               0
#endif

#ifdef LPKG_LWIP_PPPOS
#define PPPOS_SUPPORT               1
#else
#define PPPOS_SUPPORT               0
#endif

#define PAP_SUPPORT                 1      /* Set > 0 for PAP. */
#define CHAP_SUPPORT                1      /* Set > 0 for CHAP. */
#define MSCHAP_SUPPORT              0      /* Set > 0 for MSCHAP (NOT FUNCTIONAL!) */
#define CBCP_SUPPORT                0      /* Set > 0 for CBCP (NOT FUNCTIONAL!) */
#define CCP_SUPPORT                 0      /* Set > 0 for CCP (NOT FUNCTIONAL!) */
#define VJ_SUPPORT                  1      /* Set > 0 for VJ header compression. */
#define MD5_SUPPORT                 1      /* Set > 0 for MD5 (see also CHAP) */

#endif /* PPP_SUPPORT */

/**
 * LWIP_POSIX_SOCKETS_IO_NAMES==1: Enable POSIX-style sockets functions names.
 * Disable this option if you use a POSIX operating system that uses the same
 * names (read, write & close). (only used if you use sockets.c)
 */
#ifndef LWIP_POSIX_SOCKETS_IO_NAMES
#define LWIP_POSIX_SOCKETS_IO_NAMES     0
#endif

/**
 * LWIP_TCP_KEEPALIVE==1: Enable TCP_KEEPIDLE, TCP_KEEPINTVL and TCP_KEEPCNT
 * options processing. Note that TCP_KEEPIDLE and TCP_KEEPINTVL have to be set
 * in seconds. (does not require sockets.c, and will affect tcp.c)
 */
#ifndef LWIP_TCP_KEEPALIVE
#define LWIP_TCP_KEEPALIVE              1
#endif

/**
 * LWIP_NETIF_HOSTNAME==1: Support netif hostname
 */
#ifndef LWIP_NETIF_HOSTNAME
#define LWIP_NETIF_HOSTNAME             1
#endif

/**
 * LWIP_NETIF_API==1: Support netif api (in netifapi.c)
 */
#ifndef LWIP_NETIF_API
#define LWIP_NETIF_API                  1
#endif

/* MEMP_NUM_SYS_TIMEOUT: the number of simulateously active timeouts. */
#define MEMP_NUM_SYS_TIMEOUT       (LWIP_TCP + IP_REASSEMBLY + LWIP_ARP + (2*LWIP_DHCP) + LWIP_AUTOIP + LWIP_IGMP + LWIP_DNS + PPP_SUPPORT)

/*
 * LWIP_COMPAT_SOCKETS==1: Enable BSD-style sockets functions names.
 * (only used if you use sockets.c)
 */
#ifdef SAL_USING_POSIX
#define LWIP_COMPAT_SOCKETS             0
#else
#ifndef LWIP_COMPAT_SOCKETS
#define LWIP_COMPAT_SOCKETS             1
#endif
#endif

/**
 * LWIP_SO_SNDTIMEO==1: Enable send timeout for sockets/netconns and
 * SO_SNDTIMEO processing.
 */
#ifndef LWIP_SO_SNDTIMEO
#define LWIP_SO_SNDTIMEO                1
#endif

/**
 * LWIP_SO_RCVTIMEO==1: Enable receive timeout for sockets/netconns and
 * SO_RCVTIMEO processing.
 */
#ifndef LWIP_SO_RCVTIMEO
#define LWIP_SO_RCVTIMEO                1
#endif

/**
 * LWIP_SO_RCVBUF==1: Enable SO_RCVBUF processing.
 */
#ifndef LWIP_SO_RCVBUF
#define LWIP_SO_RCVBUF                  1
#endif

/**
 * If LWIP_SO_RCVBUF is used, this is the default value for recv_bufsize.
 */
#ifndef RECV_BUFSIZE_DEFAULT
#define RECV_BUFSIZE_DEFAULT            8192
#endif

/**
 * SO_REUSE==1: Enable SO_REUSEADDR option.
 */
#ifndef SO_REUSE
#define SO_REUSE                        0
#endif


#if LPKG_USING_LWIP_VER_NUM >= 0x20000 /* >= v2.0.0 */
#define LWIP_HOOK_IP4_ROUTE_SRC(dest, src)  lwip_ip4_route_src(dest, src)
#include "lwip/ip_addr.h"
struct netif *lwip_ip4_route_src(const ip4_addr_t *dest, const ip4_addr_t *src);
#endif /* LPKG_USING_LWIP_VER_NUM >= 0x20000 */

#endif

#endif /* __LWIPOPTS_H__ */
