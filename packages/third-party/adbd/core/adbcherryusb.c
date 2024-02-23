/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-11-16     ZYH          first version
 * 2018-12-13     heyuanjie87  add file operations
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtservice.h>
#include <rtdevice.h>
/* need macro RT_USING_POSIX */
#include <dfs_file.h>
#include <dfs_poll.h>

#ifdef LPKG_ADB_TR_CHERRYUSB_ENABLE
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "adb.h"

#define WINUSB_ADB
#define WCID_VENDOR_CODE 0x17

/* default for ADB */
#ifndef WINUSB_INTERF_SUBCLASS
#define WINUSB_INTERF_SUBCLASS 0x42
#endif
#ifndef WINUSB_INTERF_PROTOCOL
#define WINUSB_INTERF_PROTOCOL 0x01
#endif

__ALIGN_BEGIN const uint8_t WCID_StringDescriptor_MSOS[18] __ALIGN_END = {
    ///////////////////////////////////////
    /// MS OS string descriptor
    ///////////////////////////////////////
    0x12,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    /* MSFT100 */
    'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00, /* wcChar_7 */
    '1', 0x00, '0', 0x00, '0', 0x00,            /* wcChar_7 */
    WCID_VENDOR_CODE,                           /* bVendorCode */
    0x00,                                       /* bReserved */
};

__ALIGN_BEGIN const uint8_t WINUSB_WCIDDescriptor[40] __ALIGN_END = {
    ///////////////////////////////////////
    /// WCID descriptor
    ///////////////////////////////////////
    0x28, 0x00, 0x00, 0x00,                   /* dwLength */
    0x00, 0x01,                               /* bcdVersion */
    0x04, 0x00,                               /* wIndex */
    0x01,                                     /* bCount */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* bReserved_7 */

    ///////////////////////////////////////
    /// WCID function descriptor
    ///////////////////////////////////////
    0x00, /* bFirstInterfaceNumber */
    0x01, /* bReserved */
    /* Compatible ID */
#ifdef WINUSB_ADB
    'A', 'D', 'B', 0x00, 0x00, 0x00, 0x00, 0x00, /* cCID_8: ADB */
#else
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00, /* cCID_8: WINUSB */
#endif
    /*  */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* cSubCID_8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             /* bReserved_6 */
};

__ALIGN_BEGIN const uint8_t WINUSB_IF0_WCIDProperties [142] __ALIGN_END = {
  ///////////////////////////////////////
  /// WCID property descriptor
  ///////////////////////////////////////
  0x8e, 0x00, 0x00, 0x00,                           /* dwLength */
  0x00, 0x01,                                       /* bcdVersion */
  0x05, 0x00,                                       /* wIndex */
  0x01, 0x00,                                       /* wCount */

  ///////////////////////////////////////
  /// registry propter descriptor
  ///////////////////////////////////////
  0x84, 0x00, 0x00, 0x00,                           /* dwSize */
  0x01, 0x00, 0x00, 0x00,                           /* dwPropertyDataType */
  0x28, 0x00,                                       /* wPropertyNameLength */
  /* DeviceInterfaceGUID */
  'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00,       /* wcName_20 */
  'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00,       /* wcName_20 */
  't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00,       /* wcName_20 */
  'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00,       /* wcName_20 */
  'U', 0x00, 'I', 0x00, 'D', 0x00, 0x00, 0x00,      /* wcName_20 */
  0x4e, 0x00, 0x00, 0x00,                           /* dwPropertyDataLength */
#ifdef WINUSB_ADB
  /* {6860DC3C-C05F-4807-8807-1CA861CC1D76} */
  '{', 0x00, '6', 0x00, '8', 0x00, '6', 0x00,       /* wcData_39 */
  '0', 0x00, 'D', 0x00, 'C', 0x00, '3', 0x00,       /* wcData_39 */
  'C', 0x00, '-', 0x00, 'C', 0x00, '0', 0x00,       /* wcData_39 */
  '5', 0x00, 'F', 0x00, '-', 0x00, '4', 0x00,       /* wcData_39 */
  '8', 0x00, '0', 0x00, '7', 0x00, '-', 0x00,       /* wcData_39 */
  '8', 0x00, '8', 0x00, '0', 0x00, '7', 0x00,       /* wcData_39 */
  '-', 0x00, '1', 0x00, 'C', 0x00, 'A', 0x00,       /* wcData_39 */
  '8', 0x00, '6', 0x00, '1', 0x00, 'C', 0x00,       /* wcData_39 */
  'C', 0x00, '1', 0x00, 'D', 0x00, '7', 0x00,       /* wcData_39 */
  '6', 0x00, '}', 0x00, 0x00, 0x00,                 /* wcData_39 */
#else
  /* {1D4B2365-4749-48EA-B38A-7C6FDDDD7E26} */
  '{', 0x00, '1', 0x00, 'D', 0x00, '4', 0x00,       /* wcData_39 */
  'B', 0x00, '2', 0x00, '3', 0x00, '6', 0x00,       /* wcData_39 */
  '5', 0x00, '-', 0x00, '4', 0x00, '7', 0x00,       /* wcData_39 */
  '4', 0x00, '9', 0x00, '-', 0x00, '4', 0x00,       /* wcData_39 */
  '8', 0x00, 'E', 0x00, 'A', 0x00, '-', 0x00,       /* wcData_39 */
  'B', 0x00, '3', 0x00, '8', 0x00, 'A', 0x00,       /* wcData_39 */
  '-', 0x00, '7', 0x00, 'C', 0x00, '6', 0x00,       /* wcData_39 */
  'F', 0x00, 'D', 0x00, 'D', 0x00, 'D', 0x00,       /* wcData_39 */
  'D', 0x00, '7', 0x00, 'E', 0x00, '2', 0x00,       /* wcData_39 */
  '6', 0x00, '}', 0x00, 0x00, 0x00,                 /* wcData_39 */
#endif
};

