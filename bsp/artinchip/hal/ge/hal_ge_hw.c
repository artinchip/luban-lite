/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include "hal_ge_hw.h"
#include "hal_ge_reg.h"
#include "mpp_types.h"

#define CSC_COEFFS_NUM 12

static const unsigned int yuv2rgb_bt601[CSC_COEFFS_NUM] = {
    0x04a8, 0x0000, 0x0662, 0x3212,
    0x04a8, 0x1e70, 0x1cc0, 0x087a,
    0x04a8, 0x0811, 0x0000, 0x2eb4
};

static const unsigned int yuv2rgb_bt709[CSC_COEFFS_NUM] = {
    0x04a8, 0x0000, 0x0722, 0x3093,
    0x04a8, 0x1f27, 0x1ddf, 0x04ce,
    0x04a8, 0x0873, 0x0000, 0x2df2
};

static const unsigned int yuv2rgb_bt601_full[CSC_COEFFS_NUM] = {
    0x0400, 0x0000, 0x059c, 0x34ca,
    0x0400, 0x1ea1, 0x1d26, 0x0877,
    0x0400, 0x0717, 0x0000, 0x31d4
};

static const unsigned int yuv2rgb_bt709_full[CSC_COEFFS_NUM] = {
    0x0400, 0x0000, 0x064d, 0x3368,
    0x0400, 0x1f41, 0x1e22, 0x053e,
    0x0400, 0x076c, 0x0000, 0x3129
};

static const int rgb2yuv_bt601[CSC_COEFFS_NUM] = {
    66,  129, 25, 16,
    -38, -74, 112, 128,
    112, -94, -18, 128
};

static const int rgb2yuv_bt709[CSC_COEFFS_NUM] = {
    47, 157, 16, 16,
    -26, -87, 112, 128,
    112, -102, -10, 128
};

static const int rgb2yuv_bt601_full[CSC_COEFFS_NUM] = {
    77, 150, 29, 0,
    -42, -84, 128, 128,
    128, -106, -20, 128
};

static const int rgb2yuv_bt709_full[CSC_COEFFS_NUM] = {
    54, 183, 18, 0,
    -28, -98, 128, 128,
    128, -115, -11, 128
};

u32 ge_get_version_id(unsigned long base_addr)
{
    return readl(base_addr + GE_VERSION_ID);
}

void ge_config_src_ctrl(unsigned long base_addr,
            u32 global_alpha, u32 alpha_mode,
            u32 premul_en, u32 scan_order,
            u32 func_select, u32 fmt, u32 v_flip,
            u32 h_flip, u32 rot0_ctrl,
            u32 source_mode, u32 csc0_en)
{
    u32 value = SRC_SURFACE_G_ALPHA_MODE(global_alpha) |
            SRC_SURFACE_ALPHA_MODE(alpha_mode) |
            SRC_SURFACE_P_MUL(premul_en) |
            SRC_SURFACE_SCAN_ORDER_MODE(scan_order) |
            SRC_SURFACE_FUNC_SELECT(func_select) |
            SRC_SURFACE_FORMAT(fmt) |
            SRC_SURFACE_V_FLIP_EN(v_flip) |
            SRC_SURFACE_H_FLIP_EN(h_flip) |
            SRC_SURFACE_ROT0_CTRL(rot0_ctrl) |
            SRC_SURFACE_SOURCE_MODE(source_mode) |
            SRC_SURFACE_CSC0_EN(csc0_en) |
            SRC_SURFACE_EN;

    writel(value, base_addr + SRC_SURFACE_CTRL);
}

void ge_config_src_simple(unsigned long base_addr,
              u32 global_alpha, u32 alpha_mode,
              u32 premul_en, u32 func_select,
              u32 fmt, u32 source_mode)
{
    u32 value = SRC_SURFACE_G_ALPHA_MODE(global_alpha) |
            SRC_SURFACE_ALPHA_MODE(alpha_mode) |
            SRC_SURFACE_P_MUL(premul_en) |
            SRC_SURFACE_FUNC_SELECT(func_select) |
            SRC_SURFACE_FORMAT(fmt) |
            SRC_SURFACE_SOURCE_MODE(source_mode) |
            SRC_SURFACE_EN;

    writel(value, base_addr + SRC_SURFACE_CTRL);
}

