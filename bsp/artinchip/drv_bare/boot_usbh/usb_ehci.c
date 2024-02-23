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
#include <aic_hal.h>

#include <usbh_reg.h>
#include "usb_ehci.h"
#include "usbh_core.h"
#include <hal_syscfg.h>
#include <usb_defs.h>

//#define USBH_DEBUG 0
#define USB_EHCI_HCCR_BASE(id) (USB_HOST_REG_BASE(id))
/* Host Controller Capability Registers */

/* This is the set of interrupts handled by this driver */
#define EHCI_HANDLED_INTS (EHCI_INT_USBINT | EHCI_INT_USBERRINT | \
                           EHCI_INT_PORTSC | EHCI_INT_SYSERROR |  \
                           EHCI_INT_AAINT)

#define false 0
#define true  1

struct usb_ehci_epinfo_s;
struct usb_ehci_qh_s {
    /* Fields visible to hardware */
    struct ehci_qh_s hw __attribute__((aligned(CACHE_LINE_SIZE))); /* Hardware representation of the queue head, 48 bytes */

    struct usb_ehci_epinfo_s *epinfo; /* Endpoint used for the transfer */
    /* Internal fields used by the EHCI driver */
    u32 fqp; /* First qTD in the list (physical address) */
#if (__LONG_WIDTH__ == 32)
    u8 pad[8]; /* Padding to assure 32-byte alignment */
#endif
};

/* This structure describes one endpoint. */
struct usb_ehci_epinfo_s {
    u8 epno    : 7; /* Endpoint number */
    u8 dirin   : 1; /* 1:IN endpoint 0:OUT endpoint */
    u8 devaddr : 7; /* Device address */
    u8 toggle  : 1; /* Next data toggle */
    bool inuse;
    u8 status;          /* Retained token status bits (for debug purposes) */
    u16 maxpacket : 11; /* Maximum packet size */
    u16 xfrtype   : 2;  /* See USB_EP_ATTR_XFER_* definitions in usb.h */
    u16 speed     : 2;  /* See USB_*_SPEED definitions in ehci.h */
    int result;         /* The result of the transfer */
    u32 xfrd; /* On completion, will hold the number of bytes transferred */
    struct usbh_hubport *hport;
    struct usb_ehci_qh_s *qh;
};

/* Internal representation of the EHCI Queue Element Transfer Descriptor
 * (qTD)
 */

struct usb_ehci_qtd_s {
    /* Fields visible to hardware */
    struct ehci_qtd_s hw __attribute__((aligned(CACHE_LINE_SIZE))); /* Hardware representation of the queue head */
                            /* Internal fields used by the EHCI driver */
};

/* The following is used to manage lists of free QHs and qTDs */

struct usb_ehci_list_s {
    struct usb_ehci_list_s *flink; /* Link to next entry in the list */
                                   /* Variable length entry data follows */
};

/* List traversal callout functions */

typedef int (*foreach_qh_t)(struct usb_ehci_qh_s *qh, u32 **bp, void *arg);
typedef int (*foreach_qtd_t)(struct usb_ehci_qtd_s *qtd, u32 **bp, void *arg);

/* This structure retains the overall state of the USB host controller */

#define CONFIG_USBHOST_PIPE_NUM (3) // EP0, in, out
#define CONFIG_USB_EHCI_QH_NUM  (1)
#define CONFIG_USB_EHCI_QTD_NUM (3)
struct ehci_hcd {
    /* Queue Head (QH) pool */
    __attribute__((aligned(CACHE_LINE_SIZE))) struct usb_ehci_qh_s qhpool[CONFIG_USB_EHCI_QH_NUM];
    /* Queue Element Transfer Descriptor (qTD) pool */
    __attribute__((aligned(CACHE_LINE_SIZE))) struct usb_ehci_qtd_s qtdpool[CONFIG_USB_EHCI_QTD_NUM];
    /* List of free Queue Head (QH) structures */
    struct usb_ehci_list_s *qhfree;
    /* List of free Queue Element Transfer Descriptor (qTD) */
    struct usb_ehci_list_s *qtdfree;
    struct usb_ehci_epinfo_s chan[CONFIG_USBHOST_PIPE_NUM];
};

struct ehci_hcd g_ehci_hcd __attribute__((aligned(CACHE_LINE_SIZE)));
struct usb_ehci_qh_s g_asynchead __attribute__((aligned(CACHE_LINE_SIZE)));

static void usb_ehci_qh_free(struct usb_ehci_qh_s *qh);
static void usb_ehci_qtd_free(struct usb_ehci_qtd_s *qtd);
static int usb_ehci_reset(int id);
static int usb_ehci_wait_usbsts(int id, u32 maskbits, u32 donebits, u32 delay);

#ifdef USBH_DEBUG
void dump_hcor(volatile struct ehci_hcor_s *hcor)
{
    pr_err("usbcmd:0x%x\n", readl(&hcor->usbcmd));
    pr_err("usbsts:0x%x\n", readl(&hcor->usbsts));
    pr_err("usbintr:0x%x\n", readl(&hcor->usbintr));
    pr_err("frindex:0x%x\n", readl(&hcor->frindex));
    pr_err("ctrldssegment:0x%x\n", readl(&hcor->ctrldssegment));
    pr_err("periodiclistbase:0x%x\n", readl(&hcor->periodiclistbase));
    pr_err("asynclistaddr:0x%x\n", readl(&hcor->asynclistaddr));
    pr_err("configflag:0x%x\n", readl(&hcor->configflag));
    pr_err("portsc[0]:0x%x\n\n", readl(&hcor->portsc[0]));
}

void dump_qtd(struct usb_ehci_qtd_s *qtd)
{
    u32 addr;

    pr_err("qTD 0x%x:\n", (u32)(uintptr_t)qtd);
    pr_err("   nqp: (0x%x)\n", qtd->hw.nqp);
    pr_err("   alt: (0x%x)\n", qtd->hw.alt);
    pr_err("   token: (0x%x)\n", qtd->hw.token);
    pr_err("   bpl0: (0x%x)\n", qtd->hw.bpl[0]);
    pr_err("   bpl1: (0x%x)\n", qtd->hw.bpl[1]);
    pr_err("   bpl2: (0x%x)\n", qtd->hw.bpl[2]);
    pr_err("   bpl3: (0x%x)\n", qtd->hw.bpl[3]);
    pr_err("   bpl4: (0x%x)\n", qtd->hw.bpl[4]);

    addr = qtd->hw.nqp & 0xFFFFFFFC;
    if (addr)
        dump_qtd((void *)(uintptr_t)addr);
}

void dump_qh(struct usb_ehci_qh_s *qh)
{
    u32 qtd;

    pr_err("Queue Head 0x%x, sizeof qh %d %d %d:\n", (u32)(uintptr_t)qh,
           (int)sizeof(*qh), (int)sizeof(struct usb_ehci_qh_s),
           (int)sizeof(struct ehci_qh_s));
    pr_err("HW part:\n");
    pr_err(" qhlp: (0x%x, TYP=%x,T=%d)\n", qh->hw.hlp & 0xFFFFFFE0,
           (qh->hw.hlp & 0x6) >> 1, qh->hw.hlp & 1);
    pr_err(" epchar: (0x%x)\n", qh->hw.epchar);
    pr_err(" epcaps: (0x%x)\n", qh->hw.epcaps);
    pr_err(" cqp: (0x%x)\n", qh->hw.cqp);
    pr_err(" overlay: \n");
    pr_err("   nqp: (0x%x)\n", qh->hw.overlay.nqp);
    pr_err("   alt: (0x%x)\n", qh->hw.overlay.alt);
    pr_err("   token: (0x%x)\n", qh->hw.overlay.token);
    pr_err("   bpl0: (0x%x)\n", qh->hw.overlay.bpl[0]);
    pr_err("   bpl1: (0x%x)\n", qh->hw.overlay.bpl[1]);
    pr_err("   bpl2: (0x%x)\n", qh->hw.overlay.bpl[2]);
    pr_err("   bpl3: (0x%x)\n", qh->hw.overlay.bpl[3]);
    pr_err("   bpl4: (0x%x)\n", qh->hw.overlay.bpl[4]);
    pr_err("fqp: (0x%x)\n", qh->fqp);
    pr_err("epinfo: (0x%x)\n", (u32)(uintptr_t)qh->epinfo);

    qtd = qh->hw.overlay.nqp & 0xFFFFFFFC;
    if (qtd)
        dump_qtd((void *)(uintptr_t)qtd);
}
#else
void dump_hcor(volatile struct ehci_hcor_s *hcor)
{
}

