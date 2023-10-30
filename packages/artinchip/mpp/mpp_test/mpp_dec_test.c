/*
* Copyright (C) 2020-2022 Artinchip Technology Co. Ltd
*
*  author: <qi.xu@artinchip.com>
*  Desc: video decode demo
*/

#define LOG_TAG "dec_test"

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "artinchip_fb.h"
#include <getopt.h>
#include <rthw.h>
#include <rtthread.h>
#include "aic_core.h"

#include "bit_stream_parser.h"
#include "mpp_decoder.h"
#include "mpp_mem.h"
#include "mpp_log.h"

#define FRAME_BUF_NUM		(18)
#define MAX_TEST_FILE           (64)
#define SCREEN_WIDTH            1024
#define SCREEN_HEIGHT           600
#define FRAME_COUNT		30

struct frame_info {
	int fd[3];		// dma-buf fd
	int fd_num;		// number of dma-buf
	int used;		// if the dma-buf of this frame add to de drive
};

enum aic_thread_status {
	AIC_THREAD_STATUS_EXIT = 0,
	AIC_THREAD_STATUS_RUNNING,
	AIC_THREAD_STATUS_SUSPENDING ,
};

#define PTHREAD_DEFAULT_STACK_SIZE 8192
#define PTHREAD_DEFAULT_GUARD_SIZE 256
#define PTHREAD_DEFAULT_PRIORITY   30
#define PTHREAD_DEFAULT_SLICE      10
#define PTHREAD_NAME_MAX_LEN  16


struct dec_ctx {
	struct mpp_decoder  *decoder;
	struct frame_info frame_info[FRAME_BUF_NUM];	//

	struct bit_stream_parser *parser;

	int stream_eos;
	int render_eos;
	int dec_err;
	int cmp_data_err;

	char file_input[MAX_TEST_FILE][128];	// test file name
	int file_num;				// test file number

	int output_format;
	int display_en;

	rt_device_t render_dev;

	aicos_thread_t render_thread;
	aicos_thread_t decode_thread;

	enum aic_thread_status render_thread_status;
	enum aic_thread_status decode_thread_status;

};

static void print_help(const char* prog)
{
	printf("name: %s\n", prog);
	printf("Compile time: %s\n", __TIME__);
	printf("Usage: mpp_test [options]:\n"
		"\t-i                             input stream file name\n"
		"\t-t                             directory of test files\n"
		"\t-d                             display the picture\n"
		"\t-f                             output pixel format\n"
		"\t-l                             loop time\n"
		"\t-h                             help\n\n"
		"Example1(test single file): mpp_test -i /sdmc/test.264\n"
		"Example2(test some files) : mpp_test -t /sdmc/\n");
}

static long long get_now_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000ll + tv.tv_usec;
}

static int set_fb_layer_alpha(struct dec_ctx *ctx,int val)
{
	int ret = 0;
	struct aicfb_alpha_config alpha = {0};

	alpha.layer_id = 1;
	alpha.enable = 1;
	alpha.mode = 1;
	alpha.value = val;
	ret = rt_device_control(ctx->render_dev,AICFB_UPDATE_ALPHA_CONFIG,&alpha);

    if (ret < 0)
        loge("ioctl() failed! errno: %d\n", ret);

	return ret;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))
static void video_layer_set(struct dec_ctx *ctx,struct mpp_buf *picture_buf, struct frame_info* frame)
{
	struct aicfb_layer_data layer = {0};
	int ret = 0;
	layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
	layer.enable = 1;
	if (picture_buf->format == MPP_FMT_YUV420P || picture_buf->format == MPP_FMT_NV12
	  || picture_buf->format == MPP_FMT_NV21) {
		  // rgb format not support scale
		layer.scale_size.width = SCREEN_WIDTH;
		layer.scale_size.height= SCREEN_HEIGHT;
	}

	layer.pos.x = 0;
	layer.pos.y = 0;
	memcpy(&layer.buf, picture_buf, sizeof(struct mpp_buf));

	logi("width: %d, height %d, stride: %d, %d, crop_en: %d, crop_w: %d, crop_h: %d",
		layer.buf.size.width, layer.buf.size.height,
		layer.buf.stride[0], layer.buf.stride[1], layer.buf.crop_en,
		layer.buf.crop.width, layer.buf.crop.height);

	ret = rt_device_control(ctx->render_dev,AICFB_UPDATE_LAYER_CONFIG,&layer);

    if(ret < 0) {
        loge("update_layer_config error, %d", ret);
    }

	rt_device_control(ctx->render_dev,AICFB_WAIT_FOR_VSYNC,&layer);


}

