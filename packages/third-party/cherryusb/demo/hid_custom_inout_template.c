/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>
#include <fcntl.h>

#include "usbd_core.h"
#include "usbd_hid.h"
#ifdef LPKG_MPP
#include "artinchip_fb.h"
#endif
#ifdef AIC_MPP_VIN
#include "mpp_vin_vb.h"

/* Support two mode:
   1. Receive HID data to a DRAM buffer, then call decode_pic()
   2. Receive HID data to a flash file, then call player_demo cmd
*/

#define HID_RECV_VIDEO_FILE
#define HID_FILE_PATH       "/sdcard"
#define HID_FILE_FOR_SAVE   HID_FILE_PATH"/test_video.mp4"
#define HID_FILE_FOR_PLAY   HID_FILE_PATH"/video_for_player.mp4"

#endif

#define HID_RECV_BUF_SIZE  (1024 * 1024 + CONFIG_USB_ALIGN_SIZE)

/*!< hidraw in endpoint */
#define HIDRAW_IN_EP       0x81
#define HIDRAW_IN_EP_SIZE  1024
#define HIDRAW_IN_INTERVAL 16

/*!< hidraw out endpoint */
#define HIDRAW_OUT_EP          0x02
#define HIDRAW_OUT_EP_SIZE     1024
#define HIDRAW_OUT_EP_INTERVAL 2

#define USBD_VID           0x33C3
#define USBD_PID           0x6780
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

/*!< config descriptor size */
#define USB_HID_CONFIG_DESC_SIZ (9 + 9 + 9 + 7 + 7)

/*!< custom hid report descriptor size */
#define HID_CUSTOM_REPORT_DESC_SIZE 38

/*!< global descriptor */
static const uint8_t hid_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0002, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_HID_CONFIG_DESC_SIZ, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    /************** Descriptor of Custom interface *****************/
    0x09,                          /* bLength: Interface Descriptor size */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType: Interface descriptor type */
    0x00,                          /* bInterfaceNumber: Number of Interface */
    0x00,                          /* bAlternateSetting: Alternate setting */
    0x02,                          /* bNumEndpoints */
    0x03,                          /* bInterfaceClass: HID */
    0x00,                          /* bInterfaceSubClass : 1=BOOT, 0=no boot */
    0x00,                          /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
    0,                             /* iInterface: Index of string descriptor */
    /******************** Descriptor of Custom HID ********************/
    0x09,                    /* bLength: HID Descriptor size */
    HID_DESCRIPTOR_TYPE_HID, /* bDescriptorType: HID */
    0x00,                    /* bcdHID: HID Class Spec release number */
    0x02,
    0x00,                        /* bCountryCode: Hardware target country */
    0x01,                        /* bNumDescriptors: Number of HID class descriptors to follow */
    0x22,                        /* bDescriptorType */
    HID_CUSTOM_REPORT_DESC_SIZE, /* wItemLength: Total length of Report descriptor */
    0x00,
    /******************** Descriptor of Custom in endpoint ********************/
    0x07,                         /* bLength: Endpoint Descriptor size */
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType: */
    HIDRAW_IN_EP,                 /* bEndpointAddress: Endpoint Address (IN) */
    0x03,                         /* bmAttributes: Interrupt endpoint */
    WBVAL(HIDRAW_IN_EP_SIZE),        /* wMaxPacketSize: 4 Byte max */
    HIDRAW_IN_INTERVAL,           /* bInterval: Polling Interval */
    /******************** Descriptor of Custom out endpoint ********************/
    0x07,                         /* bLength: Endpoint Descriptor size */
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType: */
    HIDRAW_OUT_EP,                /* bEndpointAddress: Endpoint Address (IN) */
    0x03,                         /* bmAttributes: Interrupt endpoint */
    WBVAL(HIDRAW_OUT_EP_SIZE),    /* wMaxPacketSize: 4 Byte max */
    HIDRAW_OUT_EP_INTERVAL,       /* bInterval: Polling Interval */
    /* 73 */
    /*
     * string0 descriptor
     */
    USB_LANGID_INIT(USBD_LANGID_STRING),
    /*
     * string1 descriptor
     */
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    /*
     * string2 descriptor
     */
    0x26,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'H', 0x00,                  /* wcChar10 */
    'I', 0x00,                  /* wcChar11 */
    'D', 0x00,                  /* wcChar12 */
    ' ', 0x00,                  /* wcChar13 */
    'D', 0x00,                  /* wcChar14 */
    'E', 0x00,                  /* wcChar15 */
    'M', 0x00,                  /* wcChar16 */
    'O', 0x00,                  /* wcChar17 */
    /*
     * string3 descriptor
     */
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '2', 0x00,                  /* wcChar3 */
    '1', 0x00,                  /* wcChar4 */
    '2', 0x00,                  /* wcChar5 */
    '3', 0x00,                  /* wcChar6 */
    '4', 0x00,                  /* wcChar7 */
    '5', 0x00,                  /* wcChar8 */
    '6', 0x00,                  /* wcChar9 */