void dump_qtd(struct usb_ehci_qtd_s *qtd)
{
}

void dump_qh(struct usb_ehci_qh_s *qh)
{
}
#endif

int usb_hc_sw_init(void)
{
    memset(&g_ehci_hcd, 0, sizeof(struct ehci_hcd));

    /* Initialize the list of free Queue Head (QH) structures */
    for (u8 i = 0; i < CONFIG_USB_EHCI_QH_NUM; i++) {
        /* Put the QH structure in a free list */
        usb_ehci_qh_free(&g_ehci_hcd.qhpool[i]);
    }

    /* Initialize the list of free Queue Head (QH) structures */
    for (u8 i = 0; i < CONFIG_USB_EHCI_QTD_NUM; i++) {
        /* Put the QH structure in a free list */
        usb_ehci_qtd_free(&g_ehci_hcd.qtdpool[i]);
    }

    return 0;
}

void usb_hc_low_level_init(int id)
{
    u32 val, clk_usbh, clk_usb_phy; //, reset_usbphy, reset_usbh;

    if (id == 0)
        syscfg_usb_phy0_sw_host(1); /* Switch to HOST mode */

    /* Enable clock */
    clk_usbh = CLK_USBH0 + id;
    hal_clk_enable_deassertrst_iter(clk_usbh);
    clk_usb_phy = CLK_USB_PHY0 + id;
    hal_clk_enable_deassertrst_iter(clk_usb_phy);

    /* set phy type: UTMI/ULPI */
    val = readl(USBH_REG_HOST_CTL(id));
    writel((val | 0x1), USBH_REG_HOST_CTL(id));
}

int usb_hc_hw_init(int id)
{
    volatile struct ehci_hcor_s *hcor;
    int ret;
    u32 regval;
    u32 physaddr1;

    hcor = (struct ehci_hcor_s *)USB_EHCI_HCOR_BASE(id);

    usb_hc_low_level_init(id);

    /* Initialize the head of the asynchronous queue/reclamation list.
     *
     * "In order to communicate with devices via the asynchronous schedule,
     *  system software must write the ASYNDLISTADDR register with the address
     *  of a control or bulk queue head. Software must then enable the
     *  asynchronous schedule by writing a one to the Asynchronous Schedule
     *  Enable bit in the USBCMD register. In order to communicate with devices
     *  via the periodic schedule, system software must enable the periodic
     *  schedule by writing a one to the Periodic Schedule Enable bit in the
     *  USBCMD register. Note that the schedules can be turned on before the
     *  first port is reset (and enabled)."
     */

    memset(&g_asynchead, 0, sizeof(struct usb_ehci_qh_s));
    physaddr1 = (u32)(uintptr_t)&g_asynchead;
    g_asynchead.hw.hlp = physaddr1 | QH_HLP_TYP_QH;
    g_asynchead.hw.epchar = (QH_EPCHAR_H | QH_EPCHAR_EPS_FULL);
    g_asynchead.hw.overlay.nqp = (QH_NQP_T);
    g_asynchead.hw.overlay.alt = (QH_NQP_T);
    g_asynchead.hw.overlay.token = (QH_TOKEN_HALTED);
    g_asynchead.fqp = (QTD_NQP_T);
    aicos_dcache_clean_range((void *)(uintptr_t)&g_asynchead.hw,
                             ROUNDUP(sizeof(struct usb_ehci_qh_s), CACHE_LINE_SIZE));

    /* Host Controller Initialization. Paragraph 4.1 */

    /* Reset the EHCI hardware */
    ret = usb_ehci_reset(id);
    if (ret < 0) {
        pr_err("ehci reset failed.\n");
        return -1;
    }

    /* Disable all interrupts */
    writel(0, &hcor->usbintr);

    /* Clear pending interrupts.  Bits in the USBSTS register are cleared by
     * writing a '1' to the corresponding bit.
     */
    writel(EHCI_INT_ALLINTS, &hcor->usbsts);

    /* Set the Current Asynchronous List Address. */
    writel(physaddr1, &hcor->asynclistaddr);

    /* Enable the asynchronous schedule and, possibly enable the periodic
     * schedule and set the frame list size.
     */
    regval = readl(&hcor->usbcmd);
    regval &= ~(EHCI_USBCMD_HCRESET | EHCI_USBCMD_FLSIZE_MASK |
                EHCI_USBCMD_PSEN | EHCI_USBCMD_ASEN | EHCI_USBCMD_IAADB);
    regval |= EHCI_USBCMD_ASEN;
    writel(regval, &hcor->usbcmd);

    /* Start the host controller by setting the RUN bit in the USBCMD register. */
    regval = readl(&hcor->usbcmd);
    regval |= EHCI_USBCMD_RUN;

    writel(regval, &hcor->usbcmd);

    /* Route all ports to this host controller by setting the CONFIG flag. */
    regval = readl(&hcor->configflag);
    regval |= EHCI_CONFIGFLAG;
    writel(regval, &hcor->configflag);

    /* Wait for the EHCI to run (i.e., no longer report halted) */
    ret = usb_ehci_wait_usbsts(id, EHCI_USBSTS_HALTED, 0, 100 * 1000);
    if (ret < 0) {
        pr_err("Wait ehci run timeout.\n");
        return -2;
    }

    /* Enable port power */
    regval = readl(&hcor->portsc[0]);
    regval |= EHCI_PORTSC_PP;
    writel(regval, &hcor->portsc[0]);

    /* Enable EHCI interrupts.  Interrupts are still disabled at the level of
     * the interrupt controller.
     */
    writel(EHCI_HANDLED_INTS, &hcor->usbintr);
    return ret;
}

int usb_hc_hw_fast_init(int id)
{
    volatile struct ehci_hcor_s *hcor;
    int ret;
    u32 regval;

    hcor = (struct ehci_hcor_s *)USB_EHCI_HCOR_BASE(id);

    usb_hc_low_level_init(id);

    /* Reset the EHCI hardware */
    ret = usb_ehci_reset(id);
    if (ret < 0) {
        pr_err("ehci reset failed.\n");
        return -1;
    }

    /* Disable all interrupts */
    writel(0, &hcor->usbintr);

    /* Clear pending interrupts.  Bits in the USBSTS register are cleared by
     * writing a '1' to the corresponding bit.
     */
    writel(EHCI_INT_ALLINTS, &hcor->usbsts);

    /* Start the host controller by setting the RUN bit in the USBCMD register. */
    regval = readl(&hcor->usbcmd);
    regval |= EHCI_USBCMD_RUN;
    writel(regval, &hcor->usbcmd);

    /* Route all ports to this host controller by setting the CONFIG flag. */
    regval = readl(&hcor->configflag);
    regval |= EHCI_CONFIGFLAG;
    writel(regval, &hcor->configflag);

    /* Wait for the EHCI to run (i.e., no longer report halted) */
    ret = usb_ehci_wait_usbsts(id, EHCI_USBSTS_HALTED, 0, 100 * 1000);
    if (ret < 0) {
        pr_err("Wait ehci run timeout.\n");
        return -2;
    }

    /* Enable port power */
    regval = readl(&hcor->portsc[0]);
    regval |= EHCI_PORTSC_PP;
    writel(regval, &hcor->portsc[0]);

    /* Enable EHCI interrupts.  Interrupts are still disabled at the level of
     * the interrupt controller.
     */
    writel(EHCI_HANDLED_INTS, &hcor->usbintr);
    return ret;
}

