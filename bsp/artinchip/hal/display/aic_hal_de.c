/*
 * Copyright (c) 2020-2023, Artinchip Technology Co., Ltd
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <aic_core.h>
#include <aic_hal.h>

#include "artinchip_fb.h"
#include "aic_hal_disp_reg_util.h"
#include "aic_hal_de.h"

#define CSC_COEFFS_NUM  12
#define CCM_COEF_NUM    12
#define GAMMA_COEF_NUM  64

static int yuv2rgb_bt601_limit[3][4] = {
    {1192, 0, 1634, -3269},
    {1192, -401, -833, 2467},
    {1192, 2066, 0, -4131}
};

static int yuv2rgb_bt709_limit[3][4] = {
    {1192, 0, 1836, -3970},
    {1192, -218, -546, 1230},
    {1192, 2163, 0, -4624}
};

static int yuv2rgb_bt601_full[3][4] = {
    {1024, 0, 1436, -2871},
    {1024, -352, -731, 2167},
    {1024, 1815, 0, -3629}
};

static int yuv2rgb_bt709_full[3][4] = {
    {1024, 0, 1613, -3225},
    {1024, -192, -479, 1342},
    {1024, 1900, 0, -3800}
};

static int rgb2yuv_bt709_full[4][4] = {
    {218, 732, 74, 0},
    {-116, -394, 512, 128 * 1024},
    {512, -464, -46, 128 * 1024},
    {0, 0, 0, 1024},
};

static int sin_table[60] = {
    -1985,    -1922,    -1859,    -1795,    -1731,
    -1665,    -1600,    -1534,    -1467,    -1400,
    -1333,    -1265,    -1197,    -1129,    -1060,
    -990,     -921,     -851,     -781,     -711,
    -640,     -570,     -499,     -428,     -356,
    -285,     -214,     -142,      -71,        0,
    71,      142,      214,      285,      356,
    428,      499,      570,      640,      711,
    781,      851,      921,      990,     1060,
    1129,     1197,     1265,     1333,     1400,
    1467,     1534,     1600,     1665,     1731,
    1795,     1859,     1922,     1985,     2047,
};

static int cos_table[60] = {
    3582,     3616,     3649,     3681,     3712,
    3741,     3770,     3797,     3823,     3848,
    3872,     3895,     3917,     3937,     3956,
    3974,     3991,     4006,     4020,     4033,
    4045,     4056,     4065,     4073,     4080,
    4086,     4090,     4093,     4095,     4096,
    4095,     4093,     4090,     4086,     4080,
    4073,     4065,     4056,     4045,     4033,
    4020,     4006,     3991,     3974,     3956,
    3937,     3917,     3895,     3872,     3848,
    3823,     3797,     3770,     3741,     3712,
    3681,     3649,     3616,     3582,     3547,
};

/*
 * Layout of factors in table:
 * [green red hight low]
 */
static int qos_cfg[4][4] = {
    [QOS_V_P0_CFG] = {
        0x0d, 0x0d, 0x20, 0x10,
    },
    [QOS_V_P1_CFG] = {
        0x0d, 0x0d, 0x20, 0x10,
    },
    [QOS_V_P2_CFG] = {
        0x0d, 0x0d, 0x20, 0x10,
    },
    [QOS_UI_CFG] = {
        0x0d, 0x0d, 0x60, 0x40,
    },
};

#define QOS_OUTSTANDING     0x1f
#define QOS_DMAR_URGENT_TH  0x40

void de_config_prefetch_line_set(void *base_addr, u32 line)
{
    reg_set_bits(base_addr + TIMING_LINE_SET,
            TIMING_LINE_SET_PREFETCH_LINE_MASK,
            TIMING_LINE_SET_PREFETCH_LINE(line));
}

void de_config_tearing_effect(void *base_addr,
            u32 mode, u32 pulse_width)
{
    reg_set_bits(base_addr + TIMING_CTRL,
             TIMING_DE_MODE_MASK,
             TIMING_DE_MODE(mode));

    reg_set_bits(base_addr + TIMING_CTRL,
             TIMING_TE_PULSE_WIDTH_MASK,
             TIMING_TE_PULSE_WIDTH(pulse_width));
}

void de_soft_reset_ctrl(void *base_addr, u32 enable)
{
    if (enable)
        reg_set_bit(base_addr + WB_BASE, DE_SOFT_RESET_EN);
    else
        reg_clr_bit(base_addr + WB_BASE, DE_SOFT_RESET_EN);
}