struct usb_msosv1_descriptor msosv1_desc = {
    .string = WCID_StringDescriptor_MSOS,
    .vendor_code = WCID_VENDOR_CODE,
    .compat_id = WINUSB_WCIDDescriptor,
    .comp_id_property = (const uint8_t **)&WINUSB_IF0_WCIDProperties,
};

#define WINUSB_IN_EP  0x81
#define WINUSB_OUT_EP 0x02

#define USBD_VID           0x18D1 /* (Google Inc.) */
#define USBD_PID           0x4E26
#define USBD_BCD_DEVICE    0x0510
#define USBD_MAX_POWER     (0x3C * 2)
#define USBD_LANGID_STRING 0x0409 /* (English - United States) */

#define USB_CONFIG_SIZE (9 + 9 + 7 + 7)
#define INTF_NUM 1

#define WINUSB_EP_MPS_HS 512
#define WINUSB_EP_MPS_FS 64
#ifdef CONFIG_USB_HS
#define WINUSB_EP_MPS    WINUSB_EP_MPS_HS
#else
#define WINUSB_EP_MPS    WINUSB_EP_MPS_FS
#endif

#ifdef WINUSB_ADB
const uint8_t winusb_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, USBD_BCD_DEVICE, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, INTF_NUM, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xff, WINUSB_INTERF_SUBCLASS, WINUSB_INTERF_PROTOCOL, 0x04),
    USB_ENDPOINT_DESCRIPTOR_INIT(WINUSB_IN_EP, 0x02, WINUSB_EP_MPS_HS, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(WINUSB_OUT_EP, 0x02, WINUSB_EP_MPS_HS, 0x00),
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'A', 0x00,                  /* wcChar0 */
    'r', 0x00,                  /* wcChar1 */
    't', 0x00,                  /* wcChar2 */
    'I', 0x00,                  /* wcChar3 */
    'n', 0x00,                  /* wcChar4 */
    'C', 0x00,                  /* wcChar5 */
    'h', 0x00,                  /* wcChar6 */
    'i', 0x00,                  /* wcChar7 */
    'p', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x1C,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'A', 0x00,                  /* wcChar0 */
    'D', 0x00,                  /* wcChar1 */
    'B', 0x00,                  /* wcChar2 */
    ' ', 0x00,                  /* wcChar3 */
    'I', 0x00,                  /* wcChar4 */
    'n', 0x00,                  /* wcChar5 */
    't', 0x00,                  /* wcChar6 */
    'e', 0x00,                  /* wcChar7 */
    'r', 0x00,                  /* wcChar8 */
    'f', 0x00,                  /* wcChar9 */
    'a', 0x00,                  /* wcChar10 */
    'c', 0x00,                  /* wcChar11 */
    'e', 0x00,                  /* wcChar12 */
    ///////////////////////////////////////
    /// string3 descriptor "32021919830108"
    ///////////////////////////////////////
    0x1E,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '3', 0x00,                  /* wcChar0 */
    '2', 0x00,                  /* wcChar1 */
    '0', 0x00,                  /* wcChar2 */
    '2', 0x00,                  /* wcChar3 */
    '1', 0x00,                  /* wcChar4 */
    '9', 0x00,                  /* wcChar5 */
    '1', 0x00,                  /* wcChar6 */
    '9', 0x00,                  /* wcChar7 */
    '8', 0x00,                  /* wcChar8 */
    '3', 0x00,                  /* wcChar9 */
    '0', 0x00,                  /* wcChar10 */
    '1', 0x00,                  /* wcChar11 */
    '0', 0x00,                  /* wcChar12 */
    '8', 0x00,                  /* wcChar13 */
    ///////////////////////////////////////
    /// string4 descriptor
    ///////////////////////////////////////
    0x1C,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'A', 0x00,                  /* wcChar0 */
    'D', 0x00,                  /* wcChar1 */
    'B', 0x00,                  /* wcChar2 */
    ' ', 0x00,                  /* wcChar3 */
    'I', 0x00,                  /* wcChar4 */
    'n', 0x00,                  /* wcChar5 */
    't', 0x00,                  /* wcChar6 */
    'e', 0x00,                  /* wcChar7 */
    'r', 0x00,                  /* wcChar8 */
    'f', 0x00,                  /* wcChar9 */
    'a', 0x00,                  /* wcChar10 */
    'c', 0x00,                  /* wcChar11 */
    'e', 0x00,                  /* wcChar12 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,                       /* bLength */
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,   /* bDescriptorType */
    0x00, 0x02,                 /* bcdUSB */
    0x00,                       /* bDeviceClass */
    0x00,                       /* bDeviceSubClass */
    0x00,                       /* bDeviceProtocol */
    0x40,                       /* bMaxPacketSize0 */
    0x01,                       /* bNumConfigurations */
    0x00,                       /* breserved */

    ////////////////////////////////////////////////////////////
    /// Other Speed Configuration Descriptor (Full Speed)
    ////////////////////////////////////////////////////////////
    USB_OTHERSPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, INTF_NUM, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xff, WINUSB_INTERF_SUBCLASS, WINUSB_INTERF_PROTOCOL, 0x04),
    USB_ENDPOINT_DESCRIPTOR_INIT(WINUSB_IN_EP, 0x02, WINUSB_EP_MPS_FS, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(WINUSB_OUT_EP, 0x02, WINUSB_EP_MPS_FS, 0x00),
#endif
    0x00
};
#else
const uint8_t winusb_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0001, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, INTF_NUM, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xff, 0xff, 0x00, 0x04),
    USB_ENDPOINT_DESCRIPTOR_INIT(WINUSB_IN_EP, 0x02, WINUSB_EP_MPS, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(WINUSB_OUT_EP, 0x02, WINUSB_EP_MPS, 0x00),
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x2C,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'W', 0x00,                  /* wcChar10 */
    'I', 0x00,                  /* wcChar11 */
    'N', 0x00,                  /* wcChar12 */
    'U', 0x00,                  /* wcChar13 */
    'S', 0x00,                  /* wcChar14 */
    'B', 0x00,                  /* wcChar15 */
    ' ', 0x00,                  /* wcChar16 */
    'D', 0x00,                  /* wcChar17 */
    'E', 0x00,                  /* wcChar18 */
    'M', 0x00,                  /* wcChar19 */
    'O', 0x00,                  /* wcChar20 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '1', 0x00,                  /* wcChar3 */
    '0', 0x00,                  /* wcChar4 */
    '3', 0x00,                  /* wcChar5 */
    '1', 0x00,                  /* wcChar6 */
    '0', 0x00,                  /* wcChar7 */
    '0', 0x00,                  /* wcChar8 */
    '0', 0x00,                  /* wcChar9 */
    ///////////////////////////////////////
    /// string4 descriptor
    ///////////////////////////////////////
    0x30,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'W', 0x00,                  /* wcChar10 */
    'I', 0x00,                  /* wcChar11 */
    'N', 0x00,                  /* wcChar12 */
    'U', 0x00,                  /* wcChar13 */
    'S', 0x00,                  /* wcChar14 */
    'B', 0x00,                  /* wcChar15 */
    ' ', 0x00,                  /* wcChar16 */
    'D', 0x00,                  /* wcChar17 */
    'E', 0x00,                  /* wcChar18 */
    'M', 0x00,                  /* wcChar19 */
    'O', 0x00,                  /* wcChar20 */
        ' ', 0x00,                  /* wcChar16 */
    '1', 0x00,                  /* wcChar21 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x02,
    0x02,
    0x01,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};
#endif

void usbd_winusb_out(uint8_t ep, uint32_t size);
void usbd_winusb_in(uint8_t ep, uint32_t size);

struct winusb_device
{
    struct rt_device parent;
    void (*cmd_handler)(rt_uint8_t *buffer, rt_size_t size);
    rt_uint8_t cmd_buff[256];
    struct usbd_endpoint * ep_out;
    struct usbd_endpoint * ep_in;

    struct rt_wqueue wq;
    struct rt_wqueue rq;
    rt_uint16_t rdcnt;
    rt_uint16_t wrcnt;
    struct rt_ringbuffer *rrb;
    rt_bool_t enabled;
};

typedef struct winusb_device * winusb_device_t;

struct winusb_device adb_winusb_device;

struct usbd_interface intf0;

struct usbd_endpoint winusb_out_ep1 = {
    .ep_addr = WINUSB_OUT_EP,
    .ep_cb = usbd_winusb_out
};

struct usbd_endpoint winusb_in_ep1 = {
    .ep_addr = WINUSB_IN_EP,
    .ep_cb = usbd_winusb_in
};

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[WINUSB_EP_MPS];

volatile bool ep_tx_busy_flag = false;

static rt_err_t usbd_interface_enable(winusb_device_t wd)
{
    rt_ringbuffer_init(wd->rrb, wd->rrb->buffer_ptr, 1024);

    /* setup first out ep read transfer */
    usbd_ep_start_read(WINUSB_OUT_EP, read_buffer, WINUSB_EP_MPS);

    wd->enabled = RT_TRUE;

    return RT_EOK;
}

