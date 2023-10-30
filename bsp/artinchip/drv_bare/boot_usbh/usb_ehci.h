/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef _AIC_USB_EHCI_H_
#define _AIC_USB_EHCI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>

/* Host Controller Capability Register Bit Definitions **********************/

/* Paragraph 2.2 */

/* Core Capability Register Length. Paragraph 2.2.1. 8-bit length. */

/* Core Interface Version Number. Paragraph 2.2.2.  Two byte BCD encoding */

/* Core Structural Parameters. Paragraph 2.2.3 */

#define EHCI_HCSPARAMS_NPORTS_SHIFT    (0)        /* Bit 0-3: Number of physical downstream ports */
#define EHCI_HCSPARAMS_NPORTS_MASK     (15 << EHCI_HCSPARAMS_NPORTS_SHIFT)
#define EHCI_HCSPARAMS_PPC             (1 << 4)   /* Bit 4: Port Power Control */
                                                  /* Bits 5-6: Reserved */
#define EHCI_HCSPARAMS_PRR             (1 << 7)   /* Bit 7: Port Routing Rules */
#define EHCI_HCSPARAMS_NPCC_SHIFT      (8)        /* Bit 8-11: Number of Ports per Companion Controller */
#define EHCI_HCSPARAMS_NPCC_MASK       (15 << EHCI_HCSPARAMS_NPCC_SHIFT)
#define EHCI_HCSPARAMS_NCC_SHIFT       (12)       /* Bit 12-15: Number of Companion Controllers */
#define EHCI_HCSPARAMS_NCC_MASK        (15 << EHCI_HCSPARAMS_NCC_SHIFT)
#define EHCI_HCSPARAMS_PIND            (1 << 16)  /* Bit 16: Port Indicators */
                                                  /* Bits 17-19: Reserved */
#define EHCI_HCSPARAMS_DBGPORT_SHIFT   (20)       /* Bit 20-23: Debug Port Number */
#define EHCI_HCSPARAMS_DBGPORT_MASK    (15 << EHCI_HCSPARAMS_DBGPORT_SHIFT)
                                                  /* Bits 24-31: Reserved */

/* Core Capability Parameters. Paragraph 2.2.4 */

#define EHCI_HCCPARAMS_64BIT           (1 << 0)   /* Bit 0: 64-bit Addressing Capability */
#define EHCI_HCCPARAMS_PFLF            (1 << 1)   /* Bit 1: Programmable Frame List Flag */
#define EHCI_HCCPARAMS_ASPC            (1 << 2)   /* Bit 2: Asynchronous Schedule Park Capability */
                                                  /* Bit 3: Reserved */
#define EHCI_HCCPARAMS_IST_SHIFT       (4)        /* Bits 4-7: Isochronous Scheduling Threshold */
#define EHCI_HCCPARAMS_IST_MASK        (15 << EHCI_HCCPARAMS_IST_SHIFT)
#define EHCI_HCCPARAMS_EECP_SHIFT      (8)        /* Bits 8-15: EHCI Extended Capabilities Pointer */
#define EHCI_HCCPARAMS_EECP_MASK       (0xff << EHCI_HCCPARAMS_EECP_SHIFT)
                                                  /* Bits 16-31: Reserved */

/* Core Companion Port Route Description.
 * Paragraph 2.2.5. 15 x 4-bit array (60 bits)
 */

/* Host Controller Operational Register Bit Definitions *********************/

/* Paragraph 2.3 */

/* USB Command. Paragraph 2.3.1 */

#define EHCI_USBCMD_RUN                (1 << 0)   /* Bit 0: Run/Stop */
#define EHCI_USBCMD_HCRESET            (1 << 1)   /* Bit 1: Host Controller Reset */
#define EHCI_USBCMD_FLSIZE_SHIFT       (2)        /* Bits 2-3: Frame List Size */
#define EHCI_USBCMD_FLSIZE_MASK        (3 << EHCI_USBCMD_FLSIZE_SHIFT)
#define EHCI_USBCMD_FLSIZE_1024        (0 << EHCI_USBCMD_FLSIZE_SHIFT) /* 1024 elements (4096 bytes) */
#define EHCI_USBCMD_FLSIZE_512         (1 << EHCI_USBCMD_FLSIZE_SHIFT) /* 512 elements (2048 bytes) */
#define EHCI_USBCMD_FLSIZE_256         (2 << EHCI_USBCMD_FLSIZE_SHIFT) /* 256 elements (1024 bytes) */

