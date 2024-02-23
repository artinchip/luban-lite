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
#include "mpp_ge.h"

static void print_help(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -i, --input: \t\t input stream file name\n");
    printf("\t -r, --rotate: \t\t enable clockwise rotate(0/90/180/270)\n");
    printf("\t -s, --scale: \t\t enable scale(1- 1/2 scale; 2- 1/4 scale; 3- 1/8 scale)\n");
    printf("\t -l, --flip: \t\t enable flip(1-horizontal flip; 2-vertical flip; 3-ver & hor flip)\n");
    printf("\t -f, --format: \t\t output format:rgba8888/bgra8888/argb8888/abgr8888/bgr565/rgb565\n");
    printf("\t -a, --address: \t read image file from a buffer address\n");
    printf("\t -z, --size: \t\t the size of the image file in buffer\n");
    printf("\t -h, --help: \t\t print help info\n");
}

static int get_file_size(int fd, char* path)
{
    struct stat st;
    stat(path, &st);

    logi("mode: %"PRIu32", size: %ld", st.st_mode, st.st_size);

	return st.st_size;
}

static int ge_bitblt(struct ge_bitblt *blt)
{
    int ret = 0;
    struct mpp_ge *ge = mpp_ge_open();

    if (!ge) {
        loge("open ge device error\n");
    }

    ret = mpp_ge_bitblt(ge, blt);
    if (ret < 0) {
        loge("bitblt task failed\n");
        return ret;
    }

    ret = mpp_ge_emit(ge);
    if (ret < 0) {
        loge("emit task failed\n");
        return ret;
    }

    ret = mpp_ge_sync(ge);
    if (ret < 0) {
        loge("ge sync fail\n");
        return ret;
    }

    mpp_ge_close(ge);

    return 0;
}

static u32 *g_vlayer_addr = NULL;
#ifdef AIC_PAN_DISPLAY
static u32 g_fb_buf_index = 0;
#endif

static void ui_layer_buf_sync(void)
{
#ifdef AIC_PAN_DISPLAY
    struct ge_bitblt blt = {0};
    struct mpp_fb *fb = mpp_fb_open();
    struct aicfb_screeninfo info = {0};
    uint8_t *src_buf = NULL, *dst_buf = NULL;

    if (!fb)
        return;

    if (mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &info)) {
        loge("get screen info failed\n");
        goto out;
    }

    if (g_fb_buf_index) {
        src_buf = info.framebuffer + info.smem_len;
        dst_buf = info.framebuffer;
    } else {
        src_buf = info.framebuffer;
        dst_buf = info.framebuffer + info.smem_len;
    }

    blt.src_buf.buf_type    = MPP_PHY_ADDR;
    blt.src_buf.phy_addr[0] = (u32)(long)src_buf;
    blt.src_buf.format      = info.format;
    blt.src_buf.stride[0]   = info.stride;
    blt.src_buf.size.width  = info.width;
    blt.src_buf.size.height = info.height;
    blt.dst_buf.buf_type    = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = (u32)(long)dst_buf;
    blt.dst_buf.format      = info.format;
    blt.dst_buf.stride[0]   = info.stride;
    blt.dst_buf.size.width  = info.width;
    blt.dst_buf.size.height = info.height;
    ge_bitblt(&blt);

out:
    mpp_fb_close(fb);
#endif
}

static void render_frame(struct mpp_fb* fb, struct mpp_frame* frame,
                         u32 offset_x, u32 offset_y, u32 width, u32 height,
                         u32 layer_id)
{
    struct ge_bitblt blt;
    struct aicfb_screeninfo info = {0};
    struct aicfb_layer_data layer = {0};
#ifdef AIC_PAN_DISPLAY
    u32 disp_buf_addr;
    u32 fb0_buf_addr;
#endif
    u32 dst_buf_addr;

    int ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &info);
    if (ret) {
        loge("get screen info failed\n");
        return;
    }

    if (layer_id == AICFB_LAYER_TYPE_UI) {
        layer.layer_id = AICFB_LAYER_TYPE_UI;
        layer.rect_id = 0;
        if (mpp_fb_ioctl(fb, AICFB_GET_LAYER_CONFIG, &layer) < 0) {
            loge("get ui layer config failed\n");
            return;
        }

#ifdef AIC_PAN_DISPLAY
        disp_buf_addr = (u32)layer.buf.phy_addr[0];
        fb0_buf_addr = (u32)(unsigned long)info.framebuffer;
        /* Switch the double-buffer */
        if (disp_buf_addr == fb0_buf_addr) {
            dst_buf_addr = fb0_buf_addr + info.smem_len;
            g_fb_buf_index = 1;
        } else {
            dst_buf_addr = fb0_buf_addr;
            g_fb_buf_index = 0;
        }
#else
        dst_buf_addr = layer.buf.phy_addr[0];
#endif
    } else {
        layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
        if (mpp_fb_ioctl(fb, AICFB_GET_LAYER_CONFIG, &layer) < 0) {
            loge("get video layer config failed\n");
            return;
        }
        dst_buf_addr = layer.buf.phy_addr[0];
    }
    pr_info("Render to buf 0x%08x\n", dst_buf_addr);

    memset(&blt, 0, sizeof(struct ge_bitblt));
    memcpy(&blt.src_buf, &frame->buf, sizeof(struct mpp_buf));

    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = dst_buf_addr;
    blt.dst_buf.format = info.format;
    blt.dst_buf.stride[0] = info.stride;
    blt.dst_buf.size.width = info.width;
    blt.dst_buf.size.height = info.height;
    blt.dst_buf.crop_en = 1;

    blt.dst_buf.crop.x = offset_x;
    blt.dst_buf.crop.y = offset_y;
    blt.dst_buf.crop.width = blt.src_buf.crop.width;
    blt.dst_buf.crop.height = blt.src_buf.crop.height;

    logi("phy_addr: %x, stride: %d", blt.src_buf.phy_addr[0], blt.src_buf.stride[0]);
    logi("width: %d, height: %d, format: %d", blt.src_buf.size.width, blt.src_buf.size.height, blt.src_buf.format);

    ge_bitblt(&blt);

