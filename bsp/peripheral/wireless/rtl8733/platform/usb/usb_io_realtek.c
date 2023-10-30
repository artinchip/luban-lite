#include "usb_io_realtek.h"

#define REALTEK_USB_VENQT_READ		0xC0
#define REALTEK_USB_VENQT_WRITE		0x40
#define RTW_USB_CONTROL_MSG_TIMEOUT	500//ms

#define FW_START_ADDRESS	0x1000
#define VENDOR_CMD_MAX_DATA_LEN	254

#define ALIGNMENT_UNIT				32
#define MAX_VENDOR_REQ_CMD_SIZE	254		/* 8188cu SIE Support */
#define MAX_USB_IO_CTL_SIZE		(MAX_VENDOR_REQ_CMD_SIZE + ALIGNMENT_UNIT)

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef NULL
#define	NULL	0
#endif

#ifndef BIT
#define BIT(_x)	(1 << (_x))
#endif

struct rtw_urb
{
	struct urb urb;
	bool is_used;
};

#define MaxurbNum             40
static struct rtw_urb rtw_urb_read[MaxurbNum];
static struct rtw_urb rtw_urb_write[MaxurbNum];

int rtw_usb_rcvctrlpipe(struct usb_device *dev, u32 endpoint)
{
	return usb_rcvctrlpipe(dev,endpoint);
}
int rtw_usb_sndctrlpipe(struct usb_device *dev, u32 endpoint)
{
	return usb_sndctrlpipe(dev,endpoint);
}
int rtw_usb_get_bulk_in_pipe(void *priv,unsigned char ep_addr)
{	
	struct usb_interface *pusb_intf = (struct usb_interface *)priv;
	struct usb_device *udev = pusb_intf->usb_dev;
	return usb_rcvbulkpipe(udev,ep_addr);
}
int rtw_usb_get_bulk_out_pipe(void *priv,unsigned char ep_addr)
{
	struct usb_interface *pusb_intf = (struct usb_interface *)priv;
	struct usb_device *udev = pusb_intf->usb_dev;
	return usb_sndbulkpipe(udev,ep_addr);
}
struct urb * rtw_usb_alloc_urb(u16 iso_packets, int mem_flags)
{
	//return usb_alloc_urb(iso_packets,mem_flags);
	return usb_alloc_urb(iso_packets);  //xhl
}
void rtw_usb_free_urb(struct urb *urb)
{
	usb_free_urb(urb);
}
void rtw_usb_kill_urb(struct urb *urb)
{
	usb_kill_urb(urb);
}
int rtw_usb_submit_urb(struct urb *urb, int mem_flags)
{
//	return usb_submit_urb(urb, mem_flags);
	return usb_submit_urb(urb);  //xhl
}
void rtw_usb_fill_bulk_urb(struct urb *urb,struct usb_device *pDev,u32 Pipe,void *pTransferBuffer,s32 Length,usb_complete_t tCompleteFunc,void *pContext)
{
	usb_fill_bulk_urb(urb,pDev, Pipe,pTransferBuffer,Length, tCompleteFunc,pContext);
}
int rtw_usb_register(struct usb_driver *driver)
{
	return usb_register_driver(driver);
}
void rtw_usb_deregister(struct usb_driver *driver)
{
	usb_deregister(driver);
}

