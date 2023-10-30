/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_MPP_VIN_VB_H_
#define _ARTINCHIP_MPP_VIN_VB_H_

#include "aic_common.h"
#include "aic_list.h"

#include "mpp_vin.h"

enum vb_buffer_state {
    VB_BUF_STATE_DEQUEUED,   // under userspace control
    VB_BUF_STATE_QUEUED,     // queued in videobuf, but not in driver
    VB_BUF_STATE_ACTIVE,     // under driver control, possibly used in HW
    VB_BUF_STATE_DONE,       // returned from driver, not yet dequeued to user
    VB_BUF_STATE_ERROR,      // returned from driver as DONE, but the buffer has
                             // ended with an error, which will be reported
                             // to the user when it is dequeued
};

struct vb_plane {
    dma_addr_t          buf;
    unsigned int        bytesused;
    unsigned int        length;
    unsigned int        min_length;
    unsigned int        offset;
    unsigned int        data_offset;
};

struct vb_queue;

struct vb_buffer {
    struct vb_queue    *queue;
    unsigned int        index;
    unsigned int        num_planes;

    enum vb_buffer_state state;
    struct vb_plane     planes[VIN_MAX_PLANE_NUM];

    struct list_head    queued_entry;
    struct list_head    done_entry;
    struct list_head    active_entry;

    unsigned int        hw_using:1;
};

struct vb_queue {
    aicos_mutex_t       lock;

    struct vb_buffer    bufs[VIN_MAX_BUF_NUM];
    unsigned int        num_buffers;

    struct list_head    queued_list;
    unsigned int        queued_count;
    aicos_mutex_t       queued_lock;
    unsigned int        owned_by_drv_count;

    struct list_head    done_list;
    aicos_mutex_t       done_lock;

    unsigned int        streaming:1;
    unsigned int        error:1;
    unsigned int        waiting_in_dqbuf:1;
};

struct vb_ops {
    void (*buf_queue)(struct vb_buffer *vb);
    int (*start_streaming)(struct vb_queue *q);
    void (*stop_streaming)(struct vb_queue *q);
};

int vin_vb_req_buf(struct vb_queue *q,
                   char *buf, u32 size, struct vin_video_buf *vbuf);
int vin_vb_q_buf(struct vb_queue *q, u32 index);
int vin_vb_dq_buf(struct vb_queue *q, u32 *pindex);
void vin_vb_buffer_done(struct vb_buffer *vb, enum vb_buffer_state state);

int vin_vb_init(struct vb_queue *q, const struct vb_ops *ops);
int vin_vb_stream_on(struct vb_queue *q);
int vin_vb_stream_off(struct vb_queue *q);

#endif /* _ARTINCHIP_MPP_VIN_VB_H_ */