static int send_data(struct dec_ctx *data, unsigned char* buf, int buf_size)
{
	int ret = 0;
	struct mpp_packet packet;
	memset(&packet, 0, sizeof(struct mpp_packet));

	// get an empty packet
	do {
		if(data->dec_err) {
			loge("decode error, break now");
			return -1;
		}

		ret = mpp_decoder_get_packet(data->decoder, &packet, buf_size);
		//logd("mpp_dec_get_packet ret: %x", ret);
		if (ret == 0) {
			break;
		}
		//usleep(1000);
		aicos_msleep(1);
	} while (1);

	memcpy(packet.data, buf, buf_size);
	packet.size = buf_size;

	if (data->stream_eos)
		packet.flag |= PACKET_FLAG_EOS;

	ret = mpp_decoder_put_packet(data->decoder, &packet);
	logd("mpp_dec_put_packet ret %d", ret);

	return ret;
}

static void swap(int *a, int *b)
{
	int tmp = *a;
	*a = *b;
	*b = tmp;
}

void render_thread(void *p)
{
	int cur_frame_id = 0;
	int last_frame_id = 1;
	struct mpp_frame frame[2];
	struct mpp_buf *pic_buffer = NULL;
	int frame_num = 0;
	int ret;
	long long time = 0;
	long long duration_time = 0;
	int disp_frame_cnt = 0;
	long long total_duration_time = 0;
	int total_disp_frame_cnt = 0;
	struct dec_ctx *data = (struct dec_ctx*)p;

	data->render_thread_status = AIC_THREAD_STATUS_RUNNING;

	data->render_dev = rt_device_find("aicfb");

	time = get_now_us();
	//* 2. render frame until eos
	while(!data->render_eos) {
		memset(&frame[cur_frame_id], 0, sizeof(struct mpp_frame));

		if(data->dec_err)
			break;

		//* 2.1 get frame
		ret = mpp_decoder_get_frame(data->decoder, &frame[cur_frame_id]);
		if(ret == DEC_NO_RENDER_FRAME || ret == DEC_ERR_FM_NOT_CREATE
		|| ret == DEC_NO_EMPTY_FRAME) {
			//usleep(10000);
			aicos_msleep(10);
			continue;
		} else if(ret) {
			logw("mpp_dec_get_frame error, ret: %x", ret);
			data->dec_err = 1;
			break;
		}

		time = get_now_us() - time;
		duration_time += time;
		disp_frame_cnt += 1;
		data->render_eos = frame[cur_frame_id].flags & FRAME_FLAG_EOS;
		logi("decode_get_frame successful: frame id %d, number %d, flag: %d",
			frame[cur_frame_id].id, frame_num, frame[cur_frame_id].flags);

		pic_buffer = &frame[cur_frame_id].buf;

		//* 2.3 disp frame;
		if(data->display_en) {
			set_fb_layer_alpha(data,10);
			video_layer_set(data,pic_buffer, &data->frame_info[frame[cur_frame_id].id]);
		}

		//* 2.4 return the last frame
		if(frame_num) {
			ret = mpp_decoder_put_frame(data->decoder, &frame[last_frame_id]);
		}

		swap(&cur_frame_id, &last_frame_id);

		if(disp_frame_cnt > FRAME_COUNT) {
			float fps, avg_fps;
			total_disp_frame_cnt += disp_frame_cnt;
			total_duration_time += duration_time;
			fps = (float)(duration_time / 1000.0f);
			fps = (disp_frame_cnt * 1000) / fps;
			avg_fps = (float)(total_duration_time / 1000.0f);
			avg_fps = (total_disp_frame_cnt * 1000) / avg_fps;
			logi("decode speed info: fps: %.2f, avg_fps: %.2f", fps, avg_fps);
			duration_time = 0;
			disp_frame_cnt = 0;
		}
		time = get_now_us();
		frame_num++;
		if(data->display_en) {
			aicos_msleep(30);
		}
	}

	//* put the last frame when eos
	mpp_decoder_put_frame(data->decoder, &frame[last_frame_id]);

	data->render_thread_status = AIC_THREAD_STATUS_EXIT;

}