static int usb_ehci_chan_alloc(void)
{
    int chidx;

    /* Search the table of channels */
    for (chidx = 0; chidx < CONFIG_USBHOST_PIPE_NUM; chidx++) {
        /* Is this channel available? */
        if (!g_ehci_hcd.chan[chidx].inuse) {
            /* Yes... make it "in use" and return the index */
            g_ehci_hcd.chan[chidx].inuse = true;
            return chidx;
        }
    }

    pr_debug("%s, line %d: EBUSY.\n", __func__, __LINE__);
    /* All of the channels are "in-use" */
    return -EBUSY;
}

static void usb_ehci_chan_free(struct usb_ehci_epinfo_s *chan)
{
    /* Mark the channel available */
    chan->inuse = false;
}

static struct usb_ehci_qh_s *usb_ehci_qh_alloc(void)
{
    struct usb_ehci_qh_s *qh;

    /* Remove the QH structure from the freelist */
    qh = (struct usb_ehci_qh_s *)g_ehci_hcd.qhfree;
    if (qh) {
        g_ehci_hcd.qhfree = ((struct usb_ehci_list_s *)qh)->flink;
        memset(qh, 0, sizeof(struct usb_ehci_qh_s));
    }

    return qh;
}

static void usb_ehci_qh_free(struct usb_ehci_qh_s *qh)
{
    struct usb_ehci_list_s *entry = (struct usb_ehci_list_s *)qh;

    /* Put the QH structure back into the free list */
    entry->flink = g_ehci_hcd.qhfree;
    g_ehci_hcd.qhfree = entry;
}

static struct usb_ehci_qtd_s *usb_ehci_qtd_alloc(void)
{
    struct usb_ehci_qtd_s *qtd;

    /* Remove the qTD from the freelist */
    qtd = (struct usb_ehci_qtd_s *)g_ehci_hcd.qtdfree;
    if (qtd) {
        g_ehci_hcd.qtdfree = ((struct usb_ehci_list_s *)qtd)->flink;
        memset(qtd, 0, sizeof(struct usb_ehci_qtd_s));
    }

    return qtd;
}

static void usb_ehci_qtd_free(struct usb_ehci_qtd_s *qtd)
{
    struct usb_ehci_list_s *entry = (struct usb_ehci_list_s *)qtd;

    /* Put the qTD back into the free list */
    entry->flink = g_ehci_hcd.qtdfree;
    g_ehci_hcd.qtdfree = entry;
}

static int usb_ehci_qh_foreach(struct usb_ehci_qh_s *qh, u32 **bp,
                               foreach_qh_t handler, void *arg)
{
    struct usb_ehci_qh_s *next;
    u32 physaddr;
    int ret;

    while (qh) {
        /* Is this the end of the list?  Check the horizontal link pointer
         * (HLP) terminate (T) bit.  If T==1, then the HLP address is not
         * valid.
         */
        physaddr = (qh->hw.hlp);

        if ((physaddr & QH_HLP_T) != 0) {
            /* Set the next pointer to NULL.  This will terminate the loop. */
            next = NULL;
        } else if ((physaddr & QH_HLP_MASK) == (u32)(uintptr_t)&g_asynchead) {
            /* Is the next QH the asynchronous list head which will always be at
             * the end of the asynchronous queue?
             */
            /* That will also terminate the loop */
            next = NULL;
        } else {
            /* Otherwise, there is a QH structure after this one that describes
             * another transaction.
             */
            physaddr = (qh->hw.hlp) & QH_HLP_MASK;
            next = (struct usb_ehci_qh_s *)(uintptr_t)(physaddr);
        }

        /* Perform the user action on this entry.  The action might result in
         * unlinking the entry!  But that is okay because we already have the
         * next QH pointer.
         *
         * Notice that we do not manage the back pointer (bp).  If the callout
         * uses it, it must update it as necessary.
         */

        ret = handler(qh, bp, arg);

        /* If the handler returns any non-zero value, then terminate the
         * traversal early.
         */
        if (ret != 0) {
            return ret;
        }

        /* Set up to visit the next entry */
        qh = next;
    }

    return 0;
}

static int usb_ehci_qtd_foreach(struct usb_ehci_qh_s *qh, foreach_qtd_t handler,
                                void *arg)
{
    struct usb_ehci_qtd_s *qtd;
    struct usb_ehci_qtd_s *next;
    u32 physaddr;
    u32 *bp;
    int ret;

    /* Handle the special case where the queue is empty */

    bp = &qh->fqp;    /* Start of qTDs in original list */
    physaddr = (*bp); /* Physical address of first qTD in CPU order */

    if ((physaddr & QTD_NQP_T) != 0) {
        return 0;
    }

    /* Start with the first qTD in the list */
    qtd = (struct usb_ehci_qtd_s *)(uintptr_t)(physaddr);
    next = NULL;

    /* And loop until we encounter the end of the qTD list */

    while (qtd) {
        /* Is this the end of the list?  Check the next qTD pointer (NQP)
         * terminate (T) bit.  If T==1, then the NQP address is not valid.
         */

        if (((qtd->hw.nqp) & QTD_NQP_T) != 0) {
            /* Set the next pointer to NULL.  This will terminate the loop. */
            next = NULL;
        } else {
            physaddr = (qtd->hw.nqp) & QTD_NQP_NTEP_MASK;
            next = (struct usb_ehci_qtd_s *)(uintptr_t)(physaddr);
        }

        /* Perform the user action on this entry.  The action might result in
         * unlinking the entry!  But that is okay because we already have the
         * next qTD pointer.
         *
         * Notice that we do not manage the back pointer (bp).  If the call-
         * out uses it, it must update it as necessary.
         */

        ret = handler(qtd, &bp, arg);

        /* If the handler returns any non-zero value, then terminate the
         * traversal early.
         */

        if (ret != 0) {
            return ret;
        }

        /* Set up to visit the next entry */
        qtd = next;
    }

    return 0;
}

static int usb_ehci_qtd_discard(struct usb_ehci_qtd_s *qtd, u32 **bp, void *arg)
{
    aicos_dcache_clean_invalid_range((void *)&qtd->hw,
                                     ROUNDUP(sizeof(struct usb_ehci_qtd_s), CACHE_LINE_SIZE));
    /* Remove the qTD from the list by updating the forward pointer to skip
     * around this qTD.  We do not change that pointer because are repeatedly
     * removing the aTD at the head of the QH list.
     */

    **bp = qtd->hw.nqp;
    /* Then free the qTD */

    usb_ehci_qtd_free(qtd);
    return 0;
}

static int usb_ehci_qh_discard(struct usb_ehci_qh_s *qh)
{
    int ret;

    aicos_dcache_clean_invalid_range((void *)&qh->hw,
                                     ROUNDUP(sizeof(struct usb_ehci_qh_s), CACHE_LINE_SIZE));
    /* Free all of the qTD's attached to the QH */
    ret = usb_ehci_qtd_foreach(qh, usb_ehci_qtd_discard, NULL);

    /* Then free the QH itself */
    usb_ehci_qh_free(qh);
    return ret;
}

#define EHCI_FULL_SPEED (0) /* Full-Speed (12Mbs) */
#define EHCI_LOW_SPEED  (1) /* Low-Speed (1.5Mbs) */
#define EHCI_HIGH_SPEED (2) /* High-Speed (480 Mb/s) */
static inline u8 usb_ehci_speed(u8 usbspeed)
{
    u8 g_ehci_speed[4] = { 0, EHCI_LOW_SPEED, EHCI_FULL_SPEED,
                           EHCI_HIGH_SPEED };

    return g_ehci_speed[usbspeed];
}