#define EHCI_USBCMD_PSEN               (1 << 4)   /* Bit 4: Periodic Schedule Enable */
#define EHCI_USBCMD_ASEN               (1 << 5)   /* Bit 5: Asynchronous Schedule Enable */
#define EHCI_USBCMD_IAADB              (1 << 6)   /* Bit 6: Interrupt on Async Advance Doorbell */
#define EHCI_USBCMD_LRESET             (1 << 7)   /* Bit 7: Light Host Controller Reset */
#define EHCI_USBCMD_PARKCNT_SHIFT      (8)        /* Bits 8-9: Asynchronous Schedule Park Mode Count */
#define EHCI_USBCMD_PARKCNT_MASK       (3 << EHCI_USBCMD_PARKCNT_SHIFT)
                                                  /* Bit 10: Reserved */
#define EHCI_USBCMD_PARK               (1 << 11)  /* Bit 11: Asynchronous Schedule Park Mode Enable */
                                                  /* Bits 12-15: Reserved */
#define EHCI_USBCMD_ITHRE_SHIFT        (16)       /* Bits 16-23: Interrupt Threshold Control */
#define EHCI_USBCMD_ITHRE_MASK         (0xff << EHCI_USBCMD_ITHRE_SHIFT)
#define EHCI_USBCMD_ITHRE_1MF          (0x01 << EHCI_USBCMD_ITHRE_SHIFT) /* 1 micro-frame */
#define EHCI_USBCMD_ITHRE_2MF          (0x02 << EHCI_USBCMD_ITHRE_SHIFT) /* 2 micro-frames */
#define EHCI_USBCMD_ITHRE_4MF          (0x04 << EHCI_USBCMD_ITHRE_SHIFT) /* 4 micro-frames */
#define EHCI_USBCMD_ITHRE_8MF          (0x08 << EHCI_USBCMD_ITHRE_SHIFT) /* 8 micro-frames (default, 1 ms) */
#define EHCI_USBCMD_ITHRE_16MF         (0x10 << EHCI_USBCMD_ITHRE_SHIFT) /* 16 micro-frames (2 ms) */
#define EHCI_USBCMD_ITHRE_32MF         (0x20 << EHCI_USBCMD_ITHRE_SHIFT) /* 32 micro-frames (4 ms) */
#define EHCI_USBCMD_ITHRE_64MF         (0x40 << EHCI_USBCMD_ITHRE_SHIFT) /* 64 micro-frames (8 ms) */

                                                  /* Bits 24-31: Reserved */

/* USB Status. Paragraph 2.3.2 */

/* USB Interrupt Enable. Paragraph 2.3.3 */

#define EHCI_INT_USBINT                (1 << 0)   /* Bit 0:  USB Interrupt */
#define EHCI_INT_USBERRINT             (1 << 1)   /* Bit 1:  USB Error Interrupt */
#define EHCI_INT_PORTSC                (1 << 2)   /* Bit 2:  Port Change Detect */
#define EHCI_INT_FLROLL                (1 << 3)   /* Bit 3:  Frame List Rollover */
#define EHCI_INT_SYSERROR              (1 << 4)   /* Bit 4:  Host System Error */
#define EHCI_INT_AAINT                 (1 << 5)   /* Bit 5:  Interrupt on Async Advance */
#define EHCI_INT_ALLINTS               (0x3f)     /* Bits 0-5:  All interrupts */
                                                  /* Bits 6-11: Reserved */
#define EHCI_USBSTS_HALTED             (1 << 12)  /* Bit 12: HC Halted */
#define EHCI_USBSTS_RECLAM             (1 << 13)  /* Bit 13: Reclamation */
#define EHCI_USBSTS_PSS                (1 << 14)  /* Bit 14: Periodic Schedule Status */
#define EHCI_USBSTS_ASS                (1 << 15)  /* Bit 15: Asynchronous Schedule Status */
                                                  /* Bits 16-31: Reserved */

/* USB Frame Index. Paragraph 2.3.4 */

#define EHCI_FRINDEX_MASK              (0x1fff)   /* Bits 0-13: Frame index */
                                                  /* Bits 14-31: Reserved */

/* 4G Segment Selector.
 * Paragraph 2.3.5,  Bits[64:32] of data structure addresses
 */

/* Frame List Base Address. Paragraph 2.3.6 */

                                                    /* Bits 0-11: Reserved */
#define EHCI_PERIODICLISTBASE_MASK     (0xfffff000) /* Bits 12-31: Base Address (Low) */

/* Next Asynchronous List Address. Paragraph 2.3.7 */

                                                    /* Bits 0-4: Reserved */
#define EHCI_ASYNCLISTADDR_MASK        (0xffffffe0) /* Bits 5-31: Link Pointer Low (LPL) */

/* Configured Flag Register. Paragraph 2.3.8 */

