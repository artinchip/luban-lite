/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "mpp_vin.h"

#include "hal_dvp.h"

static inline void dvp_writel(u32 val, int reg)
{
    writel(val, DVP_BASE + reg);
}

static inline u32 dvp_readl(int reg)
{
    return readl(DVP_BASE + reg);
}

static void aic_dvp_reg_enable(int offset, int bit, int enable)
{
    int tmp;

    tmp = dvp_readl(offset);
    tmp &= ~bit;
    if (enable)
        tmp |= bit;

    dvp_writel(tmp, offset);
}

void aich_dvp_enable(int enable)
{
    aic_dvp_reg_enable(DVP_CTL, DVP_CTL_DROP_FRAME_EN, enable);
    aic_dvp_reg_enable(DVP_CTL, DVP_CTL_EN, enable);
}

void aich_dvp_capture_start(void)
{
    dvp_writel(DVP_OUT_CTL_CAP_ON, DVP_OUT_CTL);
}

void aich_dvp_capture_stop(void)
{
    dvp_writel(0, DVP_OUT_CTL);
}

void aich_dvp_clr_fifo(void)
{
    aic_dvp_reg_enable(DVP_CTL, DVP_CTL_CLR, 1);
}

int aich_dvp_clr_int(void)
{
    int sta = dvp_readl(DVP_IRQ_STA);

    dvp_writel(sta, DVP_IRQ_STA);
    return sta;
}

void aich_dvp_enable_int(int enable)
{
    aic_dvp_reg_enable(DVP_IRQ_EN, DVP_IRQ_EN_UPDATE_DONE, enable);
    aic_dvp_reg_enable(DVP_IRQ_EN, DVP_IRQ_EN_FRAME_DONE, enable);
}

void aich_dvp_set_pol(u32 flags)
{
    u32 href_pol, pclk_pol, vref_pol, field_pol;

    href_pol = flags & MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH ?
                DVP_IN_CFG_HREF_POL_ACTIVE_HIGH : 0;
    vref_pol = flags & MEDIA_SIGNAL_VSYNC_ACTIVE_HIGH ?
                DVP_IN_CFG_VSYNC_POL_ACTIVE_HIGH : 0;
    pclk_pol = flags & MEDIA_SIGNAL_PCLK_SAMPLE_RISING ?
                DVP_IN_CFG_PCLK_POL_RISING_EDGE : 0;
    field_pol = flags & MEDIA_SIGNAL_FIELD_EVEN_HIGH ?
                DVP_IN_CFG_FILED_POL_NORMAL : 0;
    dvp_writel(href_pol | vref_pol | pclk_pol | field_pol, DVP_IN_CFG);
}

void aich_dvp_set_cfg(struct aic_dvp_config *cfg)
{
    dvp_writel(DVP_CTL_IN_FMT(cfg->input)
            | DVP_CTL_IN_SEQ(cfg->input_seq)
            | DVP_CTL_OUT_FMT(cfg->output)
            | DVP_CTL_DROP_FRAME_EN | DVP_CTL_EN, DVP_CTL);
    dvp_writel(DVP_OUT_HOR_NUM(cfg->width), DVP_OUT_HOR_SIZE);
    dvp_writel(DVP_OUT_VER_NUM(cfg->height), DVP_OUT_VER_SIZE);

    if ((cfg->stride[0] == 0) || (cfg->stride[1] == 0)) {
        hal_log_err("Invalid stride: 0x%x 0x%x\n", cfg->stride[0], cfg->stride[1]);
        return;
    }

    dvp_writel(cfg->stride[0], DVP_OUT_LINE_STRIDE0);
    dvp_writel(cfg->stride[1], DVP_OUT_LINE_STRIDE1);
}

void aich_dvp_update_buf_addr(dma_addr_t y, dma_addr_t uv)
{
    if ((y == 0) || (uv == 0)) {
        hal_log_err("Invalid DMA address: Y 0x%x, UV 0x%x\n", (u32)y, (u32)uv);
        return;
    }
    dvp_writel(y, DVP_OUT_ADDR_BUF(0));
    dvp_writel(uv, DVP_OUT_ADDR_BUF(1));
}

void aich_dvp_update_ctl(void)
{
    dvp_writel(1, DVP_OUT_UPDATE_CTL);
}

void aich_dvp_record_mode(void)
{
    dvp_writel(0x80000000, DVP_OUT_FRA_NUM);
}

void aich_dvp_qos_cfg(u32 high, u32 low, u32 inc_thd, u32 dec_thd)
{
    u32 val = DVP_QOS_CUSTOM;

    val |= (inc_thd << DVP_QOS_INC_THR_SHIFT) & DVP_QOS_INC_THR_MASK;
    val |= (dec_thd << DVP_QOS_DEC_THR_SHIFT) & DVP_QOS_DEC_THR_MASK;
    val |= (high << DVP_QOS_HIGH_SHIFT) & DVP_QOS_HIGH_MASK;
    val |= low & DVP_QOS_LOW_MASK;
    hal_log_info("DVP QoS is enable: 0x%x\n", val);
    dvp_writel(val, DVP_QOS_CFG);
}
