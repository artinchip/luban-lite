/**
 * @file
 * Ping sender module
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 */

/**
 * This is an example of a "ping" sender (with raw API and socket API).
 * It can be used as a start point to maintain opened a network connection, or
 * like a network "watchdog" for your device.
 *
 */

#include "ping.h"

#if LWIP_RAW && defined(LPKG_LWIP_USING_PING) /* don't build if not configured for use in lwipopts.h */

#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"
#include "lwip/prot/ip4.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip_addr.h"

#if PING_USE_SOCKETS
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <string.h>
#endif /* PING_USE_SOCKETS */

#include <stdlib.h>
#include <string.h>
#include "osal/aic_osal.h"

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 1000
#endif

/** ping delay - in milliseconds */
#ifndef PING_DELAY
#define PING_DELAY 1000
#endif

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID 0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif

#ifndef DEFAULT_PING_TIMES
#define DEFAULT_PING_TIMES 4u
#endif

/* Ping usage information */
#define PING_USAGE1 "Usage:       ping <ip_address> [-t] [-n count]\n\n"
#define PING_USAGE2 "Options:\n"
#define PING_USAGE3 "  -t         ping the specified host forever.\n"
#define PING_USAGE4 "  -n count   number of echo requests to send," \
                    "valid range is from 1 to 65535.\n\n"
#define PING_USAGE5 "Notes\n"
#define PING_USAGE6 "  if '[-t]/[-n count]' appears before '[-n count]/[-t]'," \
                    "'[-n count]/[-t]' will be ignored.\n"

#define FORVER_PING_TIMES 0x7FFFFFFFul
#define FLAG_BUSY ((u16_t)1)
#define FLAG_FREE ((u16_t)0)

/* Ping UI interface */
#define PING_PRINTF(x) LWIP_PLATFORM_DIAG(x)
#define ping_ip4_addr_print_parts(a, b, c, d) \
  PING_PRINTF(("%" U16_F ".%" U16_F ".%" U16_F ".%" U16_F, a, b, c, d))

#define ping_ip4_addr_print(ipaddr)                         \
  ping_ip4_addr_print_parts(                                \
      (u16_t)((ipaddr) != NULL ? ip4_addr1_16(ipaddr) : 0), \
      (u16_t)((ipaddr) != NULL ? ip4_addr2_16(ipaddr) : 0), \
      (u16_t)((ipaddr) != NULL ? ip4_addr3_16(ipaddr) : 0), \
      (u16_t)((ipaddr) != NULL ? ip4_addr4_16(ipaddr) : 0))
#define ping_ip4_addr_print_val(ipaddr) \
  ping_ip4_addr_print_parts(            \
      ip4_addr1_16_val(ipaddr),         \
      ip4_addr2_16_val(ipaddr),         \
      ip4_addr3_16_val(ipaddr),         \
      ip4_addr4_16_val(ipaddr))

enum PING_MSG_STATUS {
  PING_MSG_READY = 0,
  PING_MSG_WAITING,
  PING_MSG_SUCCESS,
};

struct ping_msg {
  struct ping_msg *prev;
  struct ping_msg *next;
  ip_addr_t target_ip;
  u32_t conut;
  u32_t send_packet_num;
  u32_t recv_packet_num;
  u32_t sys_send_time;
  enum PING_MSG_STATUS status;
  u16_t ping_seq_num;
};

struct ping_msg *msg_list_head;
#if PING_USE_SOCKETS
static sys_sem_t ping_sem;
#else  /* PING_USE_SOCKETS */
/* ping variables */
static struct raw_pcb *ping_pcb;

static void ping_raw_init(void);
#endif /* !PING_USE_SOCKETS */

static void
ping_register(struct ping_msg *msg)
{
  struct ping_msg *temp;
  sys_prot_t prot;

  if (msg_list_head == NULL) {
    prot = sys_arch_protect();
    msg_list_head = msg;
    sys_arch_unprotect(prot);
#if PING_USE_SOCKETS
    sys_sem_signal(&ping_sem);
#else
    ping_raw_init();
#endif
    return;
  }

  temp = msg_list_head;
  while (temp->next != NULL)
    temp = temp->next;

  prot = sys_arch_protect();
  temp->next = msg;
  msg->prev = temp;
  sys_arch_unprotect(prot);
}

static void
ping_unregister(struct ping_msg *msg)
{
  struct ping_msg *pre_msg;
  sys_prot_t prot;

  prot = sys_arch_protect();
  if ((msg->next == NULL) && (msg->prev == NULL)) {
    msg_list_head = NULL;
  }
  else if ((msg->next == NULL) && msg->prev != NULL) {
    pre_msg = msg->prev;
    pre_msg->next = NULL;
    msg->prev = NULL;
  }
  else {
    pre_msg = msg->prev;
    pre_msg->next = msg;
    msg->next->prev = pre_msg;
  }
  sys_arch_unprotect(prot);

  aicos_free(MEM_DEFAULT, msg);
}