#define EHCI_CONFIGFLAG                (1 << 0)   /* Bit 0: Configure Flag */
                                                /* Bits 1-31: Reserved */

/* Port Status/Control, Port 1-n. Paragraph 2.3.9 */

#define EHCI_PORTSC_CCS                (1 << 0)   /* Bit 0: Current Connect Status */
#define EHCI_PORTSC_CSC                (1 << 1)   /* Bit 1: Connect Status Change */
#define EHCI_PORTSC_PE                 (1 << 2)   /* Bit 2: Port Enable */
#define EHCI_PORTSC_PEC                (1 << 3)   /* Bit 3: Port Enable/Disable Change */
#define EHCI_PORTSC_OCA                (1 << 4)   /* Bit 4: Over-current Active */
#define EHCI_PORTSC_OCC                (1 << 5)   /* Bit 5: Over-current Change */
#define EHCI_PORTSC_RESUME             (1 << 6)   /* Bit 6: Force Port Resume */
#define EHCI_PORTSC_SUSPEND            (1 << 7)   /* Bit 7: Suspend */
#define EHCI_PORTSC_RESET              (1 << 8)   /* Bit 8: Port Reset */
                                                  /* Bit 9: Reserved */
#define EHCI_PORTSC_LSTATUS_SHIFT      (10)       /* Bits 10-11: Line Status */
#define EHCI_PORTSC_LSTATUS_MASK       (3 << EHCI_PORTSC_LSTATUS_SHIFT)
#define EHCI_PORTSC_LSTATUS_SE0        (0 << EHCI_PORTSC_LSTATUS_SHIFT) /* SE0 Not Low-speed device, perform EHCI reset */
#define EHCI_PORTSC_LSTATUS_KSTATE     (1 << EHCI_PORTSC_LSTATUS_SHIFT) /* K-state Low-speed device, release ownership of port */
#define EHCI_PORTSC_LSTATUS_JSTATE     (2 << EHCI_PORTSC_LSTATUS_SHIFT) /* J-state Not Low-speed device, perform EHCI reset */

#define EHCI_PORTSC_PP                 (1 << 12)  /* Bit 12: Port Power */
#define EHCI_PORTSC_OWNER              (1 << 13)  /* Bit 13: Port Owner */
#define EHCI_PORTSC_PIC_SHIFT          (14)       /* Bits 14-15: Port Indicator Control */
#define EHCI_PORTSC_PIC_MASK           (3 << EHCI_PORTSC_PIC_SHIFT)
#define EHCI_PORTSC_PIC_OFF            (0 << EHCI_PORTSC_PIC_SHIFT) /* Port indicators are off */
#define EHCI_PORTSC_PIC_AMBER          (1 << EHCI_PORTSC_PIC_SHIFT) /* Amber */
#define EHCI_PORTSC_PIC_GREEN          (2 << EHCI_PORTSC_PIC_SHIFT) /* Green */

#define EHCI_PORTSC_PTC_SHIFT          (16)       /* Bits 16-19: Port Test Control */
#define EHCI_PORTSC_PTC_MASK           (15 << EHCI_PORTSC_PTC_SHIFT)
#define EHCI_PORTSC_PTC_DISABLED       (0 << EHCI_PORTSC_PTC_SHIFT) /* Test mode not enabled */
#define EHCI_PORTSC_PTC_JSTATE         (1 << EHCI_PORTSC_PTC_SHIFT) /* Test J_STATE */
#define EHCI_PORTSC_PTC_KSTATE         (2 << EHCI_PORTSC_PTC_SHIFT) /* Test K_STATE */
#define EHCI_PORTSC_PTC_SE0NAK         (3 << EHCI_PORTSC_PTC_SHIFT) /* Test SE0_NAK */
#define EHCI_PORTSC_PTC_PACKET         (4 << EHCI_PORTSC_PTC_SHIFT) /* Test Packet */
#define EHCI_PORTSC_PTC_ENABLE         (5 << EHCI_PORTSC_PTC_SHIFT) /* Test FORCE_ENABLE */

#define EHCI_PORTSC_WKCCNTE            (1 << 20)  /* Bit 20: Wake on Connect Enable */
#define EHCI_PORTSC_WKDSCNNTE          (1 << 21)  /* Bit 21: Wake on Disconnect Enable */
#define EHCI_PORTSC_WKOCE              (1 << 22)  /* Bit 22: Wake on Over-current Enable */
                                                  /* Bits 23-31: Reserved */

#define EHCI_PORTSC_ALLINTS            (EHCI_PORTSC_CSC | EHCI_PORTSC_PEC | \
                                        EHCI_PORTSC_OCC | EHCI_PORTSC_RESUME)

/* Queue Element Transfer Descriptor (qTD).
 * Paragraph 3.5
 */

