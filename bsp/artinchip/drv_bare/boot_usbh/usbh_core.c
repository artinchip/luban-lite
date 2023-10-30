/*
 * Copyright (c) 2020-2022 Artinchip Technology Co., Ltd. All rights reserved.
 *
 * Dehuang Wu <dehuang.wu@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <driver.h>
#include <aic_core.h>

#include <usbdescriptors.h>
#include <usbdevice.h>
#include "usbh_reg.h"
#include "usb_ehci.h"
#include "usbh_core.h"
#include "usb_hub.h"

#define TAG "UHCO"
/* general descriptor field offsets */
#define DESC_bLength            0 /** Length offset */
#define DESC_bDescriptorType    1 /** Descriptor type offset */
#define USB_REQUEST_BUFFER_SIZE 256

struct usbh_hubport rhport;

static int usbh_enumerate(struct usbh_hubport *hport, int id);

extern int usbh_control_transfer(usbh_epinfo_t ep,
                                 struct usb_setup_packet *setup, u8 *buffer,
                                 int id);
extern int usbh_msc_connect(struct usbh_hubport *hport, u8 intf, int id);

static void usbh_hport_activate(struct usbh_hubport *hport)
{
    struct usbh_endpoint_cfg ep0_cfg;

    memset(&ep0_cfg, 0, sizeof(struct usbh_endpoint_cfg));

    ep0_cfg.ep_addr = 0x00;
    ep0_cfg.ep_interval = 0x00;
    ep0_cfg.ep_mps = 0x08;
    ep0_cfg.ep_type = USB_ENDPOINT_TYPE_CONTROL;
    ep0_cfg.hport = hport;
    /* Allocate memory for roothub port control endpoint */
    usbh_ep_alloc(&hport->ep0, &ep0_cfg);
}

int usbh_initialize(int id)
{
    int ret;

    usb_hc_sw_init();
    ret = usb_hc_hw_init(id);
    if (ret) {
        pr_err("usb_hc_hw_init failed.\n");
        return ret;
    }

    aicos_udelay(1500);
    memset(&rhport, 0, sizeof(rhport));
    usbh_hport_activate(&rhport);
    rhport.port = 1;
    ret = usbh_portchange_wait(id);
    if (ret) {
        pr_err("usbh_portchange_wait failed.\n");
        return ret;
    }
    rhport.port_change = true;
    if (usbh_get_port_connect_status(id, rhport.port) == false) {
        pr_err("port not connected.\n");
        return -1;
    }
    rhport.connected = true;

    /* Not support external hub case, here assume it is roothub port */
    ret = usbh_reset_port(rhport.port, id);
    if (ret) {
        pr_err("port reset failed.\n");
        return ret;
    }

    /* Get the current device speed */
    rhport.speed = usbh_get_port_speed(id);
    ret = usbh_enumerate(&rhport, id);
    return ret;
}

int usbh_get_connect_id(void)
{
    int ret = -1, port = 1, id = 0, num = 1;

#if defined(AICUPG_UDISK_ENABLE)
    num = AICUPG_USB_CONTROLLER_MAX_NUM;
#endif
    for (id = 0; id < num; id++) {
        ret = usb_hc_hw_fast_init(id);
        if (ret) {
            pr_err("usb_hc_hw_fast_init failed.\n");
            return ret;
        }

        aicos_udelay(1500);
        ret = usbh_portchange_wait(id);
        if (ret) {
            pr_warn("usb %d port change wait failed.\n", id);
            continue;
        }

        if (usbh_get_port_connect_status(id, port) == false) {
            pr_warn("usb %d port not connected.\n", id);
            continue;
        } else {
            return id;
        }
    }

    return -1;
}

