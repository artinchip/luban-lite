/*
* Copyright (C) 2020-2023 ArtInChip Technology Co. Ltd
*
*  author: <jun.ma@artinchip.com>
*  Desc: aic_video_render
*/
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "mpp_types.h"
#include "artinchip_fb.h"
#include "aic_core.h"

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_list.h"
#include "aic_render.h"



#define DEV_FB "aicfb"


struct aic_fb_video_render{
	struct aic_video_render base;
	struct aicfb_layer_data layer;
	s32 fd;
	rt_device_t render_dev;
};

static s32 fb_video_render_init(struct aic_video_render *render,s32 layer,s32 dev_id)
{
    struct aic_fb_video_render *fb_render = (struct aic_fb_video_render*)render;
    fb_render->render_dev = rt_device_find(DEV_FB);

    if (fb_render->render_dev  == NULL) {
        loge("rt_device_find aicfb failed!");
        return -1;
    }

    if (strcmp(PRJ_CHIP,"d12x") == 0) {
        fb_render->layer.layer_id = AICFB_LAYER_TYPE_UI;
        rt_device_control(fb_render->render_dev,AICFB_GET_LAYER_CONFIG,&fb_render->layer);
    } else {
        fb_render->layer.layer_id = layer;
        rt_device_control(fb_render->render_dev,AICFB_GET_LAYER_CONFIG,&fb_render->layer);
        fb_render->layer.enable = 0;
        rt_device_control(fb_render->render_dev,AICFB_UPDATE_LAYER_CONFIG,&fb_render->layer);
    }

    return 0;
}


static s32 fb_video_render_destroy(struct aic_video_render *render)
{
    struct aic_fb_video_render *fb_render = (struct aic_fb_video_render*)render;

    if (strcmp(PRJ_CHIP,"d12x") == 0) {
        mpp_free(fb_render);
        return 0;
    }
    fb_render->layer.enable = 0;
    if (rt_device_control(fb_render->render_dev,AICFB_UPDATE_LAYER_CONFIG,&fb_render->layer) < 0) {
        loge("fb ioctl() AICFB_UPDATE_LAYER_CONFIG failed!");
    }
    mpp_free(fb_render);
    return 0;
}


static s32 fb_video_render_rend(struct aic_video_render *render,struct mpp_frame *frame_info)
{

	struct aic_fb_video_render *fb_render = (struct aic_fb_video_render*)render;

	if (frame_info == NULL) {
		loge("frame_info=NULL\n");
		return -1;
	}
	fb_render->layer.enable = 1;
	memcpy(&fb_render->layer.buf,&frame_info->buf,sizeof(struct mpp_buf));

	if (rt_device_control(fb_render->render_dev, AICFB_UPDATE_LAYER_CONFIG, &fb_render->layer) < 0)
		loge("fb ioctl() AICFB_UPDATE_LAYER_CONFIG failed!");

	//wait vsync (wait layer config)
	rt_device_control(fb_render->render_dev,AICFB_WAIT_FOR_VSYNC,&fb_render->layer);

	return 0;
}

static s32 fb_video_render_get_screen_size(struct aic_video_render *render,struct mpp_size *size)
{
	s32 ret = 0;
	struct aicfb_screeninfo screen_info;

	struct aic_fb_video_render *fb_render = (struct aic_fb_video_render*)render;
	if(size == NULL) {
		loge("bad params!!!!\n");
		return -1;
	}
	if (rt_device_control(fb_render->render_dev, AICFB_GET_SCREENINFO, &screen_info) < 0) {
		loge("fb ioctl() AICFB_GET_SCREEN_SIZE failed!");
		return -1;
	}
	size->width = screen_info.width;
	size->height = screen_info.height;
	return ret;
}


static s32 fb_video_render_set_dis_rect(struct aic_video_render *render,struct mpp_rect *rect)
{
	struct aic_fb_video_render *fb_render = (struct aic_fb_video_render*)render;
	if (rect == NULL) {
		loge("param error rect==NULL\n");
		return -1;
	}
	fb_render->layer.pos.x = rect->x;
	fb_render->layer.pos.y = rect->y;
	fb_render->layer.scale_size.width = rect->width;
	fb_render->layer.scale_size.height = rect->height;
	return 0;
}


static s32 fb_video_render_get_dis_rect(struct aic_video_render *render,struct mpp_rect *rect)
{
	struct aic_fb_video_render *fb_render = (struct aic_fb_video_render*)render;
	if (rect == NULL) {
		loge("param error rect==NULL\n");
		return -1;
	}
	rect->x = fb_render->layer.pos.x;
	rect->y = fb_render->layer.pos.y;
	rect->width = fb_render->layer.scale_size.width;
	rect->height = fb_render->layer.scale_size.height;
	return 0;
}


static s32 fb_video_render_set_on_off(struct aic_video_render *render,s32 enable)
{
	struct aic_fb_video_render *fb_render = (struct aic_fb_video_render*)render;
	fb_render->layer.enable = enable;
	return 0;
}

static s32 fb_video_render_get_on_off(struct aic_video_render *render,s32 *enable)
{
	struct aic_fb_video_render *fb_render = (struct aic_fb_video_render*)render;
	if (enable == NULL) {
		loge("param error rect==NULL\n");
		return -1;
	}
	*enable = fb_render->layer.enable;
	return 0;
}

s32 aic_video_render_create(struct aic_video_render **render)
{
	struct aic_fb_video_render * fb_render;
	fb_render = mpp_alloc(sizeof(struct aic_fb_video_render));
	if (fb_render == NULL) {
		loge("mpp_alloc fb_render fail!!!\n");
		*render = NULL;
		return -1;
	}

	memset(fb_render,0x00,sizeof(struct aic_fb_video_render));

	fb_render->base.init = fb_video_render_init;
	fb_render->base.destroy = fb_video_render_destroy;
	fb_render->base.rend = fb_video_render_rend;
	fb_render->base.set_dis_rect = fb_video_render_set_dis_rect;
	fb_render->base.get_dis_rect = fb_video_render_get_dis_rect;
	fb_render->base.set_on_off = fb_video_render_set_on_off;
	fb_render->base.get_on_off = fb_video_render_get_on_off;
	fb_render->base.get_screen_size = fb_video_render_get_screen_size;

	*render = &fb_render->base;
	return 0;
}