/* Next qTD Pointer.
 * Paragraph 3.5.1
 */

#define QTD_NQP_T                      (1 << 0)   /* Bit 0: Terminate */
                                                  /* Bits 1-4: Reserved */
#define QTD_NQP_NTEP_SHIFT             (5)        /* Bits 5-31: Next Transfer Element Pointer */
#define QTD_NQP_NTEP_MASK              (0xffffffe0)

/* Alternate Next qTD Pointer.
 * Paragraph 3.5.2
 */

#define QTD_AQP_T                      (1 << 0)   /* Bit 0: Terminate */
                                                  /* Bits 1-4: Reserved */
#define QTD_AQP_NTEP_SHIFT             (5)        /* Bits 5-31: Next Transfer Element Pointer */
#define QTD_AQP_NTEP_MASK              (0xffffffe0)

/* qTD Token.
 * Paragraph 3.5.3
 */

#define QTD_TOKEN_STATUS_SHIFT         (0)        /* Bits 0-7: Status */
#define QTD_TOKEN_STATUS_MASK          (0xff << QTD_TOKEN_STATUS_SHIFT)
#define QTD_TOKEN_P                    (1 << 0)   /* Bit 0 Ping State  */
#define QTD_TOKEN_ERR                  (1 << 0)   /* Bit 0 Error */
#define QTD_TOKEN_SPLITXSTATE          (1 << 1)   /* Bit 1 Split Transaction State */
#define QTD_TOKEN_MMF                  (1 << 2)   /* Bit 2 Missed Micro-Frame */
#define QTD_TOKEN_XACTERR              (1 << 3)   /* Bit 3 Transaction Error */
#define QTD_TOKEN_BABBLE               (1 << 4)   /* Bit 4 Babble Detected */
#define QTD_TOKEN_DBERR                (1 << 5)   /* Bit 5 Data Buffer Error */
#define QTD_TOKEN_HALTED               (1 << 6)   /* Bit 6 Halted */
#define QTD_TOKEN_ACTIVE               (1 << 7)   /* Bit 7 Active */
#define QTD_TOKEN_ERRORS               (0x78 << QTD_TOKEN_STATUS_SHIFT)
#define QTD_TOKEN_PID_SHIFT            (8)        /* Bits 8-9: PID Code */
#define QTD_TOKEN_PID_MASK             (3 << QTD_TOKEN_PID_SHIFT)
#define QTD_TOKEN_PID_OUT              (0 << QTD_TOKEN_PID_SHIFT) /* OUT Token generates token (E1H) */
#define QTD_TOKEN_PID_IN               (1 << QTD_TOKEN_PID_SHIFT) /* IN Token generates token (69H) */
#define QTD_TOKEN_PID_SETUP            (2 << QTD_TOKEN_PID_SHIFT) /* SETUP Token generates token (2DH) */

#define QTD_TOKEN_CERR_SHIFT           (10)       /* Bits 10-11: Error Counter */
#define QTD_TOKEN_CERR_MASK            (3 << QTD_TOKEN_CERR_SHIFT)
#define QTD_TOKEN_CPAGE_SHIFT          (12)       /* Bits 12-14: Current Page */
#define QTD_TOKEN_CPAGE_MASK           (7 << QTD_TOKEN_CPAGE_SHIFT)
#define QTD_TOKEN_IOC                  (1 << 15)  /* Bit 15: Interrupt On Complete */
#define QTD_TOKEN_NBYTES_SHIFT         (16)       /* Bits 16-30: Total Bytes to Transfer */
#define QTD_TOKEN_NBYTES_MASK          (0x7fff << QTD_TOKEN_NBYTES_SHIFT)
#define QTD_TOKEN_TOGGLE_SHIFT         (31)       /* Bit 31: Data Toggle */
#define QTD_TOKEN_TOGGLE               (1U << 31)  /* Bit 31: Data Toggle */

/* qTD Buffer Page Pointer List.
 * Paragraph 3.5.4
 */

/* Page 0 */

#define QTD_BUFPTR0_OFFFSET_SHIFT      (0)        /* Bits 0-11: Current Offset */
#define QTD_BUFPTR0_OFFFSET_MASK       (0xfff << QTD_BUFPTR0_OFFFSET_SHIFT)

/* Other pages */

                                                  /* Bits 0-11: Reserved */

/* All pages */

#define QTD_BUFPTR_SHIFT               (12)       /* Bits 12-31: Buffer Pointer List */
#define QTD_BUFPTR_MASK                (0xfffff000)

/* Queue Head. Paragraph 3.6 */

/* Queue Head Horizontal Link Pointer.
 * Paragraph 3.6.1
 */