static int parse_device_descriptor(struct usbh_hubport *hport,
                                   struct usb_device_descriptor *desc,
                                   u16 length)
{
    if (desc->bLength != USB_SIZEOF_DEVICE_DESC) {
        pr_err("invalid device bLength 0x%02x\n", desc->bLength);
        return -1;
    } else if (desc->bDescriptorType != USB_DESCRIPTOR_TYPE_DEVICE) {
        pr_err("unexpected device descriptor 0x%02x\n", desc->bDescriptorType);
        return -1;
    }

    if (length <= 8)
        return 0;
#if 0
    pr_debug("Device Descriptor:\n");
    pr_debug("bLength: 0x%02x           \n", desc->bLength);
    pr_debug("bDescriptorType: 0x%02x   \n", desc->bDescriptorType);
    pr_debug("bcdUSB: 0x%04x            \n", desc->bcdUSB);
    pr_debug("bDeviceClass: 0x%02x      \n", desc->bDeviceClass);
    pr_debug("bDeviceSubClass: 0x%02x   \n", desc->bDeviceSubClass);
    pr_debug("bDeviceProtocol: 0x%02x   \n", desc->bDeviceProtocol);
    pr_debug("bMaxPacketSize0: 0x%02x   \n", desc->bMaxPacketSize0);
    pr_debug("idVendor: 0x%04x          \n", desc->idVendor);
    pr_debug("idProduct: 0x%04x         \n", desc->idProduct);
    pr_debug("bcdDevice: 0x%04x         \n", desc->bcdDevice);
    pr_debug("iManufacturer: 0x%02x     \n", desc->iManufacturer);
    pr_debug("iProduct: 0x%02x          \n", desc->iProduct);
    pr_debug("iSerialNumber: 0x%02x     \n", desc->iSerialNumber);
    pr_debug("bNumConfigurations: 0x%02x\n", desc->bNumConfigurations);
#endif
    hport->device_desc.bLength = desc->bLength;
    hport->device_desc.bDescriptorType = desc->bDescriptorType;
    hport->device_desc.bcdUSB = desc->bcdUSB;
    hport->device_desc.bDeviceClass = desc->bDeviceClass;
    hport->device_desc.bDeviceSubClass = desc->bDeviceSubClass;
    hport->device_desc.bDeviceProtocol = desc->bDeviceProtocol;
    hport->device_desc.bMaxPacketSize0 = desc->bMaxPacketSize0;
    hport->device_desc.idVendor = desc->idVendor;
    hport->device_desc.idProduct = desc->idProduct;
    hport->device_desc.bcdDevice = desc->bcdDevice;
    hport->device_desc.iManufacturer = desc->iManufacturer;
    hport->device_desc.iProduct = desc->iProduct;
    hport->device_desc.iSerialNumber = desc->iSerialNumber;
    hport->device_desc.bNumConfigurations = desc->bNumConfigurations;
    return 0;
}

