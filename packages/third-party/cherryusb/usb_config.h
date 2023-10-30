/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

#include <aic_core.h>

#define CHERRYUSB_VERSION 0x001000

/* ================ USB common Configuration ================ */

#define CONFIG_USB_PRINTF(...) printf(__VA_ARGS__)

#define usb_malloc(size) malloc(size)
#define usb_free(ptr)    free(ptr)

#ifndef CONFIG_USB_DBG_LEVEL
#define CONFIG_USB_DBG_LEVEL USB_DBG_INFO
#endif

/* Enable print with color */
#define CONFIG_USB_PRINTF_COLOR_ENABLE

/* cache enable */
#define CONFIG_USB_DCACHE_ENABLE

/* attribute data into no cache ram */
#ifdef CONFIG_USB_DCACHE_ENABLE
#define USB_NOCACHE_RAM_SECTION
#define CONFIG_USB_ALIGN_SIZE CACHE_LINE_SIZE
#else
#define USB_NOCACHE_RAM_SECTION __attribute__((section(".noncacheable")))
#define CONFIG_USB_ALIGN_SIZE 4
#endif

/* ================= USB Device Stack Configuration ================ */

/* Ep0 max transfer buffer, specially for receiving data from ep0 out */
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 256

/* Setup packet log for debug */
// #define CONFIG_USBDEV_SETUP_LOG_PRINT

/* Check if the input descriptor is correct */
// #define CONFIG_USBDEV_DESC_CHECK

/* Enable test mode */
// #define CONFIG_USBDEV_TEST_MODE

//#define CONFIG_USBDEV_TX_THREAD
//#define CONFIG_USBDEV_RX_THREAD

#ifdef CONFIG_USBDEV_TX_THREAD
#ifndef CONFIG_USBDEV_TX_PRIO
#define CONFIG_USBDEV_TX_PRIO 4
#endif
#ifndef CONFIG_USBDEV_TX_STACKSIZE
#define CONFIG_USBDEV_TX_STACKSIZE 2048
#endif
#endif

#ifdef CONFIG_USBDEV_RX_THREAD
#ifndef CONFIG_USBDEV_RX_PRIO
#define CONFIG_USBDEV_RX_PRIO 4
#endif
#ifndef CONFIG_USBDEV_RX_STACKSIZE
#define CONFIG_USBDEV_RX_STACKSIZE 2048
#endif
#endif

#ifndef CONFIG_USBDEV_MSC_BLOCK_SIZE
#define CONFIG_USBDEV_MSC_BLOCK_SIZE 512
#endif

#ifndef CONFIG_USBDEV_MSC_MANUFACTURER_STRING
#define CONFIG_USBDEV_MSC_MANUFACTURER_STRING ""
#endif

#ifndef CONFIG_USBDEV_MSC_PRODUCT_STRING
#define CONFIG_USBDEV_MSC_PRODUCT_STRING ""
#endif

#ifndef CONFIG_USBDEV_MSC_VERSION_STRING
#define CONFIG_USBDEV_MSC_VERSION_STRING "0.01"
#endif

#ifndef CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE
#define CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE 156
#endif

#ifndef CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE
#define CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE 1536
#endif

#ifndef CONFIG_USBDEV_RNDIS_VENDOR_ID
#define CONFIG_USBDEV_RNDIS_VENDOR_ID 0x0000ffff
#endif

#ifndef CONFIG_USBDEV_RNDIS_VENDOR_DESC
#define CONFIG_USBDEV_RNDIS_VENDOR_DESC "CherryUSB"
#endif

#define CONFIG_USBDEV_RNDIS_USING_LWIP

/* ================ USB HOST Stack Configuration ================== */

#define CONFIG_USBHOST_MAX_RHPORTS          1
#define CONFIG_USBHOST_MAX_EXTHUBS          1
#define CONFIG_USBHOST_MAX_EHPORTS          4
#define CONFIG_USBHOST_MAX_INTERFACES       6
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 1
#define CONFIG_USBHOST_MAX_ENDPOINTS        4

#define CONFIG_USBHOST_MAX_CDC_ACM_CLASS 4
#define CONFIG_USBHOST_MAX_HID_CLASS     4
#define CONFIG_USBHOST_MAX_MSC_CLASS     2
#define CONFIG_USBHOST_MAX_AUDIO_CLASS   1
#define CONFIG_USBHOST_MAX_VIDEO_CLASS   1
#define CONFIG_USBHOST_MAX_RNDIS_CLASS   1

