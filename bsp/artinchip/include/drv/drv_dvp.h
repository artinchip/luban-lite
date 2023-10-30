/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_DRV_DVP_H_
#define _ARTINCHIP_DRV_DVP_H_

#include "aic_osal.h"
#include "aic_list.h"

#include "hal_dvp.h"
#include "mpp_types.h"
#include "mpp_vin_vb.h"
#include "mpp_vin.h"

#define AIC_DVP_NAME    "aic-dvp"

#ifdef AIC_DVP_DRV_V10
#define AIC_DVP_CLK_RATE    200000000   /* 200MHz */
#endif
#ifdef AIC_DVP_DRV_V11
#define AIC_DVP_CLK_RATE    150000000   /* 150MHz */
#endif
#define AIC_DVP_QOS_HIGH    0xB
#define AIC_DVP_QOS_LOW     0x7

extern const struct v4l2_subdev_ops aic_dvp_subdev_ops;

struct aic_dvp {
    struct aic_dvp_config   cfg; /* The configuration of DVP HW */
    struct dvp_out_fmt      fmt; /* The format of output data */

    /* Videobuf */
    struct vb_queue         queue;
    struct list_head        active_list;
    aicos_mutex_t           active_lock; /* lock of active buf list */
    unsigned int            sequence;
    unsigned int            streaming;

    aicos_mutex_t           lock;
};

int aic_dvp_set_in_fmt(struct mpp_video_fmt *fmt);
int aic_dvp_set_out_fmt(struct dvp_out_fmt *fmt);
int aic_dvp_stream_on(void);
int aic_dvp_stream_off(void);

int aic_dvp_req_buf(char *buf, u32 size, struct vin_video_buf *vbuf);
int aic_dvp_q_buf(u32 index);
int aic_dvp_dq_buf(u32 *pindex);

int aic_dvp_probe(void);
int aic_dvp_open(void);
int aic_dvp_close(void);

#endif /* _ARTINCHIP_DRV_DVP_H_ */