#ifdef CONFIG_USB_HS
    /*
     * device qualifier descriptor
     */
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};

/*!< custom hid report descriptor */
static const uint8_t hid_custom_report_desc[HID_CUSTOM_REPORT_DESC_SIZE] = {
    /* USER CODE BEGIN 0 */
    0x06, 0x00, 0xff, /* USAGE_PAGE (Vendor Defined Page 1) */
    0x09, 0x01,       /* USAGE (Vendor Usage 1) */
    0xa1, 0x01,       /* COLLECTION (Application) */
    0x85, 0x02,       /*   REPORT ID (0x02) */
    0x09, 0x01,       /*   USAGE (Vendor Usage 1) */
    0x15, 0x00,       /*   LOGICAL_MINIMUM (0) */
    0x26, 0xff, 0x00, /*   LOGICAL_MAXIMUM (255) */
    0x95, 0x40 - 1,   /*   REPORT_COUNT (63) */
    0x75, 0x08,       /*   REPORT_SIZE (8) */
    0x81, 0x02,       /*   INPUT (Data,Var,Abs) */
    /* <___________________________________________________> */
    0x85, 0x01,       /*   REPORT ID (0x01) */
    0x09, 0x01,       /*   USAGE (Vendor Usage 1) */
    0x15, 0x00,       /*   LOGICAL_MINIMUM (0) */
    0x26, 0xff, 0x00, /*   LOGICAL_MAXIMUM (255) */
    0x95, 0x40 - 1,   /*   REPORT_COUNT (63) */
    0x75, 0x08,       /*   REPORT_SIZE (8) */
    0x91, 0x02,       /*   OUTPUT (Data,Var,Abs) */
    /* USER CODE END 0 */
    0xC0 /*     END_COLLECTION	             */
};

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[HIDRAW_OUT_EP_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t send_buffer[HIDRAW_IN_EP_SIZE];

#define HID_STATE_IDLE 0
#define HID_STATE_BUSY 1

/*!< hid state ! Data can be sent only when state is idle  */
static volatile uint8_t custom_state;

static uint8_t *g_hid_buf_poll = NULL;
static uint8_t *g_hid_img_buf = NULL;
static uint32_t g_hid_wr_pos = 0;
static uint32_t g_hid_img_size = 0;
static uint32_t g_hid_need_wr_file = 0;
static rt_sem_t g_hid_sem = RT_NULL;
static uint32_t g_hid_only_recv = 0;

#ifdef HID_RECV_VIDEO_FILE

static rt_sem_t g_hid_play_sem = RT_NULL;

#define HID_VBUF_PLANE_NUM      1
#define HID_VBUF_PLANE_SIZE     (32 * 1024)

struct hid_vbuf {
    struct vin_video_buf vbuf;
    unsigned int         buf_len[HID_RECV_BUF_SIZE / HID_VBUF_PLANE_SIZE];
    struct vb_queue      queue;
    struct list_head     active_list;
    aicos_mutex_t        active_lock; /* lock of active buf list */
    unsigned int         sequence;
    unsigned int         streaming;
};
static struct hid_vbuf g_hid_vb = {0};

extern void player_demo_stop(void);
extern void copy(const char *src, const char *dst);
#endif // end of #ifdef HID_RECV_VIDEO_FILE

void usbd_event_handler(uint8_t event)
{
    switch (event) {
        case USBD_EVENT_RESET:
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_CONFIGURED:
            /* setup first out ep read transfer */
            usbd_ep_start_read(HIDRAW_OUT_EP, read_buffer, HIDRAW_OUT_EP_SIZE);
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

#define US_PER_SEC      1000000

static void show_speed(u64 start, u64 end, int bytes)
{
    u64 diff = end - start;

    printf("HID OUT Speed: %d bytes, %d.%03d seconds, %d KB/s\n",
           bytes, (u32)diff / US_PER_SEC, (u32)(diff % US_PER_SEC) / 1000,
           (u32)((bytes / 1024) * (US_PER_SEC / 1000) / (diff / 1000)));
}

static void notify_file_decode(void)
{
    printf("\nThe follow command can decode the image file to FB: \n");
    printf("pic_test -a 0x%lx -z %d\n", (long)g_hid_img_buf, g_hid_img_size);
    if (!g_hid_only_recv)
        rt_sem_release(g_hid_sem);
}

static void parse_file_type(uint8_t *data)
{
    if (data[0] == 0xff && data[1] == 0xd8)
        g_hid_need_wr_file = 0;
    else if (data[1] == 'P' && data[2] == 'N' && data[3] == 'G')
        g_hid_need_wr_file = 0;
    else
        g_hid_need_wr_file = 1;
}

#ifdef HID_RECV_VIDEO_FILE

static void hid_buf_reload(struct vb_buffer *buf)
{
    pr_debug("Select %d buf 0x%x\n", buf->index, (long)buf->planes[0].buf);
    buf->hw_using = 1;
    g_hid_img_buf = (uint8_t *)(ptr_t)buf->planes[0].buf;
    g_hid_wr_pos = 0;
}

static int hid_buf_done(void)
{
    struct vb_buffer *cur_buf;

    if (list_empty(&g_hid_vb.active_list)) {
        pr_err("No buf available!\n");
        return 0;
    }

    cur_buf = list_first_entry(&g_hid_vb.active_list, struct vb_buffer, active_entry);
    pr_debug("cur: index %d, hid_using %d\n",
             cur_buf->index, cur_buf->hw_using);

    /* If cur_buf is a new one queued, HID should use it first */
    if (!cur_buf->hw_using) {
        pr_debug("Good! Buf %d is free again\n", cur_buf->index);
        hid_buf_reload(cur_buf);
        return 0;
    }

    /* Release the current buffer from HID */
    list_del(&cur_buf->active_entry);
    vin_vb_buffer_done(cur_buf, VB_BUF_STATE_DONE);
    cur_buf->hw_using = 0;
    g_hid_vb.buf_len[cur_buf->index] = g_hid_img_size;
    return 0;
}

static int hid_buf_update(void)
{
    struct vb_buffer *cur_buf;
    struct vb_buffer *next_buf;

    if (!g_hid_vb.streaming)
        return 0;

    if (list_empty(&g_hid_vb.active_list)) {
        pr_warn("No buf available!\n");
        return -1;
    }

    cur_buf = list_first_entry(&g_hid_vb.active_list, struct vb_buffer, active_entry);
    pr_debug("cur: index %d, hw_using %d\n", cur_buf->index, cur_buf->hw_using);

    if (!cur_buf->hw_using) {
        hid_buf_reload(cur_buf);
        g_hid_vb.sequence++;
        return 0;
    }

    if (cur_buf == list_last_entry(&g_hid_vb.active_list, struct vb_buffer,
                                   active_entry)) {
        pr_warn("It's the last buf!\n");
        return 0;
    }

    next_buf = list_next_entry(cur_buf, active_entry);
    if (!next_buf) {
        pr_err("Next buf is invalid\n");
        return -1;
    }
    pr_debug("Next: index %d, hw_using %d\n",
             next_buf->index, next_buf->hw_using);

    /* HID can use the next buf as output. */
    if (!next_buf->hw_using) {
        hid_buf_reload(next_buf);
        g_hid_vb.sequence++;
    } else {
        /* This should not happened! */
        pr_warn("Weird! HID is using two buf %d & %d!\n",
                cur_buf->index, next_buf->index);
        return -1;
    }

    return 0;
}
#endif // end of #ifdef HID_RECV_VIDEO_FILE

static void dump_to_buf(uint8_t *data, uint32_t len)
{
    static u64 begin = 0, end = 0, total_len = 0;
    int max_len = HID_RECV_BUF_SIZE;

    if (!data || len > HIDRAW_OUT_EP_SIZE || !len) {
        pr_err("Invalid data: addr 0x%x, len %d\n", (uint32_t)(long)data, len);
        return;
    }

    if (g_hid_wr_pos == 0) {
        if (!begin) {
            begin = aic_get_time_us();
            total_len = 0;
        }
        parse_file_type(data);

#ifdef HID_RECV_VIDEO_FILE
        if (!g_hid_vb.active_lock) {
            pr_err("Must init HID vb first!\n");
            return;
        }

        if (g_hid_need_wr_file) {
            /* Need switch to the next buf */
            if (g_hid_img_size) { /* Be sure it's not the first pkt */
                aicos_mutex_take(g_hid_vb.active_lock, AICOS_WAIT_FOREVER);
                hid_buf_done();
                hid_buf_update();
                aicos_mutex_give(g_hid_vb.active_lock);
            }
            max_len = HID_VBUF_PLANE_SIZE;
        }
#endif
        g_hid_img_size = 0;
    }

    if (g_hid_wr_pos + len > max_len) {
        pr_err("The data length %d is too long\n", len);
        g_hid_wr_pos = 0;
    }

    memcpy(&g_hid_img_buf[g_hid_wr_pos], data, len);
    g_hid_wr_pos += len;
    g_hid_img_size += len;
    total_len += len;

#ifdef HID_RECV_VIDEO_FILE
    // pr_debug("Recv %d, buf 0x%08x, pos %d, sequence %d, img_size %d\n",
    //          len, g_hid_img_buf, g_hid_wr_pos,
    //          g_hid_vb.sequence, g_hid_img_size);

    if (g_hid_need_wr_file && g_hid_img_size == HID_VBUF_PLANE_SIZE) {
        g_hid_wr_pos = 0;
        return;
    }
#endif

    /* Consider the file end if length < EP_SIZE */
    if (len < HIDRAW_OUT_EP_SIZE) {
        end = aic_get_time_us();
        show_speed(begin, end, total_len);
        pr_info("Receive a whole file, last buf size %d, total size %d\n",
                g_hid_img_size, total_len);

#ifdef HID_RECV_VIDEO_FILE
        hid_buf_done();
        hid_buf_update();
#endif
        if (!g_hid_need_wr_file)
            notify_file_decode();

        if (g_hid_need_wr_file)
            g_hid_img_size = 0;
        g_hid_wr_pos = 0;
        g_hid_need_wr_file = 0;
        begin = 0;
    }
}

static void usbd_hid_custom_in_callback(uint8_t ep, uint32_t nbytes)
{
    usbd_ep_start_write(HIDRAW_IN_EP, send_buffer, nbytes);
    custom_state = HID_STATE_IDLE;
}

static void usbd_hid_custom_out_callback(uint8_t ep, uint32_t nbytes)
{
    if (nbytes)
        dump_to_buf(read_buffer, nbytes);

    usbd_ep_start_read(HIDRAW_OUT_EP, read_buffer, HIDRAW_OUT_EP_SIZE);
}

static struct usbd_endpoint custom_in_ep = {
    .ep_cb = usbd_hid_custom_in_callback,
    .ep_addr = HIDRAW_IN_EP
};

static struct usbd_endpoint custom_out_ep = {
    .ep_cb = usbd_hid_custom_out_callback,
    .ep_addr = HIDRAW_OUT_EP
};

/* function ------------------------------------------------------------------*/
/**
 * @brief            hid custom init
 * @pre              none
 * @param[in]        none
 * @retval           none
 */
struct usbd_interface intf0;

void hid_custom_init(void)
{
    usbd_desc_register(hid_descriptor);
    usbd_add_interface(usbd_hid_init_intf(&intf0, hid_custom_report_desc, HID_CUSTOM_REPORT_DESC_SIZE));
    usbd_add_endpoint(&custom_in_ep);
    usbd_add_endpoint(&custom_out_ep);

    usbd_initialize();
}

#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#include <rtdevice.h>

int usbd_hid_custom_init(void)
{
    g_hid_buf_poll = aicos_malloc(MEM_CMA, HID_RECV_BUF_SIZE);
    if (!g_hid_buf_poll) {
        pr_err("Failed to malloc(%d) CMA buffer.\n", HID_RECV_BUF_SIZE);
        return -1;
    }
    if ((long)g_hid_buf_poll % CONFIG_USB_ALIGN_SIZE)
        g_hid_buf_poll += CONFIG_USB_ALIGN_SIZE -
                        (long)g_hid_buf_poll % CONFIG_USB_ALIGN_SIZE;

    g_hid_sem = rt_sem_create("hid_recv", 0, RT_IPC_FLAG_FIFO);
    pr_info("HID recv buf: 0x%lx\n", (long)g_hid_buf_poll);
    g_hid_img_buf = g_hid_buf_poll;

#ifdef HID_RECV_VIDEO_FILE
    g_hid_play_sem = rt_sem_create("hid_play", 0, RT_IPC_FLAG_FIFO);
#endif

    hid_custom_init();
    return 0;
}
INIT_DEVICE_EXPORT(usbd_hid_custom_init);

#if defined(RT_USING_FINSH)
#include <finsh.h>
#include <msh.h>

void ui_alpha_config(u32 val);
void video_layer_init(void);
void video_layer_deinit(void);
int decode_pic(uint8_t* pic, int len, u32 offset_x, u32 offset_y,
               u32 width, u32 height, u32 layer_id);

#ifdef HID_RECV_VIDEO_FILE
static void hid_vb_buf_queue(struct vb_buffer *vb)
{
    aicos_mutex_take(g_hid_vb.active_lock, AICOS_WAIT_FOREVER);
    list_add_tail(&vb->active_entry, &g_hid_vb.active_list);
    aicos_mutex_give(g_hid_vb.active_lock);
    vb->hw_using = 0;
}

static int hid_vb_start_streaming(struct vb_queue *q)
{
    struct vb_buffer *vb;

    pr_debug("Start streaming\n");

    g_hid_vb.sequence = 0;
    aicos_mutex_take(g_hid_vb.active_lock, AICOS_WAIT_FOREVER);

    /* Prepare active_buffers for HID recv*/
    vb = list_first_entry(&g_hid_vb.active_list, struct vb_buffer, active_entry);
    hid_buf_reload(vb);

    aicos_mutex_give(g_hid_vb.active_lock);
    g_hid_vb.streaming = 1;
    return 0;
}

static void hid_vb_reclaim_all_buffers(enum vb_buffer_state state)
{
    struct vb_buffer *vb, *node;

    aicos_mutex_take(g_hid_vb.active_lock, AICOS_WAIT_FOREVER);
    list_for_each_entry_safe(vb, node, &g_hid_vb.active_list, active_entry) {
        vin_vb_buffer_done(vb, state);
        list_del(&vb->active_entry);
    }
    aicos_mutex_give(g_hid_vb.active_lock);
}

static void hid_vb_stop_streaming(struct vb_queue *q)
{
    pr_debug("Stopping capture\n");

    /* Release all active buffers */
    aicos_mutex_take(g_hid_vb.active_lock, AICOS_WAIT_FOREVER);
    hid_vb_reclaim_all_buffers(VB_BUF_STATE_ERROR);
    aicos_mutex_give(g_hid_vb.active_lock);
    g_hid_vb.streaming = 0;
}

static const struct vb_ops hid_vb_ops = {
    .buf_queue          = hid_vb_buf_queue,
    .start_streaming    = hid_vb_start_streaming,
    .stop_streaming     = hid_vb_stop_streaming,
};

static int hid_vb_init(void)
{
    struct vin_video_buf *vbuf = &g_hid_vb.vbuf;
    int i, ret = 0;

    g_hid_vb.active_lock = aicos_mutex_create();
    INIT_LIST_HEAD(&g_hid_vb.active_list);
    vin_vb_init(&g_hid_vb.queue, &hid_vb_ops);

    vbuf->num_planes   = HID_VBUF_PLANE_NUM;
    vbuf->planes[0].len = HID_VBUF_PLANE_SIZE;
    ret = vin_vb_req_buf(&g_hid_vb.queue,
                         (char *)g_hid_buf_poll, HID_RECV_BUF_SIZE, vbuf);
    if (ret < 0)
        return -1;

    printf("Prepare %d * %d * %d buffer for HID\n", HID_VBUF_PLANE_SIZE,
           vbuf->num_buffers, vbuf->num_planes);

    printf("Buf      Plane     size\n");
    for (i = 0; i < vbuf->num_buffers; i++) {
        printf("%3d 0x%08x %8d\n", i,
               vbuf->planes[i * vbuf->num_planes].buf,
               vbuf->planes[i * vbuf->num_planes].len);
    }

    for (i = 0; i < vbuf->num_buffers; i++) {
        if (vin_vb_q_buf(&g_hid_vb.queue, i) < 0)
            return -1;
    }

    vin_vb_stream_on(&g_hid_vb.queue);
    return 0;
}

static void hid_player_video(char *filename, u32 rotation_angel)
{
    char cmd[64] = "";

    snprintf(cmd, 64, "player_demo -i %s -r %d -s \n",
             filename, rotation_angel);
    printf("\nTry to decode the file as a H264 video ...\n%s\n", cmd);
    if (!g_hid_only_recv)
        msh_exec(cmd, strlen(cmd));
}

#endif // end of #ifdef HID_RECV_VIDEO_FILE

/* Decode and display a image or video file */
static void hid_dec_thread(void *arg)
{
    uint32_t cnt = 0;
    uint32_t max = *(uint32_t *)arg;
#ifdef HID_RECV_VIDEO_FILE
    u32 index = 0, len = 0;
    int ret = 0, fd = -1;
    char *data = NULL;
#else
    uint32_t layer_id = AICFB_LAYER_TYPE_UI;
    u32 offset_x, offset_y, width, height;
#endif

    ui_alpha_config(0x40);
    video_layer_init();
#ifdef HID_RECV_VIDEO_FILE
    if (hid_vb_init())
        return;
#endif

    while (cnt < max) {
#ifdef HID_RECV_VIDEO_FILE
        /* 1. Wait for a ready buffer */
        ret = vin_vb_dq_buf(&g_hid_vb.queue, &index);
        if (ret < 0) {
            // pr_info("DQ buf is unavailable, keep waiting ...\n");
            continue;
        }

        /* 2. Write the buffer to a file */
        if (fd < 0) { /* Need create a new file */
           pr_info("Create a new file: %s\n", HID_FILE_FOR_SAVE);
           fd = open(HID_FILE_FOR_SAVE, O_CREAT|O_WRONLY|O_BINARY);
           if (fd < 0) {
               pr_err("Failed to open(%s)\n", HID_FILE_FOR_SAVE);
               return;
           }
        }
        data = (char *)(ptr_t)g_hid_vb.vbuf.planes[index].buf;
        len  = g_hid_vb.buf_len[index];
        // pr_info("DQ buf %d, addr 0x%08x, len %d -> fd %d\n", index, data, len, fd);
        write(fd, data, len);
        if (len < HID_VBUF_PLANE_SIZE) { /* Consider the case as the end of file */
            close(fd);
            fd = -1;
            cnt++;

            player_demo_stop();
            pr_info("Copy %s to %s\n", HID_FILE_FOR_SAVE, HID_FILE_FOR_PLAY);
            copy(HID_FILE_FOR_SAVE, HID_FILE_FOR_PLAY);
            aicos_msleep(1000);
            rt_sem_release(g_hid_play_sem);
        }

        /* 3. Release/Queue the buffer */
        vin_vb_q_buf(&g_hid_vb.queue, index);
#else
        printf("\nWaiting for %d/%d file ...\n", cnt + 1, max);
        if (rt_sem_take(g_hid_sem, RT_WAITING_FOREVER) != RT_EOK)
            break;

        printf("Try to decode and display the file ...\n");
        if (!g_hid_need_wr_file) {
            if (cnt % 5 == 0) {
                layer_id = AICFB_LAYER_TYPE_VIDEO;
                offset_x = 0;
                offset_y = 0;
                width    = 0;
                height   = 0;
                printf("\tUpdate the video layer\n");
            } else {
                layer_id = AICFB_LAYER_TYPE_UI;
                offset_x = 100 + (cnt - 1) % 5 * 50;
                offset_y = 40 + (cnt - 1) % 5 * 50;
                width = 0;
                height = 0;
            }
            printf("Layer %d: Offset (%d, %d), Size %d x %d\n", layer_id,
                   offset_x, offset_y, width, height);
            decode_pic(g_hid_img_buf, g_hid_img_size, offset_x, offset_y,
                       width, height, layer_id);
        }
        cnt++;
#endif

    }
    video_layer_deinit();
    pr_info("Received %d files, then Exit\n", cnt);
}

static void hid_player_thread(void *arg)
{
#ifdef HID_RECV_VIDEO_FILE
    u32 rotation_angel = 0;

    while (1) {
        pr_info("\nWaiting for video file ready...\n");
        if (rt_sem_take(g_hid_play_sem, RT_WAITING_FOREVER) != RT_EOK)
            break;

        hid_player_video(HID_FILE_FOR_PLAY, (rotation_angel * 90) % 360);
        rotation_angel++;
    }
#endif
}

int cmd_test_usbd_hid_custom(int argc, char **argv)
{
    uint32_t max = 10;
    aicos_thread_t thid = NULL;

    if (argc > 1) {
        if (strncmp("debug", argv[2], 5) == 0) {
            pr_info("Only receive data, and do not decode it\n");
            g_hid_only_recv = 1;
        } else {
            max = atoi(argv[1]);
            if (max == 0) {
                pr_err("Invalid argument: %s\n", argv[1]);
                return -1;
            }
        }
    }

    thid = aicos_thread_create("hid_dec", 8192, 0, hid_dec_thread, &max);
    if (thid == NULL) {
        pr_err("Failed to create HID decode thread\n");
        return -1;
    }

    thid = aicos_thread_create("hid_player", 8192, 0, hid_player_thread, NULL);
    if (thid == NULL) {
        pr_err("Failed to create HID player thread\n");
        return -1;
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_test_usbd_hid_custom, test_usbd_hid_custom, Receive and decode a file);

#endif // end of #if defined(RT_USING_FINSH)

#endif // end of #if defined(KERNEL_RTTHREAD)
