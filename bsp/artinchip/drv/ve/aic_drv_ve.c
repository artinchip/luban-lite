/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: <qi.xu@artinchip.com>
 */

#include <rtconfig.h>
#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <aic_drv.h>
#endif
#include "aic_hal_ve.h"

struct aic_ve_client *drv_ve_open(void)
{
    return hal_ve_open();
}

int drv_ve_close(struct aic_ve_client *client)
{
    return hal_ve_close(client);
}

int drv_ve_control(struct aic_ve_client *client, int cmd, void *arg)
{
    return hal_ve_control(client, cmd, arg);
}


#if defined(KERNEL_RTTHREAD)
INIT_DEVICE_EXPORT(hal_ve_probe);
#endif