void ge_config_output_ctrl(unsigned long base_addr,
               u32 premul, int fmt,
               u32 dither_en, u32 csc2_en)
{
    u32 value = OUTPUT_P_MUL(premul) |
            OUTPUT_FORMAT(fmt) |
            DITHER_EN(dither_en) |
            OUTPUT_CSC2_EN(csc2_en);

    writel(value, base_addr + OUTPUT_CTRL);
}

void ge_dst_enable(unsigned long base_addr, u32 global_alpha,
           u32 alpha_mode, int fmt, int csc1_en)
{
    u32 value = DST_SURFACE_G_ALPHA_MODE(global_alpha) |
            DST_SURFACE_ALPHA_MODE(alpha_mode) |
            DST_SURFACE_FORMAT(fmt) |
            DST_SURFACE_CSC1_EN(csc1_en) |
            DST_SURFACE_EN;

    writel(value, base_addr + DST_SURFACE_CTRL);
}

void ge_dst_disable(unsigned long base_addr)
{
    writel(0, base_addr + DST_SURFACE_CTRL);
}

void ge_config_fill_gradient(unsigned long base_addr,
                 int width, int height,
                 u32 start_color, u32 end_color,
                 u32 direction)
{
    int a_step, r_step, g_step, b_step;
    int length;

    unsigned char start_a = (unsigned char)((start_color >> 24) & 0xff);
    unsigned char start_r = (unsigned char)((start_color >> 16) & 0xff);
    unsigned char start_g = (unsigned char)((start_color >> 8) & 0xff);
    unsigned char start_b = (unsigned char)(start_color & 0xff);

    unsigned char end_a = (unsigned char)((end_color >> 24) & 0xff);
    unsigned char end_r = (unsigned char)((end_color >> 16) & 0xff);
    unsigned char end_g = (unsigned char)((end_color >> 8) & 0xff);
    unsigned char end_b = (unsigned char)(end_color & 0xff);

    if (direction == 0)
        length = width;
    else
        length = height;

    a_step = length > 1 ?
         ((end_a - start_a) << 16) / (length - 1) : 0;
    r_step = length > 1 ?
         ((end_r - start_r) << 16) / (length - 1) : 0;
    g_step = length > 1 ?
         ((end_g - start_g) << 16) / (length - 1) : 0;
    b_step = length > 1 ?
         ((end_b - start_b) << 16) / (length - 1) : 0;

    writel(start_color, base_addr + SRC_FILL_COLOER);
    writel(SRC_GRADIENT_STEP_SET(a_step),
           base_addr + SRC_GRADIENT_A_STEP);
    writel(SRC_GRADIENT_STEP_SET(r_step),
           base_addr + SRC_GRADIENT_R_STEP);
    writel(SRC_GRADIENT_STEP_SET(g_step),
           base_addr + SRC_GRADIENT_G_STEP);
    writel(SRC_GRADIENT_STEP_SET(b_step),
           base_addr + SRC_GRADIENT_B_STEP);
}

void ge_config_fillrect(unsigned long base_addr, u32 fill_color)
{
    writel(fill_color, base_addr + SRC_FILL_COLOER);
}

void ge_config_rot1(unsigned long base_addr,
            int angle_sin, int angle_cos,
            int src_center_x, int src_center_y,
            int dst_center_x, int dst_center_y)
{
    writel(SRC_ROT1_DEGREE_SET(angle_sin, angle_cos),
           base_addr + SRC_ROT1_DEGREE);

    writel(SRC_ROT1_CENTER_SET(src_center_x, src_center_y),
           base_addr + SRC_ROT1_CENTER);

    writel(DST_ROT1_CENTER_SET(dst_center_x, dst_center_y),
           base_addr + DST_ROT1_CENTER);
}

