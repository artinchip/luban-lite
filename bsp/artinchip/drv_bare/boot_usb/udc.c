/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <driver.h>
#include <aic_core.h>

#include <usb_drv.h>
#include "udc.h"
#include "usbc.h"
#include "usbdevice.h"
#include "usb_defs.h"
#include "usb_reg.h"

static u8 rx_base_buffer[BULK_EP_HS_MPS];
static struct aic_udc aic_udc;

s32 aic_udc_init(struct usb_device *usbdev)
{
    struct aic_udc *udc = &aic_udc;

    if (usbdev == NULL)
        return -1;

    memset(udc, 0, sizeof(struct aic_udc));
    udc->gadget = usbdev;
    if (udc->gadget->state_init()) {
        pr_err("usb err: fail to init usb device\n");
        return -1;
    }

    usbc_init();

    usbc_soft_disconnect();
    usbc_clock_gating_disable();

    memset(&udc->buf, 0, sizeof(struct aic_ubuf));
    udc->buf.rx_buf = (u8 *)rx_base_buffer;

    usbc_clock_gating_enable();

    usbc_soft_connect();

    pr_debug("%s done\n", __func__);
    return 0;
}

s32 aic_udc_exit(void)
{
    return 0;
}

static void reset_connection(void)
{
    usbc_soft_disconnect();
    aicos_mdelay(10);
    usbc_soft_connect();
    aic_udc.reset_conn = 0;
}

void aic_udc_bulk_ep_reset(void)
{
    usbc_in_bulk_ep_activate(BULK_IN_EP_INDEX);
    usbc_out_bulk_ep_activate(BULK_OUT_EP_INDEX);
    usbc_flush_np_txfifo();
}

/*
 * USB Reset from HOST.
 * This is the first interrupt when device connect to host.
 */
static void reset_intr_proc(void)
{
    struct aic_udc *udc = &aic_udc;
    u32 i, setup_pkt_max, pkt_cnt, xfersiz;

    pr_debug("\n\n%s\n\n", __func__);
    usbc_intr_clear_gintsts_pending(USB_DEV_GINTSTS_USBRST);
    usbc_flush_all_txfifo();

    for (i = 0; i < USB_DEV_EP_CNT; i++) {
        usbc_in_ep_reset(i);
        usbc_out_ep_reset(i);
    }

    usbc_intr_reset();

    usbc_set_address(0);

    /*
     * Transfer configuration
     * Setup HW to recieve 1 SETUP packet after reset
     */
    setup_pkt_max = 3;
    pkt_cnt = 1;
    xfersiz = setup_pkt_max * 8;
    usbc_out_ctrl_ep_xfer_cfg(setup_pkt_max, pkt_cnt, xfersiz);

    udc->gadget->state_reset();
}

/*
 * After reset interrupt, host will enumuerate device speed.
 */
static void speed_enum_done_intr_proc(void)
{
    u8 speed;

    pr_debug("%s\n", __func__);

    usbc_intr_clear_gintsts_pending(USB_DEV_GINTSTS_ENUMDNE);

    speed = usbc_get_dev_speed();
    switch (speed) {
        case USB_DEV_SPEED_HIGH:
            pr_debug("HS, phy clock 30MHz or 60MHz\n");
            break;
        case USB_DEV_SPEED_FULL2:
            pr_debug("FS, phy clock 30MHz or 60MHz\n");
            break;
        case USB_DEV_SPEED_FULL1:
            pr_debug("FS, phy clock 48MHz\n");
            break;
        case USB_DEV_SPEED_LOW:
            pr_debug("LS, phy clock 6MHz\n");
            break;
    }

    /*
     * Speed enumerate finish, going to perform Ctrl EP communication
     */
    usbc_set_ctrl_ep_mps(DIEPCTL0_MPS_64B);
    usbc_in_ctrl_ep_activate();
    usbc_out_ctrl_ep_activate();
}

