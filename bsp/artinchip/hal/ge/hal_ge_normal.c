/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "aic_core.h"
#include "aic_hal.h"
#include "aic_log.h"
#include "mpp_types.h"
#include "aic_drv_ge.h"
#include <sys/time.h>

#include "hal_ge_hw.h"
#include "hal_ge_reg.h"

#define AIC_GE_NAME "ge"

#define MAX_WIDTH 4096
#define MAX_HEIGHT 4096

#define ALIGN_128B(x) ALIGN_UP(x, 128)
#define GE_TIMEOUT (1000 * 2) //second
#define HW_RUNNING_EVENT        0x02

#ifdef AIC_GE_DRV_V11
#define GE_CLOCK   (150000000)
#else
#define GE_CLOCK   (200000000)
#endif

struct aic_ge_data {
    struct device   *dev;
    aicos_mutex_t   lock;
    aicos_event_t   wait;
    u32             status;
    u32             src_premul_en;
    u32             src_de_premul_en;
    u32             dst_de_premul_en;
    u32             out_premul_en;
    u32             src_alpha_coef;
    u32             dst_alpha_coef;
    u32             csc0_en;
    u32             csc1_en;
    u32             csc2_en;
    bool            blend_is_rgb;
    bool            enable_dma_buf;
    u8              *dither_line_ptr;
    uintptr_t       dither_line_phys;
    enum ge_mode    ge_mode;
};

struct aic_ge_data *g_data;

static inline int ge_check_buf(struct aic_ge_data   *data,
                   struct mpp_buf *video_buf)
{
    if (video_buf->buf_type == MPP_DMA_BUF_FD &&
        !data->enable_dma_buf) {
        hal_log_err("unsupported buf type:%d\n",
            video_buf->buf_type);
        return -1;
    }

    return 0;
}

static irqreturn_t aic_ge_handler(int irq, void *ctx)
{
    struct aic_ge_data *data = (struct aic_ge_data *)g_data;

    (void)ctx;

    data->status = ge_read_status(GE_BASE);

    ge_clear_status(GE_BASE, data->status);

    /* disable interrupt*/
    ge_disable_interrupt(GE_BASE);
    aicos_event_send(data->wait, HW_RUNNING_EVENT);
    return 0;
}

static void ge_reset(struct aic_ge_data *data)
{
    hal_reset_assert(RESET_GE);
    hal_reset_deassert(RESET_GE);
}

static void dump_regs()
{
    int i=0;
    for (i=0; i<256; i+=4) {
        hal_log_info("%08x: %08x %08x %08x %08x\n", (u32)GE_BASE + 4*i,
            readl(GE_BASE + 4*i), readl(GE_BASE + 4*i+4),
            readl(GE_BASE + 4*i+8), readl(GE_BASE + 4*i+12));
    }
}

static int ge_start_and_wait(struct aic_ge_data *data)
{
    int ret;
    uint32_t recved;

    data->status = 0;

    ge_enable_interrupt(GE_BASE);
    ge_start(GE_BASE);

    ret = aicos_event_recv(data->wait,
                            HW_RUNNING_EVENT,
                            &recved,
                            GE_TIMEOUT);

    ge_clear_status(GE_BASE, data->status);

    if (data->status == 0) {
        hal_log_err("ge timeout\n");
        dump_regs();
        ge_disable_interrupt(GE_BASE);
        ge_reset(data);
        return ret;
    } else if ((data->status & GE_CTRL_FINISH_IRQ_EN) == 0) {
        dump_regs();
        hal_log_err("ge error status:%08x\n", data->status);
        ge_reset(data);
    }

    return 0;
}

static inline bool is_rgb(enum mpp_pixel_format format)
{
    switch (format) {
    case MPP_FMT_ARGB_8888:
    case MPP_FMT_ABGR_8888:
    case MPP_FMT_RGBA_8888:
    case MPP_FMT_BGRA_8888:
    case MPP_FMT_XRGB_8888:
    case MPP_FMT_XBGR_8888:
    case MPP_FMT_RGBX_8888:
    case MPP_FMT_BGRX_8888:
    case MPP_FMT_RGB_888:
    case MPP_FMT_BGR_888:
    case MPP_FMT_ARGB_1555:
    case MPP_FMT_ABGR_1555:
    case MPP_FMT_RGBA_5551:
    case MPP_FMT_BGRA_5551:
    case MPP_FMT_RGB_565:
    case MPP_FMT_BGR_565:
    case MPP_FMT_ARGB_4444:
    case MPP_FMT_ABGR_4444:
    case MPP_FMT_RGBA_4444:
    case MPP_FMT_BGRA_4444:
        return true;
    default:
        break;
    }
    return false;
}

static inline bool need_blend(struct ge_ctrl *ctrl)
{
    if (ctrl->alpha_en || ctrl->ck_en)
        return true;
    else
        return false;
}

static void set_csc_flow(struct aic_ge_data *data,
             struct ge_ctrl *ctrl,
             enum mpp_pixel_format src_format,
             enum mpp_pixel_format dst_format)
{
    bool src_is_rgb = is_rgb(src_format);
    bool dst_is_rgb = is_rgb(dst_format);
    bool is_blending = need_blend(ctrl);

    data->blend_is_rgb = true;

    if (!src_is_rgb &&
        (is_blending || dst_is_rgb)) {
        data->csc0_en = 1;
    } else if (!src_is_rgb) {
        data->blend_is_rgb = false;
        data->csc0_en = 0;
    } else {
        data->csc0_en = 0;
    }

    if (is_blending && !dst_is_rgb)
        data->csc1_en = 1;
    else
        data->csc1_en = 0;

    if (data->blend_is_rgb && !dst_is_rgb)
        data->csc2_en = 1;
    else
        data->csc2_en = 0;
}

