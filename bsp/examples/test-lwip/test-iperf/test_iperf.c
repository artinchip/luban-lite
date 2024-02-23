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
 * Author: Dirk Ziegelmeier <dziegel@gmx.de>
 *
 */

#include <stdio.h>
#include <string.h>
#include "lwip/apps/lwiperf.h"
#include "lwip/err.h"
#if defined(RT_USING_FINSH)
#include <finsh.h>
#elif defined(AIC_CONSOLE_BARE_DRV)
#include <console.h>
#endif

static void *lwiperf_session;

static void
iperf_report_callback(void *arg, enum lwiperf_report_type report_type,
  const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
  u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(local_addr);
  LWIP_UNUSED_ARG(local_port);

  printf("IPERF report: type=%d, remote: %s:%d, total bytes: %"U32_F", duration in ms: %"U32_F", kbits/s: %"U32_F"\n",
    (int)report_type, ipaddr_ntoa(remote_addr), (int)remote_port, bytes_transferred, ms_duration, bandwidth_kbitpsec);
}

int iperf(int argc, char **argv)
{
  if ((argc < 2) || (argc > 3)) {
    printf("Argument error\n");
    return ERR_VAL;
  }

  if (strcmp(argv[1], "-s") == 0) {
    if (lwiperf_session != NULL) {
      printf("Iperf server is running\n");
      return ERR_ALREADY;
    }
    lwiperf_session = lwiperf_start_tcp_server_default(iperf_report_callback, NULL);
  } else if ((strcmp(argv[1], "-c") == 0) && (argc == 3)) {
    ip_addr_t remote_ip;
    ipaddr_aton(argv[2], &remote_ip);
    lwiperf_start_tcp_client_default(&remote_ip, iperf_report_callback, NULL);
  } else {
    printf("Argument error\n");
    return ERR_VAL;
  }

  return ERR_OK;
}

#if defined(RT_USING_FINSH)
MSH_CMD_EXPORT_ALIAS(iperf, iperf, iperf [-s]/[-c server ip]);
#elif defined(AIC_CONSOLE_BARE_DRV)
CONSOLE_CMD(iperf, iperf, "iperf [-s]/[-c server ip]");
#endif
