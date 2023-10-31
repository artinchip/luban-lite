#ifndef _USB_IO_REALTEK_H
#define _USB_IO_REALTEK_H

#include "customer_rtos_service.h"

#include <usb.h>
typedef struct USB_DATA_S{
	void *usb_intf;
}USB_DATA,*PUSB_DATA;
typedef  void (*usb_complete)(void *arg, unsigned int result);


typedef struct _USB_BUS_OPS{
	int (*register_drv)(struct usb_driver *driver);
	void (*deregister_drv)(struct usb_driver *driver);
	void (*usb_disendpoint)(u8 nr_endpoint,void *priv);
	int (*usb_ctrl_req)(void *priv,unsigned char bdir_in,unsigned short wvalue,unsigned char *buf,unsigned int len);
	int (*usb_get_speed_info)(void *priv);
	int (*usb_get_in_ep_info)(void *priv,unsigned char *pipe_num_array);
	int (*usb_get_out_ep_info)(void *priv,unsigned char *pipe_num_array);
	unsigned char(*usb_get_bulk_in_pipe)(void *priv,unsigned char ep_addr);
	unsigned char(*usb_get_bulk_out_pipe)(void *priv,unsigned char ep_addr);
	int (*usb_bulk_in)(void *priv,unsigned char pipe,unsigned char *buf,unsigned int len,usb_complete callback,void *arg);
	int (*usb_bulk_out)(void *priv,unsigned char pipe,unsigned char *buf,unsigned int len,usb_complete callback,void *arg);
	int (*usb_cancel_bulk_in)(void *priv);
	int (*usb_cancel_bulk_out)(void *priv);
}USB_BUS_OPS;


extern USB_BUS_OPS rtw_usb_bus_ops;
#endif
