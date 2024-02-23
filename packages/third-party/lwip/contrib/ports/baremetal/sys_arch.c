/*
 * Copyright (c) 2006-2022, ArtInChip
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <arch/sys_arch.h>
#include <lwip/sys.h>
#include <lwip/opt.h>
#include <lwip/stats.h>
#include <lwip/err.h>
#include <lwip/debug.h>
#include <lwip/netif.h>
#include <lwip/netifapi.h>
#include <lwip/tcpip.h>
#include <lwip/sio.h>
#include <lwip/init.h>
#include <lwip/dhcp.h>
#include <lwip/inet.h>

#include <aic_osal.h>

void sys_init(void)
{

}

/* ====================== System ====================== */

sys_prot_t sys_arch_protect(void)
{
	unsigned long state;
	aicos_local_irq_save(&state);

    return state;
}

void sys_arch_unprotect(sys_prot_t pval)
{
	csi_irq_restore(pval);
}

void sys_arch_assert(const char *file, int line)
{
    printf("\nAssertion: %d in %s", line, file);
}

u32_t sys_jiffies(void)
{
    return (u32)aic_get_time_ms();
}

u32_t sys_now(void)
{
    return aic_get_time_ms();
}


#ifdef RT_LWIP_PPP
u32_t sio_read(sio_fd_t fd, u8_t *buf, u32_t size)
{
	#if 0
    u32_t len;

    RT_ASSERT(fd != RT_NULL);

    len = rt_device_read((rt_device_t)fd, 0, buf, size);
    if (len <= 0)
        return 0;

    return len;
	#endif
	
	return size;
}

u32_t sio_write(sio_fd_t fd, u8_t *buf, u32_t size)
{
//    RT_ASSERT(fd != RT_NULL);
//    return rt_device_write((rt_device_t)fd, 0, buf, size);
     return size;
}

void sio_read_abort(sio_fd_t fd)
{
//    rt_kprintf("read_abort\n");
}

void ppp_trace(int level, const char *format, ...)
{
    aic_log(level,  const char *format, ...);
}
#endif /* RT_LWIP_PPP */

#ifdef LWIP_HOOK_IP4_ROUTE_SRC
struct netif *lwip_ip4_route_src(const ip4_addr_t *dest, const ip4_addr_t *src)
{
    struct netif *netif;

    /* iterate through netifs */
    for (netif = netif_list; netif != NULL; netif = netif->next)
    {
        /* is the netif up, does it have a link and a valid address? */
        if (netif_is_up(netif) && netif_is_link_up(netif) && !ip4_addr_isany_val(*netif_ip4_addr(netif)))
        {
            /* gateway matches on a non broadcast interface? (i.e. peer in a point to point interface) */
            if (src != NULL)
            {
                if (ip4_addr_cmp(src, netif_ip4_addr(netif)))
                {
                    return netif;
                }
            }
        }
    }
    netif = netif_default;
    return netif;
}
#endif /* LWIP_HOOK_IP4_ROUTE_SRC */