static struct usb_ehci_qh_s *
usb_ehci_qh_create(struct usb_ehci_epinfo_s *epinfo)
{
    struct usb_ehci_qh_s *qh;
    u32 regval;
    u8 hubaddr;
    u8 hubport;
    struct usb_ehci_epinfo_s *ep0info;
    struct usbh_hubport *rhport;

    rhport = epinfo->hport;

    ep0info = (struct usb_ehci_epinfo_s *)rhport->ep0;
    /* Allocate a new queue head structure */

    qh = usb_ehci_qh_alloc();
    if (qh == NULL) {
        return NULL;
    }

    /* Save the endpoint information with the QH itself */
    qh->epinfo = epinfo;

    /* Write QH endpoint characteristics:
     *
     * FIELD    DESCRIPTION                     VALUE/SOURCE
     * -------- ------------------------------- --------------------
     * DEVADDR  Device address                  Endpoint structure
     * I        Inactivate on Next Transaction  0
     * ENDPT    Endpoint number                 Endpoint structure
     * EPS      Endpoint speed                  Endpoint structure
     * DTC      Data toggle control             1
     * MAXPKT   Max packet size                 Endpoint structure
     * C        Control endpoint                Calculated
     * RL       NAK count reloaded              0
     */

    regval = ((u32)epinfo->devaddr << QH_EPCHAR_DEVADDR_SHIFT) |
             ((u32)epinfo->epno << QH_EPCHAR_ENDPT_SHIFT) |
             ((u32)usb_ehci_speed(epinfo->speed) << QH_EPCHAR_EPS_SHIFT) |
             QH_EPCHAR_DTC |
             ((u32)epinfo->maxpacket << QH_EPCHAR_MAXPKT_SHIFT) |
             ((u32)0 << QH_EPCHAR_RL_SHIFT);

    /* Paragraph 3.6.3: "Control Endpoint Flag (C). If the QH.EPS field
     * indicates the endpoint is not a high-speed device, and the endpoint
     * is an control endpoint, then software must set this bit to a one.
     * Otherwise it should always set this bit to a zero."
     */

    if (epinfo->speed != USB_SPEED_HIGH &&
        epinfo->xfrtype == USB_ENDPOINT_TYPE_CONTROL) {
        regval |= QH_EPCHAR_C;
    }

    /* Save the endpoint characteristics word with the correct byte order */
    qh->hw.epchar = (regval);

    /* Write QH endpoint capabilities
     *
     * FIELD    DESCRIPTION                     VALUE/SOURCE
     * -------- ------------------------------- --------------------
     * SSMASK   Interrupt Schedule Mask         Depends on epinfo->xfrtype
     * SCMASK   Split Completion Mask           0
     * HUBADDR  Hub Address                     roothub port devaddr
     * PORT     Port number                     RH port index
     * MULT     High band width multiplier      1
     */

    hubaddr = ep0info->devaddr;
    hubport = rhport->port;

    regval = ((u32)hubaddr << QH_EPCAPS_HUBADDR_SHIFT) |
             ((u32)hubport << QH_EPCAPS_PORT_SHIFT) |
             ((u32)1 << QH_EPCAPS_MULT_SHIFT);

    qh->hw.epcaps = (regval);

    /* Mark this as the end of this list.  This will be overwritten if/when the
     * next qTD is added to the queue.
     */

    qh->hw.hlp = (QH_HLP_T);
    qh->hw.overlay.nqp = (QH_NQP_T);
    qh->hw.overlay.alt = (QH_AQP_T);
    return qh;
}

int usbh_ep_alloc(usbh_epinfo_t *ep, const struct usbh_endpoint_cfg *ep_cfg)
{
    struct usb_ehci_epinfo_s *epinfo;
    struct usbh_hubport *hport;
    int chidx;

    hport = ep_cfg->hport;
    chidx = usb_ehci_chan_alloc();
    if (chidx < 0) {
        pr_err("%s, line %d: NOMEM.\n", __func__, __LINE__);
        return -ENOMEM;
    }
    epinfo = &g_ehci_hcd.chan[chidx];

    memset(epinfo, 0, sizeof(struct usb_ehci_epinfo_s));

    epinfo->epno = ep_cfg->ep_addr & 0x7f;
    epinfo->dirin = (ep_cfg->ep_addr & 0x80) ? 1 : 0;
    epinfo->devaddr = hport->dev_addr;
    epinfo->maxpacket = ep_cfg->ep_mps;
    epinfo->xfrtype = ep_cfg->ep_type;
    epinfo->speed = hport->speed;
    epinfo->hport = hport;

    /* restore variable */
    epinfo->inuse = true;

    *ep = (usbh_epinfo_t)epinfo;
    return 0;
}

int usbh_ep_free(usbh_epinfo_t ep)
{
    struct usb_ehci_epinfo_s *epinfo = (struct usb_ehci_epinfo_s *)ep;

    usb_ehci_chan_free(epinfo);

    return 0;
}

static int usb_ehci_reset(int id)
{
    u32 regval = 0;
    u64 start_us;
    volatile struct ehci_hcor_s *hcor;

    hcor = (struct ehci_hcor_s *)USB_EHCI_HCOR_BASE(id);

    /* Make sure that the EHCI is halted:  "When [the Run/Stop] bit is set to
     * 0, the Host Controller completes the current transaction on the USB and
     * then halts. The HC Halted bit in the status register indicates when the
     * Host Controller has finished the transaction and has entered the
     * stopped state..."
     */

    writel(0, &hcor->usbcmd);

    /* "... Software should not set [HCRESET] to a one when the HCHalted bit in
     *  the USBSTS register is a zero. Attempting to reset an actively running
     *  host controller will result in undefined behavior."
     */

    start_us = aic_get_time_us();
    do {
        /* Wait and update the timeout counter */
        if ((aic_get_time_us() - start_us) > 1000)
            break;

        /* Get the current value of the USBSTS register.  This loop will
         * terminate when either the timeout exceeds one millisecond or when
         * the HCHalted bit is no longer set in the USBSTS register.
         */

        regval = readl(&hcor->usbsts);
    } while ((regval & EHCI_USBSTS_HALTED) == 0);

    /* Is the EHCI still running?  Did we timeout? */
    if ((regval & EHCI_USBSTS_HALTED) == 0) {
        pr_err("Wait ehci halt timeout.\n");
        return -ETIMEDOUT;
    }

    /* Now we can set the HCReset bit in the USBCMD register to
     * initiate the reset
     */

    regval = readl(&hcor->usbcmd);
    regval |= EHCI_USBCMD_HCRESET;
    writel(regval, &hcor->usbcmd);

    /* Wait for the HCReset bit to become clear */

    start_us = aic_get_time_us();
    do {
        /* Wait and update the timeout counter */
        if ((aic_get_time_us() - start_us) > 1000)
            break;

        /* Get the current value of the USBCMD register.  This loop will
         * terminate when either the timeout exceeds one second or when the
         * HCReset bit is no longer set in the USBSTS register.
         */

        regval = readl(&hcor->usbcmd);
    } while ((regval & EHCI_USBCMD_HCRESET) != 0);

    /* Return either success or a timeout */

    return (regval & EHCI_USBCMD_HCRESET) != 0 ? -ETIMEDOUT : 0;
}

static int usb_ehci_wait_usbsts(int id, u32 maskbits, u32 donebits, u32 timeout)
{
    u32 regval;
    u32 start, cur;
    volatile struct ehci_hcor_s *hcor;

    hcor = (struct ehci_hcor_s *)USB_EHCI_HCOR_BASE(id);

    start = aic_get_time_us();
    do {
        /* Read the USBSTS register and check for a system error */
        regval = readl(&hcor->usbsts);
        if ((regval & EHCI_INT_SYSERROR) != 0) {
            return -EIO;
        }

        /* Mask out the bits of interest */
        regval &= maskbits;

        cur = aic_get_time_us();
        /* Loop until the masked bits take the specified value or until a
         * timeout occurs.
         */
        if (!(regval != donebits))
            break;

    } while ((cur - start) < timeout);

    /* We got here because either the waited for condition or a timeout
     * occurred.  Return a value to indicate which.
     */
    return (regval == donebits) ? 0 : -ETIMEDOUT;
}

