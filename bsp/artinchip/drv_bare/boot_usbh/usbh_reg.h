/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef _SOC_USBHOST_REG_H_
#define _SOC_USBHOST_REG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>

#define USB_HOST_REG_BASE(id)  ((USB_HOST0_BASE) + (id)*0x10000)
#define USBH_REG_HOST_CTL(id)  (USB_HOST_REG_BASE(id) + 0x800)
#define USB_EHCI_HCOR_BASE(id) ((void *)(USB_HOST_REG_BASE(id) + 0x10))

#ifdef __cplusplus
}
#endif

#endif