static u32 rxfifo_data_intr_proc(u8 *buf, u32 *ep_id)
{
    u32 status_pop, data_len = 0, pktsts;

    usbc_intr_gintmsk_disable(USB_DEV_GINTMSK_RXFLVL);

    status_pop = usbc_get_status_pop();

    *ep_id = (status_pop & USB_DEV_GRXSTSP_EPNUM) >> 0;
    data_len = (status_pop & USB_DEV_GRXSTSP_BCNT) >> 4;
    pktsts = (status_pop & USB_DEV_GRXSTSP_PKTSTS) >> 17;

    if (pktsts == GRXSTSP_PKTSTS_SETUP_RECV)
        pr_debug("%s, GRXSTSP_PKTSTS_SETUP_RECV\n", __func__);
    if (pktsts == GRXSTSP_PKTSTS_OUTDATA_RECV)
        pr_debug("%s, GRXSTSP_PKTSTS_OUTDATA_RECV\n", __func__);
    if (pktsts == GRXSTSP_PKTSTS_OUT_NAK)
        pr_debug("%s, GRXSTSP_PKTSTS_OUT_NAK\n", __func__);
    if (pktsts == GRXSTSP_PKTSTS_OUTXFER_COMPLETED)
        pr_debug("%s, GRXSTSP_PKTSTS_OUTXFER_COMPLETED\n", __func__);
    if (pktsts == GRXSTSP_PKTSTS_SETUP_COMPLETED)
        pr_debug("%s, GRXSTSP_PKTSTS_SETUP_COMPLETED\n", __func__);
    pr_debug("%s, datalen 0x%X\n", __func__, data_len);

    switch (pktsts) {
        case GRXSTSP_PKTSTS_SETUP_RECV:
        case GRXSTSP_PKTSTS_OUTDATA_RECV:
            /*
             * Read SETUP/OUT transaction's data
             * Two types data will be read here:
             *  - SETUP Request
             *  - CBW
             */
            usbc_ep_recv_data_cpu(buf, data_len);
            break;
        case GRXSTSP_PKTSTS_OUT_NAK:
            break;
        case GRXSTSP_PKTSTS_OUTXFER_COMPLETED:
            break;
        case GRXSTSP_PKTSTS_SETUP_COMPLETED:
            break;
    }
    usbc_intr_gintmsk_enable(USB_DEV_GINTMSK_RXFLVL);

    return data_len;
}

static void ctrl_ep_income_req_proc(u8 *buf, u32 len)
{
    s32 ret = 0;
    struct aic_udc *udc = &aic_udc;
    struct usb_device_request *req = &udc->std_req;

    pr_debug("%s\n", __func__);
    if (len != sizeof(struct usb_device_request)) {
        pr_err("Error, EP0 recv data should be usb_device_request\n");
        ret = -1;
        goto out;
    }
    if (udc->gadget == NULL) {
        pr_err("Error, no active device\n");
        ret = -1;
        goto out;
    }

    memcpy(req, buf, len);
    if (USB_REQ_TYPE_STANDARD == (req->bmRequestType & USB_REQ_TYPE_MASK))
        ret = udc->gadget->standard_req_proc(req);
    else
        ret = udc->gadget->nonstandard_req_proc(req);

out:
    if (ret != 0) {
        /*
         * Control EP, not supported request, should return STALL
         * to host. Set STALL on control EP here, and set ready to
         * receive next SETUP packet, STALL on control EP will be clear
         * by hardware when receive SETUP PID.
         */
        usbc_out_ctrl_ep_set_stall();
        usbc_in_ctrl_ep_set_stall();
        usbc_out_ctrl_ep_xfer_cfg(3, 1, 24);
    }
}

/*
 * Interrupt from OUT EP
 */
