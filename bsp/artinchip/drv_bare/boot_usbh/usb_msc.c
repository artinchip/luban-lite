/*
 * Copyright (c) 2020-2022 Artinchip Technology Co., Ltd. All rights reserved.
 *
 * Dehuang Wu <dehuang.wu@artinchip.com>
 */

#include <string.h>
#include "usb_ehci.h"
#include "usbh_core.h"
#include "usb_msc.h"
#include "usb_scsi.h"

//#define DEBUG_CBW
#define LBA_SIZE 512

static struct usbh_msc g_msc_class __attribute__((aligned(CACHE_LINE_SIZE)));

static int usbh_msc_get_maxlun(struct usbh_msc *msc_class, u8 *buffer)
{
    struct usb_setup_packet *setup = &msc_class->hport->setup;

    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_CLASS |
                           USB_REQUEST_RECIPIENT_INTERFACE;
    setup->bRequest = MSC_REQUEST_GET_MAX_LUN;
    setup->wValue = 0;
    setup->wIndex = msc_class->intf;
    setup->wLength = 1;

    return usbh_control_transfer(msc_class->hport->ep0, setup, buffer,
                                 msc_class->id);
}

static void usbh_msc_cbw_dump(struct CBW *cbw)
{
#ifdef DEBUG_CBW
    int i;

    pr_debug("CBW:\n");
    pr_debug("  signature: 0x%08x\n", (unsigned int)cbw->dSignature);
    pr_debug("  tag:       0x%08x\n", (unsigned int)cbw->dTag);
    pr_debug("  datlen:    0x%08x\n", (unsigned int)cbw->dDataLength);
    pr_debug("  flags:     0x%02x\n", cbw->bmFlags);
    pr_debug("  lun:       0x%02x\n", cbw->bLUN);
    pr_debug("  cblen:     0x%02x\n", cbw->bCBLength);

    pr_debug("CB:\n");
    for (i = 0; i < cbw->bCBLength; i += 8) {
        pr_debug("  %02x %02x %02x %02x %02x %02x %02x %02x\n", cbw->CB[i],
                 cbw->CB[i + 1], cbw->CB[i + 2], cbw->CB[i + 3], cbw->CB[i + 4],
                 cbw->CB[i + 5], cbw->CB[i + 6], cbw->CB[i + 7]);
    }
#endif
}

static void usbh_msc_csw_dump(struct CSW *csw)
{
#ifdef DEBUG_CBW
    pr_debug("CSW:\n");
    pr_debug("  signature: 0x%08x\n", (unsigned int)csw->dSignature);
    pr_debug("  tag:       0x%08x\n", (unsigned int)csw->dTag);
    pr_debug("  residue:   0x%08x\n", (unsigned int)csw->dDataResidue);
    pr_debug("  status:    0x%02x\n", csw->bStatus);
#endif
}

static inline int usbh_msc_scsi_testunitready(struct usbh_msc *msc_class)
{
    int nbytes;
    struct CBW *cbw;

    /* Construct the CBW */
    cbw = (struct CBW *)msc_class->tx_buffer;
    memset(cbw, 0, USB_SIZEOF_MSC_CBW);
    cbw->dSignature = MSC_CBW_Signature;

    cbw->bCBLength = SCSICMD_TESTUNITREADY_SIZEOF;
    cbw->CB[0] = SCSI_CMD_TESTUNITREADY;

    usbh_msc_cbw_dump(cbw);
    /* Send the CBW */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkout, (u8 *)cbw,
                                   USB_SIZEOF_MSC_CBW,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes < 0)
        goto out;
    /* Receive the CSW */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkin, msc_class->tx_buffer,
                                   USB_SIZEOF_MSC_CSW,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes >= 0) {
        usbh_msc_csw_dump((struct CSW *)msc_class->tx_buffer);
    }
out:
    return nbytes < 0 ? (int)nbytes : 0;
}

static inline int usbh_msc_scsi_requestsense(struct usbh_msc *msc_class)
{
    int nbytes;
    struct CBW *cbw;

    /* Construct the CBW */
    cbw = (struct CBW *)msc_class->tx_buffer;
    memset(cbw, 0, USB_SIZEOF_MSC_CBW);
    cbw->dSignature = MSC_CBW_Signature;

    cbw->bmFlags = 0x80;
    cbw->bCBLength = SCSIRESP_FIXEDSENSEDATA_SIZEOF;
    cbw->dDataLength = SCSICMD_REQUESTSENSE_SIZEOF;
    cbw->CB[0] = SCSI_CMD_REQUESTSENSE;
    cbw->CB[4] = SCSIRESP_FIXEDSENSEDATA_SIZEOF;

    usbh_msc_cbw_dump(cbw);
    /* Send the CBW */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkout, (u8 *)cbw,
                                   USB_SIZEOF_MSC_CBW,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes < 0)
        goto out;
    /* Receive the sense data response */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkin, msc_class->tx_buffer,
                                   SCSIRESP_FIXEDSENSEDATA_SIZEOF,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes < 0)
        goto out;
    /* Receive the CSW */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkin, msc_class->tx_buffer,
                                   USB_SIZEOF_MSC_CSW,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes >= 0) {
        usbh_msc_csw_dump((struct CSW *)msc_class->tx_buffer);
    }
