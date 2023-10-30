/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRANS_RW_DATA_H_
#define _TRANS_RW_DATA_H_
#include <aic_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AIC Vendor ID */
#define DRIVER_VENDOR_ID 0x33C3
/* 'fw' 0x66 0x77: product id for firmware upgrade */
#define DRIVER_PRODUCT_ID 0x6677

#define AIC_USB_SIGN_USBC        0x43425355
#define AIC_USB_SIGN_USBS        0x53425355
#define AIC_CBW_DATA_HOST_TO_DEV 0x0
#define AIC_CBW_DATA_DEV_TO_HOST 0x80
#define AIC_CSW_STATUS_PASSED    0x00
#define AIC_CSW_STATUS_FAILED    0x01
#define AIC_CSW_STATUS_PHASE_ERR 0x02

/* USB UPG Data transport layer commands */
#define TRANS_LAYER_CMD_WRITE 0x01
#define TRANS_LAYER_CMD_READ  0x02

struct aic_cbw {
    u32 dCBWSignature;
    u32 dCBWTag;
    u32 dCBWDataTransferLength;
    u8 bmCBWFlags;
    u8 reserved0;
    u8 bCBWCBLength;
    u8 bCommand;
    u8 reserved[15];
} __attribute__((packed));

struct aic_csw {
    u32 dCSWSignature;
    u32 dCSWTag;
    u32 dCSWDataResidue;
    u8 bCSWStatus;
} __attribute__((packed));

typedef s32 (*phy_rw_func)(u8 *buf, s32 len);

struct phy_data_rw {
    phy_rw_func recv;
    phy_rw_func send;
};

s32 trans_layer_rw_proc(struct phy_data_rw *rw, u8 *buffer, u32 len);

#ifdef __cplusplus
}
#endif

#endif /* _TRANS_RW_DATA_H_ */

