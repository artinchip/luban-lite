/*
 * Copyright (c) 2020-2022 Artinchip Technology Co., Ltd. All rights reserved.
 *
 * Dehuang Wu <dehuang.wu@artinchip.com>
 */

#ifndef _AIC_USB_HOST_H_
#define _AIC_USB_HOST_H_

int usbh_initialize(int id);
int usbh_get_connect_id(void);
u32 usbh_msc_read(u32 start_sector, u32 nsect, u8 *buffer);

#endif /* _AIC_USB_HOST_H_ */
