/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <aic_hal.h>
#include <aic_hal_de.h>

#include "disp_com.h"

#define MAX_LAYER_NUM 2
#define MAX_RECT_NUM 4
#define RECT_NUM_SHIFT 2
#define CONFIG_NUM   (MAX_LAYER_NUM * MAX_RECT_NUM)

#define DITHER_RGB565   0x1
#define DITHER_RGB666   0x2

#define VSYNC_EVENT    1
#define DE_TIMEOUT     100

static unsigned int event_flag = 0;

struct aic_de_dither {
    unsigned int enable;
    unsigned int red_bitdepth;
    unsigned int gleen_bitdepth;
    unsigned int blue_bitdepth;
};

struct aic_de_configs {
    const struct aicfb_layer_num *layer_num;
    const struct aicfb_layer_capability *cap;
};

struct aic_de_comp {
    void *regs;

    const struct aic_de_configs *config;

    struct aic_de_dither dither;
    struct aicfb_layer_data layers[CONFIG_NUM];
    struct aicfb_alpha_config alpha[MAX_LAYER_NUM];
    struct aicfb_ck_config ck[MAX_LAYER_NUM];
    struct aicfb_disp_prop disp_prop;
    struct aicfb_ccm_config ccm;
    struct aicfb_gamma_config gamma;

    const struct display_timing *timing;
    aicos_event_t vsync_event;
};
static struct aic_de_comp *g_aic_de_comp;

static struct aic_de_comp *aic_de_request_drvdata(void)
{
    return g_aic_de_comp;
}

static void aic_de_release_drvdata(void)
{

}

static irqreturn_t aic_de_handler(int irq, void *ctx)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();
    u32 status;

    status = de_timing_interrupt_status(comp->regs);
    de_timing_interrupt_clean_status(comp->regs, status);

    if (status & TIMING_INIT_V_BLANK_FLAG) {
        if (event_flag) {
            event_flag = 0;
            aicos_event_send(comp->vsync_event, VSYNC_EVENT);
        }
    }

    if (status & TIMING_INIT_UNDERFLOW_FLAG)
        pr_err("ERROR: DE UNDERFLOW\n");

    return IRQ_HANDLED;
}

static inline bool is_valid_layer_id(struct aic_de_comp *comp, u32 layer_id)
{
    u32 total_num = comp->config->layer_num->vi_num
            + comp->config->layer_num->ui_num;

    if (layer_id < total_num)
        return true;
    else
        return false;
}

static inline bool need_update_disp_prop(struct aic_de_comp *comp,
                     struct aicfb_disp_prop *disp_prop)
{
    if (disp_prop->bright != comp->disp_prop.bright ||
        disp_prop->contrast != comp->disp_prop.contrast ||
        disp_prop->saturation != comp->disp_prop.saturation ||
        disp_prop->hue != comp->disp_prop.hue)
        return true;

    return false;
}

static inline bool need_use_hsbc(struct aic_de_comp *comp,
                 struct aicfb_disp_prop *disp_prop)
{
    if (disp_prop->bright != 50 ||
        disp_prop->contrast != 50 ||
        disp_prop->saturation != 50 ||
        disp_prop->hue != 50)
        return true;

    return false;
}

static inline bool need_update_csc(struct aic_de_comp *comp, int color_space)
{
    /* get color space from video layer config */
    struct aicfb_layer_data *layer_data = &comp->layers[0];

    if (color_space != MPP_BUF_COLOR_SPACE_GET(layer_data->buf.flags))
        return true;

    return false;
}

static inline bool is_ui_layer(struct aic_de_comp *comp, u32 layer_id)
{
    if (comp->config->cap[layer_id].layer_type == AICFB_LAYER_TYPE_UI)
        return true;
    else
        return false;
}

static inline bool is_valid_layer_and_rect_id(struct aic_de_comp *comp,
                          u32 layer_id, u32 rect_id)
{
    u32 flags;

    if (!is_valid_layer_id(comp, layer_id))
        return false;

    flags = comp->config->cap[layer_id].cap_flags;

    if (flags & AICFB_CAP_4_RECT_WIN_FLAG) {
        if (layer_id != 0 && rect_id >= MAX_RECT_NUM)
            return false;
    }
    return true;
}

static inline bool is_support_color_key(struct aic_de_comp *comp,
                    u32 layer_id)
{
    u32 flags;

    if (!is_valid_layer_id(comp, layer_id))
        return false;

    flags = comp->config->cap[layer_id].cap_flags;

    if (flags & AICFB_CAP_CK_FLAG)
        return true;
    else
        return false;
}

static inline bool is_support_alpha_blending(struct aic_de_comp *comp,
                         u32 layer_id)
{
    u32 flags;

    if (!is_valid_layer_id(comp, layer_id))
        return false;

    flags = comp->config->cap[layer_id].cap_flags;

    if (flags & AICFB_CAP_ALPHA_FLAG)
        return true;
    else
        return false;
}

static int aic_de_set_mode(struct aic_panel *panel)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    comp->timing = panel->timings;

    return 0;
}

static int aic_de_clk_enable(void)
{
    hal_clk_enable(CLK_DE);
    hal_clk_enable(CLK_PIX);
    hal_clk_set_freq(CLK_DE, DE_FREQ);

    hal_reset_deassert(RESET_DE);
    return 0;
}

