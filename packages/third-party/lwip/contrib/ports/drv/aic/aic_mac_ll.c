/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <aic_hal.h>

#define align_cache_line(addr, len, a, l) \
unsigned long a = (unsigned long)addr; \
unsigned long l = (unsigned long)len; \
if (addr % CACHE_LINE_SIZE) { \
    a -= (addr %  CACHE_LINE_SIZE); \
    l += (addr %  CACHE_LINE_SIZE); \
} \
if (l % CACHE_LINE_SIZE) { \
    l += CACHE_LINE_SIZE - (l % CACHE_LINE_SIZE); \
}

void aicmac_low_level_init(uint32_t port, bool en)
{
    uint32_t id = CLK_GMAC0 + port;

    /* pin-mux */

    /* mac clock */
    if (en) {
        /* Set clock frequence = 50M */
        hal_clk_set_freq(id, 50000000);
        /* clock enable & reset deassert */
        hal_clk_enable_deassertrst_iter(id);
    } else {
        /* clock enable & reset deassert */
        hal_clk_disable_assertrst(id);
    }
}

void aicmac_dcache_clean(uintptr_t addr, uint32_t len)
{
#ifndef CONFIG_MAC_USE_UNCACHE_BUF
    align_cache_line(addr, len, a, l);
    aicos_dcache_clean_range((unsigned long *)a, (int64_t)l);
#endif
}

void aicmac_dcache_invalid(uintptr_t addr, uint32_t len)
{
#ifndef CONFIG_MAC_USE_UNCACHE_BUF
    align_cache_line(addr, len, a, l);
    aicos_dcache_invalid_range((unsigned long *)a, (int64_t)l);
#endif
}

void aicmac_dcache_clean_invalid(uintptr_t addr, uint32_t len)
{
#ifndef CONFIG_MAC_USE_UNCACHE_BUF
    align_cache_line(addr, len, a, l);
    aicos_dcache_clean_invalid_range((unsigned long *)a, (int64_t)l);
#endif
}