static void out_ep_intr_proc(struct aic_ubuf *buf)
{
    struct aic_udc *udc = &aic_udc;
    u32 ep, intr_overview, ep_intr;

    pr_debug("%s\n", __func__);
    usbc_intr_clear_gintsts_pending(USB_DEV_GINTSTS_OEPINT);
    intr_overview = usbc_get_daint_out();

    /*
     * Check which EP has pending interrupt
     */
    for (ep = 0; ep < USB_DEV_EP_CNT; ep++) {
        if ((intr_overview & (0x1 << ep)) == 0)
            continue;
        /*
         * Get OUT EP's detail interrupt information
         */
        ep_intr = usbc_intr_get_doepint(ep);
        if ((ep == CTRL_EP_INDEX) && (ep_intr & USB_DEV_DOEPINT_XFRC)) {
            pr_debug("ep0 USB_DEV_DOEPINT_XFRC\n");
            usbc_intr_clear_doepint(ep, USB_DEV_DOEPINT_XFRC);

            /*
             * Going to receive next SETUP packet from
             * OUT EP0
             */
            usbc_out_ctrl_ep_xfer_cfg(3, 1, 24);
        }
        if ((ep == CTRL_EP_INDEX) && (ep_intr & USB_DEV_DOEPINT_STUP)) {
            pr_debug("USB_DEV_DOEPINT_STUP\n");
            usbc_intr_clear_doepint(ep, USB_DEV_DOEPINT_STUP);

            /*
             * Going to receive next SETUP packet from EP0
             *
             * Since USB_DEV_DOEPINT_XFRC not occur every time,
             * set it again.
             */
            usbc_out_ctrl_ep_xfer_cfg(3, 1, 24);

            /*
             * Receive SETUP request from OUT EP0 done
             */
            ctrl_ep_income_req_proc(buf->rx_buf, buf->rx_len);
            memset(buf->rx_buf, 0, buf->rx_len);
            buf->rx_len = 0;
        }
        if ((ep == BULK_OUT_EP_INDEX) && (ep_intr & USB_DEV_DOEPINT_XFRC)) {
            pr_debug("ep2 USB_DEV_DOEPINT_XFRC\n");
            usbc_intr_clear_doepint(ep, USB_DEV_DOEPINT_XFRC);
            /*
             * Two cases to issue this interrupt:
             * - Receive CBW done
             * - Receive data follows CBW. This data recv in CBW
             *   processing flow, but not wait and clear this intr.
             */
            if (udc->gadget && buf->rx_len > 0) {
                udc->gadget->state_cmd(buf->rx_buf, buf->rx_len);
                memset(buf->rx_buf, 0, buf->rx_len);
                buf->rx_len = 0;
            }

            /*
             * Finish one CBW process, set bulk ep to receive next
             * CBW
             */
            usbc_out_bulk_ep_xfer_cfg(BULK_OUT_EP_INDEX, 1, BULK_EP_HS_MPS);
        }
        if (ep_intr & USB_DEV_DOEPINT_OTEPDIS) {
            /*
             * OUT Token Received when this EP was not yet enabled.
             * (So we should enable this EP here?)
             */
            pr_debug("ep = 0x%X USB_DEV_DOEPINT_OTEPDIS\n", ep);
            usbc_intr_clear_doepint(ep, USB_DEV_DOEPINT_OTEPDIS);
        }
        if (ep_intr & USB_DEV_DOEPINT_OTEPSPR) {
            pr_debug("ep = %X USB_DEV_DOEPINT_OTEPSPR\n", ep);
            usbc_intr_clear_doepint(ep, USB_DEV_DOEPINT_OTEPSPR);
        }
        if (ep_intr & USB_DEV_DOEPINT_NAK) {
            /*
             * USB Core generate this intr when a NAK is sent
             */
            pr_debug("ep = %X USB_DEV_DOEPINT_NAK\n", ep);
            usbc_intr_clear_doepint(ep, USB_DEV_DOEPINT_NAK);
        }
    }
}

