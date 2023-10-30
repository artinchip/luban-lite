/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_clk.h"

#define to_clk_pll(_hw) container_of(_hw, struct aic_clk_pll_cfg, comm)

static int clk_pll_wait_lock(void)
{
    aic_udelay(200);
    return 0;
}

static inline void clk_pll_bypass(struct aic_clk_pll_cfg *pll, unsigned int bypass)
{
    u32 val;

    val = readl(cmu_reg(pll->offset_gen));
    val &= ~(1 << PLL_OUT_MUX);
    val |= (!bypass << PLL_OUT_MUX);
    writel(val, cmu_reg(pll->offset_gen));
}

static int clk_pll_enable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);
    u32 val;

    if (pll->flag & CLK_NO_CHANGE) {
        return 0;
    }


    val = readl(cmu_reg(pll->offset_gen));
    val |= (1 << PLL_OUT_SYS | 1 << PLL_EN_BIT);
    writel(val, cmu_reg(pll->offset_gen));

    return clk_pll_wait_lock();
}

static void clk_pll_disable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);
    u32 val;

    if (!(pll->flag & CLK_NO_CHANGE)) {
        val = readl(cmu_reg(pll->offset_gen));
        val &= ~(1 << PLL_OUT_SYS | 1 << PLL_EN_BIT);
        writel(val, cmu_reg(pll->offset_gen));
    }
}

static int clk_pll_is_enable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);
    u32 reg_val                 = readl(cmu_reg(pll->offset_gen));

    if ((reg_val & (1 << PLL_EN_BIT)) && (reg_val & (1 << PLL_OUT_SYS)))
        return 1;

    return 0;
}

static unsigned long clk_pll_recalc_rate(struct aic_clk_comm_cfg *comm_cfg,
                                         unsigned long parent_rate)
{
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);
    u32 factor_n, factor_m, factor_p, fra_in;
    u64 rate, rate_int, rate_fra;
    u8 fra_en = 0;
    u32 sdm_en = 0;

    /* PLL output mux is CLK_24M */
    if (!((readl(cmu_reg(pll->offset_gen)) >> PLL_OUT_MUX) & 0x1))
        return CLOCK_24M;

    factor_n = (readl(cmu_reg(pll->offset_gen)) >> PLL_FACTORN_BIT) &
               PLL_FACTORN_MASK;
    factor_m = (readl(cmu_reg(pll->offset_gen)) >> PLL_FACTORM_BIT) &
               PLL_FACTORM_MASK;
    factor_p = (readl(cmu_reg(pll->offset_gen)) >> PLL_FACTORP_BIT) &
               PLL_FACTORP_MASK;
    if (pll->type == AIC_PLL_FRA)
        fra_en = (readl(cmu_reg(pll->offset_fra)) >> PLL_FRAC_EN_BIT) & 0x1;

    if (pll->type == AIC_PLL_INT || !fra_en)
        rate = parent_rate / (factor_p + 1) * (factor_n + 1) / (factor_m + 1);
    else {
        fra_in = readl(cmu_reg(pll->offset_fra)) & PLL_FRAC_DIV_MASK;
        rate_int =
                parent_rate / (factor_p + 1) * (factor_n + 1) / (factor_m + 1);
        rate_fra = (u64)parent_rate / (factor_p + 1) * fra_in;
        do_div(rate_fra, PLL_FRAC_DIV_MASK * (factor_m + 1));
        rate = rate_int + rate_fra;
    }

    if (pll->type == AIC_PLL_SDM) {
        sdm_en = readl(cmu_reg(pll->offset_sdm));
        sdm_en >>= 31;

        /* Pll is SDM mode, but SDM is not enable
         * Create a small deviation, let sys setup again
         */
        if (sdm_en == 0)
            rate += 1;
    }
#ifdef FPGA_BOARD_ARTINCHIP
    rate = fpga_board_rate[pll->id];
#endif
    return rate;
}

static long clk_pll_round_rate(struct aic_clk_comm_cfg *comm_cfg,
                               unsigned long rate, unsigned long *prate)
{
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);
    u32 factor_n, factor_m, factor_p;
    long rrate, vco_rate;
    unsigned long parent_rate = *prate;

    if (pll->type == AIC_PLL_FRA)
            return rate;

    /* The frequency constraint of PLL_VCO is between 768M and 1560M */
    if (rate < PLL_VCO_MIN)
        factor_m = DIV_ROUND_UP(PLL_VCO_MIN, rate) - 1;
    else
        factor_m = 0;

    if (factor_m > PLL_FACTORM_MASK)
        factor_m = PLL_FACTORM_MASK;

    vco_rate = (factor_m + 1) * rate;
    if (vco_rate > PLL_VCO_MAX)
        vco_rate = PLL_VCO_MAX;

    factor_p = (vco_rate % parent_rate) ? 1 : 0;
    if (!factor_p)
        return rate;
    else if (!(vco_rate % (parent_rate / (factor_p + 1))))
        return rate;

    factor_n = vco_rate / parent_rate * (factor_p + 1) - 1;

    rrate = parent_rate / (factor_p + 1) * (factor_n + 1) / (factor_m + 1);

#ifdef FPGA_BOARD_ARTINCHIP
    rrate = fpga_board_rate[pll->id];
#endif
    return rrate;
}

