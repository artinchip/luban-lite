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
#include <video/artinchip_fb.h>
#include <sys/ioctl.h>
#include <alsa/asoundlib.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "aic_audio_render.h"

/* dev_id and dev_soun map
now only one dev
  0 -----DEV_SOUND_DEFAULT
*/

#define DEV_SOUND_DEFAULT "default"

#define AIC_AUDIO_STATUS_PLAY 0
#define AIC_AUDIO_STATUS_PAUSE 1
#define AIC_AUDIO_STATUS_STOP 2




struct aic_alsa_audio_render{
	struct aic_audio_render base;
	struct aic_audio_render_attr attr;
	snd_pcm_t      *alsa_handle;
	snd_pcm_hw_params_t *alse_hw_param;
	int status;

};


s32 alsa_audio_render_init(struct aic_audio_render *render,s32 dev_id)
{
	s32 ret = 0;
	s32 rate;
	snd_pcm_uframes_t period_size = 32*1024;
	snd_pcm_hw_params_t *alse_hw_param;
	struct aic_alsa_audio_render *alsa_render = (struct aic_alsa_audio_render*)render;

	if(render == NULL){
		loge("param error!!!\n");
		return -1;
	}

	if(snd_pcm_open(&alsa_render->alsa_handle, DEV_SOUND_DEFAULT, SND_PCM_STREAM_PLAYBACK, 0) < 0){
		loge("snd_pcm_open failed!!!\n");
		return -1;
	}
	snd_pcm_hw_params_alloca(&alse_hw_param);
	alsa_render->alse_hw_param = alse_hw_param;
	if(snd_pcm_hw_params_any(alsa_render->alsa_handle, alsa_render->alse_hw_param) < 0){
		loge("snd_pcm_hw_params_any failed!!!\n");
	}
	if(snd_pcm_hw_params_set_access(alsa_render->alsa_handle, alsa_render->alse_hw_param,SND_PCM_ACCESS_RW_INTERLEAVED) < 0){
		loge("snd_pcm_hw_params_set_access failed!!!\n");
		return -1;
	}
	if(snd_pcm_hw_params_set_format(alsa_render->alsa_handle, alsa_render->alse_hw_param, SND_PCM_FORMAT_S16_LE) < 0){
		loge("snd_pcm_hw_params_set_format failed!!!\n");
		return -1;
	}
	if(snd_pcm_hw_params_set_channels(alsa_render->alsa_handle, alsa_render->alse_hw_param, alsa_render->attr.channels) < 0){
		loge("snd_pcm_hw_params_set_channels failed!!!\n");
		return -1;
	}
	rate = alsa_render->attr.sample_rate;
	if(snd_pcm_hw_params_set_rate_near(alsa_render->alsa_handle, alsa_render->alse_hw_param, &rate, 0) < 0){
		loge("snd_pcm_hw_params_set_rate_near failed!!!\n");
		return -1;
	}
	if ((float)alsa_render->attr.sample_rate * 1.05 < rate || (float)alsa_render->attr.sample_rate * 0.95 > rate) {
		logw("rate is not accurate(request: %d, got: %d)", alsa_render->attr.sample_rate, rate);
	}

	period_size = alsa_render->attr.smples_per_frame;

	if(snd_pcm_hw_params_set_period_size_near(alsa_render->alsa_handle, alsa_render->alse_hw_param, &period_size, 0) < 0){
		loge("snd_pcm_hw_params_set_period_size_near failed!!!\n");
		return -1;
	}

	period_size = 4*period_size;

	if(snd_pcm_hw_params_set_buffer_size_near(alsa_render->alsa_handle, alsa_render->alse_hw_param, &period_size) < 0){
		loge("snd_pcm_hw_params_set_period_size_near failed!!!\n");
		return -1;
	}


	if(snd_pcm_hw_params(alsa_render->alsa_handle, alsa_render->alse_hw_param) < 0){
		loge("Unable to install hw params:");
		return -1;
	}

	alsa_render->status = AIC_AUDIO_STATUS_PLAY;

	return 0;
}


s32 alsa_audio_render_destroy(struct aic_audio_render *render)
{
	struct aic_alsa_audio_render *alsa_render = (struct aic_alsa_audio_render*)render;

	if(render == NULL){
		loge("param error!!!\n");
		return -1;
	}
	if (alsa_render->alsa_handle){
		if (snd_pcm_drain(alsa_render->alsa_handle) < 0) {
			loge("snd_pcm_drain failed");
		}
		if (snd_pcm_close(alsa_render->alsa_handle) < 0){
			loge("snd_pcm_close failed!!!\n");
			return -1;
		}else{
			alsa_render->alsa_handle = NULL;
			logd("snd_pcm_close ok \n");
		}
	}
	mpp_free(alsa_render);
	return 0;
}

s32 alsa_audio_render_set_attr(struct aic_audio_render *render,struct aic_audio_render_attr *attr)
{
	struct aic_alsa_audio_render *alsa_render = (struct aic_alsa_audio_render*)render;
	if(render == NULL || attr == NULL){
		loge("param error!!!\n");
		return -1;
	}
	alsa_render->attr.bits_per_sample = attr->bits_per_sample;
	alsa_render->attr.channels = attr->channels;
	alsa_render->attr.sample_rate = attr->sample_rate;
	alsa_render->attr.smples_per_frame = attr->smples_per_frame;
	logi("bits_per_sample:%d,"\
		 "channels:%d,"\
		 "sample_rate:%d,"\
		 "smples_per_frame:%d\n"\
		 ,alsa_render->attr.bits_per_sample
		 ,alsa_render->attr.channels
		 ,alsa_render->attr.sample_rate
		 ,alsa_render->attr.smples_per_frame);
	return 0;

}