static void np_txfifo_empty_intr_proc(void)
{
    usbc_intr_clear_gintsts_pending(USB_DEV_GINTSTS_NPTXFE);
    pr_debug("%s\n", __func__);

    /*
     * Disable NPTXFE interrupt here, when to enable?
     * - When application is going to send some data, after configure
     *   xfer size, can enable this interrupt to check if FIFO is ok to
     *   write data
     */
    usbc_intr_gintmsk_disable(USB_DEV_GINTMSK_NPTXFE);
}

static void in_ep_intr_proc(void)
{
    u32 ep, intr_overview, ep_intr;

    pr_debug("%s\n", __func__);
    usbc_intr_clear_gintsts_pending(USB_DEV_GINTSTS_IEPINT);
    intr_overview = usbc_get_daint_in();

    /*
     * Check which EP has pending interrupt
     */
    for (ep = 0; ep < USB_DEV_EP_CNT; ep++) {
        if ((intr_overview & (0x1 << ep)) == 0)
            continue;

        /*
         * Get IN EP's detail interrupt information
         */
        ep_intr = usbc_intr_get_diepint(ep);
        if ((ep == CTRL_EP_INDEX) && (ep_intr & USB_DEV_DIEPINT_XFRC)) {
            pr_debug("ep0 USB_DEV_DIEPINT_XFRC\n");
            usbc_intr_clear_diepint(ep, USB_DEV_DIEPINT_XFRC);
            // usbc_out_ctrl_ep_xfer_cfg(3, 1, 24);
        }
        if ((ep == BULK_IN_EP_INDEX) && (ep_intr & USB_DEV_DIEPINT_XFRC)) {
            pr_debug("ep1 USB_DEV_DIEPINT_XFRC\n");
            usbc_intr_clear_diepint(ep, USB_DEV_DIEPINT_XFRC);
        }

        /*
         * Timeout Condition
         * HW detected a timeout condition on the USB for the last IN
         * token on the EP.
         */
        if (ep_intr & USB_DEV_DIEPINT_TOC) {
            pr_debug("ep %X USB_DEV_DIEPINT_TOC\n", ep);
            usbc_intr_clear_diepint(ep, USB_DEV_DIEPINT_TOC);
        }

        /*
         * IN Token Rexeived when TXFIFO is Emptry
         * Indicates that an IN token was received when the associated
         * TxFIFO was empty.
         */
        if (ep_intr & USB_DEV_DIEPINT_ITTXFE) {
            pr_debug("ep %X USB_DEV_DIEPINT_TOC\n", ep);
            usbc_intr_clear_diepint(ep, USB_DEV_DIEPINT_ITTXFE);
        }

        /*
         * IN EP NAK Effective
         * The interrupt indicates that the IN EP NAK bit set by the
         * application has taken effect in the core.
         */
        if (ep_intr & USB_DEV_DIEPINT_INEPNE) {
            pr_debug("ep %X USB_DEV_DIEPINT_INEPNE\n", ep);
            usbc_intr_clear_diepint(ep, USB_DEV_DIEPINT_INEPNE);
        }

        /*
         * EP Disabled
         * This bit indicates that the EP is disabled per the
         * application's request.
         */
        if (ep_intr & USB_DEV_DIEPINT_EPDISD) {
            pr_debug("ep %X USB_DEV_DIEPINT_EPDISD\n", ep);
            usbc_intr_clear_diepint(ep, USB_DEV_DIEPINT_EPDISD);
        }

        /*
         * TXFIFO Empty
         * Indicates that the TxFIFO for this EP is either half or
         * completely empty.
         */
        if (ep_intr & USB_DEV_DIEPINT_TXFE) {
            pr_debug("ep %X USB_DEV_DIEPINT_TXFE\n", ep);
            usbc_intr_clear_diepint(ep, USB_DEV_DIEPINT_TXFE);
        }
    }
}