static int parse_config_descriptor(struct usbh_hubport *hport,
                                   struct usb_configuration_descriptor *desc,
                                   u16 length)
{
    u32 total_len = 0;
    u8 ep_num = 0;
    u8 intf_num = 0;
    u8 *p = (u8 *)desc;
    struct usb_interface_descriptor *intf_desc;
    struct usb_endpoint_descriptor *ep_desc;

    if (desc->bLength != USB_SIZEOF_CONFIG_DESC) {
        pr_err("invalid config bLength 0x%02x\n", desc->bLength);
        return -1;
    } else if (desc->bDescriptorType != USB_DESCRIPTOR_TYPE_CONFIGURATION) {
        pr_err("unexpected config descriptor 0x%02x\n", desc->bDescriptorType);
        return -1;
    }
    if (length <= USB_SIZEOF_CONFIG_DESC) {
        return 0;
    }
#if 0
    pr_debug("Config Descriptor:\n");
    pr_debug("bLength: 0x%02x             \n", desc->bLength);
    pr_debug("bDescriptorType: 0x%02x     \n", desc->bDescriptorType);
    pr_debug("wTotalLength: 0x%04x        \n", desc->wTotalLength);
    pr_debug("bNumInterfaces: 0x%02x      \n", desc->bNumInterfaces);
    pr_debug("bConfigurationValue: 0x%02x \n", desc->bConfigurationValue);
    pr_debug("iConfiguration: 0x%02x      \n", desc->iConfiguration);
    pr_debug("bmAttributes: 0x%02x        \n", desc->bmAttributes);
    pr_debug("bMaxPower: 0x%02x           \n", desc->bMaxPower);
#endif
    hport->config.config_desc.bLength = desc->bLength;
    hport->config.config_desc.bDescriptorType = desc->bDescriptorType;
    hport->config.config_desc.wTotalLength = desc->wTotalLength;
    hport->config.config_desc.bNumInterfaces = desc->bNumInterfaces;
    hport->config.config_desc.bConfigurationValue = desc->bConfigurationValue;
    hport->config.config_desc.iConfiguration = desc->iConfiguration;
    hport->config.config_desc.iConfiguration = desc->iConfiguration;
    hport->config.config_desc.bmAttributes = desc->bmAttributes;
    hport->config.config_desc.bMaxPower = desc->bMaxPower;

    if (length <= USB_SIZEOF_CONFIG_DESC)
        return 0;

    while (p[DESC_bLength] && (total_len < desc->wTotalLength) &&
           (intf_num < desc->bNumInterfaces)) {
        p += p[DESC_bLength];
        total_len += p[DESC_bLength];
        if (p[DESC_bDescriptorType] != USB_DESCRIPTOR_TYPE_INTERFACE)
            continue;
        intf_desc = (struct usb_interface_descriptor *)p;
#if 0
        pr_debug("Interface Descriptor:\n");
        pr_debug("bLength: 0x%02x            \n", intf_desc->bLength);
        pr_debug("bDescriptorType: 0x%02x    \n", intf_desc->bDescriptorType);
        pr_debug("bInterfaceNumber: 0x%02x   \n", intf_desc->bInterfaceNumber);
        pr_debug("bAlternateSetting: 0x%02x  \n", intf_desc->bAlternateSetting);
        pr_debug("bNumEndpoints: 0x%02x      \n", intf_desc->bNumEndpoints);
        pr_debug("bInterfaceClass: 0x%02x    \n", intf_desc->bInterfaceClass);
        pr_debug("bInterfaceSubClass: 0x%02x \n", intf_desc->bInterfaceSubClass);
        pr_debug("bInterfaceProtocol: 0x%02x \n", intf_desc->bInterfaceProtocol);
        pr_debug("iInterface: 0x%02x         \n", intf_desc->iInterface);
#endif
        memset(&hport->config.intf[intf_num], 0, sizeof(struct usbh_interface));
        hport->config.intf[intf_num].intf_desc.bLength = intf_desc->bLength;
        hport->config.intf[intf_num].intf_desc.bDescriptorType =
            intf_desc->bDescriptorType;
        hport->config.intf[intf_num].intf_desc.bInterfaceNumber =
            intf_desc->bInterfaceNumber;
        hport->config.intf[intf_num].intf_desc.bAlternateSetting =
            intf_desc->bAlternateSetting;
        hport->config.intf[intf_num].intf_desc.bNumEndpoints =
            intf_desc->bNumEndpoints;
        hport->config.intf[intf_num].intf_desc.bInterfaceClass =
            intf_desc->bInterfaceClass;
        hport->config.intf[intf_num].intf_desc.bInterfaceSubClass =
            intf_desc->bInterfaceSubClass;
        hport->config.intf[intf_num].intf_desc.bInterfaceProtocol =
            intf_desc->bInterfaceProtocol;
        hport->config.intf[intf_num].intf_desc.iInterface =
            intf_desc->iInterface;
        ep_num = 0;
        while (p[DESC_bLength] && (total_len < desc->wTotalLength) &&
               (ep_num < intf_desc->bNumEndpoints)) {
            p += p[DESC_bLength];
            total_len += p[DESC_bLength];
            if (p[DESC_bDescriptorType] != USB_DESCRIPTOR_TYPE_ENDPOINT)
                continue;
            ep_desc = (struct usb_endpoint_descriptor *)p;
#if 0
            pr_debug("Endpoint Descriptor:\n");
            pr_debug("bLength: 0x%02x          \n", ep_desc->bLength);
            pr_debug("bDescriptorType: 0x%02x  \n", ep_desc->bDescriptorType);
            pr_debug("bEndpointAddress: 0x%02x \n", ep_desc->bEndpointAddress);
            pr_debug("bmAttributes: 0x%02x     \n", ep_desc->bmAttributes);
            pr_debug("wMaxPacketSize: 0x%04x   \n", ep_desc->wMaxPacketSize);
            pr_debug("bInterval: 0x%02x        \n", ep_desc->bInterval);
#endif
            memset(&hport->config.intf[intf_num].ep[ep_num], 0,
                   sizeof(struct usbh_endpoint));
            hport->config.intf[intf_num].ep[ep_num].ep_desc.bLength =
                ep_desc->bLength;
            hport->config.intf[intf_num].ep[ep_num].ep_desc.bDescriptorType =
                ep_desc->bDescriptorType;
            hport->config.intf[intf_num].ep[ep_num].ep_desc.bEndpointAddress =
                ep_desc->bEndpointAddress;
            hport->config.intf[intf_num].ep[ep_num].ep_desc.bmAttributes =
                ep_desc->bmAttributes;
            hport->config.intf[intf_num].ep[ep_num].ep_desc.wMaxPacketSize =
                ep_desc->wMaxPacketSize;
            hport->config.intf[intf_num].ep[ep_num].ep_desc.bInterval =
                ep_desc->bInterval;
            ep_num++;
        }
        intf_num++;
    }
    return 0;
}