#define QH_HLP_T                       (1 << 0)   /* Bit 0: Terminate, QH HL pointer invalid */
#define QH_HLP_TYP_SHIFT               (1)        /* Bits 1-2: Type */
#define QH_HLP_TYP_MASK                (3 << QH_HLP_TYP_SHIFT)
#define QH_HLP_TYP_ITD                 (0 << QH_HLP_TYP_SHIFT) /* Isochronous Transfer Descriptor */
#define QH_HLP_TYP_QH                  (1 << QH_HLP_TYP_SHIFT) /* Queue Head */
#define QH_HLP_TYP_SITD                (2 << QH_HLP_TYP_SHIFT) /* Split Transaction Isochronous Transfer Descriptor */
#define QH_HLP_TYP_FSTN                (3 << QH_HLP_TYP_SHIFT) /* Frame Span Traversal Node */

                                                    /* Bits 3-4: Reserved */
#define QH_HLP_MASK                    (0xffffffe0) /* Bits 5-31: Queue Head Horizontal Link Pointer */

/* Endpoint Capabilities/Characteristics. Paragraph 3.6.2 */

/* Endpoint Characteristics: Queue Head DWord. Table 3-19 */

#define QH_EPCHAR_DEVADDR_SHIFT        (0)        /* Bitx 0-6: Device Address */
#define QH_EPCHAR_DEVADDR_MASK         (0x7f << QH_EPCHAR_DEVADDR_SHIFT)
#define QH_EPCHAR_I                    (1 << 7)   /* Bit 7: Inactivate on Next Transaction */
#define QH_EPCHAR_ENDPT_SHIFT          (8)        /* Bitx 8-11: Endpoint Number */
#define QH_EPCHAR_ENDPT_MASK           (15 << QH_EPCHAR_ENDPT_SHIFT)
#define QH_EPCHAR_EPS_SHIFT            (12)       /* Bitx 12-13: Endpoint Speed */
#define QH_EPCHAR_EPS_MASK             (3 << QH_EPCHAR_EPS_SHIFT)
#define QH_EPCHAR_EPS_FULL             (0 << QH_EPCHAR_EPS_SHIFT) /* Full-Speed (12Mbs) */
#define QH_EPCHAR_EPS_LOW              (1 << QH_EPCHAR_EPS_SHIFT) /* Low-Speed (1.5Mbs) */
#define QH_EPCHAR_EPS_HIGH             (2 << QH_EPCHAR_EPS_SHIFT) /* High-Speed (480 Mb/s) */

#define QH_EPCHAR_DTC                  (1 << 14)  /* Bit 14: Data Toggle Control */
#define QH_EPCHAR_H                    (1 << 15)  /* Bit 15: Head of Reclamation List Flag */
#define QH_EPCHAR_MAXPKT_SHIFT         (16)       /* Bitx 16-26: Maximum Packet Length */
#define QH_EPCHAR_MAXPKT_MASK          (0x7ff << QH_EPCHAR_MAXPKT_SHIFT)
#define QH_EPCHAR_C                    (1 << 27)  /* Bit 27: Control Endpoint Flag */
#define QH_EPCHAR_RL_SHIFT             (28)       /* Bitx 28-31: Nak Count Reload */
#define QH_EPCHAR_RL_MASK              (15 << QH_EPCHAR_RL_SHIFT)

/* Endpoint Capabilities: Queue Head DWord 2. Table 3-20 */

#define QH_EPCAPS_SSMASK_SHIFT         (0)        /* Bitx 0-7: Interrupt Schedule Mask (�Frame S-mask) */
#define QH_EPCAPS_SSMASK_MASK          (0xff << QH_EPCAPS_SSMASK_SHIFT)
#define QH_EPCAPS_SSMASK(n)            ((n) <<  QH_EPCAPS_SSMASK_SHIFT)
#define QH_EPCAPS_SCMASK_SHIFT         (8)        /* Bitx 8-15: Split Completion Mask (�Frame C-Mask) */
#define QH_EPCAPS_SCMASK_MASK          (0xff << QH_EPCAPS_SCMASK_SHIFT)
#define QH_EPCAPS_SCMASK(n)            ((n) << QH_EPCAPS_SCMASK_SHIFT)
#define QH_EPCAPS_HUBADDR_SHIFT        (16)       /* Bitx 16-22: Hub Address */
#define QH_EPCAPS_HUBADDR_MASK         (0x7f << QH_EPCAPS_HUBADDR_SHIFT)
#define QH_EPCAPS_HUBADDR(n)           ((n) << QH_EPCAPS_HUBADDR_SHIFT)
#define QH_EPCAPS_PORT_SHIFT           (23)       /* Bit 23-29: Port Number */
#define QH_EPCAPS_PORT_MASK            (0x7f << QH_EPCAPS_PORT_SHIFT)
#define QH_EPCAPS_PORT(n)              ((n) << QH_EPCAPS_PORT_SHIFT)
#define QH_EPCAPS_MULT_SHIFT           (30)       /* Bit 30-31: High-Bandwidth Pipe Multiplier */
#define QH_EPCAPS_MULT_MASK            (3 << QH_EPCAPS_MULT_SHIFT)
#define QH_EPCAPS_MULT(n)              ((n) << QH_EPCAPS_MULT_SHIFT)

