/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aic_core.h"
#include "aic_reboot_reason.h"

int drv_wri_init(void)
{
    /* From WRI V1.2, the follow API is defined in WRI instead of RTC */
#if defined(AIC_WRI_DRV_V12) || defined(AIC_RTC_DRV_V10) || defined(AIC_RTC_DRV_V11)
    aic_get_reboot_reason();
    aic_show_startup_time();
#endif
    return 0;
}
INIT_ENV_EXPORT(drv_wri_init);

#if defined(RT_USING_FINSH)
#include <finsh.h>

#ifdef AIC_USING_WDT
void cmd_reboot(int argc, char **argv);
#endif

static void cmd_aicupg(int argc, char **argv)
{
    aic_set_reboot_reason(REBOOT_REASON_UPGRADE);
#ifdef AIC_USING_WDT
    cmd_reboot(0, NULL);
#endif
}

MSH_CMD_EXPORT_ALIAS(cmd_aicupg, aicupg, Reboot to the upgrade mode);

#endif
