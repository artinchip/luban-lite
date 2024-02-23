/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _MPP_VIDEO_IN_H_
#define _MPP_VIDEO_IN_H_

#include "aic_common.h"

#define VIN_MAX_BUF_NUM         32
#define VIN_MAX_PLANE_NUM       2

/*
 * Signal polarity flags
 * Note: in BT.656 mode HSYNC, FIELD, and VSYNC are unused
 * MEDIA_SIGNAL_[HV]SYNC* flags should be also used for specifying
 * configuration of hardware that uses [HV]REF signals
 */
#define MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH            BIT(2)
#define MEDIA_SIGNAL_HSYNC_ACTIVE_LOW             BIT(3)
#define MEDIA_SIGNAL_VSYNC_ACTIVE_HIGH            BIT(4)
#define MEDIA_SIGNAL_VSYNC_ACTIVE_LOW             BIT(5)
#define MEDIA_SIGNAL_PCLK_SAMPLE_RISING           BIT(6)
#define MEDIA_SIGNAL_PCLK_SAMPLE_FALLING          BIT(7)
#define MEDIA_SIGNAL_DATA_ACTIVE_HIGH             BIT(8)
#define MEDIA_SIGNAL_DATA_ACTIVE_LOW              BIT(9)
/* FIELD = 0/1 - Field1 (odd)/Field2 (even) */
#define MEDIA_SIGNAL_FIELD_EVEN_HIGH              BIT(10)
/* FIELD = 1/0 - Field1 (odd)/Field2 (even) */
#define MEDIA_SIGNAL_FIELD_EVEN_LOW               BIT(11)
/* Active state of Sync-on-green (SoG) signal, 0/1 for LOW/HIGH respectively. */
#define MEDIA_SIGNAL_VIDEO_SOG_ACTIVE_HIGH        BIT(12)
#define MEDIA_SIGNAL_VIDEO_SOG_ACTIVE_LOW         BIT(13)
#define MEDIA_SIGNAL_DATA_ENABLE_HIGH             BIT(14)
#define MEDIA_SIGNAL_DATA_ENABLE_LOW              BIT(15)

enum media_colorspace {
    /*
     * Default colorspace, i.e. let the driver figure it out.
     * Can only be used with video capture.
     */
    MEDIA_COLORSPACE_DEFAULT       = 0,

    /* SMPTE 170M: used for broadcast NTSC/PAL SDTV */
    MEDIA_COLORSPACE_SMPTE170M     = 1,

    /* Obsolete pre-1998 SMPTE 240M HDTV standard, superseded by Rec 709 */
    MEDIA_COLORSPACE_SMPTE240M     = 2,

    /* Rec.709: used for HDTV */
    MEDIA_COLORSPACE_REC709        = 3,

    /*
     * Deprecated, do not use. No driver will ever return this. This was
     * based on a misunderstanding of the bt878 datasheet.
     */
    MEDIA_COLORSPACE_BT878         = 4,

    /*
     * NTSC 1953 colorspace. This only makes sense when dealing with
     * really, really old NTSC recordings. Superseded by SMPTE 170M.
     */
    MEDIA_COLORSPACE_470_SYSTEM_M  = 5,

    /*
     * EBU Tech 3213 PAL/SECAM colorspace. This only makes sense when
     * dealing with really old PAL/SECAM recordings. Superseded by
     * SMPTE 170M.
     */
    MEDIA_COLORSPACE_470_SYSTEM_BG = 6,

    /*
     * Effectively shorthand for MEDIA_COLORSPACE_SRGB, MEDIA_YCBCR_ENC_601
     * and MEDIA_QUANTIZATION_FULL_RANGE. To be used for (Motion-)JPEG.
     */
    MEDIA_COLORSPACE_JPEG          = 7,

    /* For RGB colorspaces such as produces by most webcams. */
    MEDIA_COLORSPACE_SRGB          = 8,

    /* opRGB colorspace */
    MEDIA_COLORSPACE_OPRGB         = 9,

    /* BT.2020 colorspace, used for UHDTV. */
    MEDIA_COLORSPACE_BT2020        = 10,

    /* Raw colorspace: for RAW unprocessed images */
    MEDIA_COLORSPACE_RAW           = 11,

    /* DCI-P3 colorspace, used by cinema projectors */
    MEDIA_COLORSPACE_DCI_P3        = 12,
};

enum media_power_line_frequency {
    MEDIA_POWER_LINE_FREQUENCY_DISABLED = 0,
    MEDIA_POWER_LINE_FREQUENCY_50HZ     = 1,
    MEDIA_POWER_LINE_FREQUENCY_60HZ     = 2,
    MEDIA_POWER_LINE_FREQUENCY_AUTO     = 3,
};

#define fourcc(a, b, c, d)\
    ((u32)(a) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

/* two planes -- one Y, one Cr + Cb interleaved  */
#define DVP_OUT_FMT_NV12    fourcc('N', 'V', '1', '2') /* 12  Y/CbCr 4:2:0  */
#define DVP_OUT_FMT_NV16    fourcc('N', 'V', '1', '6') /* 16  Y/CbCr 4:2:2  */

struct dvp_plane_pix_format {
    u32 sizeimage;
    u32 bytesperline;
};

/* The output video format of DVP */
struct dvp_out_fmt {
    u32 width;
    u32 height;
    u32 pixelformat;
    u32 field;
    u32 colorspace;
    u8  num_planes;
    struct dvp_plane_pix_format plane_fmt[VIN_MAX_PLANE_NUM];
};

struct vin_video_plane {
    int buf;
    int len;
};

/* The buffer information of vin_video_buf is used by user.
   By contrast, vin_vb and DVP driver will use vb_queue. */
struct vin_video_buf {
    u32 num_buffers;
    u32 num_planes;
    struct vin_video_plane planes[VIN_MAX_PLANE_NUM * VIN_MAX_BUF_NUM];
};

/* ioctl command of DVP */
#define DVP_IN_G_FMT        _IOWR('V',  4, struct dvp_in_fmt)
#define DVP_IN_S_FMT        _IOWR('V',  5, struct dvp_in_fmt)
#define DVP_OUT_S_FMT       _IOWR('V',  6, struct dvp_out_fmt)

#define DVP_STREAM_ON       _IO('V', 18)
#define DVP_STREAM_OFF      _IO('V', 19)
#define DVP_REQ_BUF         _IOWR('V',  8, struct vin_video_buf)
#define DVP_Q_BUF           _IOWR('V', 15, int)
#define DVP_DQ_BUF          _IOWR('V', 17, int)

int mpp_vin_init(char *camera);
void mpp_vin_deinit(void);

/* DVP & Camera ioctrl API, defined int mpp_vin.c */
int mpp_dvp_ioctl(int cmd, void *arg);

#endif /* _MPP_VIDEO_IN_H_ */