static int
ping_request(ip_addr_t *ip, u32_t count)
{
  struct ping_msg *msg;

  msg = aicos_malloc(MEM_DEFAULT, sizeof(struct ping_msg));
  if (msg == NULL) {
    PING_PRINTF(("Error lack of memory resource\n"));
    return -1;
  }
  msg->target_ip = *ip;
  msg->conut = count;
  msg->next = NULL;
  msg->prev = NULL;
  msg->ping_seq_num = 0;
  msg->recv_packet_num = 0;
  msg->status = PING_MSG_READY;
  msg->send_packet_num = 0;

  ping_register(msg);

  return 0;
}

/** Prepare a echo ICMP request */
static void
ping_prepare_echo(struct icmp_echo_hdr *iecho, u16_t len, struct ping_msg *msg)
{
  size_t i;
  size_t data_len = len - sizeof(struct icmp_echo_hdr);

  ICMPH_TYPE_SET(iecho, ICMP_ECHO);
  ICMPH_CODE_SET(iecho, 0);
  iecho->chksum = 0;
  iecho->id = PING_ID;
  iecho->seqno = lwip_htons(++(msg->ping_seq_num));

  /* fill the additional data buffer with some data */
  for (i = 0; i < data_len; i++) {
    ((char *)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
  }

  iecho->chksum = inet_chksum(iecho, len);
}

static void
ping_finish_report(struct ping_msg *msg)
{
  struct ping_msg *p_msg = msg;
  u32_t per;

  per = (u32_t)(100 * (p_msg->send_packet_num - p_msg->recv_packet_num) / p_msg->send_packet_num);
  PING_PRINTF(("\nPing statistics for "));
  ping_ip4_addr_print(&p_msg->target_ip);
  PING_PRINTF((":\n    Packets: Sent = %" U32_F " Received = %" U32_F
               " Lost = %" U32_F "(%" U32_F "%% loss)\n",
               p_msg->send_packet_num,
               p_msg->recv_packet_num,
               (p_msg->send_packet_num - p_msg->recv_packet_num),
               per));
}

#if PING_USE_SOCKETS

/* Ping using the socket ip */
static err_t
ping_send(int s, const ip_addr_t *addr, struct ping_msg *msg)
{
  int err;
  struct icmp_echo_hdr *iecho;
  struct sockaddr_storage to;
  size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
  LWIP_ASSERT("ping_size is too big", ping_size <= 0xffff);

#if LWIP_IPV6
  if (IP_IS_V6(addr) && !ip6_addr_isipv4mappedipv6(ip_2_ip6(addr))) {
    /* todo: support ICMP6 echo */
    return ERR_VAL;
  }
#endif /* LWIP_IPV6 */

  iecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
  if (!iecho) {
    return ERR_MEM;
  }

  ping_prepare_echo(iecho, (u16_t)ping_size, msg);

#if LWIP_IPV4
  if (IP_IS_V4(addr)) {
    struct sockaddr_in *to4 = (struct sockaddr_in *)&to;
    to4->sin_len = sizeof(to4);
    to4->sin_family = AF_INET;
    inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(addr));
  }
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
  if (IP_IS_V6(addr)) {
    struct sockaddr_in6 *to6 = (struct sockaddr_in6 *)&to;
    to6->sin6_len = sizeof(to6);
    to6->sin6_family = AF_INET6;
    inet6_addr_from_ip6addr(&to6->sin6_addr, ip_2_ip6(addr));
  }
#endif /* LWIP_IPV6 */

  err = lwip_sendto(s, iecho, ping_size, 0, (struct sockaddr *)&to, sizeof(to));

  mem_free(iecho);

  return (err ? ERR_OK : ERR_VAL);
}