static void set_alpha_rules(struct aic_ge_data *data,
                enum ge_pd_rules rules)
{
    switch (rules) {
    case GE_PD_NONE:
        data->src_alpha_coef = 2;
        data->dst_alpha_coef = 3;
        break;
    case GE_PD_CLEAR:
        data->src_alpha_coef = 0;
        data->dst_alpha_coef = 0;
        break;
    case GE_PD_SRC:
        data->src_alpha_coef = 1;
        data->dst_alpha_coef = 0;
        break;
    case GE_PD_SRC_OVER:
        data->src_alpha_coef = 1;
        data->dst_alpha_coef = 3;
        break;
    case GE_PD_DST_OVER:
        data->src_alpha_coef = 5;
        data->dst_alpha_coef = 1;
        break;
    case GE_PD_SRC_IN:
        data->src_alpha_coef = 4;
        data->dst_alpha_coef = 0;
        break;
    case GE_PD_DST_IN:
        data->src_alpha_coef = 0;
        data->dst_alpha_coef = 2;
        break;
    case GE_PD_SRC_OUT:
        data->src_alpha_coef = 5;
        data->dst_alpha_coef = 0;
        break;
    case GE_PD_DST_OUT:
        data->src_alpha_coef = 0;
        data->dst_alpha_coef = 3;
        break;
    case GE_PD_SRC_ATOP:
        data->src_alpha_coef = 4;
        data->dst_alpha_coef = 3;
        break;
    case GE_PD_DST_ATOP:
        data->src_alpha_coef = 5;
        data->dst_alpha_coef = 2;
        break;
    case GE_PD_ADD:
        data->src_alpha_coef = 1;
        data->dst_alpha_coef = 1;
        break;
    case GE_PD_XOR:
        data->src_alpha_coef = 5;
        data->dst_alpha_coef = 3;
        break;
    case GE_PD_DST:
        data->src_alpha_coef = 0;
        data->dst_alpha_coef = 1;
        break;
    default:
        data->src_alpha_coef = 2;
        data->dst_alpha_coef = 3;
        break;
    }
}

/* must call set_premuliply after set_alpha_rules */
int set_premuliply(struct aic_ge_data *data,
           enum mpp_pixel_format src_format,
           enum mpp_pixel_format dst_format,
           int src_premul,
           int dst_premul,
           int is_fill_color,
           struct ge_ctrl *ctrl)
{
    if (src_format > MPP_FMT_BGRA_4444)
        src_premul = 0;

    if (dst_format > MPP_FMT_BGRA_4444)
        dst_premul = 0;

    if (src_premul == 0 && dst_premul == 0) {
        data->src_premul_en = 0;
        data->src_de_premul_en = 0;
        data->dst_de_premul_en = 0;
        data->out_premul_en = 0;

        if (is_fill_color == 0 &&
            ctrl->src_alpha_mode == 0 &&
            ctrl->alpha_en &&
            ctrl->ck_en == 0 &&
            data->src_alpha_coef == 2 &&
            data->dst_alpha_coef == 3) {
            data->src_premul_en = 1;
            data->src_alpha_coef = 1;
        }
    } else if (src_premul == 1 && dst_premul == 0) {
        data->src_premul_en = 0;
        data->src_de_premul_en = 1;
        data->dst_de_premul_en = 0;
        data->out_premul_en = 0;

        if (ctrl->src_alpha_mode == 0 &&
            ctrl->alpha_en &&
            ctrl->ck_en == 0 &&
            data->src_alpha_coef == 2 &&
            data->dst_alpha_coef == 3) {
            data->src_de_premul_en = 0;
            data->src_alpha_coef = 1;
        }
    } else if (src_premul == 1 && dst_premul == 1) {
        data->src_premul_en = 0;
        data->src_de_premul_en = 1;
        data->dst_de_premul_en = 1;
        data->out_premul_en = 1;
    } else if (src_premul == 0 && dst_premul == 1) {
        data->src_premul_en = 0;
        data->src_de_premul_en = 0;
        data->dst_de_premul_en = 1;
        data->out_premul_en = 1;
    }

    return 0;
}

static int check_bitblt(struct aic_ge_data *data, struct ge_bitblt *blt)
{
    enum mpp_pixel_format src_format = blt->src_buf.format;
    enum mpp_pixel_format dst_format = blt->dst_buf.format;
    struct mpp_rect *src_rect = &blt->src_buf.crop;
    struct mpp_rect *dst_rect = &blt->dst_buf.crop;
    struct mpp_size *src_size = &blt->src_buf.size;
    struct mpp_size *dst_size = &blt->dst_buf.size;

    if (MPP_SCAN_ORDER_GET(blt->ctrl.flags)) {
        if (MPP_ROTATION_GET(blt->ctrl.flags)) {
            hal_log_err( "%s failed, scan order unsupport rot0\n",
                __func__);
            return -1;
        }
        if (blt->ctrl.dither_en) {
            hal_log_err("%s failed, scan order unsupport dither\n",
                __func__);
            return -1;
        }
        if (!is_rgb(src_format) || !is_rgb(dst_format)) {
            hal_log_err("%s failed, scan order just support rgb format\n",
                __func__);
            return -1;
        }
    }

    if (blt->ctrl.dither_en) {
        if (!is_rgb(dst_format)) {
            hal_log_err("%s failed, invalid dst format with the dither func on\n",
                __func__);
            return -1;
        }

        if (!g_data->dither_line_ptr) {
            hal_log_err("%s failed, dither function is closed and cannot be used\n",
            __func__);
            blt->ctrl.dither_en = 0;
        }
    }

    if (blt->src_buf.crop_en) {
        if (src_rect->x < 0 ||
            src_rect->y < 0 ||
            src_rect->x >= src_size->width ||
            src_rect->y >= src_size->height) {
            hal_log_err("%s failed, invalid src crop\n",
                __func__);
            return -1;
        }
    }

    if (blt->dst_buf.crop_en) {
        if (dst_rect->x < 0 ||
            dst_rect->y < 0 ||
            dst_rect->x >= dst_size->width ||
            dst_rect->y >= dst_size->height) {
            hal_log_err("%s failed, invalid dst crop\n",
                __func__);
            return -1;
        }
    }

    if (!blt->src_buf.crop_en) {
        src_rect->x = 0;
        src_rect->y = 0;
        src_rect->width = src_size->width;
        src_rect->height = src_size->height;
    }

    if (!blt->dst_buf.crop_en) {
        dst_rect->x = 0;
        dst_rect->y = 0;
        dst_rect->width = dst_size->width;
        dst_rect->height = dst_size->height;
    }

    switch (src_format) {
    case MPP_FMT_YUV420P:
    case MPP_FMT_NV12:
    case MPP_FMT_NV21:
        src_rect->x = src_rect->x & (~1);
        src_rect->y = src_rect->y & (~1);
        src_rect->width = src_rect->width & (~1);
        src_rect->height = src_rect->height & (~1);
        src_size->width = src_size->width & (~1);
        src_size->height = src_size->height & (~1);
        break;
    case MPP_FMT_YUV422P:
    case MPP_FMT_NV16:
    case MPP_FMT_NV61:
    case MPP_FMT_YUYV:
    case MPP_FMT_YVYU:
    case MPP_FMT_UYVY:
    case MPP_FMT_VYUY:
        src_rect->x = src_rect->x & (~1);
        src_rect->width = src_rect->width & (~1);
        src_size->width = src_size->width & (~1);
        break;
    default:
        break;
    }

    switch (dst_format) {
    case MPP_FMT_YUV420P:
    case MPP_FMT_NV12:
    case MPP_FMT_NV21:
        dst_rect->x = dst_rect->x & (~1);
        dst_rect->y = dst_rect->y & (~1);
        dst_rect->width = dst_rect->width & (~1);
        dst_rect->height = dst_rect->height & (~1);
        dst_size->width = dst_size->width & (~1);
        dst_size->height = dst_size->height & (~1);
        break;
    case MPP_FMT_YUV422P:
    case MPP_FMT_NV16:
    case MPP_FMT_NV61:
    case MPP_FMT_YUYV:
    case MPP_FMT_YVYU:
    case MPP_FMT_UYVY:
    case MPP_FMT_VYUY:
        dst_rect->x = dst_rect->x & (~1);
        dst_rect->width = dst_rect->width & (~1);
        dst_size->width = dst_size->width & (~1);
        break;
    default:
        break;
    }

    /* crop src invalid region */
    if ((src_rect->x + src_rect->width) > src_size->width)
        src_rect->width = src_size->width - src_rect->x;

    if ((src_rect->y + src_rect->height) > src_size->height)
        src_rect->height = src_size->height - src_rect->y;

    /* crop dst invalid region */
    if ((dst_rect->x + dst_rect->width) > dst_size->width)
        dst_rect->width = dst_size->width - dst_rect->x;

    if ((dst_rect->y + dst_rect->height) > dst_size->height)
        dst_rect->height = dst_size->height - dst_rect->y;

    if (src_rect->height > MAX_HEIGHT ||
             src_rect->width > MAX_WIDTH) {
        hal_log_err("invalid src size, over the largest\n");
        return -1;
    }

    if (dst_rect->height > MAX_HEIGHT ||
             dst_rect->width > MAX_WIDTH) {
        hal_log_err("invalid dst size, over the largest\n");
        return -1;
    }

    if (!is_rgb(src_format) &&
            (src_rect->width < 8 || src_rect->height < 8))  {
        hal_log_err("invalid src size, the min size of yuv is 8x8\n");

        return -1;
    }

    if (!is_rgb(dst_format) &&
            (dst_rect->width < 8 || dst_rect->height < 8))  {
        hal_log_err("invalid dst size, the min size of yuv is 8x8\n");

        return -1;;
    }

    return 0;
}

