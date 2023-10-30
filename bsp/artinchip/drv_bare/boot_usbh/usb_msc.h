/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef _AIC_USB_MSC_H_
#define _AIC_USB_MSC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>
#include "usb_scsi.h"

/* MSC Subclass Codes */
#define MSC_SUBCLASS_RBC           0x01 /* Reduced block commands (e.g., flash devices) */
#define MSC_SUBCLASS_SFF8020I_MMC2 0x02 /* SFF-8020i/MMC-2 (ATAPI) (e.g., C/DVD) */
#define MSC_SUBCLASS_QIC157        0x03 /* QIC-157 (e.g., tape device) */
#define MSC_SUBCLASS_UFI           0x04 /* e.g. floppy device */
#define MSC_SUBCLASS_SFF8070I      0x05 /* SFF-8070i (e.g. floppy disk) */
#define MSC_SUBCLASS_SCSI          0x06 /* SCSI transparent */

/* MSC Protocol Codes */
#define MSC_PROTOCOL_CBI_INT   0x00 /* CBI transport with command completion interrupt */
#define MSC_PROTOCOL_CBI_NOINT 0x01 /* CBI transport without command completion interrupt */
#define MSC_PROTOCOL_BULK_ONLY 0x50 /* Bulk only transport */

/* MSC Request Codes */
#define MSC_REQUEST_RESET       0xFF
#define MSC_REQUEST_GET_MAX_LUN 0xFE

/** MSC Command Block Wrapper (CBW) Signature */
#define MSC_CBW_Signature 0x43425355
/** Bulk-only Command Status Wrapper (CSW) Signature */
#define MSC_CSW_Signature 0x53425355

/** MSC Command Block Status Values */
#define CSW_STATUS_CMD_PASSED  0x00
#define CSW_STATUS_CMD_FAILED  0x01
#define CSW_STATUS_PHASE_ERROR 0x02

#define MSC_MAX_CDB_LEN (16) /* Max length of SCSI Command Data Block */

/** MSC Bulk-Only Command Block Wrapper (CBW) */
struct CBW {
    u32 dSignature;  /* 'USBC' = 0x43425355 */
    u32 dTag;        /* Depends on command id */
    u32 dDataLength; /* Number of bytes that host expects to transfer */
    u8 bmFlags;      /* Bit 7: Direction=IN (other obsolete or reserved) */
    u8 bLUN;         /* LUN (normally 0) */
    u8 bCBLength;    /* len of cdb[] */
    u8 CB[MSC_MAX_CDB_LEN]; /* Command Data Block */
} __attribute__((packed));

#define USB_SIZEOF_MSC_CBW 31

/** MSC Bulk-Only Command Status Wrapper (CSW) */
struct CSW {
    u32 dSignature;   /* 'USBS' = 0x53425355 */
    u32 dTag;         /* Same tag as original command */
    u32 dDataResidue; /* Amount not transferred */
    u8 bStatus;       /* Status of transfer */
} __attribute__((packed));

#define USB_SIZEOF_MSC_CSW 13

/*Length of template descriptor: 23 bytes*/
#define MSC_DESCRIPTOR_LEN         (9 + 7 + 7)
#define CONFIG_USBHOST_MSC_TIMEOUT 2000

#define SET_BE32(field, value)            \
    do {                                  \
        (field)[0] = (u8)((value) >> 24); \
        (field)[1] = (u8)((value) >> 16); \
        (field)[2] = (u8)((value) >> 8);  \
        (field)[3] = (u8)((value) >> 0);  \
    } while (0)
#define SET_BE16(field, value)           \
    do {                                 \
        (field)[0] = (u8)((value) >> 8); \
        (field)[1] = (u8)((value) >> 0); \
    } while (0)

#define GET_BE16(field) (((u16)(field)[0] << 8) | ((u16)(field)[1]))

#define GET_BE32(field)                                  \
    (((u32)(field)[0] << 24) | ((u32)(field)[1] << 16) | \
     ((u32)(field)[2] << 8) | ((u32)(field)[3] << 0))

struct usbh_msc {
    u8 tx_buffer[64];
    u8 intf; /* Data interface number */
    u8 sdchar;
    usbh_epinfo_t bulkin;  /* Bulk IN endpoint */
    usbh_epinfo_t bulkout; /* Bulk OUT endpoint */
    u32 blocknum;          /* Number of blocks on the USB mass storage device */
    u16 blocksize;         /* Block size of USB mass storage device */
    int id;
    struct usbh_hubport *hport;
};

int usbh_msc_connect(struct usbh_hubport *hport, u8 intf, int id);

#ifdef __cplusplus
}
#endif

#endif
