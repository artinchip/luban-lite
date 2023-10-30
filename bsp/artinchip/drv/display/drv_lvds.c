/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <aic_hal.h>
#include <aic_hal_lvds.h>

#include "disp_com.h"

struct aic_lvds_comp
{
    void *regs;
    u64 sclk_rate;
    u64 pll_disp_rate;
    struct aic_panel *panel;
};
static struct aic_lvds_comp *g_aic_lvds_comp;

static struct aic_lvds_comp *aic_lvds_request_drvdata(void)
{
    return g_aic_lvds_comp;
}

static void aic_lvds_release_drvdata(void)
{

}

static void lvds_0_lines(struct aic_lvds_comp *comp, u32 value)
{
    reg_write(comp->regs + LVDS_0_SWAP, value);
}

static void lvds_1_lines(struct aic_lvds_comp *comp, u32 value)
{
    reg_write(comp->regs + LVDS_1_SWAP, value);
}

static void lvds_0_pol(struct aic_lvds_comp *comp, u32 value)
{
    reg_write(comp->regs + LVDS_0_POL_CTL, value);
}

static void lvds_1_pol(struct aic_lvds_comp *comp, u32 value)
{
    reg_write(comp->regs + LVDS_1_POL_CTL, value);
}

static void lvds_phy_0_init(struct aic_lvds_comp *comp, u32 value)
{
    reg_write(comp->regs + LVDS_0_PHY_CTL, value);
}

static void lvds_phy_1_init(struct aic_lvds_comp *comp, u32 value)
{
    reg_write(comp->regs + LVDS_1_PHY_CTL, value);
}

static int aic_lvds_clk_enable(void)
{
    struct aic_lvds_comp *comp = aic_lvds_request_drvdata();
    u32 pixclk = comp->panel->timings->pixelclock;

    hal_clk_set_freq(CLK_PLL_FRA2, comp->pll_disp_rate);
    hal_clk_set_rate(CLK_SCLK, comp->sclk_rate, comp->pll_disp_rate);
    hal_clk_set_rate(CLK_PIX, pixclk, comp->sclk_rate);

    hal_clk_enable(CLK_PLL_FRA2);
    hal_clk_enable(CLK_SCLK);
    hal_clk_enable(CLK_LVDS);

    hal_reset_deassert(RESET_LVDS);

    aic_lvds_release_drvdata();
    return 0;
}

static int aic_lvds_clk_disable(void)
{
    hal_reset_assert(RESET_LVDS);

    hal_clk_disable(CLK_LVDS);
    hal_clk_disable(CLK_SCLK);
    return 0;
}

static int aic_lvds_enable(void)
{
    struct aic_lvds_comp *comp = aic_lvds_request_drvdata();
    struct panel_lvds *lvds = comp->panel->lvds;

    if (AIC_LVDS_LINES)
    {
        lvds_0_lines(comp, AIC_LVDS_LINES);
        lvds_1_lines(comp, AIC_LVDS_LINES);
    }

    if (AIC_LVDS_POL)
    {
        lvds_0_pol(comp, AIC_LVDS_POL);
        lvds_1_pol(comp, AIC_LVDS_POL);
    }

    lvds_phy_0_init(comp, AIC_LVDS_PHY);
    lvds_phy_1_init(comp, AIC_LVDS_PHY);

    reg_set_bits(comp->regs + LVDS_CTL,
             LVDS_CTL_MODE_MASK
             | LVDS_CTL_LINK_MASK
             | LVDS_CTL_SWAP_MASK
             | LVDS_CTL_SYNC_MODE_MASK,
             LVDS_CTL_MODE(lvds->mode)
             | LVDS_CTL_LINK(lvds->link_mode)
             | LVDS_CTL_SWAP_EN(AIC_LVDS_LINK_SWAP_EN)
             | LVDS_CTL_SYNC_MODE_EN(AIC_LVDS_SYNC_MODE_EN));

    reg_set_bit(comp->regs + LVDS_CTL, LVDS_CTL_EN);

    aic_lvds_release_drvdata();
    return 0;
}

static int aic_lvds_disable(void)
{
    struct aic_lvds_comp *comp = aic_lvds_request_drvdata();

    reg_clr_bit(comp->regs + LVDS_CTL, LVDS_CTL_EN);
    aic_lvds_release_drvdata();
    return 0;
}

static int aic_lvds_attach_panel(struct aic_panel *panel)
{
    struct aic_lvds_comp *comp = aic_lvds_request_drvdata();
    u32 pixclk = panel->timings->pixelclock;
    struct panel_lvds *lvds = panel->lvds;
    u64 pll_disp_rate = 0;
    int i = 0;

    comp->panel = panel;

    if (lvds->link_mode != DUAL_LINK)
        comp->sclk_rate = pixclk * 7;
    else
        comp->sclk_rate = pixclk * 3 + (pixclk >> 1); // * 3.5

    pll_disp_rate = comp->sclk_rate;
    while (pll_disp_rate < PLL_DISP_FREQ_MIN)
    {
        pll_disp_rate = comp->sclk_rate * (2 << i);
        i++;
    }
    comp->pll_disp_rate = pll_disp_rate;

    aic_lvds_release_drvdata();
    return 0;
}

struct di_funcs aic_lvds_func = {
    .clk_enable = aic_lvds_clk_enable,
    .clk_disable = aic_lvds_clk_disable,
    .enable = aic_lvds_enable,
    .disable = aic_lvds_disable,
    .attach_panel = aic_lvds_attach_panel,
};

static int aic_lvds_probe(void)
{
    struct aic_lvds_comp *comp;

    comp = aicos_malloc(0, sizeof(*comp));
    if (!comp)
    {
        pr_err("allloc lvds comp failed\n");
        return -ENOMEM;
    }

    memset(comp, 0, sizeof(*comp));

    comp->regs = (void *)LVDS_BASE;
    g_aic_lvds_comp = comp;

    return 0;
}

static void aic_lvds_remove(void)
{

}

struct platform_driver artinchip_lvds_driver = {
    .name = "artinchip-lvds",
    .component_type = AIC_LVDS_COM,
    .probe = aic_lvds_probe,
    .remove = aic_lvds_remove,
    .di_funcs = &aic_lvds_func,
};