int usbh_portchange_wait(int id)
{
    u32 usbsts, pending, regval, timeout;
    u64 start_us;
    volatile struct ehci_hcor_s *hcor;

#ifdef AICUPG_UDISK_VERSION3_SUPPORT
    timeout = 150000; // Some Udisk need to wait for more than 1s
#else
    timeout = 1000;
#endif

    hcor = (struct ehci_hcor_s *)USB_EHCI_HCOR_BASE(id);

    start_us = aic_get_time_us();
    do {
        /* Read Interrupt Status and mask out interrupts that are not enabled. */
        usbsts = readl(&hcor->usbsts);
        regval = readl(&hcor->usbintr);

        /* Handle all unmasked interrupt sources */
        pending = usbsts & regval;

        /* Clear all pending interrupts */
        writel(usbsts & EHCI_INT_ALLINTS, &hcor->usbsts);

        if ((pending & EHCI_INT_PORTSC) != 0) {
            pr_warn("Port status changed.\n");
            return 0;
        }
    } while ((aic_get_time_us() - start_us) < timeout);

    return -1;
}

int usbh_get_port_connect_status(int id, int port)
{
    u32 portsc;
    bool connected = false;
    volatile struct ehci_hcor_s *hcor;

    hcor = (struct ehci_hcor_s *)USB_EHCI_HCOR_BASE(id);

    if (port <= 0) {
        pr_err("port id not correct %d\n", port);
        return false;
    }

    /* Only one root hub port */
    portsc = readl(&hcor->portsc[port - 1]);

    /* Handle port connection status change (CSC) events */
    if ((portsc & EHCI_PORTSC_CSC) != 0) {
        if ((portsc & EHCI_PORTSC_CCS) == EHCI_PORTSC_CCS) {
            /* Connected ... Did we just become connected? */
            pr_warn("port connected\n");
            connected = true;
        } else {
            pr_warn("port disconnected\n");
            connected = false;
        }
    }

    /* Clear all pending port interrupt sources by writing a '1' to the
     * corresponding bit in the PORTSC register.  In addition, we need
     * to preserve the values of all R/W bits (RO bits don't matter)
     */
    writel(portsc, &hcor->portsc[port - 1]);

    return connected;
}

int usbh_reset_port(int port, int id)
{
    u64 start_us;
    u32 regval;
    volatile struct ehci_hcor_s *hcor;

    hcor = (struct ehci_hcor_s *)USB_EHCI_HCOR_BASE(id);

    regval = readl(&hcor->portsc[port - 1]);
    regval &= ~EHCI_PORTSC_PE;
    regval |= EHCI_PORTSC_RESET;
    writel(regval, &hcor->portsc[port - 1]);

    aic_mdelay(3);

    regval = readl(&hcor->portsc[port - 1]);
    regval &= ~EHCI_PORTSC_RESET;
    writel(regval, &hcor->portsc[port - 1]);

    /* Wait for the port reset to complete
     *
     * Paragraph 2.3.9:
     *
     *  "Note that when software writes a zero to this bit there may be a
     *   delay before the bit status changes to a zero. The bit status will
     *   not read as a zero until after the reset has completed. If the port
     *   is in high-speed mode after reset is complete, the host controller
     *   will automatically enable this port (e.g. set the Port Enable bit
     *   to a one). A host controller must terminate the reset and stabilize
     *   the state of the port within 2 milliseconds of software transitioning
     *   this bit from a one to a zero ..."
     */

    start_us = aic_get_time_us();
    while ((readl(&hcor->portsc[port - 1]) & EHCI_PORTSC_RESET) != 0) {
        if ((aic_get_time_us() - start_us) > 3000)
            return -ETIMEDOUT;
    }

    return 0;
}

u8 usbh_get_port_speed(int id)
{
    /* Defined by individual manufacturers */
    u32 regval;
    volatile struct ehci_hcor_s *hcor;

    hcor = (struct ehci_hcor_s *)USB_EHCI_HCOR_BASE(id);

    regval = readl(&hcor->portsc[0]);
    if ((regval & EHCI_PORTSC_LSTATUS_MASK) == EHCI_PORTSC_LSTATUS_KSTATE) {
        pr_err("USB_SPEED_LOW\n");
        return USB_SPEED_LOW;
    }

    if (regval & EHCI_PORTSC_PE) {
        pr_err("USB_SPEED_HIGH\n");
        return USB_SPEED_HIGH;
    }

    pr_info("USB_SPEED_FULL\n");
    return USB_SPEED_FULL;
}

int usbh_ep0_reconfigure(usbh_epinfo_t ep, u8 dev_addr, u8 ep_mps, u8 speed)
{
    struct usb_ehci_epinfo_s *epinfo;
    int ret = 0;

    epinfo = (struct usb_ehci_epinfo_s *)ep;
    epinfo->devaddr = dev_addr;
    epinfo->speed = speed;
    epinfo->maxpacket = ep_mps;

    return ret;
}

static int usb_ehci_qtd_addbpl(struct usb_ehci_qtd_s *qtd, const void *buffer,
                               u32 buflen)
{
    u32 physaddr, nbytes, next, ndx;

    aicos_dcache_clean_invalid_range((void *)buffer, ROUNDUP(buflen, CACHE_LINE_SIZE));
    physaddr = (u32)(uintptr_t)buffer;
    for (ndx = 0; ndx < 5; ndx++) {
        /* Write the physical address of the buffer into the qTD buffer
         * pointer list.
         */

        qtd->hw.bpl[ndx] = (physaddr);

        /* Get the next buffer pointer (in the case where we will have to
         * transfer more then one chunk).  This buffer must be aligned to a
         * 4KB address boundary.
         */

        next = (physaddr + 4096) & ~4095;

        /* How many bytes were included in the last buffer?  Was it the whole
         * thing?
         */

        nbytes = next - physaddr;
        if (nbytes >= buflen) {
            /* Yes... it was the whole thing.  Break out of the loop early. */
            break;
        }

        /* Adjust the buffer length and physical address for the next time
         * through the loop.
         */

        buflen -= nbytes;
        physaddr = next;
    }

    /* Handle the case of a huge buffer > 4*4KB = 16KB */

    if (ndx >= 5) {
        pr_err("%s, line %d\n", __func__, __LINE__);
        return -EFBIG;
    }

    return 0;
}

static struct usb_ehci_qtd_s *usb_ehci_qtd_setupphase(struct usb_ehci_epinfo_s *epinfo,
                        struct usb_setup_packet *setup)
{
    struct usb_ehci_qtd_s *qtd;
    u32 regval;
    int ret;

    /* Allocate a new Queue Element Transfer Descriptor (qTD) */
    qtd = usb_ehci_qtd_alloc();
    if (qtd == NULL) {
        return NULL;
    }

    /* Mark this as the end of the list (this will be overwritten if another
     * qTD is added after this one).
     */

    qtd->hw.nqp = (QTD_NQP_T);
    qtd->hw.alt = (QTD_AQP_T);

    /* Write qTD token:
     *
     * FIELD    DESCRIPTION                     VALUE/SOURCE
     * -------- ------------------------------- --------------------
     * STATUS   Status                          QTD_TOKEN_ACTIVE
     * PID      PID Code                        QTD_TOKEN_PID_SETUP
     * CERR     Error Counter                   3
     * CPAGE    Current Page                    0
     * IOC      Interrupt on complete           0
     * NBYTES   Total Bytes to Transfer         8
     * TOGGLE   Data Toggle                     0
     */

    regval = QTD_TOKEN_ACTIVE | QTD_TOKEN_PID_SETUP |
             ((u32)3 << QTD_TOKEN_CERR_SHIFT) |
             ((u32)8 << QTD_TOKEN_NBYTES_SHIFT);