void de_config_update_enable(void *base_addr, u32 enable)
{
    reg_write(base_addr + DE_CONFIG_UPDATE, enable);
}

void de_set_dither(void *base_addr, u32 r_depth,
           u32 g_depth, u32 b_depth, u32 enable)
{
    reg_write(base_addr + OUTPUT_COLOR_DEPTH,
          OUTPUT_COLOR_DEPTH_SET(r_depth, g_depth, b_depth));

    if (enable) {
        reg_set_bit(base_addr + DITHER_RAND_SEED, DE_RAND_DITHER_EN);
        reg_set_bit(base_addr + DE_CTRL, DE_CTRL_DITHER_EN);
    } else {
        reg_clr_bit(base_addr + DE_CTRL, DE_CTRL_DITHER_EN);
    }
}

void de_set_mode(void *base_addr, u32 mode)
{
    reg_set_bits(base_addr + TIMING_CTRL,
            TIMING_DE_MODE_MASK, TIMING_DE_MODE(mode));
}

void de_set_te_pulse_width(void *base_addr, u32 width)
{
    reg_set_bits(base_addr + TIMING_CTRL,
            TIMING_TE_PULSE_WIDTH_MASK, TIMING_TE_PULSE_WIDTH(width));
}

u32 de_set_te_pinmux(const char *name)
{
#if defined(AIC_DE_DRV_V10) || defined(AIC_DE_V10)
    char *pins[] = { "PC.6", "PD.2", "PF.15" };
    unsigned int func[] = { 4, 4, 2 };
#elif defined(AIC_DE_DRV_V11) || defined(AIC_DE_V11)
    char *pins[] = { "PA.1", "PC.6" };
    unsigned int func[] = { 8, 6 };
#elif defined(AIC_DE_DRV_V12) || defined(AIC_DE_V12)
char *pins[] = { "PA.1", "PC.6" };
unsigned int func[] = { 8, 6 };
#endif
    unsigned int g, p, i;
    long pin = 0;

    for (i = 0; i < ARRAY_SIZE(pins); i++) {
        if (strncasecmp(name, pins[i], strlen(pins[i])) == 0) {
            pin = hal_gpio_name2pin(pins[i]);
            if (pin < 0)
                return -1;

            g = GPIO_GROUP(pin);
            p = GPIO_GROUP_PIN(pin);
            hal_gpio_set_func(g, p, func[i]);
            hal_gpio_set_bias_pull(g, p, PIN_PULL_DIS);
            hal_gpio_set_drive_strength(g, p, 3);
            return 0;
        }
    }
    return -1;
}

void de_qos_config(void *base_addr, int *value)
{
    reg_set_bits(base_addr,
        DMAR_QOS_GREEN_MASK      | DMAR_QOS_HIGH_MASK |
        DMAR_QOS_RED_MASK        | DMAR_QOS_LOW_MASK,
        DMAR_QOS_GREEN(value[0]) | DMAR_QOS_HIGH(value[2]) |
        DMAR_QOS_RED(value[1])   | DMAR_QOS_LOW(value[3]));
}

void de_qos_urgent_config(void *base_addr,
                    u32 only_active_en, u32 urgent_en,
                    u32 urgent_th, u32 outstanding)
{
    if (urgent_en)
        reg_set_bit(base_addr + QOS_V_URGENT, DMAR_URGENT_EN);

#ifdef AIC_DE_DRV_V11
    if (only_active_en)
        reg_set_bit(base_addr + QOS_V_URGENT, ONLY_ACTIVE_REGION_EN);

    reg_set_bits(base_addr + QOS_V_URGENT,
                DMAR_URGENT_TH_MASK,
                DMAR_URGENT_TH(urgent_th));
#else
    reg_set_bits(base_addr + QOS_V_URGENT,
        OUTSTANDING_MASK         | DMAR_URGENT_TH_MASK,
        OUTSTANDING(outstanding) | DMAR_URGENT_TH(urgent_th));
#endif
}

void de_set_qos(void *base_addr)
{
    void *base_reg;
    int i;

    de_qos_urgent_config(base_addr, 1, 1,
            QOS_DMAR_URGENT_TH, QOS_OUTSTANDING);

    for (i = 0; i < 4; i++) {
        base_reg = base_addr + QOS_V_P0 + i * 0x4;

        de_qos_config(base_reg, qos_cfg[i]);
    }
}

