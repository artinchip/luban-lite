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

static void print_help(char *program)
{
    printf("Compile time: %s\n", __TIME__);
    printf("Usage: %s [options]", program);
    printf("\t -i, --input: \t\t input stream file name\n");
    printf("\t -r, --rotate: \t\t enable clockwise rotate(0/90/180/270)\n");
    printf("\t -s, --scale: \t\t enable scale(1- 1/2 scale; 2- 1/4 scale; 3- 1/8 scale)\n");
    printf("\t -l, --flip: \t\t enable flip(1-horizontal flip; 2-vertical flip; 3-ver & hor flip)\n");
    printf("\t -f, --format: \t\t output format:rgba8888/bgra8888/argb8888/abgr8888/bgr565/rgb565\n");
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
    set_ui_layer_alpha(fb, 10);

    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
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
    if(ret < 0) {
        loge("update_layer_config error, %d", ret);
    }
    aicos_msleep(2000);
}


void pic_test(int argc, char **argv)
{
    int out_format = MPP_FMT_ABGR_8888;
    int ret = 0;
    int file_len = 0;
    int input_fd = 0;
    int type = MPP_CODEC_VIDEO_DECODER_MJPEG;
    int rot_flip_flag = 0;
    int ver_scale = 0;
    int hor_scale = 0;
    struct mpp_fb *fb = NULL;
    char *ptr = NULL;

    if(argc < 2) {
        print_help(argv[0]);
        return;
    }

    optind = 0;
    int c;
    while ((c = getopt(argc, argv, "i:f:r:l:s:h")) != -1) {
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

        case 'f':
            logd("optarg: %s", optarg);
            if (!strcmp(optarg, "argb8888")) {
                out_format = MPP_FMT_ARGB_8888;
            } else if(!strcmp(optarg, "abgr8888")) {
                out_format = MPP_FMT_ABGR_8888;
            } else if(!strcmp(optarg, "abgr8888")) {
                out_format = MPP_FMT_ABGR_8888;
            } else if(!strcmp(optarg, "bgra8888")) {
                out_format = MPP_FMT_BGRA_8888;
            } else if(!strcmp(optarg, "rgba8888")) {
                out_format = MPP_FMT_RGBA_8888;
            } else if(!strcmp(optarg, "bgr565")) {
                out_format = MPP_FMT_BGR_565;
            } else if(!strcmp(optarg, "rgb565")) {
                out_format = MPP_FMT_RGB_565;
            } else if(!strcmp(optarg, "nv12")) {
                out_format = MPP_FMT_NV12;
            }

            continue;
        case 'r':
            if (!strcmp(optarg, "0")) {
                rot_flip_flag |= MPP_ROTATION_0;
            } else if(!strcmp(optarg, "90")) {
                rot_flip_flag |= MPP_ROTATION_90;
            } else if(!strcmp(optarg, "180")) {
                rot_flip_flag |= MPP_ROTATION_180;
            } else if(!strcmp(optarg, "270")) {
                rot_flip_flag |= MPP_ROTATION_270;
            }
            continue;
        case 'l':
            if (!strcmp(optarg, "1")) {
                rot_flip_flag |= MPP_FLIP_H;
            } else if(!strcmp(optarg, "2")) {
                rot_flip_flag |= MPP_FLIP_V;
            } else if(!strcmp(optarg, "3")) {
                rot_flip_flag |= (MPP_FLIP_H | MPP_FLIP_V);
            }
            continue;
        case 's':
            ver_scale = atoi(optarg);

            hor_scale = ver_scale = ver_scale > 3 ? 3: ver_scale;
            continue;
        case 'h':
        default:
            return print_help(argv[0]);
        }
    }

    fb = mpp_fb_open();

    //* 1. create mpp_decoder
    struct mpp_decoder* dec = mpp_decoder_create(type);

    struct decode_config config;
    config.bitstream_buffer_size = (file_len + 1023) & (~1023);
    config.extra_frame_num = 0;
    config.packet_count = 1;
    config.pix_fmt = out_format;

#ifdef AIC_VE_DRV_V10
    if(type == MPP_CODEC_VIDEO_DECODER_MJPEG)
        config.pix_fmt = MPP_FMT_NV12;
#endif

    if(rot_flip_flag) {
        logw("rot_flip_flag: %d", rot_flip_flag);
        mpp_decoder_control(dec, MPP_DEC_INIT_CMD_SET_ROT_FLIP_FLAG, &rot_flip_flag);
    }

    if(ver_scale || hor_scale) {
        struct mpp_scale_ratio scale;
        scale.hor_scale = hor_scale;
        scale.ver_scale = ver_scale;
        mpp_decoder_control(dec, MPP_DEC_INIT_CMD_SET_SCALE, &scale);
    }

    //* 2. init mpp_decoder
    mpp_decoder_init(dec, &config);

    //* 3. get an empty packet from mpp_decoder
    struct mpp_packet packet;
    memset(&packet, 0, sizeof(struct mpp_packet));
    mpp_decoder_get_packet(dec, &packet, file_len);

    //* 4. copy data to packet
    int r_len = read(input_fd, packet.data, file_len);
    packet.size = r_len;
    packet.flag = PACKET_FLAG_EOS;
    logi("read len: %d", r_len);

    //* 5. put the packet to mpp_decoder
    mpp_decoder_put_packet(dec, &packet);

    //* 6. decode
    //time_start(mpp_decoder_decode);
    ret = mpp_decoder_decode(dec);
    if(ret < 0) {
        loge("decode error");
        goto out;
    }
    //time_end(mpp_decoder_decode);

    //* 7. get a decoded frame
    struct mpp_frame frame;
    memset(&frame, 0, sizeof(struct mpp_frame));
    mpp_decoder_get_frame(dec, &frame);

    //* 8. compare data
    render_frame(fb, &frame);

    //* 9. return this frame
    mpp_decoder_put_frame(dec, &frame);

out:
    //* 10. destroy mpp_decoder
    mpp_decoder_destory(dec);

    if (fb)
        mpp_fb_close(fb);

    if(input_fd > 0)
        close(input_fd);
}

MSH_CMD_EXPORT_ALIAS(pic_test, pic_test, picture decode test);