void decode_thread(void *p)
{
	struct dec_ctx *data = (struct dec_ctx*)p;
	int ret = 0;

	int dec_num = 0;
	data->decode_thread_status = AIC_THREAD_STATUS_RUNNING;
	while(!data->render_eos) {
		ret = mpp_decoder_decode(data->decoder);
		if(ret == DEC_NO_READY_PACKET || ret == DEC_NO_EMPTY_FRAME) {
			aicos_msleep(1);
			continue;
		} else if( ret ) {
			logw("decode ret: %x", ret);
			data->dec_err = 1;
			break;
		}
		dec_num ++;
		aicos_msleep(1);
	}

	data->decode_thread_status = AIC_THREAD_STATUS_EXIT;

}

static int get_file_size(char* path)
{
    struct stat st;
    stat(path, &st);

    logi("mode: %d, size: %d", (int)st.st_mode, (int)st.st_size);

    return st.st_size;
}

int dec_decode(struct dec_ctx *data, char* filename)
{
	int ret;
	int file_fd;
	unsigned char *buf = NULL;
	size_t buf_size = 0;
	int dec_type = MPP_CODEC_VIDEO_DECODER_H264;

	logd("dec_test start");

	if (filename) {
		char* ptr = strrchr(filename, '.');
		if (!strcmp(ptr, ".h264") || !strcmp(ptr, ".264")) {
			dec_type = MPP_CODEC_VIDEO_DECODER_H264;
		} else if (!strcmp(ptr, ".jpg")) {
			dec_type = MPP_CODEC_VIDEO_DECODER_MJPEG;
		} else if (!strcmp(ptr, ".png")) {
			dec_type = MPP_CODEC_VIDEO_DECODER_PNG;
		}
		logi("file type: 0x%02X", dec_type);
	}

	//* 1. read data
	file_fd = open(filename, O_RDONLY);
	if (file_fd < 0) {
		loge("failed to open input file %s", filename);
		ret = -1;
		goto out;
	}
	buf_size = get_file_size(filename);

	//* 2. create and init mpp_decoder
	data->decoder = mpp_decoder_create(dec_type);
	if (!data->decoder) {
		loge("mpp_dec_create failed");
		ret = -1;
		goto out;
	}

	struct decode_config config;
	if(dec_type == MPP_CODEC_VIDEO_DECODER_PNG || dec_type == MPP_CODEC_VIDEO_DECODER_MJPEG)
		config.bitstream_buffer_size = (buf_size + 1023) & (~1023);
	else
		config.bitstream_buffer_size = 1024*1024;
	config.extra_frame_num = 1;
	config.packet_count = 10;
	config.pix_fmt = data->output_format;
	if(dec_type == MPP_CODEC_VIDEO_DECODER_PNG)
		config.pix_fmt = MPP_FMT_ARGB_8888;
	ret = mpp_decoder_init(data->decoder, &config);
	if (ret) {
		logd("%p mpp_dec_init type %d failed", data->decoder, dec_type);
		goto out;
	}

	//* 3. create decode thread
	data->decode_thread = aicos_thread_create("decode_thread"
												,PTHREAD_DEFAULT_STACK_SIZE
												,PTHREAD_DEFAULT_PRIORITY
												,decode_thread
												,data);

	//* 4. create render thread
	data->decode_thread = aicos_thread_create("render_thread"
												,PTHREAD_DEFAULT_STACK_SIZE
												,PTHREAD_DEFAULT_PRIORITY
												,render_thread
												,data);

	//* 5. send data
	if (dec_type == MPP_CODEC_VIDEO_DECODER_H264) {
		data->parser = bs_create(file_fd);
		struct mpp_packet packet;
		memset(&packet, 0, sizeof(struct mpp_packet));

		while((packet.flag & PACKET_FLAG_EOS) == 0) {
			memset(&packet, 0, sizeof(struct mpp_packet));
			bs_prefetch(data->parser, &packet);
			logi("bs_prefetch, size: %d", packet.size);

			// get an empty packet
			do {
				if(data->dec_err) {
					loge("decode error, break now");
					return -1;
				}

				ret = mpp_decoder_get_packet(data->decoder, &packet, packet.size);
				//logd("mpp_dec_get_packet ret: %x", ret);
				if (ret == 0) {
					break;
				}
				aicos_msleep(1);
			} while (1);


			bs_read(data->parser, &packet);
			ret = mpp_decoder_put_packet(data->decoder, &packet);
		}

		bs_close(data->parser);
	} else {
		buf = (unsigned char *)mpp_alloc(buf_size);
		if (!buf) {
			loge("malloc buf failed");
			ret = -1;
			goto out;
		}

		if(read(file_fd, buf, buf_size) <= 0) {
			loge("read data error");
			data->stream_eos = 1;
			ret = -1;
			goto out;
		}

		data->stream_eos = 1;
		ret = send_data(data, buf, buf_size);
	}

	while(data->decode_thread_status != AIC_THREAD_STATUS_EXIT){
		//usleep(1000);
		aicos_msleep(1);
	}
	while(data->render_thread_status != AIC_THREAD_STATUS_EXIT){
		aicos_msleep(1);
	}

	if(data->cmp_data_err)
		ret = -1;

out:
	if (data->decoder) {
		mpp_decoder_destory(data->decoder);
		data->decoder = NULL;
	}

	if (buf)
		mpp_free(buf);
	if(file_fd)
		close(file_fd);

	return ret;
}