    qtd->hw.token = (regval);

    /* Add the buffer data */
    ret = usb_ehci_qtd_addbpl(qtd, (u8 *)setup, 8);
    if (ret < 0) {
        usb_ehci_qtd_free(qtd);
        return NULL;
    }

    /* Add the data transfer size to the count in the epinfo structure */
    epinfo->xfrd += 8;

    return qtd;
}

static struct usb_ehci_qtd_s *usb_ehci_qtd_dataphase(struct usb_ehci_epinfo_s *epinfo, void *buffer,
                       int buflen, u32 tokenbits)
{
    struct usb_ehci_qtd_s *qtd;
    u32 regval;
    int ret;

    /* Allocate a new Queue Element Transfer Descriptor (qTD) */

    qtd = usb_ehci_qtd_alloc();
    if (qtd == NULL) {
        pr_err("%s qtd null\n", __func__);
        return NULL;
    }

    /* Mark this as the end of the list (this will be overwritten if another
     * qTD is added after this one).
     */

    qtd->hw.nqp = (QTD_NQP_T);
    qtd->hw.alt = (QTD_AQP_T);

    /* Write qTD token:
     *
     * FIELD    DESCRIPTION                     VALUE/SOURCE
     * -------- ------------------------------- --------------------
     * STATUS   Status                          QTD_TOKEN_ACTIVE
     * PID      PID Code                        Contained in tokenbits
     * CERR     Error Counter                   3
     * CPAGE    Current Page                    0
     * IOC      Interrupt on complete           Contained in tokenbits
     * NBYTES   Total Bytes to Transfer         buflen
     * TOGGLE   Data Toggle                     Contained in tokenbits
     */

    regval = tokenbits | QTD_TOKEN_ACTIVE | ((u32)3 << QTD_TOKEN_CERR_SHIFT) |
             ((u32)buflen << QTD_TOKEN_NBYTES_SHIFT);

    qtd->hw.token = (regval);

    ret = usb_ehci_qtd_addbpl(qtd, buffer, buflen);
    if (ret < 0) {
        pr_err("%s qtd addbpl failed\n", __func__);
        usb_ehci_qtd_free(qtd);
        return NULL;
    }

    /* Add the data transfer size to the count in the epinfo structure */
    epinfo->xfrd += buflen;

    return qtd;
}

static struct usb_ehci_qtd_s *usb_ehci_qtd_statusphase(u32 tokenbits)
{
    struct usb_ehci_qtd_s *qtd;
    u32 regval;

    /* Allocate a new Queue Element Transfer Descriptor (qTD) */

    qtd = usb_ehci_qtd_alloc();
    if (qtd == NULL) {
        return NULL;
    }

    /* Mark this as the end of the list (this will be overwritten if another
     * qTD is added after this one).
     */

    qtd->hw.nqp = (QTD_NQP_T);
    qtd->hw.alt = (QTD_AQP_T);

    /* Write qTD token:
     *
     * FIELD    DESCRIPTION                     VALUE/SOURCE
     * -------- ------------------------------- --------------------
     * STATUS   Status                          QTD_TOKEN_ACTIVE
     * PID      PID Code                        Contained in tokenbits
     * CERR     Error Counter                   3
     * CPAGE    Current Page                    0
     * IOC      Interrupt on complete           QTD_TOKEN_IOC
     * NBYTES   Total Bytes to Transfer         0
     * TOGGLE   Data Toggle                     Contained in tokenbits
     */

    regval = tokenbits | QTD_TOKEN_ACTIVE | QTD_TOKEN_IOC |
             ((u32)3 << QTD_TOKEN_CERR_SHIFT);

    qtd->hw.token = (regval);

    return qtd;
}

static int usb_ehci_qtd_flush(struct usb_ehci_qtd_s *qtd, u32 **bp, void *arg)
{
    /* Flush the D-Cache, i.e., make the contents of the memory match the
    * contents of the D-Cache in the specified address range and invalidate
    * the D-Cache to force re-loading of the data from memory when next
    * accessed.
    */

    aicos_dcache_clean_invalid_range((void *)(uintptr_t)&qtd->hw,
                                     ROUNDUP(sizeof(struct ehci_qtd_s), CACHE_LINE_SIZE));

    return 0;
}

static int usb_ehci_qh_flush(struct usb_ehci_qh_s *qh)
{
    /* Flush the QH first.  This will write the contents of the D-cache to RAM
    * and invalidate the contents of the D-cache so that the next access will
    * be reloaded from D-Cache.
    */

    aicos_dcache_clean_invalid_range((void *)(uintptr_t)&qh->hw,
                                     ROUNDUP(sizeof(struct ehci_qh_s), CACHE_LINE_SIZE));

    /* Then flush all of the qTD entries in the queue */

    return usb_ehci_qtd_foreach(qh, usb_ehci_qtd_flush, NULL);
}

static void usb_ehci_qh_enqueue(struct usb_ehci_qh_s *qhead,
                                struct usb_ehci_qh_s *qh)
{
    u32 physaddr;

    /* Set the internal fqp field.  When we transverse the QH list later,
     * we need to know the correct place to start because the overlay may no
     * longer point to the first qTD entry.
     */

    qh->fqp = qh->hw.overlay.nqp;

    /* Add the new QH to the head of the asynchronous queue list.
     *
     * First, attach the old head as the new QH HLP and flush the new QH and
     * its attached qTDs to RAM.
     */

    qh->hw.hlp = qhead->hw.hlp;
    usb_ehci_qh_flush(qh);
    /* Then set the new QH as the first QH in the asynchronous queue */

    physaddr = (u32)(uintptr_t)qh;
    qhead->hw.hlp = (physaddr | QH_HLP_TYP_QH);
    aicos_dcache_clean_range((void *)(uintptr_t)&qhead->hw,
                             ROUNDUP(sizeof(struct ehci_qh_s), CACHE_LINE_SIZE));
}

static int usb_ehci_ioc_setup(struct usb_ehci_epinfo_s *epinfo)
{
    int ret = -ENODEV;

    /* Is the device still connected? */

    if (epinfo->hport->connected) {
        /* We want to be awakened by IOC interrupt */
        epinfo->status = 0;      /* No status yet */
        epinfo->xfrd = 0;        /* Nothing transferred yet */
        epinfo->result = -EBUSY; /* Transfer in progress */
        ret = 0;                 /* We are good to go */
    }
    return ret;
}

static int usb_ehci_qtd_ioccheck(struct usb_ehci_qtd_s *qtd, u32 **bp,
                                 void *arg)
{
    struct usb_ehci_epinfo_s *epinfo = (struct usb_ehci_epinfo_s *)arg;

    aicos_dcache_invalid_range((void *)(uintptr_t)&qtd->hw,
                               ROUNDUP(sizeof(struct usb_ehci_qtd_s), CACHE_LINE_SIZE));
    /* Remove the qTD from the list
     *
     * NOTE that we don't check if the qTD is active nor do we check if there
     * are any errors reported in the qTD.  If the transfer halted due to
     * an error, then qTDs in the list after the error qTD will still appear
     * to be active.
     */

    **bp = qtd->hw.nqp;

    /* Subtract the number of bytes left untransferred.  The epinfo->xfrd
     * field is initialized to the total number of bytes to be transferred
     * (all qTDs in the list).  We subtract out the number of untransferred
     * bytes on each transfer and the final result will be the number of bytes
     * actually transferred.
     */

    epinfo->xfrd -= ((qtd->hw.token) & QTD_TOKEN_NBYTES_MASK) >> QTD_TOKEN_NBYTES_SHIFT;

    /* Release this QH by returning it to the free list */

    usb_ehci_qtd_free(qtd);

    return 0;
}