/* Current qTD Link Pointer.  Table 3-21 */

#define QH_CQP_NTEP_SHIFT              (5)        /* Bits 5-31: Next Transfer Element Pointer */
#define QH_CQP_NTEP_MASK               (0xffffffe0)

/* Transfer Overlay.  Paragraph 3.6.3
 *
 * NOTES:
 * 1. Same as the field of the same name in struct ehci_qtd_s
 * 2. Similar to the field of the same name in struct ehci_qtd_s, but with
 *    some additional bitfields.
 */

/* Next qTD Pointer (NOTE 1) */

#define QH_NQP_T                       (1 << 0)   /* Bit 0: Terminate */
                                                  /* Bits 1-4: Reserved */
#define QH_NQP_NTEP_SHIFT              (5)        /* Bits 5-31: Next Transfer Element Pointer */
#define QH_NQP_NTEP_MASK               (0xffffffe0)

/* Alternate Next qTD Pointer.  Table 3.7 (NOTE 2) */

#define QH_AQP_T                       (1 << 0)   /* Bit 0: Terminate */
#define QH_AQP_NAKCNT                  (1)        /* Bits 1-4: Nak Counter */
#define QH_AQP_NTEP_SHIFT              (5)        /* Bits 5-31: Next Transfer Element Pointer */
#define QH_AQP_NTEP_MASK               (0xffffffe0)

/* qTD Token (NOTE 1) */

#define QH_TOKEN_STATUS_SHIFT          (0)        /* Bits 0-7: Status */
#define QH_TOKEN_STATUS_MASK           (0xff << QH_TOKEN_STATUS_SHIFT)
#define QH_TOKEN_P                     (1 << 0)   /* Bit 0 Ping State  */
#define QH_TOKEN_ERR                   (1 << 0)   /* Bit 0 Error */
#define QH_TOKEN_SPLITXSTATE           (1 << 1)   /* Bit 1 Split Transaction State */
#define QH_TOKEN_MMF                   (1 << 2)   /* Bit 2 Missed Micro-Frame */
#define QH_TOKEN_XACTERR               (1 << 3)   /* Bit 3 Transaction Error */
#define QH_TOKEN_BABBLE                (1 << 4)   /* Bit 4 Babble Detected */
#define QH_TOKEN_DBERR                 (1 << 5)   /* Bit 5 Data Buffer Error */
#define QH_TOKEN_HALTED                (1 << 6)   /* Bit 6 Halted */
#define QH_TOKEN_ACTIVE                (1 << 7)   /* Bit 7 Active */
#define QH_TOKEN_ERRORS                (0x78 << QH_TOKEN_STATUS_SHIFT)
#define QH_TOKEN_PID_SHIFT             (8)        /* Bits 8-9: PID Code */
#define QH_TOKEN_PID_MASK              (3 << QH_TOKEN_PID_SHIFT)
#define QH_TOKEN_PID_OUT               (0 << QH_TOKEN_PID_SHIFT) /* OUT Token generates token (E1H) */
#define QH_TOKEN_PID_IN                (1 << QH_TOKEN_PID_SHIFT) /* IN Token generates token (69H) */
#define QH_TOKEN_PID_SETUP             (2 << QH_TOKEN_PID_SHIFT) /* SETUP Token generates token (2DH) */

#define QH_TOKEN_CERR_SHIFT            (10)       /* Bits 10-11: Error Counter */
#define QH_TOKEN_CERR_MASK             (3 << QH_TOKEN_CERR_SHIFT)
#define QH_TOKEN_CPAGE_SHIFT           (12)       /* Bits 12-14: Current Page */
#define QH_TOKEN_CPAGE_MASK            (7 << QH_TOKEN_CPAGE_SHIFT)
#define QH_TOKEN_IOC                   (1 << 15)  /* Bit 15: Interrupt On Complete */
#define QH_TOKEN_NBYTES_SHIFT          (16)       /* Bits 16-30: Total Bytes to Transfer */
#define QH_TOKEN_NBYTES_MASK           (0x7fff << QH_TOKEN_NBYTES_SHIFT)
#define QH_TOKEN_TOGGLE_SHIFT          (31)       /* Bit 31: Data Toggle */
#define QH_TOKEN_TOGGLE                (1 << 31)  /* Bit 31: Data Toggle */

