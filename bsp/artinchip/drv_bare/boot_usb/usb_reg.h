/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef _SOC_USB_H_
#define _SOC_USB_H_

#ifdef __cplusplus
extern "C" {
#endif

#define USB_DEV_REG_BASE (USB_DEV_BASE)

/* Global Registers */
#define USB_AHB_BASIC     (volatile void *)(USB_DEV_REG_BASE + 0x000)
#define USB_DEV_INIT      (volatile void *)(USB_DEV_REG_BASE + 0x004)
#define USB_PHY_IF        (volatile void *)(USB_DEV_REG_BASE + 0x008)
#define USB_ULPI_PHY      (volatile void *)(USB_DEV_REG_BASE + 0x00C)
#define USB_DEV_GINTSTS   (volatile void *)(USB_DEV_REG_BASE + 0x010)
#define USB_DEV_GINTMSK   (volatile void *)(USB_DEV_REG_BASE + 0x014)
#define USB_DEV_GRXFSIZ   (volatile void *)(USB_DEV_REG_BASE + 0x018)
#define USB_DEV_GRXSTSP   (volatile void *)(USB_DEV_REG_BASE + 0x01C)
#define USB_DEV_GNPTXFSIZ (volatile void *)(USB_DEV_REG_BASE + 0x020)
#define USB_DEV_GNPTXSTS  (volatile void *)(USB_DEV_REG_BASE + 0x024)

/* Device Registers */
#define USB_DEV_DCFG      (volatile void *)(USB_DEV_REG_BASE + 0x200)
#define USB_DEV_DCTL      (volatile void *)(USB_DEV_REG_BASE + 0x204)
#define USB_DEV_DSTS      (volatile void *)(USB_DEV_REG_BASE + 0x208)
#define USB_DEV_DIEPMSK   (volatile void *)(USB_DEV_REG_BASE + 0x20C)
#define USB_DEV_DOEPMSK   (volatile void *)(USB_DEV_REG_BASE + 0x210)
#define USB_DEV_DAINT     (volatile void *)(USB_DEV_REG_BASE + 0x214)
#define USB_DEV_DAINTMSK  (volatile void *)(USB_DEV_REG_BASE + 0x218)
#define USB_DEV_DIEPCTL0  (volatile void *)(USB_DEV_REG_BASE + 0x220)
#define USB_DEV_DIEPINT0  (volatile void *)(USB_DEV_REG_BASE + 0x260)
#define USB_DEV_DIEPTSIZ0 (volatile void *)(USB_DEV_REG_BASE + 0x2A0)
#define USB_DEV_DIEPCTL(i) \
    (volatile void *)(USB_DEV_REG_BASE + 0x220 + (i * 0x04))
#define USB_DEV_DIEPINT(i) \
    (volatile void *)(USB_DEV_REG_BASE + 0x260 + (i * 0x04))
#define USB_DEV_DIEPTSIZ(i) \
    (volatile void *)(USB_DEV_REG_BASE + 0x2A0 + (i * 0x04))
#define USB_DEV_DOEPCTL0  (volatile void *)(USB_DEV_REG_BASE + 0x240)
#define USB_DEV_DOEPINT0  (volatile void *)(USB_DEV_REG_BASE + 0x280)
#define USB_DEV_DOEPTSIZ0 (volatile void *)(USB_DEV_REG_BASE + 0x2C0)
#define USB_DEV_DOEPCTL(i) \
    (volatile void *)(USB_DEV_REG_BASE + 0x240 + (i * 0x04))
#define USB_DEV_DOEPINT(i) \
    (volatile void *)(USB_DEV_REG_BASE + 0x280 + (i * 0x04))
#define USB_DEV_DOEPTSIZ(i) \
    (volatile void *)(USB_DEV_REG_BASE + 0x2C0 + (i * 0x04))
#define USB_DEV_DFIFO0 (volatile void *)(USB_DEV_REG_BASE + 0x1000)
#define USB_DEV_DFIFO1 (volatile void *)(USB_DEV_REG_BASE + 0x2000)

#define USB_DEV_EP_CNT      3 /* Only activated 3 EP */
#define USB_DEV_SPEED_HIGH  0x00
#define USB_DEV_SPEED_FULL2 0x01
#define USB_DEV_SPEED_LOW   0x10
#define USB_DEV_SPEED_FULL1 0x11

/* GINTMSK bits (Interrupt enable bits) */
#define USB_DEV_GINTMSK_MMIS         (0x1U << 1)
#define USB_DEV_GINTMSK_DEVINT       (0x1U << 2)
#define USB_DEV_GINTMSK_SOF          (0x1U << 3)
#define USB_DEV_GINTMSK_RXFLVL       (0x1U << 4)
#define USB_DEV_GINTMSK_NPTXFE       (0x1U << 5)
#define USB_DEV_GINTMSK_GINAKEFF     (0x1U << 6)
#define USB_DEV_GINTMSK_GONAKEFF     (0x1U << 7)
#define USB_DEV_GINTMSK_ESUSP        (0x1U << 10)
#define USB_DEV_GINTMSK_USBSUSP      (0x1U << 11)
#define USB_DEV_GINTMSK_USBRST       (0x1U << 12)
#define USB_DEV_GINTMSK_ENUMDNE      (0x1U << 13)
#define USB_DEV_GINTMSK_ISOODRP      (0x1U << 14)
#define USB_DEV_GINTMSK_EOPF         (0x1U << 15)
#define USB_DEV_GINTMSK_EPMIS        (0x1U << 17)
#define USB_DEV_GINTMSK_IEPINT       (0x1U << 18)
#define USB_DEV_GINTMSK_OEPINT       (0x1U << 19)
#define USB_DEV_GINTMSK_INCOMPISOIN  (0x1U << 20)
#define USB_DEV_GINTMSK_INCOMPISOOUT (0x1U << 21)
#define USB_DEV_GINTMSK_FSUSP        (0x1U << 22)
#define USB_DEV_GINTMSK_HPRTINT      (0x1U << 24)
#define USB_DEV_GINTMSK_HCINT        (0x1U << 25)
#define USB_DEV_GINTMSK_PTXFE        (0x1U << 26)
#define USB_DEV_GINTMSK_CIDSCHG      (0x1U << 28)
#define USB_DEV_GINTMSK_DISCINT      (0x1U << 29)
#define USB_DEV_GINTMSK_SRQI         (0x1U << 30)
#define USB_DEV_GINTMSK_WUI          (0x1U << 31)

/* GINTSTS */
#define USB_DEV_GINTSTS_CMOD         (0x1U << 0)
#define USB_DEV_GINTSTS_MMIS         (0x1U << 1)
#define USB_DEV_GINTSTS_DEVINT       (0x1U << 2)
#define USB_DEV_GINTSTS_SOF          (0x1U << 3)
#define USB_DEV_GINTSTS_RXFLVL       (0x1U << 4)
#define USB_DEV_GINTSTS_NPTXFE       (0x1U << 5)
#define USB_DEV_GINTSTS_GINAKEFF     (0x1U << 6)
#define USB_DEV_GINTSTS_GONAKEFF     (0x1U << 7)
#define USB_DEV_GINTSTS_ESUSP        (0x1U << 10)
#define USB_DEV_GINTSTS_USBSUSP      (0x1U << 11)
#define USB_DEV_GINTSTS_USBRST       (0x1U << 12)
#define USB_DEV_GINTSTS_ENUMDNE      (0x1U << 13)
#define USB_DEV_GINTSTS_ISOODRP      (0x1U << 14)
#define USB_DEV_GINTSTS_EOPF         (0x1U << 15)
#define USB_DEV_GINTSTS_EPMIS        (0x1U << 17)
#define USB_DEV_GINTSTS_IEPINT       (0x1U << 18)
#define USB_DEV_GINTSTS_OEPINT       (0x1U << 19)
#define USB_DEV_GINTSTS_INCOMPISOIN  (0x1U << 20)
#define USB_DEV_GINTSTS_INCOMPISOOUT (0x1U << 21)
#define USB_DEV_GINTSTS_FSUSP        (0x1U << 22)
#define USB_DEV_GINTSTS_HPRTINT      (0x1U << 24)
#define USB_DEV_GINTSTS_HCINT        (0x1U << 25)
#define USB_DEV_GINTSTS_PTXFE        (0x1U << 26)
#define USB_DEV_GINTSTS_CIDSCHG      (0x1U << 28)
#define USB_DEV_GINTSTS_DISCINT      (0x1U << 29)
#define USB_DEV_GINTSTS_SRQI         (0x1U << 30)
#define USB_DEV_GINTSTS_WUI          (0x1U << 31)

/*
 * DAINTMSK
 * All interrupt enable bits
 */
#define USB_DEV_DAINTMSK_IEP (0xFFFFU << 0)
#define USB_DEV_DAINTMSK_OEP (0xFFFFU << 16)

/*
 * DOEPMSK
 * Device Out EP Common interrupt enable bits
 */
#define USB_DEV_DOEPMSK_XFRC    (0x1U << 0)
#define USB_DEV_DOEPMSK_EPD     (0x1U << 1)
#define USB_DEV_DOEPMSK_AHBERR  (0x1U << 2)
#define USB_DEV_DOEPMSK_SETUP   (0x1U << 3)
#define USB_DEV_DOEPMSK_OTEPD   (0x1U << 4)
#define USB_DEV_DOEPMSK_OTEPSPR (0x1U << 5)
#define USB_DEV_DOEPMSK_B2BSTUP (0x1U << 6)
#define USB_DEV_DOEPMSK_OPE     (0x1U << 8)
#define USB_DEV_DOEPMSK_BOI     (0x1U << 9)
#define USB_DEV_DOEPMSK_BERR    (0x1U << 12)
#define USB_DEV_DOEPMSK_NAK     (0x1U << 13)
#define USB_DEV_DOEPMSK_NYET    (0x1U << 14)

/*
 * DIEPMSK
 * Device IN EP Common interrupt enable bits
 */
#define USB_DEV_DIEPMSK_XFRC       (0x1U << 0)
#define USB_DEV_DIEPMSK_EPD        (0x1U << 1)
#define USB_DEV_DIEPMSK_TMO        (0x1U << 3)
#define USB_DEV_DIEPMSK_ITTXFE     (0x1U << 4)
#define USB_DEV_DIEPMSK_ITEPMIS    (0x1U << 5)
#define USB_DEV_DIEPMSK_INEPNAKEFF (0x1U << 6)
#define USB_DEV_DIEPMSK_TXFUR      (0x1U << 8)
#define USB_DEV_DIEPMSK_BI         (0x1U << 9)

/*
 * GRXSTSP: Global Deivce Mode Receive Status Read and Pop Register
 * Information about received packets.
 */
#define USB_DEV_GRXSTSP_EPNUM  (0xF << 0)
#define USB_DEV_GRXSTSP_BCNT   (0x7FF << 4)
#define USB_DEV_GRXSTSP_DPID   (0x3 << 15)
#define USB_DEV_GRXSTSP_PKTSTS (0xF << 17)
#define USB_DEV_GRXSTSP_FN     (0xF << 21)

#define GRXSTSP_PID_DATA0                (0x0)
#define GRXSTSP_PID_DATA1                (0x2)
#define GRXSTSP_PID_DATA2                (0x1)
#define GRXSTSP_PID_MDATA                (0x3)
#define GRXSTSP_PKTSTS_OUT_NAK           (0x1)
#define GRXSTSP_PKTSTS_OUTDATA_RECV      (0x2)
#define GRXSTSP_PKTSTS_OUTXFER_COMPLETED (0x3)
#define GRXSTSP_PKTSTS_SETUP_COMPLETED   (0x4)
#define GRXSTSP_PKTSTS_SETUP_RECV        (0x6)

/*
 * DOEPINT
 */
#define USB_DEV_DOEPINT_XFRC      (0x1 << 0)
#define USB_DEV_DOEPINT_EPDISD    (0x1 << 1)
#define USB_DEV_DOEPINT_AHBERR    (0x1 << 2)
#define USB_DEV_DOEPINT_STUP      (0x1 << 3)
#define USB_DEV_DOEPINT_OTEPDIS   (0x1 << 4)
#define USB_DEV_DOEPINT_OTEPSPR   (0x1 << 5)
#define USB_DEV_DOEPINT_B2BSTUP   (0x1 << 6)
#define USB_DEV_DOEPINT_OUTPKTERR (0x1 << 8)
#define USB_DEV_DOEPINT_NAK       (0x1 << 13)
#define USB_DEV_DOEPINT_NYET      (0x1 << 14)
#define USB_DEV_DOEPINT_STPKTRX   (0x1 << 15)

/*
 * DIEPINT
 */
#define USB_DEV_DIEPINT_XFRC       (0x1 << 0)
#define USB_DEV_DIEPINT_EPDISD     (0x1 << 1)
#define USB_DEV_DIEPINT_AHBERR     (0x1 << 2)
#define USB_DEV_DIEPINT_TOC        (0x1 << 3)
#define USB_DEV_DIEPINT_ITTXFE     (0x1 << 4)
#define USB_DEV_DIEPINT_INEPNM     (0x1 << 5)
#define USB_DEV_DIEPINT_INEPNE     (0x1 << 6)
#define USB_DEV_DIEPINT_TXFE       (0x1 << 7)
#define USB_DEV_DIEPINT_TXFIFOUDRN (0x1 << 8)
#define USB_DEV_DIEPINT_BNA        (0x1 << 9)
#define USB_DEV_DIEPINT_PKTDRPSTS  (0x1 << 11)
#define USB_DEV_DIEPINT_BERR       (0x1 << 12)
#define USB_DEV_DIEPINT_NAK        (0x1 << 13)

/*
 * DIEPCTL
 */
#define USB_DEV_DIEPCTL0_MPSIZ         (0x3 << 0)
#define USB_DEV_DIEPCTL_MPSIZ          (0x7FF << 0)
#define USB_DEV_DIEPCTL_NEXTEP         (0xF << 11)
#define USB_DEV_DIEPCTL_USBAEP         (0x1 << 15)
#define USB_DEV_DIEPCTL_EONUM_DPID     (0x1 << 16)
#define USB_DEV_DIEPCTL_NAKSTS         (0x1 << 17)
#define USB_DEV_DIEPCTL_EPTYP          (0x3 << 18)
#define USB_DEV_DIEPCTL_STALL          (0x1 << 21)
#define USB_DEV_DIEPCTL_TXFNUM         (0xF << 22)
#define USB_DEV_DIEPCTL_CNAK           (0x1 << 26)
#define USB_DEV_DIEPCTL_SNAK           (0x1 << 27)
#define USB_DEV_DIEPCTL_SD0PID_SEVNFRM (0x1 << 28)
#define USB_DEV_DIEPCTL_SODDFRM        (0x1 << 29)
#define USB_DEV_DIEPCTL_EPDIS          (0x1 << 30)
#define USB_DEV_DIEPCTL_EPENA          (0x1U << 31)

#define DIEPCTL0_MPS_64B (0x00)
#define DIEPCTL0_MPS_34B (0x01)
#define DIEPCTL0_MPS_16B (0x10)
#define DIEPCTL0_MPS_8B  (0x11)

/*
 * DOEPCTL
 */
#define USB_DEV_DOEPCTL_MPSIZ          (0x7FF << 0)
#define USB_DEV_DOEPCTL_NEXTEP         (0xF << 11)
#define USB_DEV_DOEPCTL_USBAEP         (0x1 << 15)
#define USB_DEV_DOEPCTL_NAKSTS         (0x1 << 17)
#define USB_DEV_DOEPCTL_EPTYP          (0x3 << 18)
#define USB_DEV_DOEPCTL_SNPM           (0x1 << 20)
#define USB_DEV_DOEPCTL_STALL          (0x1 << 21)
#define USB_DEV_DOEPCTL_CNAK           (0x1 << 26)
#define USB_DEV_DOEPCTL_SNAK           (0x1 << 27)
#define USB_DEV_DOEPCTL_SD0PID_SEVNFRM (0x1 << 28)
#define USB_DEV_DOEPCTL_SODDFRM        (0x1 << 29)
#define USB_DEV_DOEPCTL_EPDIS          (0x1 << 30)
#define USB_DEV_DOEPCTL_EPENA          (0x1U << 31)

/*
 * GNPTXSTS
 */
#define USB_DEV_GNPTXSTS_FIFO_SPC_AVAIL  (0xFFFF << 0)
#define USB_DEV_GNPTXSTS_QUEUE_SPC_AVAIL (0xFF << 16)

/*
 * USB_DEV_DCFG
 */
#define USB_DEV_DCFG_DSPD      (0x3 << 0)
#define USB_DEV_DCFG_NZLSOHSK  (0x1 << 2)
#define USB_DEV_DCFG_DAD       (0x7F << 4)
#define USB_DEV_DCFG_PFIVL     (0x3 << 11)
#define USB_DEV_DCFG_PERSCHIVL (0x3 << 24)

/*
 * USB_DEV_DSTS
 */
#define USB_DEV_DSTS_SUSPSTS (0x1 << 0)
#define USB_DEV_DSTS_ENUMSPD (0x3 << 1)
#define USB_DEV_DSTS_EERR    (0x1 << 3)
#define USB_DEV_DSTS_FNSOF   (0x3FFF << 8)

/*
 * USB_DEV_DCTL
 */
#define USB_DEV_DCTL_RWUSIG   (0x1 << 0)
#define USB_DEV_DCTL_SDIS     (0x1 << 1)
#define USB_DEV_DCTL_GINSTS   (0x1 << 2)
#define USB_DEV_DCTL_GONSTS   (0x1 << 3)
#define USB_DEV_DCTL_TCTL     (0x7 << 4)
#define USB_DEV_DCTL_SGINAK   (0x1 << 7)
#define USB_DEV_DCTL_CGINAK   (0x1 << 8)
#define USB_DEV_DCTL_SGONAK   (0x1 << 9)
#define USB_DEV_DCTL_CGONAK   (0x1 << 10)
#define USB_DEV_DCTL_POPRGDNE (0x1 << 11)

/*
 * USB_PHY_IF
 */
#define USB_PHY_ULPI_UTMI_SEL (0x1 << 4U)

/*
 * GRSTCTL
 */

#define USB_DEV_GRSTCTL_HSRST (0x1 << 1U)
#define USB_DEV_GRSTCTL_FCRST (0x1 << 2U)

/*
 * USB_AHB_BASIC
 */

#define USB_AHB_BASIC_DMAREQ (0x1 << 1U)
#define USB_AHB_BASIC_AHBIDL (0x1 << 2U)

/*
 * USB_DEV_INIT
 */

#define USB_DEV_INIT_CSRST   (0x1 << 0U)
#define USB_DEV_INIT_RXFFLSH (0x1 << 1U)
#define USB_DEV_INIT_TXFFLSH (0x1 << 2U)
#define USB_DEV_INIT_TXFNUM  (0x1F << 3U)

#ifdef __cplusplus
}
#endif

#endif