#define CONFIG_USBHOST_DEV_NAMELEN 16

#ifndef CONFIG_USBHOST_PSC_PRIO
#define CONFIG_USBHOST_PSC_PRIO 4
#endif
#ifndef CONFIG_USBHOST_PSC_STACKSIZE
#define CONFIG_USBHOST_PSC_STACKSIZE 2048
#endif

//#define CONFIG_USBHOST_GET_STRING_DESC

/* Ep0 max transfer buffer */
#define CONFIG_USBHOST_REQUEST_BUFFER_LEN 512

#ifndef CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT
#define CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT 500
#endif

#ifndef CONFIG_USBHOST_MSC_TIMEOUT
#define CONFIG_USBHOST_MSC_TIMEOUT 5000
#endif

/* ================ USB Device Port Configuration ================*/

#ifdef AIC_USING_USB0_DEVICE
/* AIC Device Controller Configuration */
#define CONFIG_USB_AIC_DC_PORT      1  /* 0 = FullSpeed, 1 = HighSpeed */
#define CONFIG_USB_AIC_DC_BASE      (USB_DEV_BASE)
#define CONFIG_USB_AIC_DC_CLK       (CLK_USBD)
#define CONFIG_USB_AIC_DC_RESET     (RESET_USBD)
#define CONFIG_USB_AIC_DC_PHY_CLK   (CLK_USB_PHY0)
#define CONFIG_USB_AIC_DC_PHY_RESET (RESET_USBPHY0)
#define CONFIG_USB_AIC_DC_IRQ_NUM   (USB_DEV_IRQn)
#define USB_NUM_BIDIR_ENDPOINTS     5
#ifndef LPKG_CHERRYUSB_DEVICE_AIC_CPU
#define CONFIG_USB_AIC_DMA_ENABLE
#endif
#endif
//#define USBD_IRQHandler USBD_IRQHandler
//#define USB_BASE (0x40080000UL)
//#define USB_NUM_BIDIR_ENDPOINTS 4

/* ================ USB Host Port Configuration ==================*/

#define CONFIG_USBHOST_PIPE_NUM 10

/* ================ EHCI Configuration ================ */

#if defined(AIC_USING_USB0_HOST)
#define CONFIG_USB_EHCI_HCCR_BASE       (USB_HOST0_BASE)
#define CONFIG_USB_EHCI_HCOR_BASE       (USB_HOST0_BASE + 0x10)
#define CONFIG_USB_EHCI_CLK             (CLK_USBH0)
#define CONFIG_USB_EHCI_RESET           (RESET_USBH0)
#define CONFIG_USB_EHCI_PHY_CLK         (CLK_USB_PHY0)
#define CONFIG_USB_EHCI_PHY_RESET       (RESET_USBPHY0)
#define CONFIG_USB_EHCI_IRQ_NUM         (USB_HOST0_EHCI_IRQn)
#elif defined(AIC_USING_USB1_HOST)
#define CONFIG_USB_EHCI_HCCR_BASE       (USB_HOST1_BASE)
#define CONFIG_USB_EHCI_HCOR_BASE       (USB_HOST1_BASE + 0x10)
#define CONFIG_USB_EHCI_CLK             (CLK_USBH1)
#define CONFIG_USB_EHCI_RESET           (RESET_USBH1)
#define CONFIG_USB_EHCI_PHY_CLK         (CLK_USB_PHY1)
#define CONFIG_USB_EHCI_PHY_RESET       (RESET_USBPHY1)
#define CONFIG_USB_EHCI_IRQ_NUM         (USB_HOST1_EHCI_IRQn)
#else
#define CONFIG_USB_EHCI_HCCR_BASE       (0x20072000)
#define CONFIG_USB_EHCI_HCOR_BASE       (0x20072000 + 0x10)
#endif
#define CONFIG_USB_EHCI_FRAME_LIST_SIZE 1024
// #define CONFIG_USB_EHCI_INFO_ENABLE
// #define CONFIG_USB_ECHI_HCOR_RESERVED_DISABLE
#define CONFIG_USB_EHCI_CONFIGFLAG
#define CONFIG_USB_EHCI_PORT_POWER

#endif
