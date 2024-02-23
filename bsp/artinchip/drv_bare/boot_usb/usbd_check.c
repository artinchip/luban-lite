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
#include <aic_core.h>
#include <aic_hal.h>
#include <usb_reg.h>
#include <usb_drv.h>
#include <hal_syscfg.h>
#include "usbc.h"

/*
 * When the USB0 works as device, checking if it is connecting with PC will
 * spend about 200ms, it is too long if we just waiting here.
 *
 * To save the waiting time, we split the checking to 2 parts:
 *   1. Initialize it when bus clock is set
 *   2. Double check after Linux image is loaded(for Falcon mode)
 *   3. If the status is connecting, than force to load u-boot again to force
 *      run u-boot
 */
extern struct usb_device usbupg_device;
void usbd_connection_check_start(void)
{
#ifndef AIC_SYSCFG_DRV_V12
    syscfg_usb_phy0_sw_host(0);
#endif
    aic_udc_init(&usbupg_device);
}

void usbd_connection_check_end(void)
{
    usbc_soft_disconnect();
    hal_clk_disable_assertrst(CLK_USBD);
    hal_clk_disable_assertrst(CLK_USB_PHY0);
}

extern void reset_intr_proc(void);
int usbd_connection_check_status(void)
{
    u32 sts;
    u64 start, timeout = 500;

    start = aic_get_time_ms();
    do {
        sts = usbc_intr_get_gintsts();
        if (sts & USB_DEV_GINTSTS_USBRST)
            reset_intr_proc();

        if (sts & USB_DEV_GINTSTS_ENUMDNE) {
            usbc_intr_clear_gintsts_pending(USB_DEV_GINTSTS_ENUMDNE);
            return 1;
        }
    } while ((aic_get_time_ms() - start) < timeout);

    return 0;
}