void aic_udc_state_loop(void)
{
    struct aic_udc *udc = &aic_udc;
    u32 gintsts, rxlen, ep_id;

    if (aic_udc.reset_conn) {
        reset_connection();
        return;
    }
    /* Read status registers */
    gintsts = usbc_intr_get_gintsts();

    if (gintsts & USB_DEV_GINTSTS_USBRST)
        reset_intr_proc();

    if (gintsts & USB_DEV_GINTSTS_ENUMDNE)
        speed_enum_done_intr_proc();

    if (gintsts & USB_DEV_GINTSTS_RXFLVL) {
        rxlen = rxfifo_data_intr_proc(udc->buf.rx_buf, &ep_id);
        /*
         * Maybe is not data receive event, check here
         */
        if (rxlen > 0) {
            udc->buf.rx_len = rxlen;
            udc->buf.ep_id = ep_id;
        }
    }

    if (gintsts & USB_DEV_GINTSTS_NPTXFE)
        np_txfifo_empty_intr_proc();

    /* Read status again after tx/rx fifo proc */
    gintsts = usbc_intr_get_gintsts();

    if (gintsts & USB_DEV_GINTSTS_IEPINT)
        in_ep_intr_proc();

    if (gintsts & USB_DEV_GINTSTS_OEPINT)
        out_ep_intr_proc(&udc->buf);
}

u32 aic_udc_ctrl_ep_send(u8 *buf, u32 len)
{
    u32 pkt_siz, sndlen, all_todo = 0, pkt_todo = 0;
    u32 maxtry = 100;

    if (len == 0) {
        /*
         * Configure xfer pkt and size
         */
        usbc_in_ctrl_ep_xfer_cfg(3, 0);
        return 0;
    }

    all_todo = len;

    while (all_todo > 0) {
        /*
         * One packet one time for EP0
         */
        if (all_todo > CTL_EP0_MPS)
            pkt_siz = CTL_EP0_MPS;
        else
            pkt_siz = all_todo;
        usbc_in_ctrl_ep_xfer_cfg(1, pkt_siz);

        pkt_todo = pkt_siz;
        maxtry = 10000;
        do {
            sndlen = usbc_ctrl_ep_send_data(buf, pkt_todo);
            if (sndlen == 0) {
                aicos_udelay(1);
                maxtry--;
                continue;
            }
            pkt_todo -= sndlen;
            buf += sndlen;
        } while (pkt_todo > 0 && maxtry > 0);

        all_todo -= (pkt_siz - pkt_todo);
        if (pkt_todo > 0)
            break; /* Not finished */
    }

    if (all_todo > 0) {
        /* Send data timeout, Host encounter problem, just
         * abort operation here.
         */
        usbc_flush_all_txfifo();
    }

    return (len - all_todo);
}

s32 aic_udc_set_configuration(s32 config_param)
{
    /*
     * Send out one zero length packet as IN transaction
     */
    aic_udc_ctrl_ep_send(NULL, 0);

    /*
     * When host set configuration, that means enumeration is done.
     * Now set bulk ep to ready for CBW
     */
    usbc_out_bulk_ep_xfer_cfg(BULK_OUT_EP_INDEX, 1, BULK_EP_HS_MPS);
    return 0;
}

s32 aic_udc_set_address(u8 address)
{
    usbc_set_address(address);

    /*
     * Send out one zero length packet after set new address
     */
    aic_udc_ctrl_ep_send(NULL, 0);
    return 0;
}

