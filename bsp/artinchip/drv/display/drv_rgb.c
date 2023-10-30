/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <aic_hal.h>
#include <aic_hal_rgb.h>

#include "disp_com.h"

struct aic_rgb_comp
{
    void *regs;

    struct aic_panel *panel;
    u64 sclk_rate;
    u64 pll_disp_rate;
};
static struct aic_rgb_comp *g_aic_rgb_comp;

static struct aic_rgb_comp *aic_rgb_request_drvdata(void)
{
    return g_aic_rgb_comp;
}

static void aic_rgb_release_drvdata(void)
{

}

static int aic_rgb_clk_enable(void)
{
    struct aic_rgb_comp *comp = aic_rgb_request_drvdata();
    u32 pixclk = comp->panel->timings->pixelclock;

    hal_clk_set_freq(CLK_PLL_FRA2, comp->pll_disp_rate);
    hal_clk_set_rate(CLK_SCLK, comp->sclk_rate, comp->pll_disp_rate);
    hal_clk_set_rate(CLK_PIX, pixclk, comp->sclk_rate);

    hal_clk_enable(CLK_PLL_FRA2);
    hal_clk_enable(CLK_SCLK);
    hal_clk_enable(CLK_RGB);

    hal_reset_deassert(RESET_RGB);

    aic_rgb_release_drvdata();
    return 0;
}

static int aic_rgb_clk_disable(void)
{
    hal_reset_assert(RESET_RGB);

    hal_clk_disable(CLK_RGB);
    hal_clk_disable(CLK_SCLK);

    return 0;
}

static void aic_rgb_swap(void)
{
    struct aic_rgb_comp *comp = aic_rgb_request_drvdata();
    struct panel_rgb *rgb = comp->panel->rgb;

    if (rgb->data_mirror)
        reg_set_bits(comp->regs + RGB_DATA_SEQ_SEL,
            RGB_DATA_OUT_SEL_MASK, RGB_DATA_OUT_SEL(7));

    if (rgb->data_order)
        reg_write(comp->regs + RGB_DATA_SEL, rgb->data_order);

    if (rgb->clock_phase)
        reg_set_bits(comp->regs + RGB_CLK_CTL,
            CKO_PHASE_SEL_MASK,
            CKO_PHASE_SEL(rgb->clock_phase));

    aic_rgb_release_drvdata();
}

static int aic_rgb_enable(void)
{
    struct aic_rgb_comp *comp = aic_rgb_request_drvdata();
    struct panel_rgb *rgb = comp->panel->rgb;

    reg_set_bits(comp->regs + RGB_LCD_CTL,
            RGB_LCD_CTL_MODE_MASK,
            RGB_LCD_CTL_MODE(rgb->mode));

    switch (rgb->mode) {
    case PRGB:
        reg_set_bits(comp->regs + RGB_LCD_CTL,
                RGB_LCD_CTL_PRGB_MODE_MASK,
                RGB_LCD_CTL_PRGB_MODE(rgb->format));
        aic_rgb_swap();
        break;
    case SRGB:
        if (rgb->format)
            reg_set_bit(comp->regs + RGB_LCD_CTL,
                    RGB_LCD_CTL_SRGB_MODE);
        aic_rgb_swap();
    default:
        pr_err("Invalid mode %d\n", rgb->mode);
        break;
    }

    reg_set_bit(comp->regs + RGB_LCD_CTL, RGB_LCD_CTL_EN);
    aic_rgb_release_drvdata();

    return 0;
}

static int aic_rgb_disable(void)
{
    struct aic_rgb_comp *comp = aic_rgb_request_drvdata();

    reg_clr_bit(comp->regs + RGB_LCD_CTL, RGB_LCD_CTL_EN);
    aic_rgb_release_drvdata();
    return 0;
}

static int aic_rgb_attach_panel(struct aic_panel *panel)
{
    struct aic_rgb_comp *comp = aic_rgb_request_drvdata();
    u32 pixclk = panel->timings->pixelclock;
    struct panel_rgb *rgb = panel->rgb;
    u64 pll_disp_rate = 0;
    int i = 0;

    comp->panel = panel;

    if (rgb->mode == PRGB)
#ifdef AIC_DISP_RGB_DRV_V11
        comp->sclk_rate = pixclk * 2;
#else
        comp->sclk_rate = pixclk * 4;
#endif
    else if (rgb->mode == SRGB)
#ifdef AIC_DISP_RGB_DRV_V11
        comp->sclk_rate = pixclk * 6;
#else
        comp->sclk_rate = pixclk * 12;
#endif

    pll_disp_rate = comp->sclk_rate;
    while (pll_disp_rate < PLL_DISP_FREQ_MIN)
    {
        pll_disp_rate = comp->sclk_rate * (2 << i);
        i++;
    }
    comp->pll_disp_rate = pll_disp_rate;

    aic_rgb_release_drvdata();
    return 0;
}

struct di_funcs aic_rgb_func = {
    .clk_enable = aic_rgb_clk_enable,
    .clk_disable = aic_rgb_clk_disable,
    .enable = aic_rgb_enable,
    .disable = aic_rgb_disable,
    .attach_panel = aic_rgb_attach_panel,
};

static int aic_rgb_probe(void)
{
    struct aic_rgb_comp *comp;

    comp = aicos_malloc(0, sizeof(*comp));
    if (!comp)
    {
        pr_err("alloc rgb comp failed\n");
        return -ENOMEM;
    }

    memset(comp, 0, sizeof(*comp));

    comp->regs = (void *)LCD_BASE;
    g_aic_rgb_comp = comp;

    return 0;
}

static void aic_rgb_remove(void)
{

}

struct platform_driver artinchip_rgb_driver = {
    .name = "artinchip-rgb",
    .component_type = AIC_RGB_COM,
    .probe = aic_rgb_probe,
    .remove = aic_rgb_remove,
    .di_funcs = &aic_rgb_func,
};

