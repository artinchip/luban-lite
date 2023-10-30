/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_DRV_BARE_H_
#define _ARTINCHIP_AIC_DRV_BARE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_hal.h>
#include "driver.h"
#include "aic_tlsf.h"
#include "tlsf.h"
#include "heap.h"
#include "aic_stdio.h"
#include "console.h"
#include "uart.h"
#include "mmc.h"
#include "mtd.h"
#ifdef AIC_SPINAND_DRV
#include "spinand_port.h"
#endif
#ifdef AIC_BOOT_USB_DRV
#include "usb_drv.h"
#endif
#include "wdt.h"

#ifdef __cplusplus
}
#endif

#endif /* _ARTINCHIP_AIC_DRV_BARE_H_ */