void de_colorbar_ctrl(void *base_addr, u32 enable)
{
    if (enable)
        reg_set_bit(base_addr + DE_MODE_SELECT,
                 DE_MODE_SELECT_COLOR_BAR);
    else
        reg_clr_bit(base_addr + DE_MODE_SELECT,
                 DE_MODE_SELECT_COLOR_BAR);
}

u32 de_get_version_id(void *base_addr)
{
    return reg_read(base_addr + DE_VERSION_ID);
}

void de_set_video_layer_info(void *base_addr, u32 w, u32 h,
                 enum mpp_pixel_format format,
                 u32 stride0, u32 stride1,
                 u32 addr0, u32 addr1, u32 addr2,
                 u32 x_offset, u32 y_offset)
{
    reg_set_bits(base_addr + VIDEO_LAYER_CTRL,
             VIDEO_LAYER_CTRL_INPUT_FORMAT_MASK,
             VIDEO_LAYER_CTRL_INPUT_FORMAT(format));

    reg_write(base_addr + VIDEO_LAYER_INPUT_SIZE,
          VIDEO_LAYER_INPUT_SIZE_SET(w, h));
    reg_write(base_addr + VIDEO_LAYER_STRIDE,
          VIDEO_LAYER_STRIDE_SET(stride0, stride1));
    reg_write(base_addr + VIDEO_LAYER_ADDR0, addr0);
    reg_write(base_addr + VIDEO_LAYER_ADDR1, addr1);
    reg_write(base_addr + VIDEO_LAYER_ADDR2, addr2);
    reg_write(base_addr + VIDEO_LAYER_OFFSET,
          VIDEO_LAYER_OFFSET_SET(x_offset, y_offset));
}

void de_set_video_layer_tile_offset(void *base_addr,
                    u32 p0_x_offset, u32 p0_y_offset,
                    u32 p1_x_offset, u32 p1_y_offset)
{
    reg_write(base_addr + VIDEO_LAYER_TILE_OFFSET0,
          VIDEO_LAYER_TILE_OFFSET0_SET(p0_x_offset, p0_y_offset));

    reg_write(base_addr + VIDEO_LAYER_TILE_OFFSET1,
          VIDEO_LAYER_TILE_OFFSET1_SET(p1_x_offset, p1_y_offset));
}

void de_video_layer_enable(void *base_addr, u32 enable)
{
    if (enable)
        reg_set_bit(base_addr + VIDEO_LAYER_CTRL, VIDEO_LAYER_CTRL_EN);
    else
        reg_clr_bit(base_addr + VIDEO_LAYER_CTRL, VIDEO_LAYER_CTRL_EN);
}

void de_set_csc0_coefs(void *base_addr, int color_space)
{
    const int *coefs;
    int i;

    switch (color_space) {
    case MPP_COLOR_SPACE_BT601:
        coefs = &yuv2rgb_bt601_limit[0][0];
        break;
    case MPP_COLOR_SPACE_BT709:
        coefs = &yuv2rgb_bt709_limit[0][0];
        break;
    case MPP_COLOR_SPACE_BT601_FULL_RANGE:
        coefs = &yuv2rgb_bt601_full[0][0];
        break;
    case MPP_COLOR_SPACE_BT709_FULL_RANGE:
        coefs = &yuv2rgb_bt709_full[0][0];
        break;
    default:
        coefs = &yuv2rgb_bt601_limit[0][0];
        break;
    }

    for (i = 0; i < CSC_COEFFS_NUM; i++) {
        if (i == 3 || i == 7 || i == 11)
            reg_write(base_addr + VIDEO_LAYER_CSC0_COEF(i),
                  CSC0_COEF_OFFSET_SET(coefs[i]));
        else
            reg_write(base_addr + VIDEO_LAYER_CSC0_COEF(i),
                  CSC0_COEF_SET(coefs[i]));
    }
}