int usbctrl_vendorreq(void *priv,unsigned char bdir_in,unsigned short value,unsigned char *buf,unsigned int len)
{	
	struct usb_interface *pusb_intf = (struct usb_interface *)priv;
	struct usb_device *udev = pusb_intf->usb_dev;
	unsigned char request = 0x05;
	unsigned int pipe;
	int status = 0;
	unsigned char reqtype;
	unsigned char *pIo_buf;
	int vendorreq_times = 0;

	unsigned char tmp_buf[MAX_USB_IO_CTL_SIZE];

	/* Acquire IO memory for vendorreq */

	if (len > MAX_VENDOR_REQ_CMD_SIZE) {
		printf("[%s] Buffer len error ,vendor request failed\n", __FUNCTION__);
		status = -22;
		goto exit;
	}

	/* Added by Albert 2010/02/09 */
	/* For mstar platform, mstar suggests the address for USB IO should be 32 bytes alignment. */
	/* Trying to fix it here. */
	pIo_buf = tmp_buf + ALIGNMENT_UNIT - ((unsigned int)(tmp_buf) & 0x1f);//32字节对齐

	while (++vendorreq_times <= 10) {
		rtw_memset(pIo_buf, 0, len);

		if (bdir_in == 0x01) {
			pipe = usb_rcvctrlpipe(udev, 0);/* read_in */
			reqtype =  REALTEK_USB_VENQT_READ;		
		} else {
			pipe = usb_sndctrlpipe(udev, 0);/* write_out */
			reqtype =  REALTEK_USB_VENQT_WRITE;		
			rtw_memcpy(pIo_buf, buf, len);
		}
		status = usb_control_msg(udev, pipe, request, reqtype, value, 0, pIo_buf, len, RTW_USB_CONTROL_MSG_TIMEOUT);

		if (status == len) {  /* Success this control transfer. */
			if (bdir_in == 0x01) {
				/* For Control read transfer, we have to copy the read data from pIo_buf to pdata. */
				rtw_memcpy(buf, pIo_buf, len);
			}
		} else { /* error cases */
			printf("reg 0x%x, usb %s %u fail, status:%d value=0x%x, vendorreq_times:%d\n"
				, value, (bdir_in == 0x01) ? "read" : "write" , len, status, *(u32 *)buf, vendorreq_times);
		}
		/* firmware download is checksumed, don't retry */
		if ((value >= FW_START_ADDRESS) || status == len)
			break;
	}
exit:
	return status;

}

extern void usb_read_port_complete(void *arg, unsigned int  result);
static int rtw_usb_read_complete(struct urb* purb)
{	
	int i =0;
	usb_read_port_complete(purb->context, purb->actual_length);	
	if (purb->status != 0){
		printf("\r\n###=> rtw_usb_read_port_complete => urb.status(%d)\n", purb->status);
	}
	for(;i<MaxurbNum;i++){
		if(&(rtw_urb_read[i].urb) == purb){
			rtw_memset(purb, 0, sizeof(rtw_urb_read[i].urb));
			rtw_urb_read[i].is_used = 0;
			break;
		}
	}
	if(i ==MaxurbNum ){
		printf("\r\n Error: some urb pointer we want read have not recored!");
	}
	usb_free_urb(purb);
	return purb->status;
}

static int rtw_usb_bulk_in(void *priv,unsigned char pipe,unsigned char *buf,unsigned int len,usb_complete_t callback,void *arg){
	struct usb_interface *pusb_intf = (struct usb_interface *)priv;
	struct usb_device *udev = pusb_intf->usb_dev;
	int status;
	int i = 0;
	struct urb *purb;
	for(;i<MaxurbNum;i++){
		if(!(rtw_urb_read[i].is_used)){
			rtw_urb_read[i].is_used = 1;	//set the used to 1 first then use it
			purb = &rtw_urb_read[i].urb;
			rtw_memset(purb, 0, sizeof(rtw_urb_read[i].urb));
			break;
		}
	}
	if(i == MaxurbNum)
		printf("\r\n Error: something error in urb allocate in %s",__func__);
	
	usb_fill_bulk_urb(purb,
                     udev,
                     pipe,
                     buf,
                     len,
                     rtw_usb_read_complete,
                     arg);
	status = usb_submit_urb(purb);
	return status;
}

extern void usb_write_port_complete(void *arg, unsigned int  result);
static int rtw_usb_write_complete(struct urb* purb)
{		
	int i = 0;
	if (purb->status == 0){
		usb_write_port_complete(purb->context, purb->actual_length);	
	}else{
		printf("\r\n###=> rtw_usb_write_port_complete => urb.status(%d)\n", purb->status);
	}
	for(;i<MaxurbNum;i++){
		if(&(rtw_urb_write[i].urb) == purb){
			rtw_memset(purb, 0, sizeof(rtw_urb_write[i].urb));
			rtw_urb_write[i].is_used = 0;
			break;
		}
	}
	if(i ==MaxurbNum ){
		printf("\r\n Error: some urb pointer we want write have not recored!");
	}
	usb_free_urb(purb);
	return purb->status;
}