static int usb_ehci_qh_ioccheck(struct usb_ehci_qh_s *qh, u32 **bp, void *arg)
{
    struct usb_ehci_epinfo_s *epinfo;
    u32 token;
    int ret;

    aicos_dcache_invalid_range((void *)(uintptr_t)&qh->hw,
                               ROUNDUP(sizeof(struct ehci_qh_s), CACHE_LINE_SIZE));
    /* Get the endpoint info pointer from the extended QH data.  Only the
     * g_asynchead QH can have a NULL epinfo field.
     */

    epinfo = qh->epinfo;

    /* Paragraph 3.6.3:  "The nine DWords in [the Transfer Overlay] area
     * represent a transaction working space for the host controller.  The
     * general operational model is that the host controller can detect
     * whether the overlay area contains a description of an active transfer.
     * If it does not contain an active transfer, then it follows the Queue
     * Head Horizontal Link Pointer to the next queue head.  The host
     * controller will never follow the Next Transfer Queue Element or
     * Alternate Queue Element pointers unless it is actively attempting to
     * advance the queue ..."
     */

    /* Is the qTD still active? */

    token = (qh->hw.overlay.token);

    if ((token & QH_TOKEN_ACTIVE) != 0) {
        /* Yes... we cannot process the QH while it is still active.  Return
         * zero to visit the next QH in the list.
         */
        *bp = &qh->hw.hlp;
        return 0;
    }

    /* Remove all active, attached qTD structures from the inactive QH */
    ret = usb_ehci_qtd_foreach(qh, usb_ehci_qtd_ioccheck, (void *)qh->epinfo);
    if (ret < 0) {
    }
    /* If there is no longer anything attached to the QH, then remove it from
     * the asynchronous queue.
     */

    if ((qh->fqp & QTD_NQP_T) == 0) {
        /* the horizontal link pointer of this QH will become the
         * next back pointer.
         */

        *bp = &qh->hw.hlp;
        aicos_dcache_clean_range((void *)(uintptr_t)bp,
                                 ROUNDUP(sizeof(uint32_t), CACHE_LINE_SIZE));
        return 0;
    }

    /* Not terminal case: */

    /* Set the forward link of the previous QH to point to the next
     * QH in the list.
     */

    **bp = qh->hw.hlp;
    /* Check for errors, update the data toggle */

    if ((token & QH_TOKEN_ERRORS) == 0) {
        /* No errors.. Save the last data toggle value */

        epinfo->toggle = (token >> QTD_TOKEN_TOGGLE_SHIFT) & 1;

        /* Report success */
        epinfo->status = 0;
        epinfo->result = 0;

    } else {
        /* An error occurred */
        epinfo->status = (token & QH_TOKEN_STATUS_MASK) >> QH_TOKEN_STATUS_SHIFT;

        /* The HALT condition is set on a variety of conditions:  babble,
         * error counter countdown to zero, or a STALL.  If we can rule
         * out babble (babble bit not set) and if the error counter is
         * non-zero, then we can assume a STALL. In this case, we return
         * -PERM to inform the class driver of the stall condition.
         */

        if ((token & (QH_TOKEN_BABBLE | QH_TOKEN_HALTED)) == QH_TOKEN_HALTED &&
            (token & QH_TOKEN_CERR_MASK) != 0) {
            /* It is a stall,  Note that the data toggle is reset
             * after the stall.
             */
            epinfo->result = -EPERM;
            epinfo->toggle = 0;
        } else {
            /* Otherwise, it is some kind of data transfer error */
            epinfo->result = -EIO;
        }
    }

    /* Then release this QH by returning it to the free list */
    usb_ehci_qh_free(qh);

    return 0;
}

static int usb_ehci_transfer_wait(int id, struct usb_ehci_epinfo_s *epinfo,
                                  u32 timeout)
{
    int ret = 0;
    u32 start, cur;
    u32 usbsts, pending, regval;
    struct usb_ehci_qh_s *qh = NULL;
    volatile struct ehci_hcor_s *hcor;
    u32 *bp;

    hcor = (struct ehci_hcor_s *)USB_EHCI_HCOR_BASE(id);

    start = aic_get_time_us();
    do {
        /* Read Interrupt Status and mask out interrupts that are not enabled. */
        usbsts = readl(&hcor->usbsts);
        regval = readl(&hcor->usbintr);

        /* Handle all unmasked interrupt sources */
        pending = usbsts & regval;

        /* Clear all pending interrupts */
        writel(usbsts & EHCI_INT_ALLINTS, &hcor->usbsts);

        cur = aic_get_time_us();
        if ((pending & (EHCI_INT_USBINT | EHCI_INT_USBERRINT)) == 0)
            continue;

        bp = (u32 *)&g_asynchead.hw.hlp;
        qh = (struct usb_ehci_qh_s *)(uintptr_t)((*bp) & QH_HLP_MASK);
        /* If the asynchronous queue is empty, then the forward point in the
         * asynchronous queue head will point back to the queue head.
         */
        if (qh && qh != &g_asynchead) {
            /* Then traverse and operate on every QH and qTD in the asynchronous
             * queue
             */
            usb_ehci_qh_foreach(qh, &bp, usb_ehci_qh_ioccheck, NULL);
        }

        if (epinfo->result >= 0) {
            break;
        }
    } while (cur - start < timeout);

    ret = epinfo->result;

    if (ret < 0) {
        return ret;
    }

    /* Transfer completed successfully.  Return the number of bytes transferred.*/
    return epinfo->xfrd;
}

#define USBHOST_CONTROL_TRANSFER_TIMEOUT 2000000

static void ehci_disable_async(int id)
{
    u32 regval;
    volatile struct ehci_hcor_s *hcor;

    hcor = (struct ehci_hcor_s *)USB_EHCI_HCOR_BASE(id);

    regval = readl(&hcor->usbcmd);
    regval &= ~(EHCI_USBCMD_ASEN);
    writel(regval, &hcor->usbcmd);

    usb_ehci_wait_usbsts(id, EHCI_USBSTS_ASS, 0, USBHOST_CONTROL_TRANSFER_TIMEOUT);
}

static void ehci_enable_async(int id)
{
    u32 regval;
    volatile struct ehci_hcor_s *hcor;

    hcor = (struct ehci_hcor_s *)USB_EHCI_HCOR_BASE(id);

    regval = (u32)(uintptr_t)&g_asynchead;
    /* Set the Current Asynchronous List Address. */
    writel(regval, &hcor->asynclistaddr);

    regval = readl(&hcor->usbcmd);
    regval |= (EHCI_USBCMD_ASEN);
    writel(regval, &hcor->usbcmd);

    usb_ehci_wait_usbsts(id, EHCI_USBSTS_ASS, EHCI_USBSTS_ASS, USBHOST_CONTROL_TRANSFER_TIMEOUT);
}