static void set_alpha_rules_and_premul(struct aic_ge_data *data,
                       struct ge_ctrl *ctrl,
                       enum mpp_pixel_format src_format,
                       enum mpp_pixel_format dst_format,
                       u32 src_buf_flags,
                       u32 dst_buf_flags,
                       int is_fill_color)
{
    if (ctrl->alpha_en)
        set_alpha_rules(data, ctrl->alpha_rules);

    set_premuliply(data, src_format, dst_format,
               MPP_BUF_PREMULTIPLY_GET(src_buf_flags),
               MPP_BUF_PREMULTIPLY_GET(dst_buf_flags),
               is_fill_color, ctrl);
}

static int ge_config_scaler(struct aic_ge_data *data,
                struct ge_bitblt *blt)
{
    enum mpp_pixel_format format;
    int in_w[2];
    int in_h[2];
    int out_w;
    int out_h;
    int channel_num;
    int scaler_en;
    int rot0_degree;
    int i;
    int dx[2];
    int dy[2];
    int h_phase[2];
    int v_phase[2];

    channel_num = 1;
    scaler_en = 1;
    format = blt->src_buf.format;
    rot0_degree = MPP_ROTATION_GET(blt->ctrl.flags);

    in_w[0] = blt->src_buf.crop.width;
    in_h[0] = blt->src_buf.crop.height;

    if (rot0_degree == MPP_ROTATION_90 ||
        rot0_degree == MPP_ROTATION_270) {
        out_w = blt->dst_buf.crop.height;
        out_h = blt->dst_buf.crop.width;
    } else {
        out_w = blt->dst_buf.crop.width;
        out_h = blt->dst_buf.crop.height;
    }

