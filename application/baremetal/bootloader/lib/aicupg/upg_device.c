/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <string.h>
#include <usbdescriptors.h>
#include <usbdevice.h>
#include <usb_drv.h>
#include <aic_core.h>
#include <aicupg.h>
#include <trans_rw_data.h>

#ifdef AICUPG_DEBUG
#undef pr_err
#undef pr_warn
#undef pr_info
#undef pr_debug
#define pr_err printf
#define pr_warn printf
#define pr_info printf
#define pr_debug printf
#endif

#define AIC_BULK_EP_HS_MPS 512

#if defined(AICUPG_USB_ENABLE)
struct usb_misc_descriptors {
    struct usb_configuration_descriptor config;
    struct usb_interface_descriptor intf;
    struct usb_endpoint_descriptor ep_in;
    struct usb_endpoint_descriptor ep_out;
} __attribute__((packed));

static u8 usb_connect_flag = 0;
#define STR_COUNT 3
static struct usb_string_descriptor *str_desc_table[STR_COUNT];

static struct usb_device_descriptor device_desc = {
    .bLength = sizeof(struct usb_device_descriptor),
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x200,
    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 0x40,
    .idVendor = DRIVER_VENDOR_ID,
    .idProduct = DRIVER_PRODUCT_ID,
    .bcdDevice = 0x0101,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 0,
    .bNumConfigurations = 1,
};

static struct usb_qualifier_descriptor qualifier_desc = {
    .bLength = sizeof(struct usb_qualifier_descriptor),
    .bDescriptorType = USB_DT_DEVICE_QUALIFIER,
    .bcdUSB = 0x200,
    .bDeviceClass = 0xff,
    .bDeviceSubClass = 0xff,
    .bDeviceProtocol = 0xff,
    .bMaxPacketSize0 = 0x40,
    .bNumConfigurations = 1,
    .breserved = 0,
};

static struct usb_misc_descriptors misc_desc = {
    .config = {
        .bLength             = sizeof(struct usb_configuration_descriptor),
        .bDescriptorType     = USB_DT_CONFIG,
        .wTotalLength        = sizeof(struct usb_misc_descriptors),
        .bNumInterfaces      = 1,
        .bConfigurationValue = 1,
        .iConfiguration      = 0,
        .bmAttributes        = 0x80, //not self powered
        .bMaxPower           = 0xFA, // Max 500mA(0xfa * 2)
    },
    .intf = {
        .bLength             = sizeof(struct usb_interface_descriptor),
        .bDescriptorType     = USB_DT_INTERFACE,
        .bInterfaceNumber    = 0x00,
        .bAlternateSetting   = 0x00,
        .bNumEndpoints       = 0x02,
        .bInterfaceClass     = 0xff,
        .bInterfaceSubClass  = 0xff,
        .bInterfaceProtocol  = 0xff,
        .iInterface          = 0,
    },
    .ep_in = {
        .bLength             = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType     = USB_DT_ENDPOINT,
        .bEndpointAddress    = (0x80 | BULK_IN_EP_INDEX),
        .bmAttributes        = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize      = AIC_BULK_EP_HS_MPS,
        .bInterval           = 0x00,
    },
    .ep_out = {
        .bLength             = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType     = USB_DT_ENDPOINT,
        .bEndpointAddress    = BULK_OUT_EP_INDEX,
        .bmAttributes        = USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize      = AIC_BULK_EP_HS_MPS,
        .bInterval           = 0x00,
    },
};

static void string_descriptors_init(void)
{
    static u8 lang[4] = { 4, USB_DT_STRING, 0x9, 0x4 };
    static u8 manufacturer[22] = {
        22, USB_DT_STRING, 'A', 0,   'r', 0,   't', 0,   'i', 0, 'n',
        0,  'c',           0,   'h', 0,   'i', 0,   'p', 0,   0, 0
    };
    static u8 product[36] = { 36,  USB_DT_STRING,
                              'A', 0,
                              'r', 0,
                              't', 0,
                              'i', 0,
                              'n', 0,
                              'c', 0,
                              'h', 0,
                              'i', 0,
                              'p', 0,
                              ' ', 0,
                              'D', 0,
                              'e', 0,
                              'v', 0,
                              'i', 0,
                              'c', 0,
                              'e', 0,
                              0,   0 };
    str_desc_table[0] = (struct usb_string_descriptor *)lang;
    str_desc_table[1] = (struct usb_string_descriptor *)manufacturer;
    str_desc_table[2] = (struct usb_string_descriptor *)product;
}

