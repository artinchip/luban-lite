/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef __DRV_CAMERA_H__
#define __DRV_CAMERA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rtdef.h"

#define CAMERA_NAME_OV      "ov-cam"

#define CAMERA_CMD_START    (RT_DEVICE_CTRL_BASE(CAMERA) + 0x01)
#define CAMERA_CMD_STOP     (RT_DEVICE_CTRL_BASE(CAMERA) + 0x02)
/* Argument type: struct mpp_video_fmt * */
#define CAMERA_CMD_GET_FMT  (RT_DEVICE_CTRL_BASE(CAMERA) + 0x03)

#ifdef __cplusplus
}
#endif

#endif