    switch (format) {
    case MPP_FMT_ARGB_8888:
    case MPP_FMT_ABGR_8888:
    case MPP_FMT_RGBA_8888:
    case MPP_FMT_BGRA_8888:
    case MPP_FMT_XRGB_8888:
    case MPP_FMT_XBGR_8888:
    case MPP_FMT_RGBX_8888:
    case MPP_FMT_BGRX_8888:
    case MPP_FMT_RGB_888:
    case MPP_FMT_BGR_888:
    case MPP_FMT_ARGB_1555:
    case MPP_FMT_ABGR_1555:
    case MPP_FMT_RGBA_5551:
    case MPP_FMT_BGRA_5551:
    case MPP_FMT_RGB_565:
    case MPP_FMT_BGR_565:
    case MPP_FMT_ARGB_4444:
    case MPP_FMT_ABGR_4444:
    case MPP_FMT_RGBA_4444:
    case MPP_FMT_BGRA_4444:
        if (in_w[0] == out_w && in_h[0] == out_h) {
            scaler_en = 0;
        } else {
            dx[0] = (in_w[0] << 16) / out_w;
            dy[0] = (in_h[0] << 16) / out_h;
            h_phase[0] = (dx[0] >= 65536) ?
                     ((dx[0] >> 1) - 32768) :
                     (dx[0] >> 1);
            v_phase[0] = (dy[0] >= 65536) ?
                     ((dy[0] >> 1) - 32768) :
                     (dy[0] >> 1);
        }
        break;
    case MPP_FMT_YUV400:
        dx[0] = (in_w[0] << 16) / out_w;
        dy[0] = (in_h[0] << 16) / out_h;
        h_phase[0] = dx[0] >> 1;
        v_phase[0] = dy[0] >> 1;
        break;
    case MPP_FMT_YUV420P:
    case MPP_FMT_NV12:
    case MPP_FMT_NV21:
        channel_num = 2;
        in_w[1] = in_w[0] >> 1;
        in_h[1] = in_h[0] >> 1;

        dx[0] = (in_w[0] << 16) / out_w;
        dy[0] = (in_h[0] << 16) / out_h;
        h_phase[0] = dx[0] >> 1;
        v_phase[0] = dy[0] >> 1;

        dx[0] = dx[0] & (~1);
        dy[0] = dy[0] & (~1);
        h_phase[0] = h_phase[0] & (~1);
        v_phase[0] = v_phase[0] & (~1);

        /* change init phase */
        if (((dx[0] - h_phase[0]) >> 16) > 4) {
            h_phase[0] += (((dx[0] - h_phase[0]) >> 16) - 4) << 16;
        }

        if (((dy[0] - v_phase[0]) >> 16) > 3) {
            v_phase[0] += (((dy[0] - v_phase[0]) >> 16) - 4) << 16;
        }

        dx[1] = dx[0] >> 1;
        dy[1] = dy[0] >> 1;
        h_phase[1] = h_phase[0] >> 1;
        v_phase[1] = v_phase[0] >> 1;
        break;
    case MPP_FMT_YUV422P:
    case MPP_FMT_NV16:
    case MPP_FMT_NV61:
    case MPP_FMT_YUYV:
    case MPP_FMT_YVYU:
    case MPP_FMT_UYVY:
    case MPP_FMT_VYUY:
        channel_num = 2;

        in_w[1] = in_w[0] >> 1;
        in_h[1] = in_h[0];

        dx[0] = (in_w[0] << 16) / out_w;
        dy[0] = (in_h[0] << 16) / out_h;
        h_phase[0] = dx[0] >> 1;
        v_phase[0] = dy[0] >> 1;

        dx[0] = dx[0] & (~1);
        h_phase[0] = h_phase[0] & (~1);

        /* change init phase */
        if (((dx[0] - h_phase[0]) >> 16) > 4) {
            h_phase[0] += (((dx[0] - h_phase[0]) >> 16) - 4) << 16;
        }

        dx[1] = dx[0] >> 1;
        dy[1] = dy[0];
        h_phase[1] = h_phase[0] >> 1;
        v_phase[1] = v_phase[0];
        break;
    case MPP_FMT_YUV444P:
        channel_num = 2;
        in_w[1] = in_w[0];
        in_h[1] = in_h[0];

        dx[0] = (in_w[0] << 16) / out_w;
        dy[0] = (in_h[0] << 16) / out_h;
        h_phase[0] = dx[0] >> 1;
        v_phase[0] = dy[0] >> 1;

        dx[1] = dx[0];
        dy[1] = dy[0];
        h_phase[1] = h_phase[0];
        v_phase[1] = v_phase[0];
        break;
    default:
        scaler_en = 0;
        hal_log_err("invalid format: %d\n", format);
        return -1;
    }

    if (scaler_en) {
        if (is_rgb(format) &&
                (in_w[0] < 4 || in_h[0] < 4)) {
            hal_log_err(
                "the min size of rgb is 4x4, when scaler enable\n");
            return -1;
        }

        if (is_rgb(blt->dst_buf.format) &&
                (out_h < 4 || out_w < 4)) {
            hal_log_err(
                "the min size of rgb is 4x4, when scaler enable\n");
            return -1;
        }

        for (i = 0; i < channel_num; i++) {
            ge_set_scaler0(GE_BASE, in_w[i], in_h[i],
                       out_w, out_h,
                       dx[i], dy[i],
                       h_phase[i], v_phase[i],
                       i);
        }
        ge_scaler0_enable(GE_BASE, 1);
    } else {
        ge_scaler0_enable(GE_BASE, 0);
    }

    return 0;
}

/*
 *@addr[]: in/out addr
 *
 */
static int ge_buf_crop(u32 addr[], u32 stride[],
               enum mpp_pixel_format format,
               u32 x_offset,
               u32 y_offset,
               u32 width,
               u32 height)
{
    int offset;

    switch (format) {
    case MPP_FMT_ARGB_8888:
    case MPP_FMT_ABGR_8888:
    case MPP_FMT_RGBA_8888:
    case MPP_FMT_BGRA_8888:
    case MPP_FMT_XRGB_8888:
    case MPP_FMT_XBGR_8888:
    case MPP_FMT_RGBX_8888:
    case MPP_FMT_BGRX_8888:
        addr[0] += x_offset * 4 + y_offset * stride[0];
        break;
    case MPP_FMT_RGB_888:
    case MPP_FMT_BGR_888:
        addr[0] += x_offset * 3 + y_offset * stride[0];
        break;
    case MPP_FMT_ARGB_1555:
    case MPP_FMT_ABGR_1555:
    case MPP_FMT_RGBA_5551:
    case MPP_FMT_BGRA_5551:
    case MPP_FMT_RGB_565:
    case MPP_FMT_BGR_565:
    case MPP_FMT_ARGB_4444:
    case MPP_FMT_ABGR_4444:
    case MPP_FMT_RGBA_4444:
    case MPP_FMT_BGRA_4444:
        addr[0] += x_offset * 2 + y_offset * stride[0];
        break;
    case MPP_FMT_YUV420P:
        addr[0] += x_offset + y_offset * stride[0];
        offset = (x_offset >> 1) + (y_offset >> 1) * stride[1];
        addr[1] += offset;
        addr[2] += offset;
        break;
    case MPP_FMT_NV12:
    case MPP_FMT_NV21:
        addr[0] += x_offset + y_offset * stride[0];
        addr[1] += x_offset + (y_offset >> 1) * stride[1];
        break;
    case MPP_FMT_YUV400:
        addr[0] += x_offset + y_offset * stride[0];
        break;
    case MPP_FMT_YUV422P:
        addr[0] += x_offset + y_offset * stride[0];
        offset = (x_offset >> 1) + y_offset * stride[1];
        addr[1] += offset;
        addr[2] += offset;
        break;
    case MPP_FMT_NV16:
    case MPP_FMT_NV61:
        addr[0] += x_offset + y_offset * stride[0];
        addr[1] += x_offset + y_offset * stride[1];
        break;
    case MPP_FMT_YUYV:
    case MPP_FMT_YVYU:
    case MPP_FMT_UYVY:
    case MPP_FMT_VYUY:
        addr[0] += (x_offset << 1) + y_offset * stride[0];
        break;
    case MPP_FMT_YUV444P:
        addr[0] += x_offset + y_offset * stride[0];
        addr[1] += x_offset + y_offset * stride[1];
        addr[2] += x_offset + y_offset * stride[1];
        break;
    default:
        return -1;
    }

    return 0;
}