static s32 usb_get_status(struct usb_device_request *req)
{
    u8 len = 0, rep;
    u8 buf[2];

    if (req->wLength == 0) {
        /* sent zero packet */
        aic_udc_ctrl_ep_send(NULL, 0);
        return AIC_USB_REQ_OP_ERR;
    }

    len = min(req->wLength, (u16)2);
    rep = (req->bmRequestType & USB_REQ_RECIPIENT_MASK);

    if (rep == USB_RECIP_DEVICE) {
        /* buf[0]
         * Bit[0]: Self-power or bus-power
         *         - 0: Bus-power
         *         - 1: Self-power
         * Bit[1]: Remote wakeup
         *         - 0: Disabled
         *         - 1: Enabled
         */
        buf[0] = 1;
        buf[1] = 0;
    }
    if (rep == USB_RECIP_INTERFACE) {
        buf[0] = 0;
        buf[1] = 0;
    }
    if (rep == USB_RECIP_ENDPOINT) {
        buf[0] = 0;
        buf[1] = 0;
    }

    aic_udc_ctrl_ep_send(buf, len);

    return AIC_USB_REQ_SUCCESSED;
}

static s32 usb_set_configuration(struct usb_device_request *req)
{
    pr_debug("set configuration\n");
    /* Only support 1 configuration so nak anything else */
    if (req->wValue == 1) {
        aic_udc_bulk_ep_reset();
    } else {
        pr_err("err: invalid wValue, (0, %d)\n", req->wValue);
        return AIC_USB_REQ_OP_ERR;
    }

    aic_udc_set_configuration(req->wValue);

    usb_connect_flag = 1;
    return AIC_USB_REQ_SUCCESSED;
}

static s32 usb_set_address(struct usb_device_request *req)
{
    u8 address;

    address = req->wValue & 0x7f;
    aicos_udelay(10);

    pr_debug("set address 0x%x\n", address);
    aic_udc_set_address(address);

    return AIC_USB_REQ_SUCCESSED;
}

static s32 usb_set_interface(struct usb_device_request *req)
{
    pr_debug("set interface\n");
    /* Only support interface 0, alternate 0 */
    if ((req->wIndex == 0) && (req->wValue == 0)) {
        /*
         * Reset EP for Buik transfer
         */
        aic_udc_bulk_ep_reset();
    } else {
        pr_err("err: invalid wIndex and wValue, (0, %d), (0, %d)\n",
               req->wIndex, req->wValue);
        return AIC_USB_REQ_OP_ERR;
    }

    return AIC_USB_REQ_SUCCESSED;
}

static s32 usb_get_descriptor(struct usb_device_request *req)
{
    u32 len = 0;
    u8 str_idx;
    s32 bytes_total = 0, ret = AIC_USB_REQ_DEVICE_NOT_SUPPORTED;
    struct usb_device_descriptor *dev_desc;
    struct usb_qualifier_descriptor *qua_desc;

    switch (req->wValue >> 8) {
        case USB_DT_DEVICE: {
            pr_debug("get USB_DT_DEVICE\n");
            dev_desc = &device_desc;
            len = min(req->wLength, (u16)sizeof(struct usb_device_descriptor));
            aic_udc_ctrl_ep_send((u8 *)dev_desc, len);
            ret = AIC_USB_REQ_SUCCESSED;
            break;
        }
        case USB_DT_CONFIG: {
            pr_debug("get USB_DT_CONFIG\n");
            bytes_total = sizeof(struct usb_misc_descriptors);
            if (bytes_total > 0) {
                len = min(req->wLength, (u16)bytes_total);
                aic_udc_ctrl_ep_send((u8 *)&misc_desc, len);
                ret = AIC_USB_REQ_SUCCESSED;
            }
            break;
        }
        case USB_DT_STRING: {
            pr_debug("get USB_DT_STRING\n");
            str_idx = req->wValue & 0xff;
            /* Language ID */
            if (str_idx < STR_COUNT) {
                len = str_desc_table[str_idx]->bLength;
                aic_udc_ctrl_ep_send((u8 *)str_desc_table[str_idx], len);
                ret = AIC_USB_REQ_SUCCESSED;
            } else {
                pr_debug("string line 0x%x is not supported\n", str_idx);
            }
            break;
        }
        case USB_DT_DEVICE_QUALIFIER: {
            pr_debug("get qualifier descriptor\n");
            if (req->wLength < sizeof(struct usb_qualifier_descriptor))
                return ret;

            qua_desc = &qualifier_desc;
            aic_udc_ctrl_ep_send((u8 *)qua_desc, qua_desc->bLength);
            ret = AIC_USB_REQ_SUCCESSED;
            break;
        }
        default:
            pr_err("err: unkown wValue(0x%x)\n", req->wValue);
            break;
    }

    return ret;
}