static void
ping_recv(int s, struct ping_msg *head_msg)
{
  char buf[64];
  int len;
  struct sockaddr_storage from;
  int fromlen = sizeof(from);
  struct ping_msg *msg;
  struct ping_msg *temp_msg;

  while ((len = lwip_recvfrom(s, buf, sizeof(buf), 0,
         (struct sockaddr *)&from, (socklen_t *)&fromlen)) > 0) {
    if (len >= (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr)))
    {
      ip_addr_t fromaddr;
      memset(&fromaddr, 0, sizeof(fromaddr));

#if LWIP_IPV4
      if (from.ss_family == AF_INET) {
        struct sockaddr_in *from4 = (struct sockaddr_in *)&from;
        inet_addr_to_ip4addr(ip_2_ip4(&fromaddr), &from4->sin_addr);
        IP_SET_TYPE_VAL(fromaddr, IPADDR_TYPE_V4);
      }
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
      if (from.ss_family == AF_INET6) {
        struct sockaddr_in6 *from6 = (struct sockaddr_in6 *)&from;
        inet6_addr_to_ip6addr(ip_2_ip6(&fromaddr), &from6->sin6_addr);
        IP_SET_TYPE_VAL(fromaddr, IPADDR_TYPE_V6);
      }
#endif /* LWIP_IPV6 */

      for (msg = head_msg; msg != NULL; msg = msg->next) {
        if (!memcmp(&msg->target_ip, &fromaddr, sizeof(fromaddr)))
          break;
      }
      if (msg == NULL)
        continue;

        /* todo: support ICMP6 echo */
#if LWIP_IPV4
      if (IP_IS_V4_VAL(fromaddr))
      {
        struct ip_hdr *iphdr;
        struct icmp_echo_hdr *iecho;

        iphdr = (struct ip_hdr *)buf;
        iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
        if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(msg->ping_seq_num))) {
          PING_PRINTF(("Reply from "));
          ping_ip4_addr_print(&fromaddr);
          PING_PRINTF((": time=%" U32_F "ms\n", (sys_now() - msg->sys_send_time)));
          /* do some ping result processing */
          PING_RESULT((ICMPH_TYPE(iecho) == ICMP_ER));
          msg->recv_packet_num++;
          msg->status = PING_MSG_SUCCESS;
        }
      }

#endif /* LWIP_IPV4 */
    }
    fromlen = sizeof(from);
  }

  for (temp_msg = head_msg; temp_msg != NULL; temp_msg = temp_msg->next) {
    /* check timeout */
    if (temp_msg->status == PING_MSG_WAITING) {
      PING_PRINTF(("Request timed out\n"));
    }
    else if (temp_msg->status == PING_MSG_READY)
      continue;

    if ((--(temp_msg->conut) == 0)) {
      ping_finish_report(temp_msg);
      ping_unregister(temp_msg);
    }
  }

  /* do some ping result processing */
  PING_RESULT(0);
}

static void
ping_thread(void *arg)
{
  int s;
  int ret;
  struct ping_msg *temp_msg;
  ip_addr_t *ping_remote;

#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
  int timeout = PING_RCV_TIMEO;
#else
  struct timeval timeout;
  timeout.tv_sec = PING_RCV_TIMEO / 1000;
  timeout.tv_usec = (PING_RCV_TIMEO % 1000) * 1000;
#endif
  LWIP_UNUSED_ARG(arg);

#if LWIP_IPV6
  if (IP_IS_V4(ping_target) || ip6_addr_isipv4mappedipv6(ip_2_ip6(ping_target))) {
    s = lwip_socket(AF_INET6, SOCK_RAW, IP_PROTO_ICMP);
  }
  else {
    s = lwip_socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6);
  }
#else
  s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
#endif
  if (s < 0) {
    return;
  }

  ret = lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  LWIP_ASSERT("setting receive timeout failed", ret == 0);
  LWIP_UNUSED_ARG(ret);

  while (1) {
    sys_arch_sem_wait(&ping_sem, 0);
    do
    {
      for (temp_msg = msg_list_head; temp_msg != NULL; temp_msg = temp_msg->next) {
        ping_remote = &temp_msg->target_ip;
        if (ping_send(s, ping_remote, temp_msg) == ERR_OK) {
          temp_msg->sys_send_time = sys_now();
          temp_msg->send_packet_num++;
          temp_msg->status = PING_MSG_WAITING;
        }
        else {
          PING_PRINTF(("ping: send "));
          ping_ip4_addr_print(ping_remote);
          PING_PRINTF((" - error\n"));
        }
      }
      ping_recv(s, msg_list_head);
    } while (msg_list_head != NULL);
  }
}

static int ping_thread_init(void)
{
  if (sys_sem_new(&ping_sem, 0) != ERR_OK)
    LWIP_ASSERT("failed to create ping_thread mbox", 0);

  sys_thread_new("ping_thread", ping_thread, NULL, 2048, LPKG_PING_THREAD_PRIORITY);

  return 0;
}

INIT_DEVICE_EXPORT(ping_thread_init);
#else  /* PING_USE_SOCKETS */

