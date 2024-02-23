/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#include <posix/string.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "aic_log.h"
#include "aic_osal.h"
#include "aic_drv_gpio.h"

#include "mpp_vin.h"
#ifdef AIC_USING_CAMERA
#include "drv_camera.h"
#endif

#ifdef AIC_DISPLAY_DRV
#include "artinchip_fb.h"
#include "mpp_fb.h"
#endif

/* Global macro and variables */

#define VID_BUF_NUM             3
#define VID_BUF_PLANE_NUM       2
#define VID_SCALE_OFFSET        0

static const char sopts[] = "f:c:h";
static const struct option lopts[] = {
    {"format",        required_argument, NULL, 'f'},
    {"capture",       required_argument, NULL, 'c'},
    {"usage",               no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

struct aic_dvp_data {
    int w;
    int h;
    int frame_size;
    int frame_cnt;
    int dst_fmt;  // output format
    struct mpp_video_fmt src_fmt;
    uint32_t num_buffers;
    struct vin_video_buf binfo;
};

static struct aic_dvp_data g_vdata = {0};
#ifdef AIC_DISPLAY_DRV
static struct mpp_fb *g_fb = NULL;
static struct aicfb_screeninfo g_fb_info = {0};
#endif

/* Functions */

static void usage(char *program)
{
    printf("Usage: %s [options]: \n", program);
    printf("\t -f, --format\t\tformat of input video, NV16/NV12 etc\n");
    printf("\t -c, --count\t\tthe number of capture frame \n");
    printf("\t -u, --usage \n");
    printf("\n");
    printf("Example: %s -f nv16 -c 1\n", program);
}

static long long int str2int(char *_str)
{
    if (_str == NULL) {
        pr_err("The string is empty!\n");
        return -1;
    }

    if (strncmp(_str, "0x", 2))
        return atoi(_str);
    else
        return strtoll(_str, NULL, 16);
}

int get_fb_info(void)
{
    int ret = 0;
#ifdef AIC_DISPLAY_DRV

    ret = mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &g_fb_info);
    if (ret < 0)
        pr_err("ioctl() failed! errno: -%d\n", -ret);
#endif
    pr_info("Screen width: %d, height %d\n",
            g_fb_info.width, g_fb_info.height);
    return ret;
}

int set_ui_layer_alpha(int val)
{
    int ret = 0;
#ifdef AIC_DISPLAY_DRV
    struct aicfb_alpha_config alpha = {0};

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.enable = 1;
    alpha.mode = 1;
    alpha.value = val;
    ret = mpp_fb_ioctl(g_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    if (ret < 0)
        pr_err("ioctl() failed! errno: -%d\n", -ret);
#endif
    return ret;
}

int sensor_get_fmt(void)
{
    int ret = 0;
    struct mpp_video_fmt f = {0};

    ret = mpp_dvp_ioctl(DVP_IN_G_FMT, &f);
    if (ret < 0) {
        pr_err("ioctl() failed! err -%d\n", -ret);
        // return -1;
    }

    g_vdata.src_fmt = f;
    g_vdata.w = g_vdata.src_fmt.width;
    g_vdata.h = g_vdata.src_fmt.height;
    pr_info("Sensor format: w %d h %d, code 0x%x, bus 0x%x, colorspace 0x%x\n",
            f.width, f.height, f.code, f.bus_type, f.colorspace);
    return 0;
}

int dvp_subdev_set_fmt(void)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_IN_S_FMT, &g_vdata.src_fmt);
    if (ret < 0) {
        pr_err("ioctl() failed! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

int dvp_cfg(int width, int height, int format)
{
    int ret = 0;
    struct dvp_out_fmt f = {0};

    f.width = g_vdata.src_fmt.width;
    f.height = g_vdata.src_fmt.height;
    f.pixelformat = format;
    f.num_planes = VID_BUF_PLANE_NUM;

    ret = mpp_dvp_ioctl(DVP_OUT_S_FMT, &f);
    if (ret < 0) {
        pr_err("ioctl() failed! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

int dvp_request_buf(struct vin_video_buf *vbuf)
{
    int i;

    if (mpp_dvp_ioctl(DVP_REQ_BUF, (void *)vbuf) < 0) {
        pr_err("ioctl() failed!\n");
        return -1;
    }

    pr_info("Buf   Plane[0]     size   Plane[1]     size\n");
    for (i = 0; i < vbuf->num_buffers; i++) {
        pr_info("%3d 0x%x %8d 0x%x %8d\n", i,
            vbuf->planes[i * vbuf->num_planes].buf,
            vbuf->planes[i * vbuf->num_planes].len,
            vbuf->planes[i * vbuf->num_planes + 1].buf,
            vbuf->planes[i * vbuf->num_planes + 1].len);
    }

    return 0;
}

void dvp_release_buf(int num)
{
#if 0
    int i;
    struct video_buf_info *binfo = NULL;

    for (i = 0; i < num; i++) {
        binfo = &g_vdata.binfo[i];
        if (binfo->vaddr) {
            munmap(binfo->vaddr, binfo->len);
            binfo->vaddr = NULL;
        }
    }
#endif
}

int dvp_queue_buf(int index)
{
    if (mpp_dvp_ioctl(DVP_Q_BUF, (void *)(ptr_t)index) < 0) {
        pr_err("ioctl() failed!\n");
        return -1;
    }

    return 0;
}

int dvp_dequeue_buf(int *index)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_DQ_BUF, (void *)index);
    if (ret < 0) {
        pr_err("ioctl() failed! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

int dvp_start(void)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_STREAM_ON, NULL);
    if (ret < 0) {
        pr_err("ioctl() failed! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

int dvp_stop(void)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_STREAM_OFF, NULL);
    if (ret < 0) {
        pr_err("ioctl() failed! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

#define DVP_SCALE       1

int video_layer_disable(void)
{
    int ret = 0;
#ifdef AIC_DISPLAY_DRV
    struct aicfb_layer_data layer = {0};
    layer.enable = 0;
    ret = mpp_fb_ioctl(g_fb, AICFB_UPDATE_LAYER_CONFIG, &layer);
    if (ret < 0)
	pr_err("g_fb ioctl AICFB_UPDATE_LAYER_CONFIG failed !");

#endif
    return ret;
}

int video_layer_set(struct aic_dvp_data *vdata, int index)
{
#ifdef AIC_DISPLAY_DRV
    int i;
    struct aicfb_layer_data layer = {0};
    struct vin_video_buf *binfo = &vdata->binfo;

    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    layer.enable = 1;
#if DVP_SCALE
#if 1
    layer.scale_size.width = g_fb_info.width - VID_SCALE_OFFSET * 2;
    layer.scale_size.height = g_fb_info.height - VID_SCALE_OFFSET * 2;
    layer.pos.x = VID_SCALE_OFFSET;
    layer.pos.y = VID_SCALE_OFFSET;
#else
    /* Reduce the size to fb0*1/2 */
    layer.scale_size.width = g_fb_info.width / 2;
    layer.scale_size.height = g_fb_info.height / 2;
    layer.pos.x = g_fb_info.width / 2 - VID_SCALE_OFFSET;
    layer.pos.y = g_fb_info.height / 2 - VID_SCALE_OFFSET;
#endif
#else
    layer.scale_size.width = vdata->w;
    layer.scale_size.height = vdata->h;
    layer.pos.x = g_fb_info.width - vdata->w;
    layer.pos.y = 0;
#endif
    layer.buf.size.width = vdata->w;
    layer.buf.size.height = vdata->h;
    if (vdata->dst_fmt == MPP_FMT_NV16)
        layer.buf.format = MPP_FMT_NV16;
    else
        layer.buf.format = MPP_FMT_NV12;
    layer.buf.buf_type = MPP_PHY_ADDR;

    for (i = 0; i < VID_BUF_PLANE_NUM; i++) {
        layer.buf.stride[i] = vdata->w;
        layer.buf.phy_addr[i] = binfo->planes[index * VID_BUF_PLANE_NUM + i].buf;
    }

    if (mpp_fb_ioctl(g_fb, AICFB_UPDATE_LAYER_CONFIG, &layer) < 0) {
        pr_err("ioctl() failed!\n");
        return -1;
    }
#endif
    return 0;
}

#define NS_PER_SEC      1000000000

static void show_fps(struct timespec *start, struct timespec *end, int cnt)
{
     double diff;

    if (end->tv_nsec < start->tv_nsec) {
        diff = (double)(NS_PER_SEC + end->tv_nsec - start->tv_nsec)/NS_PER_SEC;
        diff += end->tv_sec - 1 - start->tv_sec;
    } else {
        diff = (double)(end->tv_nsec - start->tv_nsec)/NS_PER_SEC;
        diff += end->tv_sec - start->tv_sec;
    }

    printf("\nDVP frame rate: %d.%d, frame %d / %d.%d seconds\n",
           (u32)(cnt / diff), (u32)(cnt * 10 / diff) % 10, cnt,
           (u32)diff, (u32)(diff * 10) % 10);
}

static void test_dvp_thread(void *arg)
{
    int i, index = 0;
    struct timespec begin, now;

    if (dvp_request_buf(&g_vdata.binfo) < 0)
        return;

    for (i = 0; i < g_vdata.binfo.num_buffers; i++) {
        if (dvp_queue_buf(i) < 0)
            return;
    }

    if (dvp_start() < 0)
        return;

#if DVP_SCALE
    pr_info("DVP scale is enable\n");
#else
    pr_info("DVP scale is disable\n");
#endif

    clock_gettime(CLOCK_REALTIME, &begin);
    for (i = 0; i < g_vdata.frame_cnt; i++) {
        if (dvp_dequeue_buf(&index) < 0)
            break;
        // pr_debug("Set the buf %d to video layer\n", index);
        if (video_layer_set(&g_vdata, index) < 0)
            break;
        dvp_queue_buf(index);

        if (i && (i % 1000 == 0)) {
            clock_gettime(CLOCK_REALTIME, &now);
            show_fps(&begin, &now, i);
        }
    }
    if ((i - 1) % 1000 != 0) {
        clock_gettime(CLOCK_REALTIME, &now);
        show_fps(&begin, &now, i);
    }

    dvp_stop();
    dvp_release_buf(g_vdata.binfo.num_buffers);
    mpp_vin_deinit();
    if (g_fb) {
	video_layer_disable();
        mpp_fb_close(g_fb);
    }
}

static void cmd_test_dvp(int argc, char **argv)
{
    int c;
    aicos_thread_t thid = NULL;

    memset(&g_vdata, 0, sizeof(struct aic_dvp_data));
    g_vdata.dst_fmt = MPP_FMT_NV16;
    g_vdata.frame_cnt = 1;
    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'f':
            if (strncasecmp("nv12", optarg, strlen(optarg)) == 0)
                g_vdata.dst_fmt = MPP_FMT_NV12;
            continue;

        case 'c':
            g_vdata.frame_cnt = str2int(optarg);
            continue;

        case 'h':
            usage(argv[0]);
            return;

        default:
            break;
        }
    }

    pr_info("Capture %d frames from camera\n", g_vdata.frame_cnt);
    pr_info("DVP out format: %s\n",
            g_vdata.dst_fmt == MPP_FMT_NV16 ? "NV16" : "NV12");

    if (mpp_vin_init(CAMERA_NAME_OV))
        return;

    if (sensor_get_fmt() < 0)
        goto error_out;

    if (dvp_subdev_set_fmt() < 0)
        goto error_out;

    if (g_vdata.dst_fmt == MPP_FMT_NV16)
        g_vdata.frame_size = g_vdata.w * g_vdata.h * 2;
    else if (g_vdata.dst_fmt == DVP_OUT_FMT_NV12)
        g_vdata.frame_size = (g_vdata.w * g_vdata.h * 3) >> 1;

    g_fb = mpp_fb_open();
    if (!g_fb) {
        pr_err("Failed to open FB\n");
        goto error_out;
    }

    if (get_fb_info() < 0)
        goto error_out;

    if (set_ui_layer_alpha(15) < 0)
        goto error_out;

    if (dvp_cfg(g_vdata.w, g_vdata.h, g_vdata.dst_fmt) < 0)
        goto error_out;

    g_vdata.num_buffers = VID_BUF_NUM;

    thid = aicos_thread_create("test_dvp", 4096, 0, test_dvp_thread, NULL);
    if (thid == NULL) {
        pr_err("Failed to create DVP thread\n");
        return;
    }
    return;

error_out:
    mpp_vin_deinit();
    if (g_fb)
        mpp_fb_close(g_fb);
}
MSH_CMD_EXPORT_ALIAS(cmd_test_dvp, test_dvp, Test DVP and camera);