static int clk_pll_set_rate(struct aic_clk_comm_cfg *comm_cfg,
                            unsigned long rate, unsigned long parent_rate)
{
    u32 factor_n, factor_m, factor_p, reg_val;
    u64 val, fra_in = 0;
    u8 fra_en, factor_m_en;
    unsigned long vco_rate;
    u32 ppm_max, sdm_amp, sdm_en = 0;
    u64 sdm_step;
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);

    if (pll->flag & CLK_NO_CHANGE)
        return 0;

    if (rate == CLOCK_24M) {
        val = readl(cmu_reg(pll->offset_gen));
        val &= ~(1 << PLL_OUT_MUX);
        writel(val, cmu_reg(pll->offset_gen));
        return 0;
    }

    /* Switch the output of PLL to 24MHz */
    clk_pll_bypass(pll, 1);

    /* Calculate PLL parameters.
     * The frequency constraint of PLL_VCO
     * is between 768M and 1560M
     */
    if (rate < PLL_VCO_MIN)
        factor_m = DIV_ROUND_UP(PLL_VCO_MIN, rate) - 1;
    else
        factor_m = 0;

    if (factor_m > PLL_FACTORM_MASK)
        factor_m = PLL_FACTORM_MASK;

    if (factor_m)
        factor_m_en = 1;
    else
        factor_m_en = 0;

    vco_rate = (factor_m + 1) * rate;
    if (vco_rate > PLL_VCO_MAX)
        vco_rate = PLL_VCO_MAX;

    factor_p = (vco_rate % parent_rate) ? 1 : 0;
    factor_n = vco_rate * (factor_p + 1) / parent_rate  - 1;

    reg_val = readl(cmu_reg(pll->offset_gen));
    reg_val &= ~0xFFFF;
    reg_val |= (factor_m_en << PLL_FACTORM_EN_BIT) |
            (factor_n << PLL_FACTORN_BIT) |
            (factor_m << PLL_FACTORM_BIT) |
            (factor_p << PLL_FACTORP_BIT);
    writel(reg_val, cmu_reg(pll->offset_gen));

    if (pll->type == AIC_PLL_FRA) {
        val = rate % (parent_rate * (factor_n + 1) /
                  (factor_m + 1) / (factor_p + 1));
        fra_en = val ? 1 : 0;
        if (fra_en) {
            fra_in = val * (factor_p + 1) *
                 (factor_m + 1) * PLL_FRAC_DIV_MASK;
            do_div(fra_in, parent_rate);
        }
        /* Configure fractional division */
        writel(fra_en << PLL_FRAC_EN_BIT | fra_in, cmu_reg(pll->offset_fra));
    } else if (pll->type == AIC_PLL_SDM) {
        sdm_en = readl(cmu_reg(pll->offset_sdm));
        sdm_en >>= 31;

        ppm_max = 1000000 / (factor_n + 1);
        /* 1% spread */
        if (ppm_max < PLL_SDM_SPREAD_PPM)
            sdm_amp = 0;
        else
            sdm_amp = PLL_SDM_AMP_MAX -
                PLL_SDM_SPREAD_PPM *
                PLL_SDM_AMP_MAX / ppm_max;

        /* SDM uses triangular wave, 33KHz by default  */
        sdm_step = (PLL_SDM_AMP_MAX - sdm_amp) * 2 *
            PLL_SDM_SPREAD_FREQ;
        do_div(sdm_step, parent_rate);
        if (sdm_step > 511)
            sdm_step = 511;

        reg_val = (1UL << PLL_SDM_EN_BIT) |
              (2 << PLL_SDM_MODE_BIT) |
              (sdm_step << PLL_SDM_STEP_BIT) |
              (3 << PLL_SDM_FREQ_BIT) |
              (sdm_amp << PLL_SDM_AMP_BIT);

        writel(reg_val, cmu_reg(pll->offset_sdm));
    }

    reg_val = readl(cmu_reg(pll->offset_gen));
    reg_val |= (1 << PLL_OUT_SYS | 1 << PLL_EN_BIT);
    writel(reg_val, cmu_reg(pll->offset_gen));

    if (!clk_pll_wait_lock())
        clk_pll_bypass(pll, 0);
    else {
        //pr_err("%d not lock\n", pll->id);
        return -EAGAIN;
    }

    return 0;
}

static unsigned int clk_pll_get_parent(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_pll_cfg *pll = to_clk_pll(comm_cfg);

    return pll->parent_id;
}

const struct aic_clk_ops aic_clk_pll_ops = {
    .enable      = clk_pll_enable,
    .disable     = clk_pll_disable,
    .is_enabled  = clk_pll_is_enable,
    .recalc_rate = clk_pll_recalc_rate,
    .round_rate  = clk_pll_round_rate,
    .set_rate    = clk_pll_set_rate,
    .get_parent  = clk_pll_get_parent,
};

void hal_clk_pll_lowpower(void)
{
#if defined(AIC_CMU_DRV_V10)
    #if 0
    *(volatile uint32_t *)(CMU_BASE+PLL_IN_REG) &= ~(0x7U << 29);
    #endif
#elif defined(AIC_CMU_DRV_V11)
    *(volatile uint32_t *)(CMU_BASE+PLL_IN_REG) &= ~((0x7U << 29) | (0x1U << 1));
#elif defined(AIC_CMU_DRV_V12)
    *(volatile uint32_t *)(CMU_BASE+PLL_IN_REG) &= ~((0x7U << 29) | (0x1U << 1));
#endif
}