void ge_config_shear(unsigned long base_addr,
             int offset_x, int offset_y,
             int angle_tan)
{
    writel(SRC_SHEAR_SET(angle_tan),
           base_addr + SRC_SHEAR_DEGREE);

    writel(DST_SHEAR_OFFSET_SET(offset_x, offset_y),
           base_addr + DST_SHEAR_OFFSET);
}

void ge_config_color_key(unsigned long base_addr, u32 ck_value)
{
    writel(ck_value, base_addr + COLORKEY_MATCH_COLOR);
}

void ge_config_dither(unsigned long base_addr, u32 dither_addr)
{
    writel(dither_addr, base_addr + DITHER_BGN_ADDR);
}

void ge_config_blend(unsigned long base_addr,
             u32 src_de_premul, u32 dst_de_premul,
             u32 alpha_ctrl, u32 src_alpha_coef,
             u32 dst_alpha_coef, u32 ck_en,
             u32 alpha_en)
{
    u32 value;

    value = SRC_DE_P_MUL(src_de_premul) |
        DST_DE_P_MUL(dst_de_premul) |
        OUTPUT_ALPHA_CTRL(alpha_ctrl) |
        SRC_ALPHA_COEF(src_alpha_coef) |
        DST_ALPHA_COEF(dst_alpha_coef) |
        CK_EN(ck_en) |
        ALPHA_BLEND_EN(alpha_en);

    writel(value, base_addr + BLENDING_CTRL);
}

/**
 *@ channel: scaler channel
 * 0: y channel/argb channel
 * 1: c channel
 */
void ge_set_scaler0(unsigned long base_addr,
            u32 input_w, u32 input_h,
            u32 output_w, u32 output_h,
            int dx, int dy,
            int h_phase, int v_phase,
            u32 channel)
{
    writel(SCALER0_INPUT_SIZE_SET(input_w, input_h),
           base_addr + SCALER0_INPUT_SIZE(channel));
    writel(SCALER0_OUTPUT_SIZE_SET(output_w, output_h),
           base_addr + SCALER0_OUTPUT_SIZE(channel));
    writel(SCALER0_H_INIT_PHASE_SET(h_phase),
           base_addr + SCALER0_H_INIT_PHASE(channel));
    writel(SCALER0_H_RATIO_SET(dx),
           base_addr + SCALER0_H_RATIO(channel));
    writel(SCALER0_V_INIT_PHASE_SET(v_phase),
           base_addr + SCALER0_V_INIT_PHASE(channel));
    writel(SCALER0_V_RATIO_SET(dy),
           base_addr + SCALER0_V_RATIO(channel));
}

/**
 *@ enable
 * 0: disable scaler
 * 1: enable scaler
 */
void ge_scaler0_enable(unsigned long base_addr, u32 enable)
{
    writel(enable, base_addr + SCALER0_CTRL);
}

void ge_set_csc_coefs(unsigned long base_addr, int color_space, u32 csc)
{
    const u32 *coefs;
    int i;

    switch (color_space) {
    case MPP_COLOR_SPACE_BT601:
        coefs = yuv2rgb_bt601;
        break;
    case MPP_COLOR_SPACE_BT709:
        coefs = yuv2rgb_bt709;
        break;
    case MPP_COLOR_SPACE_BT601_FULL_RANGE:
        coefs = yuv2rgb_bt601_full;
        break;
    case MPP_COLOR_SPACE_BT709_FULL_RANGE:
        coefs = yuv2rgb_bt709_full;
        break;
    default:
        coefs = yuv2rgb_bt601;
        break;
    }

    if (csc == 0) {
        for (i = 0; i < CSC_COEFFS_NUM; i++)
            writel(coefs[i], base_addr + CSC0_COEF(i));
    } else if (csc == 1) {
        for (i = 0; i < CSC_COEFFS_NUM; i++)
            writel(coefs[i], base_addr + CSC1_COEF(i));
    }
}