static int ge_config_addr(struct aic_ge_data     *data,
              struct mpp_buf   *src_buf,
              struct mpp_buf   *dst_buf,
              struct ge_ctrl     *ctrl)
{
    u32 src_w;
    u32 src_h;
    u32 dst_w;
    u32 dst_h;
    u32 src_addr[3];
    u32 dst_addr[3];
    u32 src_stride[2];
    u32 dst_stride[2];
    struct mpp_rect *src_rect;
    struct mpp_rect *dst_rect;

    src_rect = &src_buf->crop;
    dst_rect = &dst_buf->crop;

    src_w = src_rect->width;
    src_h = src_rect->height;
    dst_w = dst_rect->width;
    dst_h = dst_rect->height;

    src_addr[0] = src_buf->phy_addr[0];
    src_addr[1] = src_buf->phy_addr[1];
    src_addr[2] = src_buf->phy_addr[2];

    dst_addr[0] = dst_buf->phy_addr[0];
    dst_addr[1] = dst_buf->phy_addr[1];
    dst_addr[2] = dst_buf->phy_addr[2];

    src_stride[0] = src_buf->stride[0];
    src_stride[1] = src_buf->stride[1];
    dst_stride[0] = dst_buf->stride[0];
    dst_stride[1] = dst_buf->stride[1];

    ge_buf_crop(src_addr, src_stride,
            src_buf->format,
            src_rect->x,
            src_rect->y,
            src_w,
            src_h);

    ge_buf_crop(dst_addr, dst_stride,
            dst_buf->format,
            dst_rect->x,
            dst_rect->y,
            dst_w,
            dst_h);

    ge_set_src_info(GE_BASE, src_w, src_h,
            src_stride[0], src_stride[1],
            src_addr);

    ge_set_output_info(GE_BASE, dst_w, dst_h,
               dst_stride[0], dst_stride[1],
               dst_addr);

    if (need_blend(ctrl)) {
        ge_set_dst_info(GE_BASE, dst_w, dst_h,
                dst_stride[0], dst_stride[1],
                dst_addr);
    }

    return 0;
}

static int check_fillrect(struct aic_ge_data *data,
              struct ge_fillrect *fill)
{
    enum mpp_pixel_format dst_format = fill->dst_buf.format;
    struct mpp_rect *dst_rect = &fill->dst_buf.crop;
    struct mpp_size *dst_size = &fill->dst_buf.size;

    if (fill->dst_buf.crop_en) {
        if (dst_rect->x < 0 ||
            dst_rect->y < 0 ||
            dst_rect->x >= dst_size->width ||
            dst_rect->y >= dst_size->height) {
            hal_log_err("%s failed\n", __func__);
            return -1;
        }
    }

    switch (fill->type) {
    case GE_NO_GRADIENT:
    case GE_H_LINEAR_GRADIENT:
    case GE_V_LINEAR_GRADIENT:
        break;
    default:
        hal_log_err("invalid type: %08x\n", fill->type);
        return -1;
    }

    if (!fill->dst_buf.crop_en) {
        dst_rect->x = 0;
        dst_rect->y = 0;
        dst_rect->width = dst_size->width;
        dst_rect->height = dst_size->height;
    }

    switch (dst_format) {
    case MPP_FMT_YUV420P:
    case MPP_FMT_NV12:
    case MPP_FMT_NV21:
        dst_rect->x = dst_rect->x & (~1);
        dst_rect->y = dst_rect->y & (~1);
        dst_rect->width = dst_rect->width & (~1);
        dst_rect->height = dst_rect->height & (~1);
        dst_size->width = dst_size->width & (~1);
        dst_size->height = dst_size->height & (~1);
        break;
    case MPP_FMT_YUV422P:
    case MPP_FMT_NV16:
    case MPP_FMT_NV61:
    case MPP_FMT_YUYV:
    case MPP_FMT_YVYU:
    case MPP_FMT_UYVY:
    case MPP_FMT_VYUY:
        dst_rect->x = dst_rect->x & (~1);
        dst_rect->width = dst_rect->width & (~1);
        dst_size->width = dst_size->width & (~1);
        break;
    default:
        break;
    }

    /* crop dst invalid region */
    if ((dst_rect->x + dst_rect->width) > dst_size->width)
        dst_rect->width = dst_size->width - dst_rect->x;

    if ((dst_rect->y + dst_rect->height) > dst_size->height)
        dst_rect->height = dst_size->height - dst_rect->y;

    if (dst_rect->width > MAX_WIDTH ||
        dst_rect->height > MAX_HEIGHT) {
        hal_log_err("invalid dst size, over the largest\n");
        return -1;
    }

    if (!is_rgb(dst_format) &&
            (dst_rect->width < 8 ||
             dst_rect->height < 8)) {
        hal_log_err("invalid dst nsize, the min size of yuv is 8\n");
        return -1;
    }

    return 0;
}

static int ge_config_fillrect_addr(struct aic_ge_data *data,
                   struct ge_fillrect *fill)
{
    u32 dst_w;
    u32 dst_h;
    u32 dst_addr[3];
    u32 dst_stride[2];
    struct mpp_rect *dst_rect;

    dst_rect = &fill->dst_buf.crop;
    dst_w = dst_rect->width;
    dst_h = dst_rect->height;

    dst_addr[0] = fill->dst_buf.phy_addr[0];
    dst_addr[1] = fill->dst_buf.phy_addr[1];
    dst_addr[2] = fill->dst_buf.phy_addr[2];

    dst_stride[0] = fill->dst_buf.stride[0];
    dst_stride[1] = fill->dst_buf.stride[1];

    ge_buf_crop(dst_addr, dst_stride,
            fill->dst_buf.format,
            dst_rect->x,
            dst_rect->y,
            dst_w,
            dst_h);

    ge_set_output_info(GE_BASE, dst_w, dst_h,
               dst_stride[0], dst_stride[1],
               dst_addr);

    /* src must has the same width and height as dst in fillrect */
    ge_set_src_info(GE_BASE, dst_w, dst_h,
            dst_stride[0], dst_stride[1],
            dst_addr);

    if (need_blend(&fill->ctrl)) {
        ge_set_dst_info(GE_BASE, dst_w, dst_h,
                dst_stride[0], dst_stride[1],
                dst_addr);
    }

    return 0;
}

static int ge_fillrect(struct aic_ge_data *data,
               struct ge_fillrect *fill)
{
    int ret;
    enum mpp_pixel_format src_fmt = MPP_FMT_ARGB_8888;

    /* check buf type */
    if (ge_check_buf(data, &fill->dst_buf) < 0)
        return -1;

