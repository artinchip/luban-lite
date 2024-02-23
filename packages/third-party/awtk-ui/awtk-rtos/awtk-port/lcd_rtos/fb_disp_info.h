/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#ifndef _TK_AIC_FB_DISP_H
#define _TK_AIC_FB_DISP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "base/lcd.h"
#include "tkc/mem.h"

#include <aic_core.h>
#include <aic_time.h>
#include "mpp_fb.h"
#include "mpp_log.h"

typedef struct _fb_info_t {
  struct mpp_fb *fb;
  struct aicfb_screeninfo disp_info;

  uint8_t* fbmem0;
  uint8_t* fbmem_offline;
} fb_info_t;

#define fb_width(fb) ((fb)->disp_info.width)
#define fb_height(fb) ((fb)->disp_info.height)
#define fb_memsize(fb) ((fb)->disp_info.smem_len)
#define fb_bpp(fb) ((fb)->disp_info.bits_per_pixel)
#define fb_line_length(fb) ((fb)->disp_info.stride)
#define fb_size(fb) ((fb)->disp_info.stride * (fb)->disp_info.height)
#define fb_format(fb) ((fb)->disp_info.format)

/* use double frame buffer */
#ifdef AIC_PAN_DISPLAY
#define fb_vsize(fb) ((fb)->disp_info.stride * (fb)->disp_info.height * 2)
#define fb_number(fb) (2)
#else
#define fb_vsize(fb) ((fb)->disp_info.stride * (fb)->disp_info.height)
#define fb_number(fb) (1)
#endif

#define fb_is_1fb(fb) (fb_vsize(fb) / fb_size(fb) < 2)
#define fb_is_2fb(fb) (fb_vsize(fb) / fb_size(fb) >= 2)
#define fb_is_3fb(fb) 0

static inline bool_t fb_is_bgra5551(fb_info_t* tk_fb) {
  enum mpp_pixel_format fb_fmt = tk_fb->disp_info.format;
  if (fb_fmt == MPP_FMT_ARGB_1555)
    return TRUE;

  return FALSE;
}

static inline bool_t fb_is_bgr565(fb_info_t* tk_fb) {
  enum mpp_pixel_format fb_fmt = tk_fb->disp_info.format;
  if (fb_fmt == MPP_FMT_RGB_565)
    return TRUE;

  return FALSE;
}

static inline bool_t fb_is_rgb565(fb_info_t* tk_fb) {
  enum mpp_pixel_format fb_fmt = tk_fb->disp_info.format;
  if (fb_fmt == MPP_FMT_BGR_565)
    return TRUE;

  return FALSE;
}

static inline bool_t fb_is_rgba8888(fb_info_t* tk_fb) {
  enum mpp_pixel_format fb_fmt = tk_fb->disp_info.format;
  if (fb_fmt == MPP_FMT_ABGR_8888)
    return TRUE;

  return FALSE;
}

static inline bool_t fb_is_bgra8888(fb_info_t* tk_fb) {
  enum mpp_pixel_format fb_fmt = tk_fb->disp_info.format;
  if (fb_fmt == MPP_FMT_ARGB_8888)
    return TRUE;

  return FALSE;
}

static inline bool_t fb_is_rgb888(fb_info_t* tk_fb) {
  enum mpp_pixel_format fb_fmt = tk_fb->disp_info.format;
  if (fb_fmt == MPP_FMT_BGR_888)
    return TRUE;

  return FALSE;
}

static inline bool_t fb_is_bgr888(fb_info_t* tk_fb) {
  enum mpp_pixel_format fb_fmt = tk_fb->disp_info.format;
  if (fb_fmt == MPP_FMT_RGB_888)
    return TRUE;

  return FALSE;
}

static inline ret_t tk_fb_disp_init(fb_info_t* tk_fb)
{
  struct mpp_fb *fb = NULL;
  int result = -1;
  int fb_size = 0;

  fb = mpp_fb_open();
  if (fb == NULL) {
    log_info("can't find aic framebuffer device!");
    return RET_FAIL;
  }

  result = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &tk_fb->disp_info);
  if (result) {
    log_info("get device fb info failed!");
    return RET_FAIL;
  }

  fb_size =tk_fb->disp_info.height * tk_fb->disp_info.stride;
  tk_fb->fbmem0 = (uint8_t *)tk_fb->disp_info.framebuffer;
#ifdef AIC_PAN_DISPLAY
  tk_fb->fbmem_offline = ((uint8_t *)tk_fb->fbmem0 + fb_size);
#else
  tk_fb->fbmem_offline = (uint8_t *)malloc(fb_size(tk_fb));
  if (tk_fb->fbmem_offline == NULL)
    log_error("malloc lcd offline buffer failed!, size = %d\n", fb_size(tk_fb));
#endif
  tk_fb->fb = fb;

  return RET_OK;
}

static inline void tk_fb_disp_exit(fb_info_t* tk_fb)
{
  if (tk_fb->fb) {
    mpp_fb_close(tk_fb->fb);
    tk_fb->fb = NULL;
  }

#ifndef AIC_PAN_DISPLAY
  if (tk_fb->fbmem_offline != NULL)
    free(tk_fb->fbmem_offline);
#endif
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _TK_AIC_FB_DISP_H */
