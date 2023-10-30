/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _USB_DRV_H_
#define _USB_DRV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>
#include "usbdevice.h"

#define CTRL_EP_INDEX     (0)
#define BULK_IN_EP_INDEX  (1)
#define BULK_OUT_EP_INDEX (2)

#define AIC_USB_REQ_SUCCESSED            (0)
#define AIC_USB_REQ_DEVICE_NOT_SUPPORTED (-1)
#define AIC_USB_REQ_UNKNOWN_COMMAND      (-2)
#define AIC_USB_REQ_UNMATCHED_COMMAND    (-3)
#define AIC_USB_REQ_DATA_HUNGRY          (-4)
#define AIC_USB_REQ_OP_ERR               (-5)

struct usb_device {
    s32 (*state_init)(void);
    s32 (*state_exit)(void);
    void (*state_reset)(void);
    s32 (*standard_req_proc)(struct usb_device_request *req);
    s32 (*nonstandard_req_proc)(struct usb_device_request *req);
    s32 (*state_cmd)(u8 *buffer, u32 len);
};

s32 aic_udc_init(struct usb_device *udev);
s32 aic_udc_exit(void);
void aic_udc_state_loop(void);
s32 aic_udc_set_address(u8 address);
s32 aic_udc_set_configuration(s32 config_param);
u32 aic_udc_ctrl_ep_send(u8 *buf, u32 len);
s32 aic_udc_bulk_send(u8 *buf, s32 buf_siz);
s32 aic_udc_bulk_recv(u8 *buf, s32 siz);
s32 aic_udc_bulk_recv_dma(u8 *buf, s32 len);
void aic_udc_bulk_ep_reset(void);
void aic_udc_bulk_rx_stall(void);
void aic_udc_bulk_tx_stall(void);
void aic_udc_bulk_rx_fatal(void);
void aic_udc_bulk_tx_fatal(void);

#ifdef __cplusplus
}
#endif

#endif /* _USB_DRV_H_ */

