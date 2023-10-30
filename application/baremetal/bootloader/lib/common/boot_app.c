/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <image.h>
#include <boot.h>
#include <aic_core.h>
#include <aic_time.h>
#include <boot_time.h>
#include <console.h>
#include <boot_param.h>

void boot_app(void *app)
{
    int ret;
    void (*ep)(int);
    enum boot_device dev;

    ret = console_get_ctrlc();
    if (ret > 0)
        return;
#ifndef LPKG_USING_FDTLIB
    ep = image_get_entry_point(app);
    if (!ep) {
        printf("Entry point is null.\n");
        while(1)
            continue;
    }
#else
    ep = app;
#endif
    boot_time_trace("Run APP");
    boot_time_show();
    dev = aic_get_boot_device();
    aicos_dcache_clean();
    ep(dev);
}