static int aic_de_clk_disable(void)
{
    hal_reset_assert(RESET_DE);

    hal_clk_disable(CLK_DE);
    hal_clk_disable(CLK_PIX);
    return 0;
}

static int aic_de_timing_enable(void)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();
    u32 active_w = comp->timing->hactive;
    u32 active_h = comp->timing->vactive;
    u32 hfp = comp->timing->hfront_porch;
    u32 hbp = comp->timing->hback_porch;
    u32 vfp = comp->timing->vfront_porch;
    u32 vbp = comp->timing->vback_porch;
    u32 hsync = comp->timing->hsync_len;
    u32 vsync = comp->timing->vsync_len;

    de_config_timing(comp->regs, active_w, active_h, hfp, hbp,
            vfp, vbp, hsync, vsync);

    de_set_blending_size(comp->regs, active_w, active_h);
    de_set_ui_layer_size(comp->regs, active_w, active_h, 0, 0);

    comp->alpha[1].layer_id = AICFB_LAYER_TYPE_UI;
    comp->alpha[1].enable = 1;
    comp->alpha[1].mode = 0;
    comp->alpha[1].value = 0xff;

    de_config_prefetch_line_set(comp->regs, 2);
    de_soft_reset_ctrl(comp->regs, 1);

    de_set_qos(comp->regs);

    de_set_mode(comp->regs, DE_MODE);
#if DE_AUTO_MODE
    if (de_set_te_pinmux(TE_PIN) < 0)
        pr_err("Invalid TE pin\n");

    de_set_te_pulse_width(comp->regs, TE_PULSE_WIDTH);
#endif

    if (comp->dither.enable)
        de_set_dither(comp->regs,
                comp->dither.red_bitdepth,
                comp->dither.gleen_bitdepth,
                comp->dither.blue_bitdepth,
                comp->dither.enable);

    de_ui_alpha_blending_enable(comp->regs, comp->alpha[1].value,
                    comp->alpha[1].mode,
                    comp->alpha[1].enable);

    aicos_request_irq(DE_IRQn, aic_de_handler, 0, "aic-de", NULL);

#ifndef AIC_BOOTLOADER_CMD_PROGRESS_BAR
    de_timing_enable_interrupt(comp->regs, true, TIMING_INIT_V_BLANK_FLAG);
    de_timing_enable_interrupt(comp->regs, true, TIMING_INIT_UNDERFLOW_FLAG);
#endif

    de_timing_enable(comp->regs, 1);

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_timing_disable(void)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    de_timing_enable(comp->regs, 0);
    aic_de_release_drvdata();
    return 0;
}

static int aic_de_wait_for_vsync(void)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();
    uint32_t recved;
    int ret;

    event_flag = 1;
    ret = aicos_event_recv(comp->vsync_event,
                           VSYNC_EVENT,
                           &recved,
                           DE_TIMEOUT);
    if (ret < 0) {
            hal_log_err("DE wait vsync irq failed\n");
            return ret;
    }
    return 0;
}

static int aic_de_get_alpha_config(struct aicfb_alpha_config *alpha)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    if (is_support_alpha_blending(comp, alpha->layer_id) == false) {
        pr_err("layer %d doesn't support alpha blending\n",
            alpha->layer_id);
        aic_de_release_drvdata();
        return -EINVAL;
    }

    comp->alpha[alpha->layer_id].layer_id = alpha->layer_id;
    memcpy(alpha, &comp->alpha[alpha->layer_id],
        sizeof(struct aicfb_alpha_config));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_update_alpha_config(struct aicfb_alpha_config *alpha)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    if (is_support_alpha_blending(comp, alpha->layer_id) == false) {
        pr_err("layer %d doesn't support alpha blending\n",
                alpha->layer_id);
        aic_de_release_drvdata();
        return -EINVAL;
    }

    de_config_update_enable(comp->regs, 0);
    de_ui_alpha_blending_enable(comp->regs,
        alpha->value, alpha->mode, alpha->enable);
    de_config_update_enable(comp->regs, 1);
    memcpy(&comp->alpha[alpha->layer_id],
        alpha, sizeof(struct aicfb_alpha_config));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_get_ck_config(struct aicfb_ck_config *ck)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    if (is_support_color_key(comp, ck->layer_id) == false) {
        pr_err("Layer %d doesn't support color key blending\n", ck->layer_id);
        aic_de_release_drvdata();
        return -EINVAL;
    }
    comp->ck[ck->layer_id].layer_id = ck->layer_id;
    memcpy(ck, &comp->ck[ck->layer_id], sizeof(struct aicfb_ck_config));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_update_ck_config(struct aicfb_ck_config *ck)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    if (is_support_color_key(comp, ck->layer_id) == false) {
        pr_err("Layer %d doesn't support color key blending\n", ck->layer_id);
        aic_de_release_drvdata();
        return -EINVAL;
    }

    if (ck->enable > 1) {
        pr_err("Invalid ck enable: %d\n", ck->enable);
        aic_de_release_drvdata();
        return -EINVAL;
    }

    de_config_update_enable(comp->regs, 0);
    de_ui_layer_color_key_enable(comp->regs, ck->value, ck->enable);
    de_config_update_enable(comp->regs, 1);
    memcpy(&comp->ck[ck->layer_id], ck, sizeof(struct aicfb_ck_config));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_get_layer_config(struct aicfb_layer_data *layer_data)
{
    u32 index;
    struct aic_de_comp *comp = aic_de_request_drvdata();

    if (is_valid_layer_and_rect_id(comp, layer_data->layer_id,
                       layer_data->rect_id) == false) {
        pr_err("invalid layer_id %d or rect_id %d\n",
            layer_data->layer_id, layer_data->rect_id);
        aic_de_release_drvdata();
        return -EINVAL;
    }

    index = (layer_data->layer_id << RECT_NUM_SHIFT) + layer_data->rect_id;
    comp->layers[index].layer_id = layer_data->layer_id;
    comp->layers[index].rect_id = layer_data->rect_id;
    memcpy(layer_data, &comp->layers[index],
        sizeof(struct aicfb_layer_data));

    aic_de_release_drvdata();
    return 0;
}

