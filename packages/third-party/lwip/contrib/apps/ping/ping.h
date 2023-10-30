#ifndef LWIP_PING_H
#define LWIP_PING_H

#include "lwip/opt.h"

/**
 * PING_USE_SOCKETS: Set to 1 to use sockets, otherwise the raw api is used
 */
#ifdef LPKG_PING_USING_SOCKET
#define PING_USE_SOCKETS    1
#else
#define PING_USE_SOCKETS    0
#endif

extern void cmd_ping(int argc, char **argv);
#endif /* LWIP_PING_H */