void mpp_dec_test(int argc, char **argv)
{
    logi("mpp_dec_test");
    //usleep(1000);
    aicos_msleep(1);
	int ret = 0;
	int i, j;
	int opt;
	int loop_time = 1;

	struct dec_ctx* dec_data = (struct dec_ctx*)mpp_alloc(sizeof(struct dec_ctx));;
	memset(dec_data, 0, sizeof(struct dec_ctx));
	dec_data->output_format = MPP_FMT_YUV420P;

    optind = 0;
	while (1) {
		opt = getopt(argc, argv, "i:t:f:l:dh");
		if (opt == -1) {
			break;
		}

		logi("opt: %c", opt);
		switch (opt) {
		case 'i':
			strcpy(dec_data->file_input[0], optarg);
			dec_data->file_num = 1;
			logd("file path: %s", dec_data->file_input[0]);

			break;
		case 'f':
			if (!strcmp(optarg, "nv12")) {
				logi("output format nv12");
				dec_data->output_format = MPP_FMT_NV12;
			} else if(!strcmp(optarg, "nv21")) {
				logi("output format nv21");
				dec_data->output_format = MPP_FMT_NV21;
			} else if(!strcmp(optarg, "yuv420")) {
				logi("output format yuv420");
				dec_data->output_format = MPP_FMT_YUV420P;
			}
			break;
		case 'l':
			loop_time = atoi(optarg);
			break;
		case 't':
			//read_dir(optarg, dec_data);
			break;
		case 'd':
			dec_data->display_en = 1;
			break;
		case 'h':
			print_help(argv[0]);
		default:
			goto out;
		}
	}

	if(dec_data->file_num == 0) {
		print_help(argv[0]);
		ret = -1;
		goto out;
	}

	for(j=0; j<loop_time; j++) {
		logi("loop: %d", j);
		for(i=0; i<dec_data->file_num; i++) {
			dec_data->render_eos = 0;
			dec_data->stream_eos = 0;
			dec_data->cmp_data_err = 0;
			dec_data->dec_err = 0;

			memset(dec_data->frame_info, 0, sizeof(struct frame_info)*FRAME_BUF_NUM);
			ret = dec_decode(dec_data, dec_data->file_input[i]);
			if (0 == ret)
				logi("test successful!");
			else
				logw("test failed! ret %d", ret);
		}
	}

out:
    mpp_free(dec_data);
	return;
}

#ifdef AIC_VE_DRV_V10
MSH_CMD_EXPORT_ALIAS(mpp_dec_test, video_dec, video decode test);
#endif