static int get_hsbc_coefs(int bright, int contrast,
              int sat, int hue, int coef[][4])
{
    int sinv = sin_table[hue + 29];
    int cosv = cos_table[hue + 29];

    sat = sat + 128;
    contrast = contrast + 128;

    coef[0][0] = contrast << 3;
    coef[0][1] = 0;
    coef[0][2] = 0;
    coef[0][3] = ((bright << 3) << 8) + ((1024 - (contrast << 3)) << 4);

    coef[1][0] = 0;
    coef[1][1] = (contrast * sat * cosv) >> 16;
    coef[1][2] = (contrast * sat * sinv) >> 16;
    coef[1][3] = (1024 - (coef[1][1] + coef[1][2])) << 7;

    coef[2][0] = 0;
    coef[2][1] = (-contrast * sat * sinv) >> 16;
    coef[2][2] = (contrast * sat * cosv) >> 16;
    coef[2][3] = (1024 - (coef[2][1] + coef[2][2])) << 7;

    coef[3][0] = 0;
    coef[3][1] = 0;
    coef[3][2] = 0;
    coef[3][3] = 1024;

    return 0;
}

static void matrix_4x4_multi(int a[][4], int b[][4], int out[][4])
{
    int i, j;
    int c;

    for (j = 0; j < 4; j++) {
        for (i = 0; i < 4; i++) {
            for (c = 0; c < 4; c++)
                out[j][i] += (a[j][c] * b[c][i]) >> 10;
        }
    }
}

static int get_csc_coefs_for_hsbc(int is_rgb, int color_space,
                  int bright, int contrast,
                  int saturation, int hue,
                  u32 *csc_coef)
{
    int(*y2r)[4];
    int(*r2y)[4];
    int(*cur_coef)[4];
    int i, j, c, k;
    int hsv_coef[4][4] = {0};
    int out_coef[4][4] = {0};

    bright = bright > 100 ? 100 : bright;
    contrast = contrast > 100 ? 100 : contrast;
    saturation = saturation > 100 ? 100 : saturation;
    hue = hue > 100 ? 100 : hue;

    bright = bright * 128 / 100 - 64;
    contrast = contrast * 180 / 100 - 90;
    saturation = saturation * 128 / 100 - 64;
    hue = hue * 58 / 100 - 29;

    if (color_space == 0)
        y2r = yuv2rgb_bt601_limit;
    else if (color_space == 1)
        y2r = yuv2rgb_bt709_limit;
    else if (color_space == 2)
        y2r = yuv2rgb_bt601_full;
    else if (color_space == 3)
        y2r = yuv2rgb_bt709_full;
    else
        y2r = yuv2rgb_bt601_limit;

    get_hsbc_coefs(bright, contrast, saturation, hue, hsv_coef);

    if (is_rgb) {
        r2y = rgb2yuv_bt709_full;
        matrix_4x4_multi(hsv_coef, r2y, out_coef);
        cur_coef = out_coef;
    } else {
        cur_coef = hsv_coef;
    }

    k = 0;
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 4; i++) {
            unsigned int value;
            int accum;

            accum = 0;
            for (c = 0; c < 4; c++) {
                int cur_value =  y2r[j][c];

                if (c == 3)
                    cur_value = cur_value << 6;

                accum += cur_value * cur_coef[c][i];
            }

            if (i == 3) {
                accum = accum >> 16;
                accum = accum > 8091 ? 8091 : accum;
                accum = accum < -8092 ? -8092 : accum;
                value = (unsigned int)accum;
                value = CSC0_COEF_OFFSET_SET(value);
            } else {
                accum = accum >> 10;
                accum = accum > 4095 ? 4095 : accum;
                accum = accum < -4096 ? -4096 : accum;
                value = (unsigned int)accum;
                value = CSC0_COEF_SET(value);
            }
            csc_coef[k++] = value;
        }
    }

    return 0;
}

int de_set_hsbc_with_csc_coefs(void *base_addr, int color_space,
                   int bright, int contrast,
                   int saturation, int hue)
{
    int i;
    u32 csc_coef[CSC_COEFFS_NUM];

    get_csc_coefs_for_hsbc(0, color_space, bright, contrast, saturation,
                   hue, csc_coef);

    for (i = 0; i < CSC_COEFFS_NUM; i++)
        reg_write(base_addr + VIDEO_LAYER_CSC0_COEF(i), csc_coef[i]);

    return 0;
}

int get_rgb_hsbc_csc_coefs(int bright, int contrast, int saturation, int hue,
               u32 *csc_coef)
{
    get_csc_coefs_for_hsbc(1, 3, bright, contrast, saturation,
                   hue, csc_coef);

    return 0;
}

