/*
* Copyright (C) 2020-2022 Artinchip Technology Co. Ltd
*
*  author: <qi.xu@artinchip.com>
*  Desc: jpeg/png decode demo
*/

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <inttypes.h>
#include "aic_osal.h"
#include "mpp_fb.h"
#include "mpp_mem.h"
#include "frame_allocator.h"
#include "mpp_decoder.h"
#include "mpp_mem.h"
#include "mpp_log.h"

static const char sopts[] = "i:dh";
static const struct option lopts[] = {
    {"input",    required_argument, NULL, 'i'},
    {"help",     no_argument,       NULL, 'h'},

    {0, 0, 0, 0}
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


static void print_help(char *program)
{
    printf("Compile time: %s\n", __TIME__);
    printf("Usage: %s [options]", program);
    printf("\t -i, --input: \t\t input stream file name\n");
    printf("\t -h, --help: \t\t print help info\n");
    printf("End:\n");
}

static int get_file_size(int fd, char* path)
{
    struct stat st;
    stat(path, &st);

    logi("mode: %"PRIu32", size: %ld", st.st_mode, st.st_size);

    return st.st_size;
}

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
    aicos_msleep(2000);
}


void pic_crop_test(int argc, char **argv)
{
    int ret = 0;
    int file_len = 0;
    int input_fd = 0;
    int ext_frame_alloc = 1;
    char* ptr = NULL;
    int type = MPP_CODEC_VIDEO_DECODER_MJPEG;
    struct mpp_fb *fb = NULL;
    int buf_len;
    unsigned char* buf = NULL;
    if (argc < 2) {
        print_help(argv[0]);
        return;
    }

    optind = 0;
    int c;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'i':
            ptr = strrchr(optarg, '.');

            if (!strcmp(ptr, ".jpg")) {
                type = MPP_CODEC_VIDEO_DECODER_MJPEG;
            }
            if (!strcmp(ptr, ".png")) {
                type = MPP_CODEC_VIDEO_DECODER_PNG;
            }
            logd("decode type: 0x%02X", type);
            logd("optarg: %s", optarg);

            input_fd = open(optarg, O_RDONLY);
            if(input_fd < 0) {
                loge("open file(%s) failed, %d", optarg, input_fd);
                return;
            }

            file_len = get_file_size(input_fd, optarg);
            logw("file_len: %d", file_len);

            continue;

        case 'h':
        default:
            return print_help(argv[0]);
        }
    }

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
    printf("[%s:%d]format:%d,width:%d,height:%d,stride:%d,smem_len:%d,framebuffer:%p\n"
            ,__FUNCTION__,__LINE__
            ,g_info.format,g_info.width,g_info.height
            ,g_info.stride,g_info.smem_len,g_info.framebuffer);

    buf_len = (file_len + 1023) & (~1023);
    buf = mpp_alloc(buf_len);
    memset(buf, 0, buf_len);
    int r_len = read(input_fd, buf, file_len);
    logi("read len: %d", r_len);

    int i = 0, j = 0, k = 32;
    for (k=100; k<500; k+=5) {
        for (j=100; j<200; j+=10) {
            for (i = 0; i<100; i+=10) {
                // * 0. clear ui buffer
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
                struct mpp_dec_crop_info crop;
                crop.crop_out_x = i;
                crop.crop_out_y = i;
                crop.crop_x = j;
                crop.crop_y = j;
                crop.crop_width = k;
                crop.crop_height = k;
                mpp_decoder_control(dec, MPP_DEC_INIT_CMD_SET_CROP_INFO, &crop);
                printf( "[%s:%d]"\
                        "crop_x:%d,crop_y:%d"\
                        "crop_width:%d,crop_height:%d"\
                        "crop_out_x:%d,crop_out_x:%d\n"
                        ,__FUNCTION__,__LINE__
                        ,crop.crop_x,crop.crop_y
                        ,crop.crop_width,crop.crop_height
                        ,crop.crop_out_x,crop.crop_out_x);

                //* 2. init mpp_decoder
                mpp_decoder_init(dec, &config);

                //* 3. get an empty packet from mpp_decoder
                struct mpp_packet packet;
                memset(&packet, 0, sizeof(struct mpp_packet));
                mpp_decoder_get_packet(dec, &packet, file_len);
                //* 4. copy data to packet
                memcpy(packet.data, buf ,r_len);
                //int r_len = aos_read(input_fd, packet.data, file_len);
                packet.size = r_len;
                packet.flag = PACKET_FLAG_EOS;
                logi("read len: %d", r_len);

                //* 5. put the packet to mpp_decoder
                mpp_decoder_put_packet(dec, &packet);

                //* 6. decode
                //time_start(mpp_decoder_decode);
                ret = mpp_decoder_decode(dec);
                if (ret < 0) {
                    loge("decode error");
                    mpp_decoder_destory(dec);
                    goto out;
                }
                //time_end(mpp_decoder_decode);

                //* 7. get a decoded frame
                struct mpp_frame frame;
                memset(&frame, 0, sizeof(struct mpp_frame));
                mpp_decoder_get_frame(dec, &frame);

                //* 8. compare data
                render_frame(fb,&frame);

                //* 9. return this frame
                mpp_decoder_put_frame(dec, &frame);

                //* 10. destroy mpp_decoder
                mpp_decoder_destory(dec);
            }
        }
    }
    return;

out:
    if (buf) {
         mpp_free(buf);
    }
    if (input_fd > 0) {
        close(input_fd);
    }
    if (fb) {
        mpp_fb_close(fb);
    }
}

MSH_CMD_EXPORT_ALIAS(pic_crop_test, pic_crop_test, pic crop test);
