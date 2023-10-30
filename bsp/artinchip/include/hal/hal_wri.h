/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_WRI_H_
#define _ARTINCHIP_HAL_WRI_H_

#include "aic_common.h"

#if defined(AIC_WRI_DRV_V12) || defined(AIC_WRI_DRV_V11) || defined(AIC_WRI_DRV_V10)
enum aic_warm_reset_type {
    WRI_TYPE_POR = 0,
    WRI_TYPE_RTC,
    WRI_TYPE_EXT,
    WRI_TYPE_DM,
    WRI_TYPE_WDT,
    WRI_TYPE_TSEN,
    WRI_TYPE_CMP,
    WRI_TYPE_MAX
};
#endif

#if defined(AIC_WRI_DRV_V13)
enum aic_warm_reset_type {
    WRI_TYPE_VDD11_SP_POR = 0,
    WRI_TYPE_VDD11_SW_POR,
    WRI_TYPE_RTC_POR,
    WRI_TYPE_PIN_RST,
    WRI_TYPE_THS_RST,
    WRI_TYPE_SP_WDOG_RST,
    WRI_TYPE_SP_DM_NDM_RST,
    WRI_TYPE_SP_DM_CPU_RST,
    WRI_TYPE_CS_WDOG_RST,
    WRI_TYPE_CS_DM_NDM_RST,
    WRI_TYPE_CS_DM_CPU_RST,
    WRI_TYPE_SC_WDOG_RST,
    WRI_TYPE_SC_DM_NDM_RST,
    WRI_TYPE_SC_DM_CPU_RST,
    WRI_TYPE_SE_WDOG_RST,
    WRI_TYPE_SE_DM_NDM_RST,
    WRI_TYPE_MAX
};
#endif

enum aic_warm_reset_type aic_wr_type_get(void);
enum aic_reboot_reason aic_judge_reboot_reason(enum aic_warm_reset_type hw,
                                               u32 sw);

#endif
