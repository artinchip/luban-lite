/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <string.h>
#include <usbdescriptors.h>
#include <usbdevice.h>
#include <usb_drv.h>
#include <aic_core.h>
#include <aicupg.h>
#include <trans_rw_data.h>

#ifdef AICUPG_DEBUG
#undef pr_err
#undef pr_warn
#undef pr_info
#undef pr_debug
#define pr_err printf
#define pr_warn printf
#define pr_info printf
#define pr_debug printf
#endif

#define RECV_SIZE_USE_DMA 512
#define TRANS_DATA_BUFF_SIZE (64 * 1024)
static u8 *trans_pkt_buf = NULL;

static void trans_send_csw(struct phy_data_rw *rw, u32 tag, u8 status, u32 rest)
{
    struct aic_csw csw;

    pr_debug("Going to send CSW for tag 0x%X, status = %s\n", tag, status == 0 ? "OK" : "FAIL");
    csw.dCSWSignature = AIC_USB_SIGN_USBS;
    csw.dCSWTag = tag;
    csw.dCSWDataResidue = rest;
    csw.bCSWStatus = status;
    rw->send((u8 *)&csw, sizeof(struct aic_csw));
    pr_debug("Ack CSW tag 0x%X done\n", tag);
}

/* All income should start with CBW packet */
s32 trans_layer_rw_proc(struct phy_data_rw *rw, u8 *buffer, u32 len)
{
    struct aic_cbw cbw;
    u32 data_len, retlen, total, rest, slice;
    u8 status;

    if (len != sizeof(struct aic_cbw)) {
        pr_err("length != 31, len = 0x%X\n", len);
        /* Status error or CBW not valid, just skip pakcet here */
        return -1;
    }

    if (!trans_pkt_buf) {
        trans_pkt_buf = aicos_malloc_align(0, TRANS_DATA_BUFF_SIZE, CACHE_LINE_SIZE);
        if (!trans_pkt_buf) {
            pr_err("malloc trans pkt buf failed.\n");
            return -1;
        }
    }

    // hexdump("CBW ", buffer, len);
    /* To avoid alignment issue */
    memcpy(&cbw, buffer, sizeof(struct aic_cbw));
    if (cbw.dCBWSignature != AIC_USB_SIGN_USBC) {
        pr_err("CBW signature not correct\n");
        /* Status error or CBW not valid, just skip pakcet here */
        return -1;
    }

    status = AIC_CSW_STATUS_PASSED;
    pr_debug("\nCBW tag 0x%X\n", cbw.dCBWTag);
    switch (cbw.bCommand) {
        case TRANS_LAYER_CMD_WRITE:
            data_len = cbw.dCBWDataTransferLength;
            pr_debug("TRANS_LAYER_CMD_WRITE, data len %d\n", data_len);
            if (cbw.bmCBWFlags != 0) {
                status = AIC_CSW_STATUS_FAILED;
                trans_send_csw(rw, cbw.dCBWTag, status, data_len);
                break;
            }

            rest = data_len;
            total = 0;
            while (rest > 0) {
                if (rest >= TRANS_DATA_BUFF_SIZE)
                    slice = TRANS_DATA_BUFF_SIZE;
                else
                    slice = rest;

                retlen = rw->recv(trans_pkt_buf, slice);
                if (retlen != slice)
                    break;

                aicupg_data_packet_write(trans_pkt_buf, slice);
                total += slice;
                rest -= slice;
            }

            if (total == data_len) {
                trans_send_csw(rw, cbw.dCBWTag, status, 0);
            } else {
                status = AIC_CSW_STATUS_FAILED;
                trans_send_csw(rw, cbw.dCBWTag, status, rest);
            }

            break;
        case TRANS_LAYER_CMD_READ:
            data_len = cbw.dCBWDataTransferLength;
            pr_debug("TRANS_LAYER_CMD_READ, data len %d\n", data_len);
            if ((cbw.bmCBWFlags != 0x80)) {
                status = AIC_CSW_STATUS_FAILED;
                trans_send_csw(rw, cbw.dCBWTag, status, data_len);
                break;
            }

            rest = data_len;
            total = 0;
            while (rest > 0) {
                if (rest >= TRANS_DATA_BUFF_SIZE)
                    slice = TRANS_DATA_BUFF_SIZE;
                else
                    slice = rest;

                retlen = aicupg_data_packet_read(trans_pkt_buf, slice);
                if (retlen != slice)
                    break;

                rw->send((u8 *)trans_pkt_buf, slice);
                total += slice;
                rest -= slice;
            }

            if (total == data_len) {
                trans_send_csw(rw, cbw.dCBWTag, status, 0);
            } else {
                status = AIC_CSW_STATUS_FAILED;
                trans_send_csw(rw, cbw.dCBWTag, status, rest);
            }

            break;
        default:
            status = AIC_CSW_STATUS_PHASE_ERR;
            trans_send_csw(rw, cbw.dCBWTag, status, 0);
            break;
    }

    /*
    if (trans_pkt_buf) {
        aicos_free_align(0, trans_pkt_buf);
        trans_pkt_buf = NULL;
    }
    */

    return 0;
}
