/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef __AIC_UDC_H__
#define __AIC_UDC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>
#include "usbdevice.h"

struct aic_ubuf {
    u8 *rx_buf;
    u32 rx_len;
    u32 ep_id;
};
struct aic_udc {
    u32 dma_send_ch;
    u32 dma_recv_ch;
    u32 reset_conn;
    struct usb_device_request std_req;
    struct usb_device *gadget;
    struct aic_ubuf buf;
};

#ifdef __cplusplus
}
#endif

#endif

