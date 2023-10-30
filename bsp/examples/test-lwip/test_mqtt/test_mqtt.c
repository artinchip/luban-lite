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

#include "lwip/apps/mqtt.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if LWIP_TCP

/** Define this to a compile-time IP address initialization
 * to connect anything else than IPv4 loopback
 */
#ifndef LWIP_MQTT_EXAMPLE_IPADDR_INIT
#if LWIP_IPV4
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT = IPADDR4_INIT(IPADDR_LOOPBACK)
#else
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT
#endif
#endif

static ip_addr_t mqtt_ip LWIP_MQTT_EXAMPLE_IPADDR_INIT;
static mqtt_client_t* mqtt_client;
static mqtt_connection_status_t gstatus;

static const struct mqtt_connect_client_info_t mqtt_client_info =
{
  "test",
  NULL, /* user */
  NULL, /* pass */
  100,  /* keep alive */
  NULL, /* will_topic */
  NULL, /* will_msg */
  0,    /* will_qos */
  0     /* will_retain */
#if LWIP_ALTCP && LWIP_ALTCP_TLS
  , NULL
#endif
};

static void 
mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(data);

  printf("MQTT client \"%s\" data cb: len %d, flags %d\n", client_info->client_id,
          (int)len, (int)flags);
}

static void
mqtt_request_cb(void *arg, err_t err)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  printf("MQTT client \"%s\" request cb: err %d\n", client_info->client_id, (int)err);
}

static void
mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(client);

  printf("MQTT client \"%s\" connection cb: status %d\n", client_info->client_id, (int)status);

  gstatus = status;
  if (status != MQTT_CONNECT_ACCEPTED) {
    mqtt_client_free(client);
    mqtt_client = NULL;
  }
}

void
mqtt_example_init(char *ip)
{
  mqtt_ip.addr = ipaddr_addr(ip);
  mqtt_client = mqtt_client_new();

  mqtt_client_connect(mqtt_client,
          &mqtt_ip, MQTT_PORT,
          mqtt_connection_cb, LWIP_CONST_CAST(void*, &mqtt_client_info),
          &mqtt_client_info);

  mqtt_set_inpub_callback(mqtt_client,
          NULL,
          mqtt_incoming_data_cb,
          LWIP_CONST_CAST(void*, &mqtt_client_info));
}

void
mqtt_punlish_cb(void *arg, err_t err)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  printf("MQTT client \"%s\" publish %s\n", client_info->client_id, err?"err":"success");
}

void
printf_usage(void)
{
  printf("test_mqtt:             [-c broker_ip]/[-s topic]/[-u topic]/[-p topic payload]/[-q]\n" \
         " -c: <broker_ip>       Connect to mqtt broker\n" \
         " -s: <topic> <type>    Subscribe to [topic] with <type:0,1,2>\n" \
         " -u: <topic>           Unsubscribe to [topic]\n" \
         " -p: <topic> <payload> publish message/[payload] to [topic]\n" \
         " -q:                   Quit mqtt test\n");
}

static void
cmd_test_mqtt(int argc, char **argv)
{
  if ((strcmp(argv[1], "-c") == 0) && (argc == 3)) {
    if (mqtt_client != NULL) {
      printf("MQTT is processing\n");
      return ;
    }
    mqtt_example_init(argv[2]);
  } else if ((strcmp(argv[1], "-s") == 0) && (argc == 4)) {
    if (gstatus != MQTT_CONNECT_ACCEPTED) {
      printf("MQTT is not connected\n");
      return ;
    }
    mqtt_subscribe(mqtt_client, argv[2], atoi(argv[3]),
                   mqtt_request_cb, LWIP_CONST_CAST(void *, &mqtt_client_info));
  }
  else if ((strcmp(argv[1], "-u") == 0) && (argc == 3)) {
    if (gstatus != MQTT_CONNECT_ACCEPTED) {
      printf("MQTT is not connected\n");
      return ;
    }
    mqtt_unsubscribe(mqtt_client, argv[2], mqtt_request_cb,
                     LWIP_CONST_CAST(void *, &mqtt_client_info));
  } else if ((strcmp(argv[1], "-p") == 0) && (argc == 4)) {
    mqtt_publish(mqtt_client, argv[2], argv[3], strlen(argv[3]), 2, 0,
    		     mqtt_punlish_cb, LWIP_CONST_CAST(void *, &mqtt_client_info));
  } else if ((strcmp(argv[1], "-q") == 0) && (argc == 2)) {
    mqtt_disconnect(mqtt_client);
    mqtt_client_free(mqtt_client);
    mqtt_client = NULL;
  } else 
    printf_usage();
}

MSH_CMD_EXPORT_ALIAS(cmd_test_mqtt, test_mqtt, test_mqtt -h for more information);
#endif /* LWIP_TCP */