static bool is_valid_video_size(struct aic_de_comp *comp,
                struct aicfb_layer_data *layer_data)
{
    u32 src_width;
    u32 src_height;
    u32 x_offset;
    u32 y_offset;
    u32 active_w;
    u32 active_h;

    src_width = layer_data->buf.size.width;
    src_height = layer_data->buf.size.height;
    x_offset = layer_data->pos.x;
    y_offset = layer_data->pos.y;
    active_w = comp->timing->hactive;
    active_h = comp->timing->vactive;

    if (x_offset >= active_w || y_offset >= active_h) {
        pr_err("video layer x or y offset is invalid\n");
        return false;
    }

    if (layer_data->buf.crop_en) {
        u32 crop_x = layer_data->buf.crop.x;
        u32 crop_y = layer_data->buf.crop.y;

        if (crop_x >= src_width || crop_y >= src_height) {
            pr_err("video layer crop is invalid\n");
            return false;
        }

        if (crop_x + layer_data->buf.crop.width > src_width)
            layer_data->buf.crop.width = src_width - crop_x;

        if (crop_y + layer_data->buf.crop.height > src_height)
            layer_data->buf.crop.height = src_height - crop_y;
    }

    if (x_offset + layer_data->scale_size.width > active_w)
        layer_data->scale_size.width = active_w - x_offset;

    if (y_offset + layer_data->scale_size.height > active_h)
        layer_data->scale_size.height = active_h - y_offset;

    return true;
}

static bool is_ui_rect_win_overlap(struct aic_de_comp *comp,
                   u32 layer_id, u32 rect_id,
                   u32 x, u32 y, u32 w, u32 h)
{
    int i;
    u32 index;
    u32 cur_x, cur_y;
    u32 cur_w, cur_h;

    index = (layer_id << RECT_NUM_SHIFT);

    for (i = 0; i < MAX_RECT_NUM; i++) {
        if (rect_id == i) {
            index++;
            continue;
        }

        if (comp->layers[index].enable) {
            if (comp->layers[index].buf.crop_en) {
                cur_w = comp->layers[index].buf.crop.width;
                cur_h = comp->layers[index].buf.crop.height;
            } else {
                cur_w = comp->layers[index].buf.size.width;
                cur_h = comp->layers[index].buf.size.height;
            }
            cur_x = comp->layers[index].pos.x;
            cur_y = comp->layers[index].pos.y;

            if ((min(y + h, cur_y + cur_h) > max(y, cur_y)) &&
                (min(x + w, cur_x + cur_w) > max(x, cur_x))) {
                return true;
            }
        }
        index++;
    }
    return false;
}

static bool is_valid_ui_rect_size(struct aic_de_comp *comp,
                  struct aicfb_layer_data *layer_data)
{
    u32 src_width;
    u32 src_height;
    u32 x_offset;
    u32 y_offset;
    u32 active_w;
    u32 active_h;

    u32 w;
    u32 h;

    src_width = layer_data->buf.size.width;
    src_height = layer_data->buf.size.height;
    x_offset = layer_data->pos.x;
    y_offset = layer_data->pos.y;
    active_w = comp->timing->hactive;
    active_h = comp->timing->vactive;

    if (x_offset >= active_w || y_offset >= active_h) {
        pr_err("ui layer x or y offset is invalid\n");
        return false;
    }

    if (layer_data->buf.crop_en) {
        u32 crop_x = layer_data->buf.crop.x;
        u32 crop_y = layer_data->buf.crop.y;

        if (crop_x >= src_width || crop_y >= src_height) {
            pr_err("ui layer crop is invalid\n");
            return false;
        }

        if ((crop_x + layer_data->buf.crop.width) > src_width)
            layer_data->buf.crop.width = src_width - crop_x;

        if ((crop_y + layer_data->buf.crop.height) > src_height)
            layer_data->buf.crop.height = src_height - crop_y;

        if ((x_offset + layer_data->buf.crop.width) > active_w)
            layer_data->buf.crop.width = active_w - x_offset;

        if ((y_offset + layer_data->buf.crop.height) > active_h)
            layer_data->buf.crop.height = active_h - y_offset;

        w = layer_data->buf.crop.width;
        h = layer_data->buf.crop.height;
    } else {
        if ((x_offset + src_width) > active_w)
            layer_data->buf.size.width = active_w - x_offset;

        if ((y_offset + src_height) > active_h)
            layer_data->buf.size.height = active_h - y_offset;

        w = layer_data->buf.size.width;
        h = layer_data->buf.size.height;
    }

