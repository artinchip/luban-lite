/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __REBOOT_REASON_H__
#define __REBOOT_REASON_H__
#include "aic_common.h"

enum aic_reboot_reason {
    REBOOT_REASON_COLD = 0,
#if defined(AIC_WRI_DRV_V12) || defined(AIC_WRI_DRV_V11) || defined(AIC_WRI_DRV_V10)
    REBOOT_REASON_CMD_REBOOT = 1,
    REBOOT_REASON_CMD_SHUTDOWN = 2,
    REBOOT_REASON_SUSPEND = 3,
    REBOOT_REASON_UPGRADE = 4,
    REBOOT_REASON_FASTBOOT = 5,
#endif
#if defined(AIC_WRI_DRV_V13)
    REBOOT_REASON_CS_CMD_REBOOT = 1,
    REBOOT_REASON_SC_CMD_REBOOT = 2,
    REBOOT_REASON_SP_CMD_REBOOT = 3,
    REBOOT_REASON_CMD_SHUTDOWN = 4,
    REBOOT_REASON_SUSPEND = 5,
    REBOOT_REASON_UPGRADE = 6,
    REBOOT_REASON_FASTBOOT = 7,
#endif
    /* Some software exception reason */
    REBOOT_REASON_SW_LOCKUP = 8,
    REBOOT_REASON_HW_LOCKUP = 9,
    REBOOT_REASON_PANIC = 10,
    REBOOT_REASON_RAMDUMP = 11,

    /* Some hardware exception reason */
#if defined(AIC_WRI_DRV_V12) || defined(AIC_WRI_DRV_V11) || defined(AIC_WRI_DRV_V10)
    REBOOT_REASON_RTC = 17,
    REBOOT_REASON_EXTEND = 18,
    REBOOT_REASON_JTAG = 19,
    REBOOT_REASON_OTP = 20,
    REBOOT_REASON_UNDER_VOL = 21,
#endif
#if defined(AIC_WRI_DRV_V13)
    REBOOT_REASON_VDD11_SP_POR = 17,
    REBOOT_REASON_VDD11_SW_POR = 18,
    REBOOT_REASON_RTC_POR = 19,
    REBOOT_REASON_PIN_RST = 20,
    REBOOT_REASON_THS_RST = 21,
    REBOOT_REASON_SP_WDOG_RST = 22,
    REBOOT_REASON_SP_DM_NDM_RST = 23,
    REBOOT_REASON_SP_DM_CPU_RST = 24,
    REBOOT_REASON_CS_WDOG_RST = 25,
    REBOOT_REASON_CS_DM_NDM_RST = 26,
    REBOOT_REASON_CS_DM_CPU_RST = 27,
    REBOOT_REASON_SC_WDOG_RST = 28,
    REBOOT_REASON_SC_DM_NDM_RST = 29,
    REBOOT_REASON_SC_DM_CPU_RST = 30,
    REBOOT_REASON_SE_WDOG_RST = 31,
    REBOOT_REASON_SE_DM_NDM_RST = 32,
#endif

    REBOOT_REASON_INVALID = 0xff,
};

/* Defined in ArtInChip RTC/WRI module */

void aic_set_reboot_reason(enum aic_reboot_reason reason);
enum aic_reboot_reason aic_get_reboot_reason(void);

void aic_show_gtc_time(char *tag, u32 val);
void aic_show_startup_time(void);

#endif // end of __REBOOT_REASON_H__