    if (check_fillrect(data, fill) != 0)
        return -1;
#ifdef AIC_GE_DRV_V11
    /* check rgb type */
    if (!is_rgb(fill->dst_buf.format)) {
        hal_log_err("fill rectangle not support yuv format\n");
            return -1;
        }
#endif

    set_alpha_rules_and_premul(data, &fill->ctrl,
                   src_fmt, fill->dst_buf.format,
                   0, fill->dst_buf.flags,
                   1);

    set_csc_flow(data, &fill->ctrl,
             src_fmt, fill->dst_buf.format);

    /* config dst csc1 yuvtorgb coefs */
    if (data->csc1_en)
        ge_set_csc_coefs(GE_BASE,
                 MPP_BUF_COLOR_SPACE_GET(fill->dst_buf.flags),
                 1);

    /* config dst csc2 rgb2yuv coefs */
    if (data->csc2_en)
        ge_set_csc2_coefs(GE_BASE,
                  MPP_BUF_COLOR_SPACE_GET(fill->dst_buf.flags));

    ge_config_src_simple(GE_BASE,
                 fill->ctrl.src_global_alpha,
                 fill->ctrl.src_alpha_mode,
                 data->src_premul_en,
                 0, /* func_select */
                 src_fmt,
                 fill->type + 1); /* source_mode */

    ge_config_output_ctrl(GE_BASE,
                  data->out_premul_en,
                  fill->dst_buf.format,
                  fill->ctrl.dither_en,
                  data->csc2_en);

    if (need_blend(&fill->ctrl)) {
        ge_dst_enable(GE_BASE,
                  fill->ctrl.dst_global_alpha,
                  fill->ctrl.dst_alpha_mode,
                  fill->dst_buf.format,
                  data->csc1_en);

        ge_config_blend(GE_BASE,
                data->src_de_premul_en,
                data->dst_de_premul_en,
                0, /* disable alpha output oxff */
                data->src_alpha_coef,
                data->dst_alpha_coef,
                fill->ctrl.ck_en,
                fill->ctrl.alpha_en);

    } else {
        ge_config_blend(GE_BASE,
                data->src_de_premul_en,
                data->dst_de_premul_en,
                0, /* disable alpha output oxff */
                data->src_alpha_coef,
                data->dst_alpha_coef,
                fill->ctrl.ck_en,
                fill->ctrl.alpha_en);

        ge_dst_disable(GE_BASE);
    }

    if (fill->ctrl.ck_en)
        ge_config_color_key(GE_BASE, fill->ctrl.ck_value);

    ge_scaler0_enable(GE_BASE, 0);
    ge_config_fillrect_addr(data, fill);

    switch (fill->type) {
    case GE_NO_GRADIENT:
        ge_config_fillrect(GE_BASE, fill->start_color);
        break;
    case GE_H_LINEAR_GRADIENT:
        ge_config_fill_gradient(GE_BASE,
                    fill->dst_buf.crop.width,
                    fill->dst_buf.crop.height,
                    fill->start_color,
                    fill->end_color,
                    0);
        break;
    case GE_V_LINEAR_GRADIENT:
        ge_config_fill_gradient(GE_BASE,
                    fill->dst_buf.crop.width,
                    fill->dst_buf.crop.height,
                    fill->start_color,
                    fill->end_color,
                    1);
        break;
    default:
        break;
    }

    ret = ge_start_and_wait(data);

    return ret;
}

static int ge_bitblt(struct aic_ge_data *data, struct ge_bitblt *blt)
{
    int ret;

    /* check buf type */
    if (ge_check_buf(data, &blt->src_buf) < 0 ||
        ge_check_buf(data, &blt->dst_buf) < 0) {
        return -1;
    }
#ifdef AIC_GE_DRV_V11
    /* check rgb type */
    if (!is_rgb(blt->src_buf.format) ||
        !is_rgb(blt->dst_buf.format)) {
        hal_log_err("bitblt not support yuv format\n");
        return -1;
    }
#endif
    if (check_bitblt(data, blt) != 0)
        return -1;

    set_alpha_rules_and_premul(data, &blt->ctrl,
                   blt->src_buf.format, blt->dst_buf.format,
                   blt->src_buf.flags, blt->dst_buf.flags,
                   0);

    set_csc_flow(data, &blt->ctrl,
             blt->src_buf.format, blt->dst_buf.format);

    /* config src csc0 yuvtorgb coefs */
    if (data->csc0_en)
        ge_set_csc_coefs(GE_BASE,
                 MPP_BUF_COLOR_SPACE_GET(blt->src_buf.flags),
                 0);

    /* config dst csc1 yuvtorgb coefs */
    if (data->csc1_en)
        ge_set_csc_coefs(GE_BASE,
                 MPP_BUF_COLOR_SPACE_GET(blt->dst_buf.flags),
                 1);

    /* config dst csc2 rgb2yuv coefs */
    if (data->csc2_en)
        ge_set_csc2_coefs(GE_BASE,
                  MPP_BUF_COLOR_SPACE_GET(blt->dst_buf.flags));

    ge_config_src_ctrl(GE_BASE,
               blt->ctrl.src_global_alpha,
               blt->ctrl.src_alpha_mode,
               data->src_premul_en,
               MPP_SCAN_ORDER_GET(blt->ctrl.flags),
               0, /* func_select */
               blt->src_buf.format,
               MPP_FLIP_V_GET(blt->ctrl.flags),
               MPP_FLIP_H_GET(blt->ctrl.flags),
               MPP_ROTATION_GET(blt->ctrl.flags),
               0, /* fill buffer mode */
               data->csc0_en);

    ge_config_output_ctrl(GE_BASE,
                  data->out_premul_en,
                  blt->dst_buf.format,
                  blt->ctrl.dither_en,
                  data->csc2_en);

    if (need_blend(&blt->ctrl)) {
        ge_dst_enable(GE_BASE,
                  blt->ctrl.dst_global_alpha,
                  blt->ctrl.dst_alpha_mode,
                  blt->dst_buf.format,
                  data->csc1_en);

        ge_config_blend(GE_BASE,
                data->src_de_premul_en,
                data->dst_de_premul_en,
                0, /* disable alpha output oxff */
                data->src_alpha_coef,
                data->dst_alpha_coef,
                blt->ctrl.ck_en,
                blt->ctrl.alpha_en);

    } else {
        ge_config_blend(GE_BASE,
                data->src_de_premul_en,
                data->dst_de_premul_en,
                0, /* disable alpha output oxff */
                data->src_alpha_coef,
                data->dst_alpha_coef,
                blt->ctrl.ck_en,
                blt->ctrl.alpha_en);

        ge_dst_disable(GE_BASE);
    }

    if (blt->ctrl.ck_en)
        ge_config_color_key(GE_BASE, blt->ctrl.ck_value);

    if (blt->ctrl.dither_en)
            ge_config_dither(GE_BASE, g_data->dither_line_phys);

    ge_config_scaler(data, blt);

    ge_config_addr(data, &blt->src_buf, &blt->dst_buf, &blt->ctrl);

    ret = ge_start_and_wait(data);

    return ret;
}