static rt_err_t usbd_interface_disable(winusb_device_t wd)
{
    if (!wd->enabled)
        return RT_EOK;

    wd->enabled = RT_FALSE;

    rt_wqueue_wakeup(&(wd->rq), (void *)(POLLHUP));
    rt_wqueue_wakeup(&(wd->wq), (void *)(POLLHUP));

    return RT_EOK;
}

void usbd_event_handler(uint8_t event)
{
    winusb_device_t wd = &adb_winusb_device;

    switch (event) {
        case USBD_EVENT_RESET:
            usbd_interface_disable(wd);
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_CONFIGURED:
            usbd_interface_enable(wd);
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

void usbd_winusb_out(uint8_t ep, uint32_t size)
{
    winusb_device_t wd = &adb_winusb_device;
    int space;

    if (size == 0)
    {
        usbd_ep_start_read(WINUSB_OUT_EP, read_buffer, WINUSB_EP_MPS);
    }

    if (wd->rdcnt != 0)
    {
        rt_kprintf("err!! F:%s L:%d rdcnt != 0 rdcnt:%d\n", __FUNCTION__, __LINE__, wd->rdcnt);
    }

    space = rt_ringbuffer_space_len(wd->rrb);
    if (size <= space)
    {
        rt_ringbuffer_put(wd->rrb, read_buffer, size);
        usbd_ep_start_read(WINUSB_OUT_EP, read_buffer, WINUSB_EP_MPS);
    }
    else
    {
        wd->rdcnt = size; /* let data pending in read_buffer */
    }

    rt_wqueue_wakeup(&(wd->rq), (void *)POLLIN);
}

void usbd_winusb_in(uint8_t ep, uint32_t size)
{
    winusb_device_t wd = &adb_winusb_device;
    wd->wrcnt -= size;
    rt_wqueue_wakeup(&(wd->wq), (void *)POLLOUT);
}

/* file operations */
static int _file_open(struct dfs_fd *fd)
{
    return 0;
}

static int _file_close(struct dfs_fd *fd)
{
    return 0;
}

static int _file_ioctl(struct dfs_fd *fd, int cmd, void *args)
{
    return 0;
}

static int _file_read(struct dfs_fd *fd, void *buf, size_t size)
{
    struct winusb_device *wd;
    size_t rsize, tsize;

    wd = (struct winusb_device *)fd->data;

    if (!wd->enabled)
        return -ENODEV;

    while (!rt_ringbuffer_data_len(wd->rrb) && !wd->rdcnt)
    {
        if (fd->flags & O_NONBLOCK)
            return -EAGAIN;

        rt_wqueue_wait(&wd->rq, 0, RT_WAITING_FOREVER);
        if (!wd->enabled)
            return -ENODEV;
    }

    rsize = rt_ringbuffer_data_len(wd->rrb);
    if (rsize)
    {
        if (rsize > size)
            rsize = size;
        rsize = rt_ringbuffer_get(wd->rrb, buf, rsize);
    }

    if (wd->rdcnt)
    {
        tsize = rt_ringbuffer_space_len(wd->rrb);
        if (tsize >= wd->rdcnt)
        {
            rt_ringbuffer_put(wd->rrb, read_buffer, wd->rdcnt);
            wd->rdcnt = 0;
            usbd_ep_start_read(WINUSB_OUT_EP, read_buffer, WINUSB_EP_MPS);
        }
    }

    return rsize;
}

static int _file_write(struct dfs_fd *fd, const void *buf, size_t size)
{
    struct winusb_device *wd;
    int wlen;

    wd = (struct winusb_device *)fd->data;

    if (!wd->enabled)
        return -ENODEV;

    while (wd->wrcnt)
    {
        if (fd->flags & O_NONBLOCK)
            return -EAGAIN;

        rt_wqueue_wait(&wd->wq, 0, RT_WAITING_FOREVER);
        if (!wd->enabled)
            return -ENODEV;
    }

    wlen = size > WINUSB_EP_MPS ? WINUSB_EP_MPS : size;
    wd->wrcnt = wlen;

    usbd_ep_start_write(WINUSB_IN_EP, buf, size);

    return wlen;
}

static int _file_poll(struct dfs_fd *fd, struct rt_pollreq *req)
{
    int mask = 0;
    struct winusb_device *wd;

    wd = (struct winusb_device *)fd->data;

    if (!wd->enabled)
        return POLLHUP;
    if (wd->rdcnt || rt_ringbuffer_data_len(wd->rrb))
        mask |= POLLIN;
    else
        rt_poll_add(&wd->rq, req);

    if (!wd->wrcnt)
        mask |= POLLOUT;
    else
        rt_poll_add(&wd->wq, req);

    return mask;
}

static const struct dfs_file_ops _fops =
{
    _file_open,
    _file_close,
    _file_ioctl,
    _file_read,
    _file_write,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    _file_poll
};

static rt_err_t rt_usb_winusb_init(winusb_device_t winusb_device)
{
    rt_err_t ret;

    winusb_device->parent.type = RT_Device_Class_Miscellaneous;

    ret = rt_device_register(&winusb_device->parent, "winusb", RT_DEVICE_FLAG_RDWR);

    winusb_device->parent.fops = &_fops;
    rt_wqueue_init(&winusb_device->rq);
    rt_wqueue_init(&winusb_device->wq);
    winusb_device->rrb = rt_ringbuffer_create(1024);
    winusb_device->enabled = RT_FALSE;

    return ret;
}

int adb_winusb_init(void)
{
    int ret = 0;

    usbd_desc_register(winusb_descriptor);
    usbd_msosv1_desc_register(&msosv1_desc);
    usbd_add_interface(&intf0);
    usbd_add_endpoint(&winusb_out_ep1);
    usbd_add_endpoint(&winusb_in_ep1);
    rt_usb_winusb_init(&adb_winusb_device);
    usbd_initialize();
    return ret;
}

INIT_PREV_EXPORT(adb_winusb_init);

#endif