    /* check overlap  */
    if (is_ui_rect_win_overlap(comp, layer_data->layer_id,
                   layer_data->rect_id,
                   x_offset, y_offset,    w, h)) {
        pr_err("ui rect is overlap\n");
        return false;
    }
    return true;
}

static inline bool is_all_rect_win_disabled(struct aic_de_comp *comp,
                        u32 layer_id)
{
    int i;
    u32 index;

    index = (layer_id << RECT_NUM_SHIFT);

    for (i = 0; i < MAX_RECT_NUM; i++) {
        if (comp->layers[index].enable)
            return false;

        index++;
    }
    return true;
}

static int config_ui_layer_rect(struct aic_de_comp *comp,
                struct aicfb_layer_data *layer_data)
{
    enum mpp_pixel_format format = layer_data->buf.format;
    u32 in_w = (u32)layer_data->buf.size.width;
    u32 in_h = (u32)layer_data->buf.size.height;

    u32 stride0 = layer_data->buf.stride[0];
    u32 addr0 = layer_data->buf.phy_addr[0];
    u32 x_offset = layer_data->pos.x;
    u32 y_offset = layer_data->pos.y;
    u32 crop_en = layer_data->buf.crop_en;
    u32 crop_x = layer_data->buf.crop.x;
    u32 crop_y = layer_data->buf.crop.y;
    u32 crop_w = layer_data->buf.crop.width;
    u32 crop_h = layer_data->buf.crop.height;

    switch (format) {
    case MPP_FMT_ARGB_8888:
    case MPP_FMT_ABGR_8888:
    case MPP_FMT_RGBA_8888:
    case MPP_FMT_BGRA_8888:
    case MPP_FMT_XRGB_8888:
    case MPP_FMT_XBGR_8888:
    case MPP_FMT_RGBX_8888:
    case MPP_FMT_BGRX_8888:
        if (crop_en) {
            addr0 += crop_x * 4 + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
        }
        break;
    case MPP_FMT_RGB_888:
    case MPP_FMT_BGR_888:
        if (crop_en) {
            addr0 += crop_x * 3 + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
        }
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
        if (crop_en) {
            addr0 += crop_x * 2 + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
        }
        break;
    default:
        pr_err("invalid ui layer format: %d\n",
            format);

        return -EINVAL;
    }

    if (is_all_rect_win_disabled(comp, layer_data->layer_id)) {
        de_set_ui_layer_format(comp->regs, format);
        de_ui_layer_enable(comp->regs, 1);
    }

    de_ui_layer_set_rect(comp->regs, in_w, in_h, x_offset, y_offset,
                 stride0, addr0, layer_data->rect_id);
    de_ui_layer_rect_enable(comp->regs, layer_data->rect_id, 1);

    return 0;
}

static int de_update_csc(struct aic_de_comp *comp,
             struct aicfb_disp_prop *disp_prop,
             int color_space)
{
    if (need_use_hsbc(comp, disp_prop)) {
        int bright = (int)disp_prop->bright;
        int contrast = (int)disp_prop->contrast;
        int saturation = (int)disp_prop->saturation;
        int hue = (int)disp_prop->hue;

        de_set_hsbc_with_csc_coefs(comp->regs, color_space,
                       bright, contrast,
                       saturation, hue);
    } else {
        de_set_csc0_coefs(comp->regs, color_space);
    }

    return 0;
}

static int config_video_layer(struct aic_de_comp *comp,
                  struct aicfb_layer_data *layer_data)
{
    enum mpp_pixel_format format = layer_data->buf.format;
    u32 in_w = (u32)layer_data->buf.size.width;
    u32 in_h = (u32)layer_data->buf.size.height;
    u32 in_w_ch1;
    u32 in_h_ch1;
    u32 stride0 = layer_data->buf.stride[0];
    u32 stride1 = layer_data->buf.stride[1];
    u32 addr0, addr1, addr2;
    u32 x_offset = layer_data->pos.x;
    u32 y_offset = layer_data->pos.y;
    u32 crop_en = layer_data->buf.crop_en;
    u32 crop_x = layer_data->buf.crop.x;
    u32 crop_y = layer_data->buf.crop.y;
    u32 crop_w = layer_data->buf.crop.width;
    u32 crop_h = layer_data->buf.crop.height;
    u32 tile_p0_x_offset = 0;
    u32 tile_p0_y_offset = 0;
    u32 tile_p1_x_offset = 0;
    u32 tile_p1_y_offset = 0;
    u32 channel_num = 1;
    u32 scaler_en = 0;
    u32 scaler_w = layer_data->scale_size.width;
    u32 scaler_h = layer_data->scale_size.height;
    int color_space = MPP_BUF_COLOR_SPACE_GET(layer_data->buf.flags);

    if (!scaler_w) {
        scaler_w = in_w;
        layer_data->scale_size.width = in_w;
    }