#ifdef AIC_PAN_DISPLAY
    if (layer_id == AICFB_LAYER_TYPE_UI) {
        /* display at last */
        mpp_fb_ioctl(fb, AICFB_PAN_DISPLAY, &g_fb_buf_index);
        mpp_fb_ioctl(fb, AICFB_WAIT_FOR_VSYNC, 0);
        ui_layer_buf_sync();
    }
#endif
}

void ui_alpha_config(u32 val)
{
    struct mpp_fb *fb = mpp_fb_open();
    struct aicfb_alpha_config alpha = {0};
    int ret;

    if (!fb)
        return;

    alpha.layer_id = 1;
    alpha.enable = 1;
    alpha.mode = 1;
    alpha.value = val;
    ret = mpp_fb_ioctl(fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    if (ret < 0)
        loge("ioctl update alpha config failed!\n");

    ui_layer_buf_sync();
    mpp_fb_close(fb);
}

void video_layer_init(void)
{
    struct mpp_fb *fb = mpp_fb_open();
    struct aicfb_screeninfo info = { 0 };
    struct aicfb_layer_data vlayer = {0};
    u32 *addr;
    int ret;

    if (!fb)
        return;

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &info);
    if (ret) {
        loge("get screen info failed\n");
        goto out;
    }

    vlayer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    if (mpp_fb_ioctl(fb, AICFB_GET_LAYER_CONFIG, &vlayer) < 0) {
        loge("get video layer config failed\n");
        goto out;
    }

    addr = aicos_malloc_align(0, info.smem_len, CACHE_LINE_SIZE);
    if (!addr) {
        loge("malloc video buf failde\n");
        goto out;
    }
    g_vlayer_addr = addr;
    pr_info("The buf for video layer: 0x%08x\n", g_vlayer_addr);

    memset(addr, 0x00, info.smem_len);
    aicos_dcache_clean_invalid_range(addr, info.smem_len);

    vlayer.enable = 1;
    vlayer.buf.buf_type = MPP_PHY_ADDR;
    vlayer.buf.size.width = info.width;
    vlayer.buf.size.height = info.height;
    vlayer.buf.format = info.format;
    vlayer.buf.phy_addr[0] = (uintptr_t)addr;
    vlayer.buf.stride[0] = info.stride;

    if (mpp_fb_ioctl(fb, AICFB_UPDATE_LAYER_CONFIG, &vlayer) < 0) {
        loge("set video layer config failed\n");
        goto out;
    }

out:
    mpp_fb_close(fb);
}

void video_layer_config(void)
{
    video_layer_init();
}

void video_layer_deinit(void)
{
    struct mpp_fb *fb = mpp_fb_open();
    struct aicfb_layer_data vlayer = {0};

    if (!fb)
        return;

    vlayer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    if (mpp_fb_ioctl(fb, AICFB_GET_LAYER_CONFIG, &vlayer) < 0) {
        loge("Get video layer config failed\n");
        goto out;
    }

    vlayer.enable = 0;
    if (mpp_fb_ioctl(fb, AICFB_UPDATE_LAYER_CONFIG, &vlayer) < 0) {
        loge("Set video layer config failed\n");
        goto out;
    }

    if (g_vlayer_addr)
        aicos_free_align(0, g_vlayer_addr);

out:
    mpp_fb_close(fb);
}

void video_layer_release(void)
{
    video_layer_deinit();
}