/* Ping using the raw ip */
static u8_t
ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
  struct icmp_echo_hdr *iecho;
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(addr);
  LWIP_ASSERT("p != NULL", p != NULL);
  struct ping_msg *msg;

  if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) &&
      pbuf_remove_header(p, PBUF_IP_HLEN) == 0) {
    iecho = (struct icmp_echo_hdr *)p->payload;

    for (msg = msg_list_head; msg != NULL; msg = msg->next)
    {
      if (!memcmp(&msg->target_ip, addr, sizeof(ip_addr_t)))
        break;
    }
    if (msg == NULL) {
      pbuf_add_header(p, PBUF_IP_HLEN);
      return 0; /* don't eat the packet */
    }

    if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(msg->ping_seq_num))) {
      PING_PRINTF(("Reply from "));
      ping_ip4_addr_print(addr);
      PING_PRINTF((": time=%" U32_F "ms\n", (sys_now() - msg->sys_send_time)));

      msg->recv_packet_num++;
      msg->status = PING_MSG_SUCCESS;

      /* do some ping result processing */
      PING_RESULT(1);
      pbuf_free(p);
      return 1; /* eat the packet */
    }
    /* not eaten, restore original packet */
    pbuf_add_header(p, PBUF_IP_HLEN);
  }

  return 0; /* don't eat the packet */
}

static void
ping_send(struct raw_pcb *raw, const ip_addr_t *addr, struct ping_msg *msg)
{
  struct pbuf *p;
  struct icmp_echo_hdr *iecho;
  size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;

  LWIP_ASSERT("ping_size <= 0xffff", ping_size <= 0xffff);

  p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
  if (!p) {
    return;
  }
  if ((p->len == p->tot_len) && (p->next == NULL)) {
    iecho = (struct icmp_echo_hdr *)p->payload;

    ping_prepare_echo(iecho, (u16_t)ping_size, msg);

    raw_sendto(raw, p, addr);
    msg->send_packet_num++;
    msg->sys_send_time = sys_now();
  }
  pbuf_free(p);
}

static void
ping_timeout(void *arg)
{
  struct raw_pcb *pcb = (struct raw_pcb *)arg;
  struct ping_msg *msg;

  LWIP_ASSERT("ping_timeout: no pcb given!", pcb != NULL);

  for (msg = msg_list_head; msg != NULL; msg = msg->next) {
    /* check timeout */
    if (msg->status == PING_MSG_WAITING) {
      PING_PRINTF(("Request timed out\n"));
    }

    if (msg->status != PING_MSG_READY)
      msg->conut--;
    if (msg->conut == 0) {
      ping_finish_report(msg);
      ping_unregister(msg);
      continue;
    }
    ping_send(pcb, &msg->target_ip, msg);
    msg->status = PING_MSG_WAITING;
  }

  if (msg_list_head != NULL)
    sys_timeout(PING_DELAY, ping_timeout, pcb);
}

static void
ping_raw_init(void)
{
  ping_pcb = raw_new(IP_PROTO_ICMP);
  LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);

  raw_recv(ping_pcb, ping_recv, NULL);
  raw_bind(ping_pcb, IP_ADDR_ANY);
  sys_timeout(PING_DELAY, ping_timeout, ping_pcb);
}
#endif /* !PING_USE_SOCKETS */

static void ping_usage(void)
{
  PING_PRINTF(("%s", PING_USAGE1));
  PING_PRINTF(("%s", PING_USAGE2));
  PING_PRINTF(("%s", PING_USAGE3));
  PING_PRINTF(("%s", PING_USAGE4));
  PING_PRINTF(("%s", PING_USAGE5));
  PING_PRINTF(("%s", PING_USAGE6));
}

/* @todo: add ping dns if needed  */
void cmd_ping(int argc, char **argv)
{
  static ip_addr_t remote_ip;
  int i;
  u32_t ping_count = DEFAULT_PING_TIMES;

  if (argc < 2) {
    PING_PRINTF(("Argument error: no destination ip address!\n"));
    goto ping_help;
  }
  else if (argc > 5) {
    PING_PRINTF(("Argument error: too many argument\n"));
    goto ping_help;
  }

  if (!strcmp(argv[1], "help"))
    goto ping_help;

  if ((remote_ip.addr = ipaddr_addr(argv[1])) == IPADDR_NONE) {
    PING_PRINTF(("Ping request could not find host %s." \
                 " Please check the name and try again\n",
                  argv[1]));
    goto out;
  }

  for (i = 2; i < argc; ++i) {
    if (!strcmp(argv[i], "-t")) {
      ping_count = FORVER_PING_TIMES;
      break;
    }
    else if (!strcmp(argv[i], "-n") && (i < argc - 1)) {
      ping_count = (u32_t)atoi(argv[i + 1]);
      if ((ping_count == 0) || (ping_count > 0xFFFF)) {
        PING_PRINTF(("Bad value for option -n, valid range is from 1 to 65535\n"));
        goto out;
      }
      break;
    }
  }
  if ((i == argc) && (argc != 2)) {
    PING_PRINTF(("Bad parameter\n"));
    goto out;
  }

  ping_request(&remote_ip, ping_count);
out:
  return;
ping_help:
  ping_usage();
}
MSH_CMD_EXPORT_ALIAS(cmd_ping, ping, "ping help" for more information);
#endif /* LWIP_RAW && defined(LPKG_LWIP_USING_PING) */
