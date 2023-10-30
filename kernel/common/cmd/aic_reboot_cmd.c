/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#ifdef RT_USING_FINSH
#include <string.h>
#include <stdlib.h>
#include <finsh.h>
#include "aic_reboot_reason.h"
#include <drivers/watchdog.h>
#include "aic_core.h"
#include "aic_drv_wdt.h"

#define TIMEOUT         0

#if defined(AIC_CHIP_AIC1606CS)
#define REBOOT_REASON_CMD_REBOOT REBOOT_REASON_CS_CMD_REBOOT
#elif defined(AIC_CHIP_AIC1606SC)
#define REBOOT_REASON_CMD_REBOOT REBOOT_REASON_SC_CMD_REBOOT
#elif defined(AIC_CHIP_AIC1606SP)
#define REBOOT_REASON_CMD_REBOOT REBOOT_REASON_SP_CMD_REBOOT
#endif


void cmd_reboot(int argc, char **argv)
{
    u32 timeout = TIMEOUT;
    rt_device_t wdt_dev = RT_NULL;

    wdt_dev =  rt_device_find("wdt");
    rt_device_init(wdt_dev);

    LOG_I("Restarting system ...\n");
    aicos_msleep(100);
    #ifdef AIC_WRI_DRV
    aic_set_reboot_reason(REBOOT_REASON_CMD_REBOOT);
    #endif
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);

    aicos_msleep(1000);
    LOG_W("Watchdog doesn't work!");
}
MSH_CMD_EXPORT_ALIAS(cmd_reboot, reboot, Reboot the system);


#endif /* RT_USING_FINSH */
