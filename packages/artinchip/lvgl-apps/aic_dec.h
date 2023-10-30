/*
 * Copyright (C) 2022-2023 ArtinChip Technology Co., Ltd.
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef AIC_DEC_H
#define AIC_DEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <mpp_list.h>

struct framebuf_info {
    unsigned int addr;
    unsigned int align_addr;
    int size;
};

struct framebuf_head {
	struct mpp_list list;
	struct framebuf_info buf_info;
};

void aic_dec_create();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //AIC_DEC_H