static inline int usbh_devaddr_create(struct usbh_hubport *hport)
{
    /* Use fixed device address in BROM */
    return 1;
}

static int usbh_enumerate(struct usbh_hubport *hport, int id)
{
    struct usb_interface_descriptor *intf_desc;
    struct usb_setup_packet *setup;
    u8 ep0_buffer[USB_REQUEST_BUFFER_SIZE]
        __attribute__((aligned(CACHE_LINE_SIZE)));
    u8 descsize;
    int dev_addr;
    u8 ep_mps;
    int ret;

    setup = &hport->setup;

    /* Pick an appropriate packet size for this device
     *
     * USB 2.0, Paragraph 5.5.3 "Control Transfer Packet Size Constraints"
     *
     *  "An endpoint for control transfers specifies the maximum data
     *   payload size that the endpoint can accept from or transmit to
     *   the bus. The allowable maximum control transfer data payload
     *   sizes for full-speed devices is 8, 16, 32, or 64 bytes; for
     *   high-speed devices, it is 64 bytes and for low-speed devices,
     *   it is 8 bytes. This maximum applies to the data payloads of the
     *   Data packets following a Setup..."
     */

    if (hport->speed == USB_SPEED_HIGH) {
        /* For high-speed, we must use 64 bytes */
        ep_mps = 64;
        descsize = USB_SIZEOF_DEVICE_DESC;
    } else {
        /* Eight will work for both low- and full-speed */
        ep_mps = 8;
        descsize = 8;
    }

    /* Configure EP0 with the initial maximum packet size */
    usbh_ep0_reconfigure(hport->ep0, 0, ep_mps, hport->speed);

    /* Read the first 8 bytes of the device descriptor */
    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_STANDARD |
                           USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = USB_REQUEST_GET_DESCRIPTOR;
    setup->wValue = (u16)((USB_DESCRIPTOR_TYPE_DEVICE << 8) | 0);
    setup->wIndex = 0;
    setup->wLength = descsize;

    ret = usbh_control_transfer(hport->ep0, setup, ep0_buffer, id);
    if (ret < 0) {
        pr_err("Failed to get device descriptor,ret:%d\n", ret);
        goto errout;
    }

    parse_device_descriptor(hport, (void *)ep0_buffer, descsize);

    if (hport->speed != USB_SPEED_HIGH) {
        ret = usbh_reset_port(hport->port, id);
        if (ret) {
            pr_err("Failed to reset port.\n");
            goto errout;
        }
    }

    /* Extract the correct max packetsize from the device descriptor */
    ep_mps = ((struct usb_device_descriptor *)ep0_buffer)->bMaxPacketSize0;

    /* And reconfigure EP0 with the correct maximum packet size */
    usbh_ep0_reconfigure(hport->ep0, 0, ep_mps, hport->speed);

    /* Assign a function address to the device connected to this port */
    dev_addr = usbh_devaddr_create(hport);
    if (dev_addr < 0) {
        pr_err("Failed to allocate devaddr,ret:%d\n", ret);
        goto errout;
    }

    /* Set the USB device address */
    setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_STANDARD |
                           USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = USB_REQUEST_SET_ADDRESS;
    setup->wValue = dev_addr;
    setup->wIndex = 0;
    setup->wLength = 0;

    pr_debug("Going to set device address\n");
    ret = usbh_control_transfer(hport->ep0, setup, NULL, id);
    if (ret < 0) {
        pr_err("Failed to set devaddr,ret:%d\n", ret);
        goto errout;
    }

    /* wait device address set completely */
    aic_mdelay(2);

    /* Assign the function address to the port */
    hport->dev_addr = dev_addr;

    /* And reconfigure EP0 with the correct address */
    usbh_ep0_reconfigure(hport->ep0, dev_addr, ep_mps, hport->speed);

    /* Read the full device descriptor */
    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_STANDARD |
                           USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = USB_REQUEST_GET_DESCRIPTOR;
    setup->wValue = (u16)((USB_DESCRIPTOR_TYPE_DEVICE << 8) | 0);
    setup->wIndex = 0;
    setup->wLength = USB_SIZEOF_DEVICE_DESC;

    pr_debug("Going to get full device descriptor\n");
    ret = usbh_control_transfer(hport->ep0, setup, ep0_buffer, id);
    if (ret < 0) {
        pr_err("Failed to get full device descriptor,ret:%d\n", ret);
        goto errout;
    }

    parse_device_descriptor(hport, (void *)ep0_buffer, USB_SIZEOF_DEVICE_DESC);
    pr_debug("New device found,idVendor:%04x,idProduct:%04x,bcdDevice:%04x\n",
             ((struct usb_device_descriptor *)ep0_buffer)->idVendor,
             ((struct usb_device_descriptor *)ep0_buffer)->idProduct,
             ((struct usb_device_descriptor *)ep0_buffer)->bcdDevice);

    /* Read the first 9 bytes of the config descriptor */
    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_STANDARD |
                           USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = USB_REQUEST_GET_DESCRIPTOR;
    setup->wValue = (u16)((USB_DESCRIPTOR_TYPE_CONFIGURATION << 8) | 0);
    setup->wIndex = 0;
    setup->wLength = USB_SIZEOF_CONFIG_DESC;

    pr_debug("Going to get config descriptor\n");
    ret = usbh_control_transfer(hport->ep0, setup, ep0_buffer, id);
    if (ret < 0) {
        pr_err("Failed to get config descriptor,ret:%d\n", ret);
        goto errout;
    }

    parse_config_descriptor(hport, (void *)ep0_buffer, USB_SIZEOF_CONFIG_DESC);

    /* Read the full size of the configuration data */
    u16 wTotalLength =
        ((struct usb_configuration_descriptor *)ep0_buffer)->wTotalLength;

    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_STANDARD |
                           USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = USB_REQUEST_GET_DESCRIPTOR;
    setup->wValue = (u16)((USB_DESCRIPTOR_TYPE_CONFIGURATION << 8) | 0);
    setup->wIndex = 0;
    setup->wLength = wTotalLength;

    pr_debug("Going to get full config descriptor\n");
    ret = usbh_control_transfer(hport->ep0, setup, ep0_buffer, id);
    if (ret < 0) {
        pr_err("Failed to get full config descriptor,ret:%d\n", ret);
        goto errout;
    }

    parse_config_descriptor(hport, (void *)ep0_buffer, wTotalLength);
    pr_debug(
        "The device has %d interfaces\n",
        ((struct usb_configuration_descriptor *)ep0_buffer)->bNumInterfaces);

    /* Select device configuration 1 */
    setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_STANDARD |
                           USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = USB_REQUEST_SET_CONFIGURATION;
    setup->wValue = 1;
    setup->wIndex = 0;
    setup->wLength = 0;

    pr_debug("Going to set configuration\n");
    ret = usbh_control_transfer(hport->ep0, setup, NULL, id);
    if (ret < 0) {
        pr_err("Failed to set configuration,ret:%d\n", ret);
        goto errout;
    }

    pr_debug("Enumeration success, start loading class driver\n");

    if (hport->config.config_desc.bNumInterfaces != 1)
        return -1;
    intf_desc = &hport->config.intf[0].intf_desc;
    if ((intf_desc->bInterfaceClass != USB_CLASS_MASS_STORAGE) &&
        (intf_desc->bInterfaceSubClass != MSC_SUBCLASS_SCSI) &&
        (intf_desc->bInterfaceProtocol != MSC_PROTOCOL_BULK_ONLY)) {
        return -1;
    }

    pr_debug("Class:0x%02x,Subclass:0x%02x,Protocl:0x%02x\n",
             intf_desc->bInterfaceClass, intf_desc->bInterfaceSubClass,
             intf_desc->bInterfaceProtocol);

    ret = usbh_msc_connect(hport, 0, id);
errout:
    return ret;
}

