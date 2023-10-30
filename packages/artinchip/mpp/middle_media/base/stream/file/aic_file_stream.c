/*
* Copyright (C) 2020-2023 ArtInChip Technology Co. Ltd
*
*  author: <jun.ma@artinchip.com>
*  Desc: aic_file_stream
*/

/*why the macro definition is placed here:
after the header file ,the complier error*/

//#define _LARGEFILE64_SOURCE

//#ifndef _GNU_SOURCE
//#define _GNU_SOURCE
//#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>




#include "mpp_mem.h"
#include "mpp_log.h"
#include "aic_stream.h"
#include "aic_file_stream.h"

struct aic_file_stream{
	struct aic_stream base;
	s32 fd;
	s64 file_size;
};

static s64 file_stream_read(struct aic_stream *stream, void *buf, s64 len)
{
	s64 ret;
	struct aic_file_stream *file_stream = (struct aic_file_stream *)stream;
	ret = read(file_stream->fd, buf, len);
	return ret;
}

static s64 file_stream_tell(struct aic_stream *stream)
{
	struct aic_file_stream *file_stream = (struct aic_file_stream *)stream;
	//return lseek64(file_stream->fd, 0, SEEK_CUR);
	return lseek(file_stream->fd, 0, SEEK_CUR);
}

static s32 file_stream_close(struct aic_stream *stream)
{
	struct aic_file_stream *file_stream = (struct aic_file_stream *)stream;
	close(file_stream->fd);
	mpp_free(file_stream);
	return 0;
}

static s64 file_stream_seek(struct aic_stream *stream, s64 offset, s32 whence)
{
	struct aic_file_stream *file_stream = (struct aic_file_stream *)stream;
	//return lseek64(file_stream->fd, offset, whence);
	return lseek(file_stream->fd, offset, whence);
}

static s64 file_stream_size(struct aic_stream *stream)
{
	struct aic_file_stream *file_stream = (struct aic_file_stream *)stream;
	return file_stream->file_size;
}

s32 file_stream_open(const char* uri,struct aic_stream **stream)
{
	s32 ret = 0;

	struct aic_file_stream *file_stream = (struct aic_file_stream *)mpp_alloc(sizeof(struct aic_file_stream));
	if(file_stream == NULL){
		loge("mpp_alloc aic_stream ailed!!!!!\n");
		ret = -1;
		goto exit;
	}

	//file_stream->fd = open(uri, O_RDWR|O_LARGEFILE);
	file_stream->fd = open(uri, O_RDWR);
	if(file_stream->fd < 0){
		loge("open uri:%s failed!!!!!\n",uri);
		ret = -2;
		goto exit;
	}

	//file_stream->file_size = lseek64(file_stream->fd, 0, SEEK_END);
	//lseek64(file_stream->fd, 0, SEEK_SET);
	file_stream->file_size = lseek(file_stream->fd, 0, SEEK_END);
	lseek(file_stream->fd, 0, SEEK_SET);

	file_stream->base.read =  file_stream_read;
	file_stream->base.close = file_stream_close;
	file_stream->base.seek = file_stream_seek;
	file_stream->base.size =  file_stream_size;
	file_stream->base.tell = file_stream_tell;
	*stream = &file_stream->base;
	return ret;

exit:
	if(file_stream != NULL){
		mpp_free(file_stream);
	}
	*stream = NULL;
	return ret;
}
