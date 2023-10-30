// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2023 ArtInChip Technology Co., Ltd.
 * Authors:  Matteo <duanmt@artinchip.com>
 */

#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>

#include "aic_common.h"
#include "aic_errno.h"
#include "aic_log.h"

#ifdef AIC_USING_DVP
#include "drv_dvp.h"
#endif
#ifdef AIC_USING_CAMERA
#include "drv_camera.h"
#endif
#include "mpp_vin.h"

rt_device_t g_camera_dev = NULL;

#ifdef AIC_USING_DVP
char *g_mpp_dvp_buf = NULL;
#endif

int mpp_vin_init(char *camera)
{
    if (!camera) {
        pr_err("Must given the name of camera\n");
        return -1;
    }

    g_camera_dev = rt_device_find(camera);
    if (!g_camera_dev) {
        pr_err("Failed to find camera %s\n", camera);
        return -1;
    }
    if (rt_device_open(g_camera_dev, 'r') < 0) {
        pr_err("Failed to open camera %s\n", camera);
        return -1;
    }

#ifdef AIC_USING_DVP
    if (aic_dvp_probe())
        return -1;

    if (aic_dvp_open())
        return -1;

    if (g_mpp_dvp_buf) {
        pr_info("DVP buffer is already malloced: 0x%lx\n", (ptr_t)g_mpp_dvp_buf);
        return 0;
    }

    g_mpp_dvp_buf = (char *)aicos_malloc(MEM_CMA, AIC_MPP_VIN_BUF_SIZE);
    if (!g_mpp_dvp_buf) {
        pr_err("Failed to malloc %d buffer\n", AIC_MPP_VIN_BUF_SIZE);
        return -1;
    }
    memset(g_mpp_dvp_buf, 0, AIC_MPP_VIN_BUF_SIZE);
    pr_debug("MPP DVP buffer: 0x%lx\n", (ptr_t)g_mpp_dvp_buf);
#endif
    return 0;
}

void mpp_vin_deinit(void)
{
    if (g_camera_dev) {
        rt_device_close(g_camera_dev);
        g_camera_dev = NULL;
    }
#ifdef AIC_USING_DVP
    if (g_mpp_dvp_buf) {
        aicos_free(MEM_CMA, g_mpp_dvp_buf);
        g_mpp_dvp_buf = NULL;
    }

    aic_dvp_close();
#endif
}

int mpp_dvp_ioctl(int cmd, void *arg)
{
#ifdef AIC_USING_DVP
    char *tmp = NULL;
    u32 align_offset = 0;

    switch (cmd) {
    case DVP_IN_S_FMT:
        return aic_dvp_set_in_fmt((struct mpp_video_fmt *)arg);
        break;

    case DVP_OUT_S_FMT:
        return aic_dvp_set_out_fmt((struct dvp_out_fmt *)arg);

#ifdef AIC_USING_CAMERA
    case DVP_IN_G_FMT:
        if (g_camera_dev)
            return rt_device_control(g_camera_dev, CAMERA_CMD_GET_FMT, arg);

        pr_err("Must init camera first!\n");
        return -ENODEV;

    case DVP_STREAM_ON:
        if (g_camera_dev)
            rt_device_control(g_camera_dev, CAMERA_CMD_START, arg);
        if (aic_dvp_stream_on())
            return -1;
        return 0;

    case DVP_STREAM_OFF:
        if (g_camera_dev)
            rt_device_control(g_camera_dev, CAMERA_CMD_STOP, arg);
        return aic_dvp_stream_off();
#endif

    case DVP_REQ_BUF:
        align_offset = CACHE_LINE_SIZE - (ptr_t)g_mpp_dvp_buf%CACHE_LINE_SIZE;
        tmp = g_mpp_dvp_buf + align_offset;
        return aic_dvp_req_buf(tmp, AIC_MPP_VIN_BUF_SIZE - align_offset,
                               (struct vin_video_buf *)arg);

    case DVP_Q_BUF:
        return aic_dvp_q_buf((u32)(ptr_t)arg);

    case DVP_DQ_BUF:
        return aic_dvp_dq_buf((u32 *)arg);

    default:
        pr_err("Unsupported ioctl command: 0x%x\n", cmd);
        return -EINVAL;
    }
#endif
    return 0;
}