/* Buffer Page Pointer List (NOTE 2) */

/* Page 0 */

#define QH_BUFPTR0_OFFFSET_SHIFT      (0)        /* Bits 0-11: Current Offset */
#define QH_BUFPTR0_OFFFSET_MASK       (0xfff << QH_BUFPTR0_OFFFSET_SHIFT)

/* Page 1. Table 3.22 */

#define QH_BUFPTR1_CPROGMASK_SHIFT    (0)        /* Bits 0-7: Split-transaction Complete-split Progress */
#define QH_BUFPTR1_CPROGMASK_MASK     (0xff << QH_BUFPTR1_CPROGMASK_SHIFT)
                                                  /* Bits 8-11: Reserved */

/* Page 2. Table 3.22 */

#define QH_BUFPTR2_FRAMETAG_SHIFT     (0)        /* Bits 0-4: Split-transaction Frame Tag */
#define QH_BUFPTR2_FRAMETAG_MASK      (31 << QH_BUFPTR2_FRAMETAG_SHIFT)
#define QH_BUFPTR2_SBYTES_SHIFT       (5)        /* Bits 5-11: S-bytes */
#define QH_BUFPTR2_SBYTES_MASK        (0x7f << QH_BUFPTR2_SBYTES_SHIFT)

/* Other pages */

                                                  /* Bits 0-11: Reserved */

/* All pages */

#define QH_BUFPTR_SHIFT               (12)       /* Bits 12-31: Buffer Pointer List */
#define QH_BUFPTR_MASK                (0xfffff000)

/* Registers ****************************************************************/

/* Since the operational registers are not known a compile time, representing
 * register blocks with structures is more convenient than using individual
 * register offsets.
 */

/* Host Controller Capability Registers.
 * This register block must be positioned at a well known address.
 */

struct ehci_hccr_s {
    u8 caplength;     /* 0x00: Capability Register Length */
    u8 reserved;
    u16 hciversion;   /* 0x02: Interface Version Number */
    u32 hcsparams;    /* 0x04: Structural Parameters */
    u32 hccparams;    /* 0x08: Capability Parameters */
    u8 hcspportrt[8]; /* 0x0c: Companion Port Route Description */
};

/* Host Controller Operational Registers.
 * This register block is positioned at an offset of 'caplength' from the
 * beginning of the Host Controller Capability Registers.
 */

struct ehci_hcor_s {
    u32 usbcmd;           /* 0x00: USB Command */
    u32 usbsts;           /* 0x04: USB Status */
    u32 usbintr;          /* 0x08: USB Interrupt Enable */
    u32 frindex;          /* 0x0c: USB Frame Index */
    u32 ctrldssegment;    /* 0x10: 4G Segment Selector */
    u32 periodiclistbase; /* 0x14: Frame List Base Address */
    u32 asynclistaddr;    /* 0x18: Next Asynchronous List Address */
    u32 reserved[9];
    u32 configflag;       /* 0x40: Configured Flag Register */
    u32 portsc[15];       /* 0x44: Port Status/Control */
};

/* USB2 Debug Port Register Interface.
 *  This register block is normally found via the PCI capabalities.
 * In non-PCI implementions, you need apriori information about the
 * location of these registers.
 */

struct ehci_debug_s {
    u32 psc;     /* 0x00: Debug Port Control/Status Register */
    u32 pids;    /* 0x04: Debug USB PIDs Register */
    u32 data[2]; /* 0x08: Debug Data buffer Registers */
    u32 addr;    /* 0x10: Device Address Register */
};

/* Data Structures **********************************************************/
/* Periodic Frame List. Paragraph 3.1 */

#define PFL_T         (1 << 0) /* Bit 0: Terminate, Link pointer invalid */
#define PFL_TYP_SHIFT (1)      /* Bits 1-2: Type */
#define PFL_TYP_MASK  (3 << PFL_TYP_SHIFT)
#define PFL_TYP_ITD   (0 << PFL_TYP_SHIFT) /* Isochronous Transfer Descriptor */
#define PFL_TYP_QH    (1 << PFL_TYP_SHIFT) /* Queue Head */
#define PFL_TYP_SITD \
    (2 << PFL_TYP_SHIFT) /* Split Transaction Isochronous Transfer Descriptor */
#define PFL_TYP_FSTN (3 << PFL_TYP_SHIFT) /* Frame Span Traversal Node */

/* Bits 3-4: zero */
#define PFL_MASK (0xffffffe0) /* Bits 5-31:  Frame List Link Pointer */