    if (!scaler_h) {
        scaler_h = in_h;
        layer_data->scale_size.height = in_h;
    }

    switch (layer_data->buf.buf_type) {
    case MPP_PHY_ADDR:
        addr0 = layer_data->buf.phy_addr[0];
        addr1 = layer_data->buf.phy_addr[1];
        addr2 = layer_data->buf.phy_addr[2];
        break;
    default:
        pr_err("invalid buf type: %d\n",
            layer_data->buf.buf_type);

        return -EINVAL;
    };

    switch (format) {
    case MPP_FMT_ARGB_8888:
    case MPP_FMT_ABGR_8888:
    case MPP_FMT_RGBA_8888:
    case MPP_FMT_BGRA_8888:
    case MPP_FMT_XRGB_8888:
    case MPP_FMT_XBGR_8888:
    case MPP_FMT_RGBX_8888:
    case MPP_FMT_BGRX_8888:
        if (crop_en) {
            addr0 += crop_x * 4 + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
        }
        break;
    case MPP_FMT_RGB_888:
    case MPP_FMT_BGR_888:
        if (crop_en) {
            addr0 += crop_x * 3 + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
        }
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
        if (crop_en) {
            addr0 += crop_x * 2 + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
        }
        break;
    case MPP_FMT_YUV420P:
        channel_num = 2;
        scaler_en = 1;
        in_w = ALIGN_EVEN(in_w);
        in_h = ALIGN_EVEN(in_h);
        crop_x = ALIGN_EVEN(crop_x);
        crop_y = ALIGN_EVEN(crop_y);
        crop_w = ALIGN_EVEN(crop_w);
        crop_h = ALIGN_EVEN(crop_h);

        if (crop_en) {
            u32 ch1_offset;

            addr0 += crop_x + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;

            ch1_offset = (crop_x >> 1)
                     + (crop_y >> 1) * stride1;

            addr1 += ch1_offset;
            addr2 += ch1_offset;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h >> 1;
        break;
    case MPP_FMT_NV12:
    case MPP_FMT_NV21:
        channel_num = 2;
        scaler_en = 1;
        in_w = ALIGN_EVEN(in_w);
        in_h = ALIGN_EVEN(in_h);
        crop_x = ALIGN_EVEN(crop_x);
        crop_y = ALIGN_EVEN(crop_y);
        crop_w = ALIGN_EVEN(crop_w);
        crop_h = ALIGN_EVEN(crop_h);

        if (crop_en) {
            addr0 += crop_x + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;

            addr1 += crop_x + (crop_y >> 1) * stride1;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h >> 1;

        break;
    case MPP_FMT_YUV420_64x32_TILE:
        channel_num = 2;
        scaler_en = 1;
        in_w = ALIGN_EVEN(in_w);
        in_h = ALIGN_EVEN(in_h);
        crop_x = ALIGN_EVEN(crop_x);
        crop_y = ALIGN_EVEN(crop_y);
        crop_w = ALIGN_EVEN(crop_w);
        crop_h = ALIGN_EVEN(crop_h);

        if (crop_en) {
            u32 tile_p0_x, tile_p0_y;
            u32 tile_p1_x, tile_p1_y;
            u32 offset_p0, offset_p1;

            tile_p0_x = crop_x >> 6;
            tile_p0_x_offset = crop_x & 63;
            tile_p0_y = crop_y >> 5;
            tile_p0_y_offset = crop_y & 31;

            tile_p1_x = crop_x >> 6;
            tile_p1_x_offset = crop_x & 63;
            tile_p1_y = (crop_y >> 1) >> 5;
            tile_p1_y_offset = (crop_y >> 1) & 31;

            offset_p0 = ALIGN_64B(stride0) * 32 * tile_p0_y
                    + 64 * 32 * tile_p0_x;

            offset_p1 = ALIGN_64B(stride1) * 32 * tile_p1_y
                    + 64 * 32 * tile_p1_x;

            addr0 += offset_p0;
            addr1 += offset_p1;

            in_w = crop_w;
            in_h = crop_h;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h >> 1;
        break;
    case MPP_FMT_YUV420_128x16_TILE:
        channel_num = 2;
        scaler_en = 1;
        in_w = ALIGN_EVEN(in_w);
        in_h = ALIGN_EVEN(in_h);
        crop_x = ALIGN_EVEN(crop_x);
        crop_y = ALIGN_EVEN(crop_y);
        crop_w = ALIGN_EVEN(crop_w);
        crop_h = ALIGN_EVEN(crop_h);

        if (crop_en) {
            u32 tile_p0_x, tile_p0_y;
            u32 tile_p1_x, tile_p1_y;
            u32 offset_p0, offset_p1;

            tile_p0_x = crop_x >> 7;
            tile_p0_x_offset = crop_x & 127;

            tile_p0_y = crop_y >> 4;
            tile_p0_y_offset = crop_y & 15;

            tile_p1_x = crop_x >> 7;
            tile_p1_x_offset = crop_x & 127;

            tile_p1_y = (crop_y >> 1) >> 4;
            tile_p1_y_offset = (crop_y >> 1) & 15;

            offset_p0 = ALIGN_128B(stride0) * 16 * tile_p0_y
                    + 128 * 16 * tile_p0_x;

            offset_p1 = ALIGN_128B(stride1) * 16 * tile_p1_y
                    + 128 * 16 * tile_p1_x;

            addr0 += offset_p0;
            addr1 += offset_p1;

            in_w = crop_w;
            in_h = crop_h;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h >> 1;
        break;
    case MPP_FMT_YUV400:
        channel_num = 1;
        scaler_en = 1;

        if (crop_en) {
            addr0 += crop_x + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
        }
        break;
    case MPP_FMT_YUV422P:
        channel_num = 2;
        scaler_en = 1;

        in_w = ALIGN_EVEN(in_w);
        crop_x = ALIGN_EVEN(crop_x);
        crop_w = ALIGN_EVEN(crop_w);

        if (crop_en) {
            u32 ch1_offset;

            addr0 += crop_x + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;

            ch1_offset = (crop_x >> 1) + crop_y * stride1;
            addr1 += ch1_offset;
            addr2 += ch1_offset;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h;
        break;
    case MPP_FMT_NV16:
    case MPP_FMT_NV61:
        channel_num = 2;
        scaler_en = 1;
        in_w = ALIGN_EVEN(in_w);
        crop_x = ALIGN_EVEN(crop_x);
        crop_w = ALIGN_EVEN(crop_w);

        if (crop_en) {
            u32 ch1_offset;

            addr0 += crop_x + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;

            ch1_offset = crop_x + crop_y * stride1;
            addr1 += ch1_offset;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h;
        break;
    case MPP_FMT_YUYV:
    case MPP_FMT_YVYU:
    case MPP_FMT_UYVY:
    case MPP_FMT_VYUY:
        channel_num = 2;
        scaler_en = 1;
        in_w = ALIGN_EVEN(in_w);
        crop_x = ALIGN_EVEN(crop_x);
        crop_w = ALIGN_EVEN(crop_w);

        if (crop_en) {
            addr0 += (crop_x << 1) + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h;
        break;
    case MPP_FMT_YUV422_64x32_TILE:
        channel_num = 2;
        scaler_en = 1;
        in_w = ALIGN_EVEN(in_w);
        crop_x = ALIGN_EVEN(crop_x);
        crop_w = ALIGN_EVEN(crop_w);

        if (crop_en) {
            u32 tile_p0_x, tile_p0_y;
            u32 tile_p1_x, tile_p1_y;
            u32 offset_p0, offset_p1;

            tile_p0_x = crop_x >> 6;
            tile_p0_x_offset = crop_x & 63;
            tile_p0_y = crop_y >> 5;
            tile_p0_y_offset = crop_y & 31;

            tile_p1_x = crop_x >> 6;
            tile_p1_x_offset = crop_x & 63;
            tile_p1_y = crop_y >> 5;
            tile_p1_y_offset = crop_y & 31;

            offset_p0 = ALIGN_64B(stride0) * 32 * tile_p0_y
                    + 64 * 32 * tile_p0_x;

            offset_p1 = ALIGN_64B(stride1) * 32 * tile_p1_y
                    + 64 * 32 * tile_p1_x;

            addr0 += offset_p0;
            addr1 += offset_p1;
            in_w = crop_w;
            in_h = crop_h;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h;
        break;
    case MPP_FMT_YUV422_128x16_TILE:
        channel_num = 2;
        scaler_en = 1;
        in_w = ALIGN_EVEN(in_w);
        crop_x = ALIGN_EVEN(crop_x);
        crop_w = ALIGN_EVEN(crop_w);

        if (crop_en) {
            u32 tile_p0_x, tile_p0_y;
            u32 tile_p1_x, tile_p1_y;
            u32 offset_p0, offset_p1;

            tile_p0_x = crop_x >> 7;
            tile_p0_x_offset = crop_x & 127;

            tile_p0_y = crop_y >> 4;
            tile_p0_y_offset = crop_y & 15;

            tile_p1_x = crop_x >> 7;
            tile_p1_x_offset = crop_x & 127;

            tile_p1_y = crop_y >> 4;
            tile_p1_y_offset = crop_y & 15;

            offset_p0 = ALIGN_128B(stride0) * 16 * tile_p0_y
                    + 128 * 16 * tile_p0_x;

            offset_p1 = ALIGN_128B(stride1) * 16 * tile_p1_y
                    + 128 * 16 * tile_p1_x;

            addr0 += offset_p0;
            addr1 += offset_p1;
            in_w = crop_w;
            in_h = crop_h;
        }

        in_w_ch1 = in_w >> 1;
        in_h_ch1 = in_h;
        break;
    case MPP_FMT_YUV444P:
        channel_num = 2;
        scaler_en = 0;

        if (crop_en) {
            u32 ch1_offset;

            addr0 += crop_x + crop_y * stride0;
            in_w = crop_w;
            in_h = crop_h;
            ch1_offset = crop_x + crop_y * stride1;
            addr1 += ch1_offset;
            addr2 += ch1_offset;
        }

        break;
    default:
        pr_err("invalid video layer format: %d\n", format);

        return -EINVAL;
    }

    de_set_video_layer_info(comp->regs, in_w, in_h, format,
                stride0, stride1, addr0, addr1, addr2,
                x_offset, y_offset);

    de_set_video_layer_tile_offset(comp->regs,
                       tile_p0_x_offset, tile_p0_y_offset,
                       tile_p1_x_offset, tile_p1_y_offset);

    if (scaler_en) {
        if (need_update_csc(comp, color_space)) {
            struct aicfb_disp_prop *disp_prop = &comp->disp_prop;

            de_update_csc(comp, disp_prop, color_space);
        }

        de_set_scaler0_channel(comp->regs, in_w, in_h,
                       scaler_w, scaler_h, 0);

        if (channel_num == 2) {
            de_set_scaler0_channel(comp->regs,
                           in_w_ch1, in_h_ch1,
                           scaler_w, scaler_h, 1);
        }

        de_scaler0_enable(comp->regs, 1);
    } else {
        de_scaler0_enable(comp->regs, 0);
    }

    de_video_layer_enable(comp->regs, 1);

    return 0;
}

static inline int ui_rect_disable(struct aic_de_comp *comp,
                  u32 layer_id, u32 rect_id)
{
    de_ui_layer_rect_enable(comp->regs, rect_id, 0);
    if (is_all_rect_win_disabled(comp, layer_id))
        de_ui_layer_enable(comp->regs, 0);
    return 0;
}

static int update_one_layer_config(struct aic_de_comp *comp,
                   struct aicfb_layer_data *layer_data)
{
    u32 index;
    int ret;

    if (!is_valid_layer_and_rect_id(comp, layer_data->layer_id,
                       layer_data->rect_id)) {
        pr_err("%s() - layer_id %d or rect_id %d is invalid\n",
            __func__, layer_data->layer_id, layer_data->rect_id);
        return -EINVAL;
    }

    if (layer_data->enable == 0) {
        index = (layer_data->layer_id << RECT_NUM_SHIFT)
            + layer_data->rect_id;
        comp->layers[index].enable = 0;

        if (is_ui_layer(comp, layer_data->layer_id)) {
            ui_rect_disable(comp, layer_data->layer_id,
                    layer_data->rect_id);
        } else {
            de_video_layer_enable(comp->regs, 0);
        }
        return 0;
    }

    if (is_ui_layer(comp, layer_data->layer_id)) {
        index = (layer_data->layer_id << RECT_NUM_SHIFT)
            + layer_data->rect_id;

        if (!is_valid_ui_rect_size(comp, layer_data))
            return -EINVAL;

        ret = config_ui_layer_rect(comp, layer_data);
        if (ret != 0)
            return -EINVAL;
        else
            memcpy(&comp->layers[index], layer_data,
                   sizeof(struct aicfb_layer_data));
    } else {
        index = layer_data->layer_id << RECT_NUM_SHIFT;

        if (!is_valid_video_size(comp, layer_data)) {
            comp->layers[index].enable = 0;
            de_video_layer_enable(comp->regs, 0);
            return -EINVAL;
        }

        ret = config_video_layer(comp, layer_data);
        if (ret != 0) {
            comp->layers[index].enable = 0;
            de_video_layer_enable(comp->regs, 0);
        } else {
            memcpy(&comp->layers[index], layer_data,
                   sizeof(struct aicfb_layer_data));
        }
        return ret;
    }
    return 0;
}

static int aic_de_update_layer_config(struct aicfb_layer_data *layer_data)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();
    int ret;

    de_config_update_enable(comp->regs, 0);
    ret = update_one_layer_config(comp, layer_data);
    de_config_update_enable(comp->regs, 1);

    aic_de_release_drvdata();
    return ret;
}

static int aic_de_update_layer_config_list(struct aicfb_config_lists *list)
{
    int ret, i;
    struct aic_de_comp *comp = aic_de_request_drvdata();

    de_config_update_enable(comp->regs, 0);

    for (i = 0; i < list->num; i++) {
        if (is_ui_layer(comp, list->layers[i].layer_id)) {
            int index = (list->layers[i].layer_id << RECT_NUM_SHIFT)
                    + list->layers[i].rect_id;
            comp->layers[index].enable = 0;
        }
    }

    for (i = 0; i < list->num; i++) {
        ret = update_one_layer_config(comp, &list->layers[i]);
        if (ret)
            return ret;
    }
    de_config_update_enable(comp->regs, 1);

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_set_display_prop(struct aicfb_disp_prop *disp_prop)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    de_config_update_enable(comp->regs, 0);

    if (need_update_disp_prop(comp, disp_prop)) {
        /* get color space from video layer config */
        struct aicfb_layer_data *layer_data = &comp->layers[0];
        int color_space = MPP_BUF_COLOR_SPACE_GET(layer_data->buf.flags);

        de_update_csc(comp, disp_prop, color_space);
    }

    de_config_update_enable(comp->regs, 1);

    memcpy(&comp->disp_prop, disp_prop, sizeof(struct aicfb_disp_prop));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_get_display_prop(struct aicfb_disp_prop *disp_prop)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    memcpy(disp_prop, &comp->disp_prop, sizeof(struct aicfb_disp_prop));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_set_ccm_config(struct aicfb_ccm_config *ccm)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    de_config_update_enable(comp->regs, 0);

    if (ccm->enable) {
        de_config_ccm(comp->regs, ccm->ccm_table);
        de_ccm_ctrl(comp->regs, 1);
    } else {
        de_ccm_ctrl(comp->regs, 0);
    }

    memcpy(&comp->ccm, ccm, sizeof(struct aicfb_ccm_config));
    de_config_update_enable(comp->regs, 1);
    aic_de_release_drvdata();
    return 0;
}

static int aic_de_get_ccm_config(struct aicfb_ccm_config *ccm)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    memcpy(ccm, &comp->ccm, sizeof(struct aicfb_ccm_config));

    aic_de_release_drvdata();
    return 0;
}

static int aic_de_set_gamma_config(struct aicfb_gamma_config *gamma)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();
    int i;

    if (gamma->enable) {
        for (i = 0; i < 3; i++)
            de_config_gamma_lut(comp->regs, gamma->gamma_lut[i], i);

        de_gamma_ctrl(comp->regs, 1);
    } else {
        de_gamma_ctrl(comp->regs, 0);
    }

    memcpy(&comp->gamma, gamma, sizeof(struct aicfb_gamma_config));
    aic_de_release_drvdata();
    return 0;
}

static int aic_de_get_gamma_config(struct aicfb_gamma_config *gamma)
{
    struct aic_de_comp *comp = aic_de_request_drvdata();

    memcpy(gamma, &comp->gamma, sizeof(struct aicfb_gamma_config));

    aic_de_release_drvdata();
    return 0;
}

struct de_funcs aic_de_funcs = {
    .set_mode                 = aic_de_set_mode,
    .clk_enable               = aic_de_clk_enable,
    .clk_disable              = aic_de_clk_disable,
    .timing_enable            = aic_de_timing_enable,
    .timing_disable           = aic_de_timing_disable,
    .wait_for_vsync           = aic_de_wait_for_vsync,
    .get_alpha_config         = aic_de_get_alpha_config,
    .update_alpha_config      = aic_de_update_alpha_config,
    .get_ck_config            = aic_de_get_ck_config,
    .update_ck_config         = aic_de_update_ck_config,
    .get_layer_config         = aic_de_get_layer_config,
    .update_layer_config      = aic_de_update_layer_config,
    .update_layer_config_list = aic_de_update_layer_config_list,
    .set_display_prop         = aic_de_set_display_prop,
    .get_display_prop         = aic_de_get_display_prop,
    .set_ccm_config           = aic_de_set_ccm_config,
    .get_ccm_config           = aic_de_get_ccm_config,
    .set_gamma_config         = aic_de_set_gamma_config,
    .get_gamma_config         = aic_de_get_gamma_config,
};

static const struct aicfb_layer_num layer_num = {
    .vi_num = 1,
    .ui_num = 1,
};

static const struct aicfb_layer_capability aicfb_layer_cap[] = {
    {0, AICFB_LAYER_TYPE_VIDEO, 2048, 2048, AICFB_CAP_SCALING_FLAG},
    {1, AICFB_LAYER_TYPE_UI, 2048, 2048,
    AICFB_CAP_4_RECT_WIN_FLAG|AICFB_CAP_ALPHA_FLAG|AICFB_CAP_CK_FLAG},
};

static const struct aic_de_configs aic_de_cfg = {
    .layer_num = &layer_num,
    .cap = aicfb_layer_cap,
};

static int aic_de_probe(void)
{
    struct aic_de_comp *comp;

    comp = aicos_malloc(0, sizeof(struct aic_de_comp));
    if (!comp)
    {
        pr_err("allloc de comp failed\n");
        return -ENOMEM;
    }

    memset(comp, 0, sizeof(*comp));

    comp->regs = (void *)DE_BASE;
    comp->config = &aic_de_cfg;
    comp->vsync_event = aicos_event_create();

#ifdef AIC_DISPLAY_DITHER
    switch (AIC_DISP_OUTPUT_DEPTH)
    {
    case DITHER_RGB565:
        comp->dither.red_bitdepth = 5;
        comp->dither.gleen_bitdepth = 6;
        comp->dither.blue_bitdepth = 5;
        comp->dither.enable = 1;
        break;
    case DITHER_RGB666:
        comp->dither.red_bitdepth = 6;
        comp->dither.gleen_bitdepth = 6;
        comp->dither.blue_bitdepth = 6;
        comp->dither.enable = 1;
        break;
    default:
        pr_err("Invalid dither mode\n");
        break;
    }
#endif

    /* set display properties to default value */
    comp->disp_prop.bright = 50;
    comp->disp_prop.contrast = 50;
    comp->disp_prop.saturation = 50;
    comp->disp_prop.hue = 50;

    memset(&comp->ccm, 0x0, sizeof(struct aicfb_ccm_config));
    memset(&comp->gamma, 0x0, sizeof(struct aicfb_gamma_config));

    g_aic_de_comp = comp;

    return 0;
}

static void aic_de_remove(void)
{

}

struct platform_driver artinchip_de_driver = {
    .name = "artinchip-de",
    .component_type = AIC_DE_COM,
    .probe = aic_de_probe,
    .remove = aic_de_remove,
    .de_funcs = &aic_de_funcs,
};