static int usb_ehci_control_init(struct usb_ehci_epinfo_s *epinfo,
                                 struct usb_setup_packet *setup, u8 *buffer,
                                 u32 buflen)
{
    struct usb_ehci_qh_s *qh;
    struct usb_ehci_qtd_s *qtd;
    u32 tokenbits, physaddr, *flink, *alt, toggle;
    bool dirin = false;

    /* Create and initialize a Queue Head (QH) structure for this transfer */
    qh = usb_ehci_qh_create(epinfo);
    if (qh == NULL) {
        pr_err("Failed to create QH.\n");
        return -ENOMEM;
    }

    epinfo->qh = qh;
    /* Initialize the QH link and get the next data toggle (not used for SETUP
     * transfers)
     */

    flink = &qh->hw.overlay.nqp;
    toggle = (u32)epinfo->toggle << QTD_TOKEN_TOGGLE_SHIFT;

    /* Is there an EP0 SETUP request?  If so, we will queue two or three qTDs:
     *
     *   1) One for the SETUP phase,
     *   2) One for the DATA phase (if there is data), and
     *   3) One for the STATUS phase.
     */

    /* Allocate a new Queue Element Transfer Descriptor (qTD) for the SETUP
     * phase of the request sequence.
     */
    {
        qtd = usb_ehci_qtd_setupphase(epinfo, setup);
        if (qtd == NULL) {
            pr_err("%s, line %d: NOMEM.\n", __func__, __LINE__);
            return -ENOMEM;
        }
        /* Link the new qTD to the QH head. */

        physaddr = ((u32)(uintptr_t)qtd);
        *flink = (physaddr);

        /* Get the new forward link pointer and data toggle */

        flink = &qtd->hw.nqp;
        toggle = QTD_TOKEN_TOGGLE;
    }
    /* A buffer may or may be supplied with an EP0 SETUP transfer.  A buffer
     * will always be present for normal endpoint data transfers.
     */

    alt = NULL;

    if (buffer != NULL && buflen > 0) {
        /* Extra TOKEN bits include the data toggle, the data PID, and if
         * there is no request, an indication to interrupt at the end of this
         * transfer.
         */

        tokenbits = toggle;

        /* Get the data token direction.
         *
         * If this is a SETUP request, use the direction contained in the
         * request.  The IOC bit is not set.
         */
        if ((setup->bmRequestType & 0x80) == 0x80) {
            tokenbits |= QTD_TOKEN_PID_IN;
            dirin = true;
        } else {
            tokenbits |= QTD_TOKEN_PID_OUT;
            dirin = false;
        }

        /* Allocate a new Queue Element Transfer Descriptor (qTD) for the data
         * buffer.
         */

        qtd = usb_ehci_qtd_dataphase(epinfo, buffer, buflen, tokenbits);
        if (qtd == NULL) {
            pr_err("%s, line %d: NOMEM.\n", __func__, __LINE__);
            return -ENOMEM;
        }

        /* Link the new qTD to either QH head of the SETUP qTD. */
        physaddr = ((u32)(uintptr_t)qtd);
        *flink = (physaddr);

        /* Set the forward link pointer to this new qTD */

        flink = &qtd->hw.nqp;

        /* If this was an IN transfer, then setup a pointer alternate link.
         * The EHCI hardware will use this link if a short packet is received.
         */

        if (dirin) {
            alt = &qtd->hw.alt;
        }
    }

    {
        /* Extra TOKEN bits include the data toggle and the correct data PID. */

        tokenbits = toggle;

        /* The status phase direction is the opposite of the data phase.  If
         * this is an IN request, then we received the buffer and we will send
         * the zero length packet handshake.
         */
        if ((setup->bmRequestType & 0x80) == 0x80) {
            tokenbits |= QTD_TOKEN_PID_OUT;
        } else {
            /* Otherwise, this in an OUT request.  We send the buffer and we expect
             * to receive the NULL packet handshake.
             */
            tokenbits |= QTD_TOKEN_PID_IN;
        }

        /* Allocate a new Queue Element Transfer Descriptor (qTD) for the
         * status
         */
        qtd = usb_ehci_qtd_statusphase(tokenbits);
        if (qtd == NULL) {
            pr_err("%s, line %d: NOMEM.\n", __func__, __LINE__);
            return -ENOMEM;
        }

        /* Link the new qTD to either the SETUP or data qTD. */
        physaddr = ((u32)(uintptr_t)qtd);
        *flink = (physaddr);

        /* In an IN data qTD was also enqueued, then linked the data qTD's
         * alternate pointer to this STATUS phase qTD in order to handle short
         * transfers.
         */

        if (alt) {
            *alt = (physaddr);
        }
    }
    /* Add the new QH to the head of the asynchronous queue list */
    usb_ehci_qh_enqueue(&g_asynchead, qh);
    return 0;
}

int usbh_control_transfer(usbh_epinfo_t ep, struct usb_setup_packet *setup,
                          u8 *buffer, int id)
{
    struct usb_ehci_epinfo_s *epinfo = (struct usb_ehci_epinfo_s *)ep;
    int ret;

    /* Set the request for the IOC event well BEFORE initiating the transfer. */
    ret = usb_ehci_ioc_setup(epinfo);
    if (ret != 0) {
        pr_err("%s, line %d: setup failed.\n", __func__, __LINE__);
        goto errout_with_setup;
    }

    ehci_disable_async(id);

    ret = usb_ehci_control_init(epinfo, setup, buffer, setup->wLength);
    if (ret < 0) {
        pr_err("%s, line %d: init failed.\n", __func__, __LINE__);
        goto errout_with_iocwait;
    }

    ehci_enable_async(id);
    /* And wait for the transfer to complete */
    ret = usb_ehci_transfer_wait(id, epinfo, USBHOST_CONTROL_TRANSFER_TIMEOUT);
    if (ret < 0) {
        pr_err("%s, line %d: wait tmo.\n", __func__, __LINE__);
        goto errout_with_iocwait;
    }

    return ret;

errout_with_iocwait:
    /* Clean-up after an error */
    if (epinfo->qh) {
        usb_ehci_qh_discard(epinfo->qh);
        epinfo->qh = NULL;
    }
errout_with_setup:
    return ret;
}

int usb_ehci_bulk_init(struct usb_ehci_epinfo_s *epinfo, u8 *buffer, u32 buflen)
{
    struct usb_ehci_qh_s *qh;
    struct usb_ehci_qtd_s *qtd;
    u32 tokenbits;
    u32 physaddr;

    /* Create and initialize a Queue Head (QH) structure for this transfer */
    qh = usb_ehci_qh_create(epinfo);
    if (qh == NULL) {
        pr_err("%s, line %d: NOMEM.\n", __func__, __LINE__);
        return -ENOMEM;
    }

    epinfo->qh = qh;

    /* Initialize the QH link and get the next data toggle */
    tokenbits = (u32)epinfo->toggle << QTD_TOKEN_TOGGLE_SHIFT;

    if (buffer != NULL && buflen > 0) {
        /* Get the direction from the epinfo structure.  Since this is not an EP0 SETUP request,
         * nothing follows the data and we want the IOC interrupt when the data transfer completes.
         */
        if (epinfo->dirin) {
            tokenbits |= (QTD_TOKEN_PID_IN | QTD_TOKEN_IOC);
        } else {
            tokenbits |= (QTD_TOKEN_PID_OUT | QTD_TOKEN_IOC);
        }

        /* Allocate a new Queue Element Transfer Descriptor (qTD) for the data
         * buffer.
             */

        qtd = usb_ehci_qtd_dataphase(epinfo, buffer, buflen, tokenbits);
        if (qtd == NULL) {
            pr_err("%s, line %d: NOMEM.\n", __func__, __LINE__);
            return -ENOMEM;
        }

        /* Link the new qTD to the QH. */
        physaddr = (u32)(uintptr_t)qtd;
        qh->hw.overlay.nqp = (physaddr);
    }

    /* Add the new QH to the head of the asynchronous queue list */
    usb_ehci_qh_enqueue(&g_asynchead, qh);

    return 0;
}

int usbh_ep_bulk_transfer(usbh_epinfo_t ep, u8 *buffer, u32 buflen, u32 timeout,
                          int id)
{
    int ret;
    struct usb_ehci_epinfo_s *epinfo = (struct usb_ehci_epinfo_s *)ep;

    ret = usb_ehci_ioc_setup(epinfo);
    if (ret < 0) {
        goto errout_with_setup;
    }

    ehci_disable_async(id);

    ret = usb_ehci_bulk_init(epinfo, buffer, buflen);
    if (ret < 0) {
        goto errout_with_iocwait;
    }

    ehci_enable_async(id);

    /* And wait for the transfer to complete */
    ret = usb_ehci_transfer_wait(id, epinfo, timeout);
    if (ret < 0) {
        goto errout_with_iocwait;
    }
    return ret;

errout_with_iocwait:
    /* Clean-up after an error */
    if (epinfo->qh) {
        usb_ehci_qh_discard(epinfo->qh);
        epinfo->qh = NULL;
    }
errout_with_setup:
    return ret;
}
