/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef _USBH_CORE_H
#define _USBH_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>
#include <usbdescriptors.h>
#include "usb_hub.h"
#include "usb_ehci.h"

#define USBH_ROOT_HUB_INDEX       1 /* roothub index*/
#define USBH_EX_HUB_INDEX         2 /* external hub index */
#define USBH_HUB_PORT_START_INDEX 1 /* first hub port index */

#define USB_CLASS_MATCH_VENDOR        0x0001
#define USB_CLASS_MATCH_PRODUCT       0x0002
#define USB_CLASS_MATCH_INTF_CLASS    0x0004
#define USB_CLASS_MATCH_INTF_SUBCLASS 0x0008
#define USB_CLASS_MATCH_INTF_PROTOCOL 0x0010

#define CONFIG_USBHOST_EP_NUM   4
#define CONFIG_USBHOST_INTF_NUM 6
#define CONFIG_USBHOST_EHPORTS  4

#define USB_SIZEOF_SETUP_PACKET 8

struct usbh_hubport;
struct usbh_class_driver {
    const char *driver_name;
    int (*connect)(struct usbh_hubport *hport, u8 intf);
    int (*disconnect)(struct usbh_hubport *hport, u8 intf);
};

typedef struct usbh_endpoint {
    struct usb_endpoint_descriptor ep_desc;
} usbh_endpoint_t;

typedef struct usbh_interface {
    struct usb_interface_descriptor intf_desc;
    struct usbh_endpoint ep[CONFIG_USBHOST_EP_NUM];
    struct usbh_class_driver *class_driver;
    void *priv;
} usbh_interface_t;

typedef struct usbh_configuration {
    struct usb_configuration_descriptor config_desc;
    struct usbh_interface intf[CONFIG_USBHOST_INTF_NUM];
} usbh_configuration_t;

typedef struct usbh_hubport {
    int connected;     /* True: device connected; false: disconnected */
    int port_change;   /* True: port changed; false: port do not change */
    u8 port;           /* Hub port index */
    u8 dev_addr;       /* device address */
    u8 speed;          /* device speed */
    usbh_epinfo_t ep0; /* control ep info */
    struct usb_device_descriptor device_desc;
    struct usbh_configuration config;
    struct usb_setup_packet setup __attribute__((aligned(CACHE_LINE_SIZE)));
    struct usbh_hub *parent; /*if NULL, is roothub*/
} usbh_hubport_t;

typedef struct usbh_hub {
    u8 index;    /* Hub index */
    u8 nports;   /* Hub port number */
    u8 dev_addr; /* Hub device address */
    usbh_epinfo_t intin;
    u8 *int_buffer;
    struct hub_port_status *port_status;
    struct usb_hub_descriptor hub_desc;
} usbh_hub_t;

#ifdef __cplusplus
}
#endif

#endif