void ge_set_csc2_coefs(unsigned long base_addr, int color_space)
{
    const int *coefs;
    int i;

    switch (color_space) {
    case MPP_COLOR_SPACE_BT601:
        coefs = rgb2yuv_bt601;
        break;
    case MPP_COLOR_SPACE_BT709:
        coefs = rgb2yuv_bt709;
        break;
    case MPP_COLOR_SPACE_BT601_FULL_RANGE:
        coefs = rgb2yuv_bt601_full;
        break;
    case MPP_COLOR_SPACE_BT709_FULL_RANGE:
        coefs = rgb2yuv_bt709_full;
        break;
    default:
        coefs = rgb2yuv_bt601;
        break;
    }

    for (i = 0; i < CSC_COEFFS_NUM; i++) {
        writel(CSC2_COEF_SET(coefs[i]),
               base_addr + CSC2_COEF(i));
    }
}

void ge_set_src_info(unsigned long base_addr, u32 w, u32 h,
             u32 stride0, u32 stride1, u32 addr[])
{
    writel(SRC_INPUT_SIZE_SET(w, h),
           base_addr + SRC_SURFACE_INPUT_SIZE);

    writel(SRC_STRIDE_SET(stride0, stride1),
           base_addr + SRC_SURFACE_STRIDE);

    writel(addr[0], base_addr + SRC_SURFACE_ADDR0);
    writel(addr[1], base_addr + SRC_SURFACE_ADDR1);
    writel(addr[2], base_addr + SRC_SURFACE_ADDR2);
}

void ge_set_dst_info(unsigned long base_addr, u32 w, u32 h,
             u32 stride0, u32 stride1, u32 addr[])
{
    writel(DST_INPUT_SIZE_SET(w, h),
           base_addr + DST_SURFACE_INPUT_SIZE);

    writel(DST_STRIDE_SET(stride0, stride1),
           base_addr + DST_SURFACE_STRIDE);

    writel(addr[0], base_addr + DST_SURFACE_ADDR0);
    writel(addr[1], base_addr + DST_SURFACE_ADDR1);
    writel(addr[2], base_addr + DST_SURFACE_ADDR2);
}

void ge_set_output_info(unsigned long base_addr, u32 w, u32 h,
            u32 stride0, u32 stride1, u32 addr[])
{
    writel(OUTPUT_SIZE_SET(w, h),
           base_addr + OUTPUT_SIZE);

    writel(OUTPUT_STRIDE_SET(stride0, stride1),
           base_addr + OUTPUT_STRIDE);

    writel(addr[0], base_addr + OUTPUT_ADDR0);
    writel(addr[1], base_addr + OUTPUT_ADDR1);
    writel(addr[2], base_addr + OUTPUT_ADDR2);
}

void ge_set_hw_timeout_cycle(unsigned long base_addr, u32 clk_num)
{
    writel(clk_num, base_addr + HW_TIMEOUT_CYCLE);
}

u32 ge_read_hw_counter(unsigned long base_addr)
{
    return readl(base_addr + HW_COUNTER);
}

u32 ge_read_soft_reset_cycle(unsigned long base_addr)
{
    return readl(base_addr + HW_SOFT_RESET_CYCLE);
}

u32 ge_read_status(unsigned long base_addr)
{
    return readl(base_addr + GE_STATUS);
}

void ge_clear_status(unsigned long base_addr, u32 status)
{
    writel(status, base_addr + GE_STATUS);
}

void ge_enable_interrupt(unsigned long base_addr)
{
    writel(GE_CTRL_FINISH_IRQ_EN |
        GE_CTRL_HW_ERR_IRQ_EN |
        GE_CTRL_HW_TIMEOUT_IRQ_EN |
        GE_CTRL_HW_CMDQ_LENGTH_ERR_IQR_EN,
        base_addr + GE_CTRL);
}

void ge_disable_interrupt(unsigned long base_addr)
{
    writel(0, base_addr + GE_CTRL);
}

void ge_start(unsigned long base_addr)
{
    writel(GE_SW_RESET | GE_START_EN, base_addr + GE_START);
}
