/*
* Copyright (C) 2020-2022 Artinchip Technology Co. Ltd
*
*  author: <qi.xu@artinchip.com>
*  Desc: jpeg/png decode demo
*/


#include "mpp_decoder.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_fb.h"
#include <console.h>
#include "frame_allocator.h"


struct pic_info {
    unsigned char *addr;
    int len;
};

static struct aicfb_screeninfo g_info = {0};

/********************** frame allocator **********************************/
struct ext_frame_allocator {
    struct frame_allocator base;
};

static int alloc_frame_buffer(struct frame_allocator *p, struct mpp_frame* frame,
    int width, int height, enum mpp_pixel_format format)
{
    frame->buf.format = g_info.format;
    frame->buf.size.width = g_info.width;
    frame->buf.size.height = g_info.height;
    frame->buf.stride[0] = g_info.stride;
    frame->buf.buf_type = MPP_PHY_ADDR;
    frame->buf.phy_addr[0] = (unsigned long)g_info.framebuffer;

    return 0;
}

static int free_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame)
{
    // we use the ui layer framebuffer, do not need to free

    return 0;
}

static int close_allocator(struct frame_allocator *p)
{
    struct ext_frame_allocator* impl = (struct ext_frame_allocator*)p;
    mpp_free(impl);

    return 0;
}

static struct alloc_ops def_ops = {
    .alloc_frame_buffer = alloc_frame_buffer,
    .free_frame_buffer = free_frame_buffer,
    .close_allocator = close_allocator,
};

static struct frame_allocator* open_allocator()
{
    struct ext_frame_allocator* impl = (struct ext_frame_allocator*)mpp_alloc(sizeof(struct ext_frame_allocator));
    if(impl == NULL) {
        return NULL;
    }
    memset(impl, 0, sizeof(struct ext_frame_allocator));

    impl->base.ops = &def_ops;

    return &impl->base;
}
/********************** frame allocator end **********************************/

static int set_ui_layer_alpha(struct mpp_fb* fb, int val)
{
    int ret = 0;
    struct aicfb_alpha_config alpha = {0};

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.enable = 1;
    alpha.mode = 1;
    alpha.value = val;
    ret = mpp_fb_ioctl(fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    if (ret < 0)
        loge("ioctl() failed! errno: %d\n", ret);

    return ret;
}

static void render_frame(struct mpp_fb* fb, struct mpp_frame* frame)
{
    struct aicfb_layer_data layer = {0};
    set_ui_layer_alpha(fb, 128);

    layer.layer_id = AICFB_LAYER_TYPE_UI;
    layer.rect_id = 0;
    layer.enable = 1;
    layer.buf.phy_addr[0] = frame->buf.phy_addr[0];
    layer.buf.phy_addr[1] = frame->buf.phy_addr[1];
    layer.buf.phy_addr[2] = frame->buf.phy_addr[2];
    layer.buf.stride[0] = frame->buf.stride[0];
    layer.buf.stride[1] = frame->buf.stride[1];
    layer.buf.stride[2] = frame->buf.stride[2];
    layer.buf.size.width = frame->buf.size.width;
    layer.buf.size.height = frame->buf.size.height;
    layer.buf.crop_en = 0;
    layer.buf.format = frame->buf.format;
    layer.buf.buf_type = MPP_PHY_ADDR;

    logi("phy_addr: %x, stride: %d", layer.buf.phy_addr[0], layer.buf.stride[0]);
    logi("width: %d, height: %d, format: %d", layer.buf.size.width, layer.buf.size.height, frame->buf.format);

    int ret = mpp_fb_ioctl(fb, AICFB_UPDATE_LAYER_CONFIG, &layer);
    if (ret < 0) {
        loge("update_layer_config error, %d", ret);
    }
    aicos_mdelay(2000);
}

int  do_pic_dec_test(int argc, char **argv)
{
    int ret = 0;
    int i = 0;
    int file_len = 0;
    int ext_frame_alloc = 1;
    int type = MPP_CODEC_VIDEO_DECODER_MJPEG;
    struct mpp_fb *fb = NULL;
    struct mpp_packet packet;
    struct mpp_frame frame;

    extern const unsigned char test_pic1[239985];
    extern const unsigned char test_pic2[162126];
    extern const unsigned char test_pic3[205510];

    struct pic_info pic_infos[3] = {
        {(unsigned char *)test_pic1,239985},
        {(unsigned char *)test_pic2,162126},
        {(unsigned char *)test_pic3,205510}
    };

    fb = mpp_fb_open();
    if (!fb) {
        loge("mpp_fb_open error!!!!\n");
        goto out;
    }

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO , &g_info);
    if (ret) {
        loge("get screen info failed\n");
        goto out;
    }

    for( i = 0; i< sizeof(pic_infos)/sizeof(pic_infos[0]);i++)
    {
        file_len = (pic_infos[i].len + 1023) & (~1023);

        memset(g_info.framebuffer,0x00,g_info.smem_len);

        //* 1. create mpp_decoder
        struct mpp_decoder* dec = mpp_decoder_create(type);
        struct decode_config config;
        config.bitstream_buffer_size = (file_len + 1023) & (~1023);
        config.extra_frame_num = 0;
        config.packet_count = 1;
        config.pix_fmt = g_info.format;
        if(ext_frame_alloc) {
            struct frame_allocator* allocator = open_allocator();
            mpp_decoder_control(dec, MPP_DEC_INIT_CMD_SET_EXT_FRAME_ALLOCATOR, (void*)allocator);
        }

        //* 2. init mpp_decoder
        mpp_decoder_init(dec, &config);

        //* 3. get an empty packet from mpp_decoder
        memset(&packet, 0, sizeof(struct mpp_packet));
        mpp_decoder_get_packet(dec, &packet, pic_infos[i].len);

        //* 4. copy data to packet
        memcpy(packet.data,  pic_infos[i].addr, pic_infos[i].len);
        packet.size = pic_infos[i].len;
        packet.flag = PACKET_FLAG_EOS;

        //* 5. put the packet to mpp_decoder
        mpp_decoder_put_packet(dec, &packet);

        //* 6. decode
        //time_start(mpp_decoder_decode);
        ret = mpp_decoder_decode(dec);

        if (ret < 0) {
            loge("decode error");
            mpp_decoder_destory(dec);
            continue;
        }

        //* 7. get a decoded frame
        memset(&frame, 0, sizeof(struct mpp_frame));
        mpp_decoder_get_frame(dec, &frame);

        //* 8. compare data
        render_frame(fb,&frame);

        //* 9. return this frame
        mpp_decoder_put_frame(dec, &frame);

        //* 10. destroy mpp_decoder
        mpp_decoder_destory(dec);
    }
out:
    if (fb) {
        mpp_fb_close(fb);
    }
    return 0;
}

CONSOLE_CMD(pic_dec_test, do_pic_dec_test,"pic dec test");