/* Queue Element Transfer Descriptor (qTD). Paragraph 3.5 */

/* 32-bit version.  See EHCI Appendix B for the 64-bit version. */

struct ehci_qtd_s {
    u32 nqp;    /* 0x00-0x03: Next qTD Pointer */
    u32 alt;    /* 0x04-0x07: Alternate Next qTD Pointer */
    u32 token;  /* 0x08-0x0b: qTD Token */
    u32 bpl[5]; /* 0x0c-0x1c: Buffer Page Pointer List */
};

#define SIZEOF_EHCI_QTD_S (32) /* 8*sizeof(u32) */

/* Queue Head. Paragraph 3.6
 *
 * NOTE:
 * 1. Same as the field of the same name in struct ehci_qtd_s
 * 2. Similar to the field of the same name in struct ehci_qtd_s,
 *    but with some additional bitfields.
 */

struct ehci_overlay_s {
    u32 nqp;    /* 0x00-0x03: Next qTD Pointer (NOTE 1) */
    u32 alt;    /* 0x04-0x07: Alternate Next qTD Pointer (NOTE 2) */
    u32 token;  /* 0x08-0x0b: qTD Token (NOTE 1) */
    u32 bpl[5]; /* 0x0c-0x1c: Buffer Page Pointer List (NOTE 2) */
};

#define SIZEOF_EHCI_OVERLAY (32) /* 8*sizeof(u32) */

struct ehci_qh_s {
    u32 hlp;    /* 0x00-0x03: Queue Head Horizontal Link Pointer */
    u32 epchar; /* 0x04-0x07: Endpoint Characteristics */
    u32 epcaps; /* 0x08-0x0b: Endpoint Capabilities */
    u32 cqp;    /* 0x0c-0x0f: Current qTD Pointer */
    struct ehci_overlay_s overlay; /* 0x10-0x2c: Transfer overlay */
};

#define SIZEOF_EHCI_QH (48) /* 4*sizeof(u32) + SIZEOF_EHCI_OVERLAY */

struct usbh_endpoint_cfg {
    struct usbh_hubport *hport;
    /** The number associated with the EP in the device
     *  configuration structure
     *       IN  EP = 0x80 | \<endpoint number\>
     *       OUT EP = 0x00 | \<endpoint number\>
     */
    u8 ep_addr;
    /** Endpoint Transfer Type.
     * May be Bulk, Interrupt, Control or Isochronous
     */
    u8 ep_type;
    u8 ep_interval;
    /** Endpoint max packet size */
    u16 ep_mps;
};

/* Setup packet definition used to read raw data from USB line */
struct usb_setup_packet {
    /** Request type. Bits 0:4 determine recipient, see
     * \ref usb_request_recipient. Bits 5:6 determine type, see
     * \ref usb_request_type. Bit 7 determines data transfer direction, see
     * \ref usb_endpoint_direction.
     */
    u8 bmRequestType;

    /** Request. If the type bits of bmRequestType are equal to
     * \ref usb_request_type::LIBUSB_REQUEST_TYPE_STANDARD
     * "USB_REQUEST_TYPE_STANDARD" then this field refers to
     * \ref usb_standard_request. For other cases, use of this field is
     * application-specific. */
    u8 bRequest;

    /** Value. Varies according to request */
    u16 wValue;

    /** Index. Varies according to request, typically used to pass an index
     * or offset */
    u16 wIndex;

    /** Number of bytes to transfer */
    u16 wLength;
} __attribute__((packed));

typedef void *usbh_epinfo_t;

void dump_hcor(volatile struct ehci_hcor_s *hcor);
int usb_hc_sw_init(void);
int usb_hc_hw_init(int id);
int usb_hc_hw_fast_init(int id);
void usb_hc_low_level_init(int id);
u8 usbh_get_port_speed(int id);
int usbh_reset_port(int port, int id);
int usbh_ep0_reconfigure(usbh_epinfo_t ep, u8 dev_addr, u8 ep_mps, u8 speed);
int usbh_ep_alloc(usbh_epinfo_t *ep, const struct usbh_endpoint_cfg *ep_cfg);
int usbh_ep_free(usbh_epinfo_t ep);
int usbh_control_transfer(usbh_epinfo_t ep, struct usb_setup_packet *setup,
                          u8 *buffer, int id);
int usbh_ep_bulk_transfer(usbh_epinfo_t ep, u8 *buffer, u32 buflen, u32 timeout,
                          int id);
int usbh_portchange_wait(int id);
int usbh_get_port_connect_status(int id, int port);

#ifdef __cplusplus
}
#endif

#endif /* _AIC_USB_EHCI_H_ */