void de_set_ui_layer_size(void *base_addr, u32 w, u32 h,
              u32 x_offset, u32 y_offset)
{
    reg_write(base_addr + UI_LAYER_SIZE, UI_LAYER_SIZE_SET(w, h));
    reg_write(base_addr + UI_LAYER_OFFSET,
          UI_LAYER_OFFSET_SET(x_offset, y_offset));
}

void de_ui_alpha_blending_enable(void *base_addr, u32 g_alpha,
                 u32 alpha_mode, u32 enable)
{
    if (enable)
        reg_set_bits(base_addr + UI_LAYER_CTRL,
                 UI_LAYER_CTRL_G_ALPHA_MASK |
                 UI_LAYER_CTRL_ALPHA_MODE_MASK |
                 UI_LAYER_CTRL_ALPHA_EN,
                 UI_LAYER_CTRL_G_ALPHA(g_alpha) |
                 UI_LAYER_CTRL_ALPHA_MODE(alpha_mode) |
                 UI_LAYER_CTRL_ALPHA_EN);
    else
        reg_clr_bit(base_addr + UI_LAYER_CTRL, UI_LAYER_CTRL_ALPHA_EN);
}

void de_set_ui_layer_format(void *base_addr,
                enum mpp_pixel_format format)
{
    reg_set_bits(base_addr + UI_LAYER_CTRL,
             UI_LAYER_CTRL_INPUT_FORMAT_MASK,
             UI_LAYER_CTRL_INPUT_FORMAT(format));
}

void de_ui_layer_color_key_enable(void *base_addr, u32 color_key,
                  u32 ck_en)
{
    if (ck_en) {
        reg_write(base_addr + UI_LAYER_COLOER_KEY,
              UI_LAYER_COLOER_KEY_SET(color_key));
        reg_set_bit(base_addr + UI_LAYER_CTRL,
                 UI_LAYER_CTRL_COLOR_KEY_EN);
    } else {
        reg_clr_bit(base_addr + UI_LAYER_CTRL,
                 UI_LAYER_CTRL_COLOR_KEY_EN);
    }
}

void de_ui_layer_set_rect(void *base_addr, u32 w, u32 h,
              u32 x_offset, u32 y_offset,
              u32 stride, u32 addr, u32 id)
{
    reg_write(base_addr + UI_RECT_INPUT_SIZE(id),
          UI_RECT_INPUT_SIZE_SET(w, h));
    reg_write(base_addr + UI_RECT_OFFSET(id),
          UI_RECT_OFFSET_SET(x_offset, y_offset));
    reg_write(base_addr + UI_RECT_STRIDE(id),
          UI_RECT_STRIDE_SET(stride));
    reg_write(base_addr + UI_RECT_ADDR(id), addr);
}

void de_ui_layer_enable(void *base_addr, u32 enable)
{
    if (enable)
        reg_set_bit(base_addr + UI_LAYER_CTRL, UI_LAYER_CTRL_EN);
    else
        reg_clr_bit(base_addr + UI_LAYER_CTRL, UI_LAYER_CTRL_EN);
}

void de_ui_layer_rect_enable(void *base_addr, u32 index, u32 enable)
{
    if (enable)
        reg_set_bit(base_addr + UI_LAYER_RECT_CTRL,
                 UI_LAYER_RECT_CTRL_EN(index));
    else
        reg_clr_bit(base_addr + UI_LAYER_RECT_CTRL,
                 UI_LAYER_RECT_CTRL_EN(index));
}

/**
 *@ channel: scaler channel
 * 0: y channel
 * 1: c channel
 */
void de_set_scaler0_channel(void *base_addr, u32 input_w, u32 input_h,
                u32 output_w, u32 output_h, u32 channel)
{
    u32 dx, dy, h_phase, v_phase;

    dx = (input_w << 16) / output_w;
    dy = (input_h << 16) / output_h;

    h_phase = (dx >= 65536) ? ((dx >> 1) - 32768) : (dx >> 1);
    v_phase = (dy >= 65536) ? ((dy >> 1) - 32768) : (dy >> 1);

    reg_write(base_addr + SCALER0_INPUT_SIZE(channel),
          SCALER0_INPUT_SIZE_SET(input_w, input_h));
    reg_write(base_addr + SCALER0_OUTPUT_SIZE(channel),
          SCALER0_OUTPUT_SIZE_SET(output_w, output_h));
    reg_write(base_addr + SCALER0_H_INIT_PHASE(channel),
          SCALER0_H_INIT_PHASE_SET(h_phase));
    reg_write(base_addr + SCALER0_H_RATIO(channel),
          SCALER0_H_RATIO_SET(dx));
    reg_write(base_addr + SCALER0_V_INIT_PHASE(channel),
          SCALER0_V_INIT_PHASE_SET(v_phase));
    reg_write(base_addr + SCALER0_V_RATIO(channel),
          SCALER0_V_RATIO_SET(dy));
}