s32 alsa_audio_render_get_attr(struct aic_audio_render *render,struct aic_audio_render_attr *attr)
{
	struct aic_alsa_audio_render *alsa_render = (struct aic_alsa_audio_render*)render;
	if(render == NULL || attr == NULL){
		loge("param error!!!\n");
		return -1;
	}
	attr->bits_per_sample = alsa_render->attr.bits_per_sample;
	attr->channels = alsa_render->attr.channels;
	attr->sample_rate = alsa_render->attr.sample_rate;
	attr->smples_per_frame = alsa_render->attr.smples_per_frame;
	logd("bits_per_sample:%d,"\
		 "channels:%d,"\
		 "sample_rate:%d,"\
		 "smples_per_frame:%d\n"\
		 ,attr->bits_per_sample
		 ,attr->channels
		 ,attr->sample_rate
		 ,attr->smples_per_frame);
	return 0;


}


s32 alsa_audio_render_rend(struct aic_audio_render *render, void* pData, s32 nDataSize)
{
	struct aic_alsa_audio_render *alsa_render = (struct aic_alsa_audio_render*)render;
	s32 ret = 0;
	s32 count = 0;
	s32 pos = 0;

	if(alsa_render == NULL || pData == NULL || nDataSize == 0){
		loge("param error!!!\n");
		return -1;
	}
	count = nDataSize/(alsa_render->attr.channels*alsa_render->attr.bits_per_sample/8);
	while(count > 0) {
		ret = snd_pcm_writei(alsa_render->alsa_handle, pData+pos, count);
		if(ret == -EAGAIN || (ret>=0 && ret < count)) {
			snd_pcm_wait(alsa_render->alsa_handle, 100);
			logi("wait 100!!!\n");
		} else if(ret == -EPIPE) {// underrun
			ret = snd_pcm_prepare(alsa_render->alsa_handle);
			if (ret < 0)
				loge("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(ret));
			ret = 0;
			logi("snd_pcm_prepare!!!\n");
		} else if(ret == -ESTRPIPE){
			while ((ret = snd_pcm_resume(alsa_render->alsa_handle)) == -EAGAIN){
				usleep(1000);
				logi("snd_pcm_resume!!!\n");
			}
		} else if(ret < 0) {
			loge("write error, ret: %d", ret);
			return -1;
		}
		//logi("ret:%d\n",ret);
		if(ret) {
			count -= ret;
			pos += ret * alsa_render->attr.channels*alsa_render->attr.bits_per_sample/8;
		}
	}
	return 0;

}

s64 alsa_audio_render_get_cached_time(struct aic_audio_render *render)
{
	struct aic_alsa_audio_render *alsa_render = (struct aic_alsa_audio_render*)render;
	snd_pcm_sframes_t delayp;
	s64 delay_us;
	s32 ret = snd_pcm_delay(alsa_render->alsa_handle,&delayp);
	if(ret == 0){
		delay_us = (s64)(((float) delayp * 1000000)/alsa_render->attr.sample_rate);
	}else{
		delay_us = 0;
	}
	return delay_us;
}

s32 alsa_audio_render_pause(struct aic_audio_render *render){
	struct aic_alsa_audio_render *alsa_render = (struct aic_alsa_audio_render*)render;
	int ret = 0;
	if(alsa_render->status == AIC_AUDIO_STATUS_PLAY){
		loge("AIC_AUDIO_STATUS_PLAY,snd_pcm_hw_params_can_pause:%d\n",snd_pcm_hw_params_can_pause(alsa_render->alse_hw_param));
		ret = snd_pcm_pause(alsa_render->alsa_handle, 1);
		if(ret == 0){
			alsa_render->status = AIC_AUDIO_STATUS_PAUSE;
			loge("enter AIC_AUDIO_STATUS_PAUSE\n");
		}else{
			loge("snd_pcm_pause fail,ret:%d\n",ret);
		}
	}else if(alsa_render->status == AIC_AUDIO_STATUS_PAUSE){
		loge("AIC_AUDIO_STATUS_PAUSE");
		//while(snd_pcm_resume(alsa_render->alsa_handle)== -EAGAIN){
			//usleep(10*1000);
		//}
		ret = snd_pcm_pause(alsa_render->alsa_handle, 0);
		loge("enter AIC_AUDIO_STATUS_PLAY,ret:%d\n",ret);

		alsa_render->status = AIC_AUDIO_STATUS_PLAY;


	}else{
		loge("invaild state\n");
	}
}

s32 alsa_audio_render_get_volume(struct aic_audio_render *render)
{
	return 0;
}

s32 alsa_audio_render_set_volume(struct aic_audio_render *render,s32 vol)
{
	return 0;
}



s32 aic_audio_render_create(struct aic_audio_render **render)
{
	struct aic_alsa_audio_render * alsa_render;
	alsa_render = mpp_alloc(sizeof(struct aic_alsa_audio_render));
	if(alsa_render == NULL){
		loge("mpp_alloc alsa_render fail!!!\n");
		*render = NULL;
		return -1;
	}
	alsa_render->status = AIC_AUDIO_STATUS_STOP;
	alsa_render->base.init = alsa_audio_render_init;
	alsa_render->base.destroy = alsa_audio_render_destroy;
	alsa_render->base.set_attr = alsa_audio_render_set_attr;
	alsa_render->base.rend = alsa_audio_render_rend;
	alsa_render->base.get_cached_time = alsa_audio_render_get_cached_time;
	alsa_render->base.pause = alsa_audio_render_pause;
	alsa_render->base.set_volume = alsa_audio_render_set_volume;
	alsa_render->base.get_volume = alsa_audio_render_get_volume;
	*render = &alsa_render->base;
	return 0;
}