int decode_pic(uint8_t* pic, int len, u32 offset_x, u32 offset_y,
               u32 width, u32 height, u32 layer_id)
{
    struct mpp_fb *fb = NULL;
    int ret = 0;
    fb = mpp_fb_open();

    if (pic == NULL || len <= 0) {
        loge("Invalid parameter. pic 0x%lx, len %d\n", (ptr_t)pic, len);
        return -1;
    }

    // 1. create mpp_decoder
    struct mpp_decoder* dec = NULL;

    struct decode_config config;
    config.bitstream_buffer_size = (len + 1023) & (~1023);
    config.extra_frame_num = 0;
    config.packet_count = 1;
    config.pix_fmt = MPP_FMT_NV12;

    if (pic[0] == 0xff && pic[1] == 0xd8)
        dec = mpp_decoder_create(MPP_CODEC_VIDEO_DECODER_MJPEG);
    else if (pic[1] == 'P' && pic[2] == 'N' && pic[3] == 'G')
        dec = mpp_decoder_create(MPP_CODEC_VIDEO_DECODER_PNG);
    else
         loge("Invaild pic data\n");

    if (!dec)
        return -1;

    // 2. init mpp_decoder
    mpp_decoder_init(dec, &config);

    // 3. get an empty packet from mpp_decoder
    struct mpp_packet packet;
    memset(&packet, 0, sizeof(struct mpp_packet));
    mpp_decoder_get_packet(dec, &packet, len);

    // 4. copy data to packet
    memcpy(packet.data, pic, len);
    packet.size = len;
    packet.flag = PACKET_FLAG_EOS;

    // 5. put the packet to mpp_decoder
    mpp_decoder_put_packet(dec, &packet);

    // 6. decode
    //time_start(mpp_decoder_decode);
    ret = mpp_decoder_decode(dec);
    if(ret < 0) {
        loge("decode error");
        ret = -1;
        goto out;
    }
    //time_end(mpp_decoder_decode);

    // 7. get a decoded frame
    struct mpp_frame frame;
    memset(&frame, 0, sizeof(struct mpp_frame));
    mpp_decoder_get_frame(dec, &frame);

    // 8. compare data
    render_frame(fb, &frame, offset_x, offset_y, width, height, layer_id);

    // 9. return this frame
    mpp_decoder_put_frame(dec, &frame);

out:
    // 10. destroy mpp_decoder
    if (dec)
        mpp_decoder_destory(dec);

    if (fb)
        mpp_fb_close(fb);

    return ret;
}

int decode_jpeg(uint8_t* pic, int len, u32 offset_x, u32 offset_y,
                u32 width, u32 height, u32 layer_id)
{
    return decode_pic(pic, len, offset_x, offset_y, width, height, layer_id);
}

void pic_test(int argc, char **argv)
{
    int out_format = MPP_FMT_ABGR_8888;
    int ret = 0;
    int file_len = 0, r_len = 0;
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
    while ((c = getopt(argc, argv, "i:f:r:l:s:a:z:h")) != -1) {
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
            logi("Read image from %s, length %d", optarg, file_len);
            continue;

        case 'a':
            ptr = (char *)strtol(optarg, NULL, 16);
            if ((*ptr == 0xFF) && (*(ptr + 1) == 0xD8))
                type = MPP_CODEC_VIDEO_DECODER_MJPEG;
            else
                type = MPP_CODEC_VIDEO_DECODER_PNG;
            continue;

        case 'z':
            file_len = atoi(optarg);
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

    // 1. create mpp_decoder
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

    // 2. init mpp_decoder
    mpp_decoder_init(dec, &config);

    // 3. get an empty packet from mpp_decoder
    struct mpp_packet packet;
    memset(&packet, 0, sizeof(struct mpp_packet));
    mpp_decoder_get_packet(dec, &packet, file_len);

    // 4. copy data to packet
    if (input_fd > 0) {
        r_len = read(input_fd, packet.data, file_len);
    } else {
        memcpy(packet.data, ptr, file_len);
        r_len = file_len;
        logw("Read image from 0x%lx, length %d", (long)ptr, file_len);
    }
    packet.size = r_len;
    packet.flag = PACKET_FLAG_EOS;
    logi("read len: %d", r_len);

    // 5. put the packet to mpp_decoder
    mpp_decoder_put_packet(dec, &packet);

    // 6. decode
    //time_start(mpp_decoder_decode);
    ret = mpp_decoder_decode(dec);
    if(ret < 0) {
        loge("decode error");
        goto out;
    }
    //time_end(mpp_decoder_decode);

    // 7. get a decoded frame
    struct mpp_frame frame;
    memset(&frame, 0, sizeof(struct mpp_frame));
    mpp_decoder_get_frame(dec, &frame);

    // 8. Render the data to Framebuffer
    render_frame(fb, &frame, 0, 0, 0, 0, AICFB_LAYER_TYPE_UI);

    // 9. return this frame
    mpp_decoder_put_frame(dec, &frame);

out:
    // 10. destroy mpp_decoder
    mpp_decoder_destory(dec);

    if (fb)
        mpp_fb_close(fb);

    if(input_fd > 0)
        close(input_fd);
}

MSH_CMD_EXPORT_ALIAS(pic_test, pic_test, picture decode test);