s32 aic_udc_bulk_send(u8 *buf, s32 len)
{
    u32 pkt_cnt, sndlen, todo = 0, gintsts, maxtry = 100, ep;

    ep = BULK_IN_EP_INDEX;
    if (len == 0) {
        /*
         * Configure xfer pkt and size
         */
        pkt_cnt = 1;
        usbc_in_bulk_ep_xfer_cfg(ep, pkt_cnt, len);
        return 0;
    }

    pkt_cnt = (len + BULK_EP_HS_MPS - 1) / BULK_EP_HS_MPS;
    usbc_in_bulk_ep_xfer_cfg(ep, pkt_cnt, len);

    /*
     * Poll ITTXFE interrupt and send data to TX FIFO
     */
    maxtry = 300000;
    todo = len;
    do {
        sndlen = usbc_bulk_ep_send_data(ep, buf, todo);
        if (sndlen == 0) {
            aicos_udelay(1);
            maxtry--;
            continue;
        }
        todo -= sndlen;
        buf += sndlen;
    } while (todo > 0 && maxtry > 0);
    if (maxtry == 0)
        goto timeout;

    maxtry = 30000;
    do {
        /*
         * After write data, need to wait all data is sent out
         */
        gintsts = usbc_intr_get_gintsts();
        if (gintsts & USB_DEV_GINTSTS_NPTXFE) {
            usbc_intr_clear_gintsts_pending(USB_DEV_GINTSTS_NPTXFE);
            break;
        }
        aicos_udelay(1);
        maxtry--;
    } while (maxtry > 0);
    if (maxtry == 0)
        goto timeout;

    return len;

timeout:
    usbc_flush_all_txfifo();
    return 0;
}

s32 aic_udc_bulk_recv_dma(u8 *dest, s32 len)
{
    return aic_udc_bulk_recv(dest, len);
}

/*
 * This API used in CBW bulk data transfer scenario, typical flow is:
 * - Got CBW command, application know data size
 * - Use this API to read data after CBW
 */
s32 aic_udc_bulk_recv(u8 *buf, s32 len)
{
    u32 gintsts, pktcnt, todo, recvlen, ep_id, maxtry = 300000;
    u32 xfersize;

    if (len == 0)
        return 0;

    /*
     * Set data pkt and size going to recv
     */
    pktcnt = (len + BULK_EP_HS_MPS - 1) / BULK_EP_HS_MPS;
    xfersize = len + BULK_EP_HS_MPS - (len % BULK_EP_HS_MPS);
    usbc_out_bulk_ep_xfer_cfg(BULK_OUT_EP_INDEX, pktcnt, xfersize);

    /*
     * Wait data income interrupt
     */
    todo = len;
    do {
        gintsts = usbc_intr_get_gintsts();
        if ((gintsts & USB_DEV_GINTSTS_RXFLVL) == 0) {
            aicos_udelay(1);
            maxtry--;
            continue;
        }

        recvlen = rxfifo_data_intr_proc(buf, &ep_id);
        if (recvlen == 0)
            continue;

        /*
         * If recv data not from BULK OUT EP, just drop it
         */
        if (ep_id == BULK_OUT_EP_INDEX) {
            todo -= recvlen;
            buf += recvlen;
        }
    } while (todo > 0 && maxtry > 0);

    return (len - todo);
}

/*
 * Bulk EP stall indicates the EP is permanently halted or there is an error
 * condition on the EP, and require USB system software intervention.
 */
void aic_udc_bulk_rx_stall(void)
{
    usbc_out_ep_set_stall(BULK_OUT_EP_INDEX);
}

/*
 * Bulk EP stall indicates the EP is permanently halted or there is an error
 * condition on the EP, and require USB system software intervention.
 */
void aic_udc_bulk_tx_stall(void)
{
    usbc_in_ep_set_stall(BULK_IN_EP_INDEX);
}

/*
 * Send STALL to host to notify stop receiving data.
 * And prepare to reset connection with host.
 */
void aic_udc_bulk_rx_fatal(void)
{
    aic_udc_bulk_rx_stall();
    aic_udc.reset_conn = 1;
}

/*
 * Send STALL to host to notify it device stop sending data.
 * And prepare to reset connection with host.
 */
void aic_udc_bulk_tx_fatal(void)
{
    aic_udc_bulk_tx_stall();
    aic_udc.reset_conn = 1;
}