static int rtw_usb_bulk_out(void *priv,unsigned char pipe,unsigned char *buf,unsigned int len,usb_complete callback,void *arg){
	struct usb_interface *pusb_intf = (struct usb_interface *)priv;
	struct usb_device *udev = pusb_intf->usb_dev;
	int status;
	int i = 0;
	struct urb *purb;
	for(;i<MaxurbNum;i++){
		if(!(rtw_urb_write[i].is_used)){
			rtw_urb_write[i].is_used = 1;	//set the used to 1 first then use it
			purb = &(rtw_urb_write[i].urb);
			rtw_memset(purb, 0, sizeof(rtw_urb_write[i].urb));
			break;
		}
	}
	if(i == MaxurbNum)
		printf("\r\n Error: something error in urb allocate in %s",__func__);
	usb_fill_bulk_urb(purb,
                     udev,
                     pipe,
                     buf,
                     len,
                     rtw_usb_write_complete,
                     arg);
	status = usb_submit_urb(purb);
	return status;

}

static int rtw_usb_cancel_bulk_out(void *priv)
{
	struct usb_interface *pusb_intf = (struct usb_interface *)priv;
	int i = 0;
	for(;i<MaxurbNum;i++){
		if(rtw_urb_write[i].is_used){
			usb_kill_urb(&(rtw_urb_write[i].urb));
			rtw_memset(&(rtw_urb_write[i].urb), 0, sizeof(rtw_urb_write[i].urb));
			rtw_urb_write[i].is_used = 0;
		}
	}
	return 0;
}

static int rtw_usb_cancel_bulk_in(void *priv)
{
	struct usb_interface *pusb_intf = (struct usb_interface *)priv;
	int i = 0;
	for(;i<MaxurbNum;i++){
		if(rtw_urb_read[i].is_used){
			usb_kill_urb(&(rtw_urb_read[i].urb));
			rtw_memset(&(rtw_urb_read[i].urb), 0, sizeof(rtw_urb_read[i].urb));
			rtw_urb_read[i].is_used = 0;
		}
	}
	return 0;
}

static int rtw_usb_ctrl_req(void *priv,unsigned char bdir_in,unsigned short wvalue,unsigned char *buf,unsigned int len)
{
	return usbctrl_vendorreq(priv,bdir_in,wvalue,buf,len);
}

enum RTW_USB_SPEED {
	RTW_USB_SPEED_UNKNOWN	= 0,
	RTW_USB_SPEED_1_1	= 1,
	RTW_USB_SPEED_2		= 2,
	RTW_USB_SPEED_3		= 3,
};

