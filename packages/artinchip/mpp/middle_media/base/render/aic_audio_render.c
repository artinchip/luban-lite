/*
* Copyright (C) 2020-2023 ArtInChip Technology Co. Ltd
*
*  author: <jun.ma@artinchip.com>
*  Desc: aic_audio_render
*/
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <rtthread.h>
#include <rtdevice.h>

#include "artinchip_fb.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "aic_audio_render.h"

#define SOUND_DEVICE_NAME    "sound0"

#define AIC_AUDIO_STATUS_PLAY 0
#define AIC_AUDIO_STATUS_PAUSE 1
#define AIC_AUDIO_STATUS_STOP 2

struct aic_rt_audio_render{
    struct aic_audio_render base;
    rt_device_t snd_dev;
    struct aic_audio_render_attr attr;
    int status;

};

s32 rt_audio_render_init(struct aic_audio_render *render,s32 dev_id)
{
    long ret ;
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;
    rt_render->snd_dev = rt_device_find(SOUND_DEVICE_NAME);
    if(!rt_render->snd_dev){
        loge("rt_device_find error!\n");
        return -1;
    }
    ret = rt_device_open(rt_render->snd_dev, RT_DEVICE_OFLAG_WRONLY);
    if(ret != 0){
        loge("rt_device_open error!\n");
        return -1;
    }
    return 0;
}

s32 rt_audio_render_destroy(struct aic_audio_render *render)
{
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;
    rt_device_close(rt_render->snd_dev);
    mpp_free(rt_render);
    return 0;
}

s32 rt_audio_render_set_attr(struct aic_audio_render *render,struct aic_audio_render_attr *attr)
{
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;
    struct rt_audio_caps caps = {0};
    caps.main_type               = AUDIO_TYPE_OUTPUT;
    caps.sub_type                = AUDIO_DSP_PARAM;
    caps.udata.config.samplerate = attr->sample_rate;
    caps.udata.config.channels   = attr->channels;
    caps.udata.config.samplebits = 16;
    rt_device_control(rt_render->snd_dev, AUDIO_CTL_CONFIGURE, &caps);

    caps.main_type               = AUDIO_TYPE_MIXER;
    caps.sub_type                = AUDIO_MIXER_VOLUME;
    caps.udata.value = 65;
    rt_device_control(rt_render->snd_dev, AUDIO_CTL_CONFIGURE, &caps);

    return 0;

}

s32 rt_audio_render_get_attr(struct aic_audio_render *render,struct aic_audio_render_attr *attr)
{
    return 0;
}

s32 rt_audio_render_rend(struct aic_audio_render *render, void* pData, s32 nDataSize)
{
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;
    s32 w_len;
    w_len = rt_device_write(rt_render->snd_dev, 0, pData, nDataSize);
    if(w_len != nDataSize){
        loge("rt_device_write w_len:%d,nDataSize:%d!\n",w_len,nDataSize);
     }
    return 0;
}

s64 rt_audio_render_get_cached_time(struct aic_audio_render *render)
{
    uint32_t  delay = 0;
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;

    rt_device_control(rt_render->snd_dev, AUDIO_CTL_GETAVAIL, &delay);
    return delay;
}

s32 rt_audio_render_pause(struct aic_audio_render *render)
{
    return 0;
}

s32 rt_audio_render_get_volume(struct aic_audio_render *render)
{
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;
    struct rt_audio_caps caps = {0};
    s32 vol = 0;
    caps.main_type               = AUDIO_TYPE_MIXER;
    caps.sub_type                = AUDIO_MIXER_VOLUME;
    caps.udata.value = 0;
    rt_device_control(rt_render->snd_dev, AUDIO_CTL_GETCAPS, &caps);
    vol = caps.udata.value;
    return vol;
}

s32 rt_audio_render_set_volume(struct aic_audio_render *render,s32 vol)
{
    struct aic_rt_audio_render *rt_render = (struct aic_rt_audio_render*)render;
    struct rt_audio_caps caps = {0};
    caps.main_type               = AUDIO_TYPE_MIXER;
    caps.sub_type                = AUDIO_MIXER_VOLUME;
    caps.udata.value = vol;
    rt_device_control(rt_render->snd_dev, AUDIO_CTL_CONFIGURE, &caps);
    return 0;
}

s32 rt_audio_render_clear_cache(struct aic_audio_render *render)
{
	return 0;
}

s32 aic_audio_render_create(struct aic_audio_render **render)
{
    struct aic_rt_audio_render * rt_render;
    rt_render = mpp_alloc(sizeof(struct aic_rt_audio_render));
    if(rt_render == NULL){
        loge("mpp_alloc alsa_render fail!!!\n");
        *render = NULL;
        return -1;
    }
    rt_render->status = AIC_AUDIO_STATUS_STOP;
    rt_render->base.init = rt_audio_render_init;
    rt_render->base.destroy = rt_audio_render_destroy;
    rt_render->base.set_attr = rt_audio_render_set_attr;
    rt_render->base.rend = rt_audio_render_rend;
    rt_render->base.get_cached_time = rt_audio_render_get_cached_time;
    rt_render->base.pause = rt_audio_render_pause;
    rt_render->base.set_volume = rt_audio_render_set_volume;
    rt_render->base.get_volume = rt_audio_render_get_volume;
	rt_render->base.clear_cache = rt_audio_render_clear_cache;
    *render = &rt_render->base;
    return 0;
}