static int check_format_and_size(struct aic_ge_data *data,
                 struct mpp_buf *src_buf,
                 struct mpp_buf *dst_buf)
{
    enum mpp_pixel_format src_format = src_buf->format;
    enum mpp_pixel_format dst_format = dst_buf->format;
    struct mpp_rect *src_rect = &src_buf->crop;
    struct mpp_rect *dst_rect = &dst_buf->crop;

    if (src_buf->crop_en) {
        if (src_rect->x < 0 ||
            src_rect->y < 0 ||
            src_rect->x >= src_buf->size.width ||
            src_rect->y >= src_buf->size.height) {
            hal_log_err("src_rect->x:%d, src_rect->y:%d\n", src_rect->x, src_rect->y);
            hal_log_err("width:%d, height:%d\n", src_buf->size.width, src_buf->size.height);
            hal_log_err("%s failed, invalid src crop\n", __func__);
            return -1;
        }
    }

    if (dst_buf->crop_en) {
        if (dst_rect->x < 0 ||
            dst_rect->y < 0 ||
            dst_rect->x >= dst_buf->size.width ||
            dst_rect->y >= dst_buf->size.height) {
            hal_log_err("%s failed, invalid dst crop\n", __func__);
            return -1;
        }
    }

    if (!src_buf->crop_en) {
        src_rect->x = 0;
        src_rect->y = 0;
        src_rect->width = src_buf->size.width;
        src_rect->height = src_buf->size.height;
    }

    if (!dst_buf->crop_en) {
        dst_rect->x = 0;
        dst_rect->y = 0;
        dst_rect->width = dst_buf->size.width;
        dst_rect->height = dst_buf->size.height;
    }

    switch (src_format) {
    case MPP_FMT_ARGB_8888:
    case MPP_FMT_ABGR_8888:
    case MPP_FMT_RGBA_8888:
    case MPP_FMT_BGRA_8888:
    case MPP_FMT_XRGB_8888:
    case MPP_FMT_XBGR_8888:
    case MPP_FMT_RGBX_8888:
    case MPP_FMT_BGRX_8888:
    case MPP_FMT_RGB_888:
    case MPP_FMT_BGR_888:
    case MPP_FMT_ARGB_1555:
    case MPP_FMT_ABGR_1555:
    case MPP_FMT_RGBA_5551:
    case MPP_FMT_BGRA_5551:
    case MPP_FMT_RGB_565:
    case MPP_FMT_BGR_565:
    case MPP_FMT_ARGB_4444:
    case MPP_FMT_ABGR_4444:
    case MPP_FMT_RGBA_4444:
    case MPP_FMT_BGRA_4444:
        break;
    default:
        hal_log_err("unsupport src format:%d\n", src_format);
        return -1;
    }

    switch (dst_format) {
    case MPP_FMT_ARGB_8888:
    case MPP_FMT_ABGR_8888:
    case MPP_FMT_RGBA_8888:
    case MPP_FMT_BGRA_8888:
    case MPP_FMT_XRGB_8888:
    case MPP_FMT_XBGR_8888:
    case MPP_FMT_RGBX_8888:
    case MPP_FMT_BGRX_8888:
    case MPP_FMT_RGB_888:
    case MPP_FMT_BGR_888:
    case MPP_FMT_ARGB_1555:
    case MPP_FMT_ABGR_1555:
    case MPP_FMT_RGBA_5551:
    case MPP_FMT_BGRA_5551:
    case MPP_FMT_RGB_565:
    case MPP_FMT_BGR_565:
    case MPP_FMT_ARGB_4444:
    case MPP_FMT_ABGR_4444:
    case MPP_FMT_RGBA_4444:
    case MPP_FMT_BGRA_4444:
        break;
    default:
        hal_log_err("unsupport dst format:%d\n", dst_format);
        return -1;
    }

    /* crop src invalid region */
    if ((src_rect->x + src_rect->width) >
        src_buf->size.width)
        src_rect->width = src_buf->size.width -
                  src_rect->x;

    if ((src_rect->y + src_rect->height) >
        src_buf->size.height)
        src_rect->height = src_buf->size.height -
                   src_rect->y;

    if (src_rect->width < 4 || src_rect->height < 4 ||
            src_rect->width > MAX_WIDTH ||
            src_rect->height > MAX_HEIGHT) {
        hal_log_err("unsupport src size\n");
        return -1;
    }

    /* crop dst invalid region */
    if ((dst_rect->x + dst_rect->width) >
        dst_buf->size.width)
        dst_rect->width = dst_buf->size.width -
                  dst_rect->x;

    if ((dst_rect->y + dst_rect->height) >
        dst_buf->size.height)
        dst_rect->height = dst_buf->size.height -
                   dst_rect->y;

    if (dst_rect->width < 4 || dst_rect->height < 4 ||
            dst_rect->width > MAX_WIDTH ||
            dst_rect->height > MAX_HEIGHT) {
        hal_log_err("unsupport dst size\n");
        return -1;
    }

    return 0;
}