void de_scaler0_enable(void *base_addr, u32 enable)
{
    if (enable)
        reg_set_bit(base_addr + SCALER0_CTRL, SCALER0_CTRL_EN);
    else
        reg_clr_bit(base_addr + SCALER0_CTRL, SCALER0_CTRL_EN);
}

void de_timing_enable_interrupt(void *base_addr, u32 enable, u32 mask)
{
    if (enable)
        reg_set_bit(base_addr + TIMING_INIT, mask);
    else
        reg_clr_bits(base_addr + TIMING_INIT, mask);
}

u32 de_timing_interrupt_status(void *base_addr)
{
    return reg_read(base_addr + TIMING_STATUS);
}

void de_timing_interrupt_clean_status(void *base_addr, u32 status)
{
    reg_write(base_addr + TIMING_STATUS, status);
}

void de_timing_enable(void *base_addr, u32 enable)
{
    if (enable)
        reg_set_bit(base_addr + TIMING_CTRL, TIMING_CTRL_EN);
    else
        reg_clr_bit(base_addr + TIMING_CTRL, TIMING_CTRL_EN);
}

void de_config_timing(void *base_addr,
              u32 active_w, u32 active_h,
              u32 hfp, u32 hbp,
              u32 vfp, u32 vbp,
              u32 hsync, u32 vsync)
{
    reg_write(base_addr + TIMING_ACTIVE_SIZE,
          TIMING_ACTIVE_SIZE_SET(active_w, active_h));
    reg_write(base_addr + TIMING_H_PORCH, TIMING_H_PORCH_SET(hfp, hbp));
    reg_write(base_addr + TIMING_V_PORCH, TIMING_V_PORCH_SET(vfp, vbp));
    reg_write(base_addr + TIMING_SYNC_PLUSE,
          TIMING_SYNC_PLUSE_SET_H_V(hsync, vsync));
}

void de_set_blending_size(void *base_addr, u32 active_w, u32 active_h)
{
    reg_write(base_addr + BLENDING_OUTPUT_SIZE,
          BLENDING_OUTPUT_SIZE_SET(active_w, active_h));
}

void de_ccm_ctrl(void *base_addr, u32 enable)
{
    if (enable)
        reg_set_bit(base_addr + DE_CTRL, DE_CTRL_CCM_EN);
    else
        reg_clr_bit(base_addr + DE_CTRL, DE_CTRL_CCM_EN);
}

void de_gamma_ctrl(void *base_addr, u32 enable)
{
    if (enable)
        reg_set_bit(base_addr + DE_CTRL, DE_CTRL_GAMMA_EN);
    else
        reg_clr_bit(base_addr + DE_CTRL, DE_CTRL_GAMMA_EN);
}

void de_config_ccm(void *base_addr, const int *ccm_table)
{
    unsigned int i;

    for (i = 0; i < CCM_COEF_NUM; i++)
        reg_write(base_addr + CCM_COEF(i), ccm_table[i]);
}

void de_config_gamma_lut(void *base_addr, const u32 *gamma_table, int channel)
{
    int i, value;

    for (i = 0; i < GAMMA_COEF_NUM; i += 4) {
        value = (GAMMA_LUT0(gamma_table[i + 0]) & GAMMA_LUT0_MASK) |
                (GAMMA_LUT1(gamma_table[i + 1]) & GAMMA_LUT1_MASK) |
                (GAMMA_LUT2(gamma_table[i + 2]) & GAMMA_LUT2_MASK) |
                (GAMMA_LUT3(gamma_table[i + 3]) & GAMMA_LUT3_MASK);

        switch (channel) {
        case 0:
            reg_write(base_addr + GAMMA_RED_LUT(i / 4), value);
            break;
        case 1:
            reg_write(base_addr + GAMMA_GREEN_LUT(i / 4), value);
            break;
        default:
            reg_write(base_addr + GAMMA_BLUE_LUT(i / 4), value);
            break;
        }
    }
}