out:
    return nbytes < 0 ? (int)nbytes : 0;
}

static inline int usbh_msc_scsi_inquiry(struct usbh_msc *msc_class)
{
    int nbytes;
    struct CBW *cbw;

    /* Construct the CBW */
    cbw = (struct CBW *)msc_class->tx_buffer;
    memset(cbw, 0, USB_SIZEOF_MSC_CBW);
    cbw->dSignature = MSC_CBW_Signature;

    cbw->dDataLength = SCSIRESP_INQUIRY_SIZEOF;
    cbw->bmFlags = 0x80;
    cbw->bCBLength = SCSICMD_INQUIRY_SIZEOF;
    cbw->CB[0] = SCSI_CMD_INQUIRY;
    cbw->CB[4] = SCSIRESP_INQUIRY_SIZEOF;

    usbh_msc_cbw_dump(cbw);
    /* Send the CBW */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkout, (u8 *)cbw,
                                   USB_SIZEOF_MSC_CBW,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes < 0)
        goto out;
    /* Receive the sense data response */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkin, msc_class->tx_buffer,
                                   SCSIRESP_INQUIRY_SIZEOF,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes < 0)
        goto out;
    /* Receive the CSW */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkin, msc_class->tx_buffer,
                                   USB_SIZEOF_MSC_CSW,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes >= 0) {
        usbh_msc_csw_dump((struct CSW *)msc_class->tx_buffer);
    }
out:
    return nbytes < 0 ? (int)nbytes : 0;
}

static inline int usbh_msc_scsi_readcapacity10(struct usbh_msc *msc_class)
{
    int nbytes;
    struct CBW *cbw;

    /* Construct the CBW */
    cbw = (struct CBW *)msc_class->tx_buffer;
    memset(cbw, 0, USB_SIZEOF_MSC_CBW);
    cbw->dSignature = MSC_CBW_Signature;

    cbw->dDataLength = SCSIRESP_READCAPACITY10_SIZEOF;
    cbw->bmFlags = 0x80;
    cbw->bCBLength = SCSICMD_READCAPACITY10_SIZEOF;
    cbw->CB[0] = SCSI_CMD_READCAPACITY10;

    usbh_msc_cbw_dump(cbw);
    /* Send the CBW */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkout, (u8 *)cbw,
                                   USB_SIZEOF_MSC_CBW,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes < 0)
        goto out;
    /* Receive the sense data response */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkin, msc_class->tx_buffer,
                                   SCSIRESP_READCAPACITY10_SIZEOF,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes < 0)
        goto out;
    /* Save the capacity information */
    msc_class->blocknum = GET_BE32(&msc_class->tx_buffer[0]) + 1;
    msc_class->blocksize = GET_BE32(&msc_class->tx_buffer[4]);
    /* Receive the CSW */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkin, msc_class->tx_buffer,
                                   USB_SIZEOF_MSC_CSW,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes >= 0) {
        usbh_msc_csw_dump((struct CSW *)msc_class->tx_buffer);
    }

out:
    return nbytes < 0 ? (int)nbytes : 0;
}

static int usbh_msc_scsi_read10(struct usbh_msc *msc_class, u32 start_sector,
                const u8 *buffer, u32 nsectors)
{
    int nbytes;
    struct CBW *cbw;

    /* Construct the CBW */
    cbw = (struct CBW *)msc_class->tx_buffer;
    memset(cbw, 0, USB_SIZEOF_MSC_CBW);
    cbw->dSignature = MSC_CBW_Signature;

    cbw->dDataLength = (msc_class->blocksize * nsectors);
    cbw->bmFlags = 0x80;
    cbw->bCBLength = SCSICMD_READ10_SIZEOF;
    cbw->CB[0] = SCSI_CMD_READ10;

    SET_BE32(&cbw->CB[2], start_sector);
    SET_BE16(&cbw->CB[7], nsectors);

    usbh_msc_cbw_dump(cbw);
    /* Send the CBW */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkout, (u8 *)cbw,
                                   USB_SIZEOF_MSC_CBW,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes < 0) {
        pr_err("%s, line %d: send CBW failed.\n", __func__, __LINE__);
        goto out;
    }
    /* Receive the user data */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkin, (u8 *)buffer,
                                   msc_class->blocksize * nsectors,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes < 0) {
        pr_err("%s, line %d: recv DATA failed.\n", __func__, __LINE__);
        goto out;
    }
    /* Receive the CSW */
    nbytes = usbh_ep_bulk_transfer(msc_class->bulkin, msc_class->tx_buffer,
                                   USB_SIZEOF_MSC_CSW,
                                   CONFIG_USBHOST_MSC_TIMEOUT, msc_class->id);
    if (nbytes >= 0) {
        usbh_msc_csw_dump((struct CSW *)msc_class->tx_buffer);
    }
    if (nbytes < 0) {
        pr_err("%s, line %d: read CSW failed.\n", __func__, __LINE__);
        goto out;
    }
