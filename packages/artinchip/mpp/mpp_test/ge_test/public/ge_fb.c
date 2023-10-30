/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ZeQuan Liang <zequan.liang@artinchip.com>
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <aic_core.h>

#include "ge_fb.h"

struct ge_fb_info *fb_open(void)
{
    int ret = 0;
    struct ge_fb_info *info;

    info = (struct ge_fb_info *)aicos_malloc(MEM_CMA, sizeof(struct ge_fb_info));

    memset(info, 0, sizeof(struct ge_fb_info));

    info->fb = mpp_fb_open();
    if (!info->fb) {
        printf("mpp fb open failed\n");
        return NULL;
    }

    ret = mpp_fb_ioctl(info->fb, AICFB_GET_SCREENINFO , &info->fb_data);
    if (ret) {
        printf("mpp_fb_ioctl ops failed\n");
        return NULL;
    }

    return info;
}

void fb_close(struct ge_fb_info *info)
{
    if (!info->fb)
        mpp_fb_close(info->fb);

    aicos_free(MEM_CMA, info);
}

/* Using double frambuffers, select different buffers based on swap_flag */
void fb_swap_frame(struct ge_fb_info *info)
{
    info->swap_flag = !info->swap_flag;
}

unsigned int fb_get_cur_frame(struct ge_fb_info *info)
{
    unsigned long fb_phy = 0;

    if (!info->swap_flag)
        fb_phy = (intptr_t)info->fb_data.framebuffer;
    else {
        if (APP_FB_NUM > 1) {
            fb_phy = (intptr_t)info->fb_data.framebuffer + info->fb_data.smem_len;
        } else {
            /* used single frambuffer */
            fb_phy = (intptr_t)info->fb_data.framebuffer;
        }
    }

    return fb_phy;
}

/* wait fb dev sync*/
int fb_start_and_wait(struct ge_fb_info *info)
{
    int ret;
    if (APP_FB_NUM > 1) {
        ret = mpp_fb_ioctl(info->fb, AICFB_PAN_DISPLAY, &info->swap_flag);
        if (ret == 0) {
            ret = mpp_fb_ioctl(info->fb, AICFB_WAIT_FOR_VSYNC, &info->swap_flag);
            if (ret < 0) {
                printf("wait for sync error\n");
                return -1;
            }
        } else {
            printf("pan display fail\n");
            return -1;
        }
    }

    return 0;
}
