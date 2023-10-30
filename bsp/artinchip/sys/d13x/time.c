/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include "sys_freq.h"

void aic_udelay(u32 us)
{
    u64 start = aic_get_ticks();
    u32 cnt = us * (drv_get_sys_freq() / 1000000U);

    while (1) {
        u64 cur = aic_get_ticks();

        if (start > cur) {
            if ((start - cur) >= cnt)
                break;
        } else {
            if (cur - start >= cnt)
                break;
        }
    }
}

void aic_mdelay(u32 ms)
{
    aic_udelay(ms * 1000);
}

u64 aic_get_ticks(void)
{
    return (((u64)csi_coret_get_valueh() << 32U) | csi_coret_get_value());
}

u32 aic_get_time_us(void)
{
    u32 cnt = (drv_get_sys_freq() / 1000000U);

    return (u32)(aic_get_ticks() / cnt);
}

u32 aic_get_time_ms(void)
{
    return aic_get_time_us() / 1000;
}

#ifdef KERNEL_RTTHREAD
#include <rtthread.h>

void rt_hw_us_delay(rt_uint32_t us)
{
    aic_udelay(us);
}
#endif