static int ge_rotate(struct aic_ge_data *data, struct ge_rotation *rot)
{
    int ret;

    /* check buf type */
    if (ge_check_buf(data, &rot->src_buf) < 0 ||
        ge_check_buf(data, &rot->dst_buf) < 0) {
        return -1;
    }

    /* check rgb type */
    if (!is_rgb(rot->src_buf.format) ||
        !is_rgb(rot->dst_buf.format)) {
        hal_log_err("rotation does not support yuv format\n");
        return -1;
    }

    if (MPP_SCAN_ORDER_GET(rot->ctrl.flags)) {
        hal_log_err("opening scan order is not supported " \
                    "using the rotation\n");
                return -1;
    }

    if (check_format_and_size(data, &rot->src_buf,
                  &rot->dst_buf) != 0)
        return -1;

    set_alpha_rules_and_premul(data, &rot->ctrl,
                   rot->src_buf.format, rot->dst_buf.format,
                   rot->src_buf.flags, rot->dst_buf.flags,
                   0);

    /* rot1 only support rgb format */
    data->csc0_en = 0;
    data->csc1_en = 0;
    data->csc2_en = 0;

    ge_config_src_simple(GE_BASE,
                 rot->ctrl.src_global_alpha,
                 rot->ctrl.src_alpha_mode,
                 data->src_premul_en,
                 1, /* rot1 */
                 rot->src_buf.format,
                 0); /* fill buffer mode */

    ge_config_output_ctrl(GE_BASE,
                  data->out_premul_en,
                  rot->dst_buf.format,
                  0, /* rot1 does't support dither */
                  data->csc2_en);

    if (need_blend(&rot->ctrl)) {
        ge_dst_enable(GE_BASE,
                  rot->ctrl.dst_global_alpha,
                  rot->ctrl.dst_alpha_mode,
                  rot->dst_buf.format,
                  data->csc1_en);

        ge_config_blend(GE_BASE,
                data->src_de_premul_en,
                data->dst_de_premul_en,
                0, /* disable alpha output oxff */
                data->src_alpha_coef,
                data->dst_alpha_coef,
                rot->ctrl.ck_en,
                rot->ctrl.alpha_en);

    } else {
        ge_config_blend(GE_BASE,
                data->src_de_premul_en,
                data->dst_de_premul_en,
                0, /* disable alpha output oxff */
                data->src_alpha_coef,
                data->dst_alpha_coef,
                rot->ctrl.ck_en,
                rot->ctrl.alpha_en);

        ge_dst_disable(GE_BASE);
    }

    if (rot->ctrl.ck_en) {
        hal_log_warn("warning: rot does't support color key\n");
        rot->ctrl.ck_en = 0;
    }

    ge_scaler0_enable(GE_BASE, 0);

    ge_config_rot1(GE_BASE,
               rot->angle_sin,
               rot->angle_cos,
               rot->src_rot_center.x,
               rot->src_rot_center.y,
               rot->dst_rot_center.x,
               rot->dst_rot_center.y);

    ge_config_addr(data, &rot->src_buf, &rot->dst_buf, &rot->ctrl);

    ret = ge_start_and_wait(data);

    return ret;
}

int hal_ge_write(struct aic_ge_client *clt, const char *buff, size_t count)
{
    return -1;
}

int hal_ge_control(struct aic_ge_client *clt, int cmd, void *arg)
{
    int ret = 0;

    (void)clt;

    aicos_mutex_take(g_data->lock, AICOS_WAIT_FOREVER);

    switch (cmd) {
    case IOC_GE_VERSION:
    {
        u32 version = ge_get_version_id(GE_BASE);
        memcpy(arg, &version, sizeof(u32));
    }
    break;
    case IOC_GE_MODE:
        memcpy(arg, &g_data->ge_mode, sizeof(u32));
        break;
    case IOC_GE_FILLRECT:
    {
        struct ge_fillrect fill;

        memcpy(&fill, arg, sizeof(fill));
        ret = ge_fillrect(g_data, &fill);
    }
    break;
    case IOC_GE_BITBLT:
    {
        struct ge_bitblt blt;

        memcpy(&blt, arg,  sizeof(blt));
        ret = ge_bitblt(g_data, &blt);
    }
    break;
    case IOC_GE_ROTATE:
    {
        struct ge_rotation rot;

        memcpy(&rot, arg, sizeof(rot));
        ret = ge_rotate(g_data, &rot);
    }
    break;
    default:
        hal_log_err("Invalid ioctl: %08x\n", cmd);
        ret = -1;
    }
    aicos_mutex_give(g_data->lock);

    return ret;
}

static int ge_clk_enable(struct aic_ge_data *data)
{
    int ret;

    hal_reset_assert(RESET_GE);
    ret = hal_reset_deassert(RESET_GE);
    if (ret) {
        hal_log_err("%s() - Couldn't deassert\n", __func__);
        return ret;
    }

    ret = hal_clk_set_freq(CLK_GE, GE_CLOCK);
    if (ret < 0) {
        hal_log_err("VE set clk failed");
        return -1;
    }

    ret = hal_clk_enable(CLK_GE);
    if (ret) {
        hal_log_err("%s() - Couldn't enable mclk\n", __func__);
        return ret;
    }

    return 0;
}

struct aic_ge_client *hal_ge_open(void)
{
    struct aic_ge_client *client = NULL;

    client = aicos_malloc(GE_DEFAULT, sizeof(struct aic_ge_client));

    if (!client)
        return NULL;

    memset(client, 0, sizeof(struct aic_ge_client));

    return client;
}

int hal_ge_close(struct aic_ge_client *clt)
{
    struct aic_ge_client *client = clt;

    if (client) {
        aicos_free(GE_DEFAULT, client);
    }

    return 0;
}

int hal_ge_init(void)
{
    struct aic_ge_data *data;

    hal_log_info("%s()\n", __func__);

    data = aicos_malloc(GE_DEFAULT, sizeof(struct aic_ge_data));
    if (!data)
        return -1;

    memset(data, 0, sizeof(struct aic_ge_data));

    data->wait = aicos_event_create();
    aicos_request_irq(GE_IRQn, aic_ge_handler, 0, NULL, (void *)data);
    data->ge_mode = GE_MODE_NORMAL;
    data->lock = aicos_mutex_create();

#ifdef AIC_GE_DITHER
    int dither_line_len = ALIGN_128B(((MAX_WIDTH * 4) + 127));
    data->dither_line_ptr = (u8 *)aicos_malloc(GE_CMA,  dither_line_len);
    if (!data->dither_line_ptr) {
        hal_log_err("failed to malloc dither line buffer\n");
        return -1;
    }
    data->dither_line_phys = ALIGN_128B((uintptr_t)data->dither_line_ptr);

    aicos_dcache_clean_invalid_range((unsigned long *)(data->dither_line_phys), (MAX_WIDTH * 4));

    hal_log_info("dither line phys :0X0%08x\n", (u32)data->dither_line_phys);
#endif
    g_data = data;

    ge_clk_enable(data);

    hal_log_info("%s() end\n", __func__);

    return 0;
}

int hal_ge_deinit(void)
{
    if (g_data) {
        aicos_mutex_delete(g_data->lock);
        aicos_event_delete(g_data->wait);
        aicos_free(GE_DEFAULT, g_data);
#ifdef AIC_GE_DITHER
        aicos_free(GE_CMA, g_data->dither_line_ptr);
#endif
    }

    g_data = NULL;
    return 0;
}
