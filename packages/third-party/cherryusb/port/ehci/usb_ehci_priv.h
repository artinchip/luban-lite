#ifndef _USB_EHCI_PRIV_H
#define _USB_EHCI_PRIV_H

#include "usbh_core.h"
#include "usbh_hub.h"
#include "usb_hc_ehci.h"

#define EHCI_HCCR ((struct ehci_hccr *)(uintptr_t)(bus->hcd.reg_base + CONFIG_USB_EHCI_HCCR_OFFSET))
#define EHCI_HCOR ((struct ehci_hcor *)(uintptr_t)(bus->hcd.reg_base + CONFIG_USB_EHCI_HCOR_OFFSET))

#define EHCI_PTR2ADDR(x) ((uint32_t)(uintptr_t)(x) & ~0x1F)
#define EHCI_ADDR2QH(x)  ((struct ehci_qh_hw *)(uintptr_t)((uint32_t)(x) & ~0x1F))
#define EHCI_ADDR2QTD(x) ((struct ehci_qtd_hw *)(uintptr_t)((uint32_t)(x) & ~0x1F))
#define EHCI_ADDR2ITD(x) ((struct ehci_itd_hw *)(uintptr_t)((uint32_t)(x) & ~0x1F))

#if CONFIG_USB_EHCI_FRAME_LIST_SIZE == 1024
#define EHCI_PERIOIDIC_QH_NUM 11
#elif CONFIG_USB_EHCI_FRAME_LIST_SIZE == 512
#define EHCI_PERIOIDIC_QH_NUM 10
#elif CONFIG_USB_EHCI_FRAME_LIST_SIZE == 256
#define EHCI_PERIOIDIC_QH_NUM 9
#else
#error Unsupported frame size list size
#endif

#define CONFIG_USB_EHCI_QH_NUM  CONFIG_USBHOST_PIPE_NUM
#define CONFIG_USB_EHCI_QTD_NUM (CONFIG_USBHOST_PIPE_NUM + 3)
#define CONFIG_USB_EHCI_ITD_NUM 20

extern uint8_t usbh_get_port_speed(struct usbh_bus *bus, const uint8_t port);

struct ehci_qh_hw {
    USB_MEM_ALIGNX struct ehci_qh hw;
    USB_MEM_ALIGNX uint32_t first_qtd;
    struct usbh_urb *urb;
    uint8_t remove_in_iaad;
    usb_osal_sem_t waitsem;
} __attribute__((aligned(32)));

struct ehci_qtd_hw {
    USB_MEM_ALIGNX struct ehci_qtd hw;
    USB_MEM_ALIGNX struct usbh_urb *urb;
    uint32_t total_len;
#ifdef CONFIG_USB_DCACHE_ENABLE
    void * buffer;
    void * align_buffer;
    uint32_t buffer_len;
    uint32_t align_buffer_len;
    char dir_in; /* 0=out, 1=in */
#endif
} __attribute__((aligned(32)));

struct ehci_itd_hw {
    USB_MEM_ALIGNX struct ehci_itd hw;
    USB_MEM_ALIGNX struct usbh_urb *urb;
    uint16_t start_frame;
    uint8_t mf_unmask;
    uint8_t mf_valid;
    uint32_t pkt_idx[8];
    usb_slist_t list;
} __attribute__((aligned(32)));

struct ehci_hcd {
    bool ehci_qh_used[CONFIG_USB_EHCI_QH_NUM];
    bool ehci_qtd_used[CONFIG_USB_EHCI_QTD_NUM];
    bool ehci_itd_used[CONFIG_USB_EHCI_ITD_NUM];
};

extern struct ehci_hcd g_ehci_hcd[CONFIG_USBHOST_MAX_BUS];
extern uint32_t g_framelist[CONFIG_USBHOST_MAX_BUS][CONFIG_USB_EHCI_FRAME_LIST_SIZE];

int ehci_iso_urb_init(struct usbh_bus *bus, struct usbh_urb *urb);
void ehci_remove_itd_urb(struct usbh_bus *bus, struct usbh_urb *urb);
void ehci_scan_isochronous_list(struct usbh_bus *bus);

#endif