out:
    return nbytes < 0 ? (int)nbytes : 0;
}

int usbh_msc_connect(struct usbh_hubport *hport, u8 intf, int id)
{
    struct usbh_endpoint_cfg ep_cfg = { 0 };
    struct usb_endpoint_descriptor *ep_desc;
    int ret;

    struct usbh_msc *msc_class = &g_msc_class;

    memset(msc_class, 0, sizeof(struct usbh_msc));
    msc_class->hport = hport;
    msc_class->intf = intf;
    msc_class->id = id;

    hport->config.intf[intf].priv = msc_class;

    ret = usbh_msc_get_maxlun(msc_class, msc_class->tx_buffer);
    if (ret < 0) {
        return ret;
    }

    pr_debug("Get max LUN:%u\r\n", msc_class->tx_buffer[0] + 1);

    for (u8 i = 0; i < hport->config.intf[intf].intf_desc.bNumEndpoints; i++) {
        ep_desc = &hport->config.intf[intf].ep[i].ep_desc;

        ep_cfg.ep_addr = ep_desc->bEndpointAddress;
        ep_cfg.ep_type = ep_desc->bmAttributes & USB_ENDPOINT_TYPE_MASK;
        ep_cfg.ep_mps = ep_desc->wMaxPacketSize;
        ep_cfg.ep_interval = ep_desc->bInterval;
        ep_cfg.hport = hport;
        if (ep_desc->bEndpointAddress & 0x80) {
            usbh_ep_alloc(&msc_class->bulkin, &ep_cfg);
        } else {
            usbh_ep_alloc(&msc_class->bulkout, &ep_cfg);
        }
    }

    ret = usbh_msc_scsi_testunitready(msc_class);
    if (ret < 0) {
        pr_err("Fail to scsi_testunitready\r\n");
        return ret;
    }
    ret = usbh_msc_scsi_inquiry(msc_class);
    if (ret < 0) {
        pr_err("Fail to scsi_inquiry\r\n");
        return ret;
    }
    ret = usbh_msc_scsi_readcapacity10(msc_class);
    if (ret < 0) {
        pr_err("Fail to scsi_readcapacity10\r\n");
        return ret;
    }

    if (msc_class->blocksize) {
        pr_err("Capacity info:\r\n");
        pr_err("Block num:%d,block size:%d\r\n",
               (unsigned int)msc_class->blocknum,
               (unsigned int)msc_class->blocksize);
    } else {
        pr_err("Fail to read capacity10\r\n");
        return -ERANGE;
    }

    pr_debug("MSC ok.\n");
    return ret;
}

/* qTD, max support 1MB one transfer */
#define MAX_BULK_BLK_NUM 40
u32 usbh_msc_read(u32 start_sector, u32 nsect, u8 *buffer)
{
    int ret;
    u8 *p, *temp = NULL;
    u32 todo, rdcnt;

    todo = nsect;

    if ((u32)(uintptr_t)buffer & (4096 - 1)) {
        temp = aicos_malloc_align(0, LBA_SIZE * MAX_BULK_BLK_NUM, 4096);
        if (!temp) {
            pr_err("malloc failed.\n");
            return 0;
        }
    }

    p = buffer;
    while (todo) {
        if (todo > MAX_BULK_BLK_NUM)
            rdcnt = MAX_BULK_BLK_NUM;
        else
            rdcnt = todo;

        if ((u32)(uintptr_t)buffer & (4096 - 1)) {
            ret = usbh_msc_scsi_read10(&g_msc_class, start_sector, temp, rdcnt);
            if (ret < 0) {
                pr_err("%s failed. ret = %d\n", __func__, ret);
                aicos_free_align(0, temp);
                return 0;
            }
            memcpy(p, temp, LBA_SIZE * rdcnt);
        } else {
            ret = usbh_msc_scsi_read10(&g_msc_class, start_sector, p, rdcnt);
            if (ret < 0) {
                pr_err("%s failed. ret = %d\n", __func__, ret);
                return 0;
            }
        }

        start_sector += rdcnt;
        todo -= rdcnt;
        p += (rdcnt * LBA_SIZE);
    }

    if (temp)
        aicos_free_align(0, temp);

    return nsect;
}