static s32 usb_upg_init(void)
{
    string_descriptors_init();
    return 0;
}

static s32 usb_upg_exit(void)
{
    return 0;
}

static void usb_upg_reset(void)
{
}

static s32 usb_upg_standard_req(struct usb_device_request *req)
{
    u8 dir = 0;
    u8 rep = 0;
    s32 ret = AIC_USB_REQ_DEVICE_NOT_SUPPORTED;

    // pr_debug("%s\n", __func__);
    dir = (req->bmRequestType & USB_REQ_DIRECTION_MASK);
    rep = (req->bmRequestType & USB_REQ_RECIPIENT_MASK);

    /* standard */
    switch (req->bRequest) {
        case USB_REQ_GET_STATUS: {
            pr_debug("%s  USB_REQ_GET_STATUS\n", __func__);
            /* device-to-host */
            if (USB_DIR_IN == dir)
                ret = usb_get_status(req);
            break;
        }
        case USB_REQ_SET_ADDRESS: {
            pr_debug("%s USB_REQ_SET_ADDRESS\n", __func__);
            /* host-to-device */
            if ((USB_DIR_OUT == dir) && (USB_RECIP_DEVICE == rep))
                ret = usb_set_address(req);
            break;
        }
        case USB_REQ_GET_DESCRIPTOR: {
            pr_debug("%s  USB_REQ_GET_DESCRIPTOR\n", __func__);
            /* device-to-host */
            if ((USB_DIR_IN == dir) && (USB_RECIP_DEVICE == rep))
                ret = usb_get_descriptor(req);
            break;
        }
        case USB_REQ_GET_CONFIGURATION: {
            pr_debug("%s  USB_REQ_GET_CONFIGURATION\n", __func__);
            /* device-to-host */
            if ((USB_DIR_IN == dir) && (USB_RECIP_DEVICE == rep))
                ret = usb_get_descriptor(req);
            break;
        }
        case USB_REQ_SET_CONFIGURATION: {
            pr_debug("%s  USB_REQ_SET_CONFIGURATION\n", __func__);
            /* host-to-device */
            if ((USB_DIR_OUT == dir) && (USB_RECIP_DEVICE == rep))
                ret = usb_set_configuration(req);
            break;
        }
        case USB_REQ_SET_INTERFACE: {
            pr_debug("%s  USB_REQ_SET_INTERFACE\n", __func__);
            /* host-to-device */
            if ((USB_DIR_OUT == dir) && (USB_RECIP_INTERFACE == rep))
                ret = usb_set_interface(req);
            break;
        }
        default:
            pr_err("aic usb err: unsupport usb out request to device\n");
            break;
    }

    return ret;
}

static s32 usb_upg_nonstandard_req(struct usb_device_request *req)
{
    s32 ret = AIC_USB_REQ_DEVICE_NOT_SUPPORTED;

    pr_debug("%s\n", __func__);
    return ret;
}

static s32 usb_upg_state_cmd(u8 *buffer, u32 len)
{
    struct phy_data_rw rw;

    rw.recv = aic_udc_bulk_recv;
    rw.send = aic_udc_bulk_send;

    return trans_layer_rw_proc(&rw, buffer, len);
}

bool aic_upg_usb_connect_check(void)
{
    return usb_connect_flag;
}

struct usb_device usbupg_device = {
    .state_init = usb_upg_init,
    .state_exit = usb_upg_exit,
    .state_reset = usb_upg_reset,
    .standard_req_proc = usb_upg_standard_req,
    .nonstandard_req_proc = usb_upg_nonstandard_req,
    .state_cmd = usb_upg_state_cmd,
};
#endif