static inline int RT_usb_endpoint_dir_in(const struct usb_endpoint_descriptor *epd)
{
	return (epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN;
}

static inline int RT_usb_endpoint_dir_out(const struct usb_endpoint_descriptor *epd)
{
	return (epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT;
}

static inline int RT_usb_endpoint_xfer_int(const struct usb_endpoint_descriptor *epd)
{
	return (epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT;
}

static inline int RT_usb_endpoint_xfer_bulk(const struct usb_endpoint_descriptor *epd)
{
	return (epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK;
}

static inline int RT_usb_endpoint_is_bulk_in(const struct usb_endpoint_descriptor *epd)
{
	return RT_usb_endpoint_xfer_bulk(epd) && RT_usb_endpoint_dir_in(epd);
}

static inline int RT_usb_endpoint_is_bulk_out(const struct usb_endpoint_descriptor *epd)
{
	return RT_usb_endpoint_xfer_bulk(epd) && RT_usb_endpoint_dir_out(epd);
}

static inline u8 RT_usb_endpoint_is_int_in(const struct usb_endpoint_descriptor *epd)
{
	return RT_usb_endpoint_xfer_int(epd) && RT_usb_endpoint_dir_in(epd);
}

static inline u8 RT_usb_endpoint_num(const struct usb_endpoint_descriptor *epd)
{	
	return epd->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
}


void rtw_usb_disendpoint(u8 nr_endpoint,void *priv)
{
	struct usb_interface *pusb_intf = (struct usb_interface *)priv;
	struct usb_device *pusbd = pusb_intf->usb_dev;
	struct usb_host_interface *phost_iface;
	struct usb_host_endpoint *phost_endp;
	int i;
	phost_iface = &pusb_intf->altsetting[0];
	for (i = 0; i < nr_endpoint; i++) {
		phost_endp = phost_iface->endpoint + i;
		if (phost_endp) {
			usb_hcd_disable_endpoint(pusbd, phost_endp);
		}
	}
	pusb_intf->user_data = NULL;
}


static int rtw_usb_get_speed_info(void *priv)
{	
	struct usb_interface *pusb_intf = (struct usb_interface *)priv;
	struct usb_device  *pusbd;
	pusbd = pusb_intf->usb_dev;
	int speed;
	switch (pusbd->speed) {
	case 1:
		 printf("Low Speed Case \n");
		 (speed) = RTW_USB_SPEED_1_1;//64bit
		 break;
	case 2:
		 printf("full speed Case \n");
		 (speed) = RTW_USB_SPEED_1_1;//64bit
		 break;
	case 3:
		 printf("high speed Case \n");//512bit
		 (speed) = RTW_USB_SPEED_2;
		 break;
	default:
		(speed) = RTW_USB_SPEED_UNKNOWN;
		break;
	}
	if ((speed) == RTW_USB_SPEED_UNKNOWN) 
		pusb_intf->user_data = NULL;
	return speed;
}

static int rtw_usb_get_in_ep_info(void *priv,unsigned char *pipe_num_arrays)
{
	struct usb_interface *pusb_intf = (struct usb_interface *)priv;
	struct usb_host_endpoint		*phost_endp;
	struct usb_endpoint_descriptor	*pendp_desc;
	struct usb_host_interface		*phost_iface;
	struct usb_interface_descriptor	*piface_desc;
	int NumInPipes = 0;
	int nr_endpoint;
	int i;
	phost_iface = &pusb_intf->cur_altsetting[0];
	piface_desc = &phost_iface->desc;
	nr_endpoint = piface_desc->bNumEndpoints;
	for (i = 0; i < (nr_endpoint); i++) {
		phost_endp = phost_iface->endpoint + i;
		if (phost_endp) {
			pendp_desc = &phost_endp->desc;
			printf("\nusb_endpoint_descriptor(%d):\n", i);
			printf("bLength=%x\n", pendp_desc->bLength);
			printf("bDescriptorType=%x\n", pendp_desc->bDescriptorType);
			printf("bEndpointAddress=%x\n", pendp_desc->bEndpointAddress);
			printf("wMaxPacketSize=%d\n", pendp_desc->wMaxPacketSize);
			printf("bInterval=%x\n", pendp_desc->bInterval);
			if (RT_usb_endpoint_is_bulk_in(pendp_desc)) {
				pipe_num_arrays[NumInPipes] = RT_usb_endpoint_num(pendp_desc);
				(NumInPipes)++;
			} else if (RT_usb_endpoint_is_int_in(pendp_desc)) {
				printf("\r\n RT_usb_endpoint_is_int_in = %x, Interval = %x", RT_usb_endpoint_num(pendp_desc), pendp_desc->bInterval);
				pipe_num_arrays[NumInPipes] = RT_usb_endpoint_num(pendp_desc);
				(NumInPipes)++;
			} 
		}
	}
	return NumInPipes;
}
static int rtw_usb_get_out_ep_info(void *priv,u8 *pipe_num_array)
{
	struct usb_interface *pusb_intf = (struct usb_interface *)priv;
	struct usb_host_endpoint		*phost_endp;
	struct usb_endpoint_descriptor	*pendp_desc;
	struct usb_host_interface		*phost_iface;
	struct usb_interface_descriptor	*piface_desc;
	int NumOutPipes = 0;
	int nr_endpoint;
	int i;
	phost_iface = &pusb_intf->cur_altsetting[0];
	piface_desc = &phost_iface->desc;
	nr_endpoint = piface_desc->bNumEndpoints;
	for (i = 0; i < (nr_endpoint); i++) {
		phost_endp = phost_iface->endpoint + i;
		if (phost_endp) {
			pendp_desc = &phost_endp->desc;
			if (RT_usb_endpoint_is_bulk_out(pendp_desc)) {
				printf("\r\n RT_usb_endpoint_is_bulk_out = %x\n", RT_usb_endpoint_num(pendp_desc));
				pipe_num_array[NumOutPipes] = RT_usb_endpoint_num(pendp_desc);
				(NumOutPipes)++;
			}
		}
	}
	return NumOutPipes;
}


extern USB_BUS_OPS rtw_usb_bus_ops= {
	rtw_usb_register,
	rtw_usb_deregister,
	rtw_usb_disendpoint,
	rtw_usb_ctrl_req,
	rtw_usb_get_speed_info,
	rtw_usb_get_in_ep_info,
	rtw_usb_get_out_ep_info,
	rtw_usb_get_bulk_in_pipe,
	rtw_usb_get_bulk_out_pipe,
	rtw_usb_bulk_in,
	rtw_usb_bulk_out,
	rtw_usb_cancel_bulk_in,
	rtw_usb_cancel_bulk_out,
};

