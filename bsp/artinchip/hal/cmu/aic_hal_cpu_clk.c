/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_clk.h"

#define CPU_CLK_WR_KEY 0x2023

#define to_clk_cpu_mod(_hw)                     \
    container_of(_hw, struct aic_clk_cpu_cfg, comm)

static u32 clk_cpu_mod_write_enable(struct aic_clk_cpu_cfg *mod, u32 val_tmp)
{
    u32 val = val_tmp;
    if (mod->key_bit >= 0) {
        val &= ~(mod->key_mask << mod->key_bit);
        val |= CPU_CLK_WR_KEY << mod->key_bit;
    }
    return val;
}

static int clk_cpu_mod_enable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_cpu_cfg *mod = to_clk_cpu_mod(comm_cfg);
    u32 val;

    val = readl(cmu_reg(mod->offset_reg));
    if (mod->gate_bit >= 0)
        val |= (1 << mod->gate_bit);

    val = clk_cpu_mod_write_enable(mod, val);
    writel(val, cmu_reg(mod->offset_reg));
    return 0;
}

static void clk_cpu_mod_disable(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_cpu_cfg *mod = to_clk_cpu_mod(comm_cfg);
    u32 val;

    val = readl(cmu_reg(mod->offset_reg));

    if (mod->gate_bit >= 0)
        val &= ~(1 << mod->gate_bit);

    val = clk_cpu_mod_write_enable(mod, val);
    writel(val, cmu_reg(mod->offset_reg));
}

static int clk_cpu_mod_is_enabled(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_cpu_cfg *mod = to_clk_cpu_mod(comm_cfg);
    u32 val;

    val = readl(cmu_reg(mod->offset_reg));
    if (mod->gate_bit >= 0)
        return val & (1 << mod->gate_bit);

    return 1;
}

static unsigned long clk_cpu_mod_recalc_rate(struct aic_clk_comm_cfg *comm_cfg,
                                                unsigned long parent_rate)
{
    unsigned long rate, div0 = 0, parent_index = 0;
    struct aic_clk_cpu_cfg *mod = to_clk_cpu_mod(comm_cfg);

    parent_index = (readl(cmu_reg(mod->offset_reg)) >> mod->mux_bit) & mod->mux_mask;

    if (parent_index == 1) {
        div0 = (readl(cmu_reg(mod->offset_reg)) >> mod->div0_bit) & mod->div0_mask;
        rate = parent_rate / (div0 + 1);
    } else
        rate = parent_rate;

#ifdef CONFIG_DEBUG_ON_FPGA_BOARD_ARTINCHIP
    rate = fpga_board_rate[mod->id];
#endif
    return rate;
}

static void try_best_divider(u32 rate, u32 parent_rate,
                                u32 max_div0, u32 *div0)
{
    u32 tmp, i, min_delta = U32_MAX, best_div0 = 1;

    for (i = 1; i <= max_div0; i++) {
        tmp = i * rate;
        if (parent_rate == tmp) {
            best_div0 = i;
            goto __out;
        } else if (parent_rate < tmp) {
            best_div0 = i - 1;
            if (best_div0 <= 0)
                best_div0 = 1;
            goto __out;
        }

        if (abs(parent_rate - tmp) < min_delta) {
            min_delta = abs(parent_rate - tmp);
            best_div0 = i;
        }
    }

__out:
    *div0 = best_div0;
}

static unsigned int clk_cpu_mod_get_parent(struct aic_clk_comm_cfg *comm_cfg)
{
    struct aic_clk_cpu_cfg *mod = to_clk_cpu_mod(comm_cfg);

    return (readl(cmu_reg(mod->offset_reg)) >> mod->mux_bit) & mod->mux_mask;
}

static int clk_cpu_mod_set_parent(struct aic_clk_comm_cfg *comm_cfg,
                                    unsigned int parent_id)
{
    struct aic_clk_cpu_cfg *mod = to_clk_cpu_mod(comm_cfg);
    u32 val;
    u32 i;
    u32 index = 0xFFFF;

    for (i = 0; i < mod->num_parents; i++) {
        if (parent_id == mod->parent_ids[i]) {
            index = i;
            break;
        }
    }

    if (index == 0xFFFF) {
        return -1;
    }

    val = readl(cmu_reg(mod->offset_reg));
    val &= ~(mod->mux_mask << mod->mux_bit);
    val |= index << mod->mux_bit;

    val = clk_cpu_mod_write_enable(mod, val);
    writel(val, cmu_reg(mod->offset_reg));

    return 0;
}

static int clk_cpu_mod_set_rate(struct aic_clk_comm_cfg *comm_cfg,
                                unsigned long rate,
                                unsigned long parent_rate)
{
    struct aic_clk_cpu_cfg *mod = to_clk_cpu_mod(comm_cfg);
    u32 val, div0, parent_index;

    val = readl(cmu_reg(mod->offset_reg));
    parent_index = (readl(cmu_reg(mod->offset_reg)) >> mod->mux_bit) & mod->mux_mask;

    if (parent_index == 1) {
        try_best_divider(rate, parent_rate, mod->div0_mask + 1, &div0);
        val &= ~(mod->div0_mask << mod->div0_bit);
        val |= ((div0 - 1) << mod->div0_bit);
    }

    val = clk_cpu_mod_write_enable(mod, val);
    writel(val, cmu_reg(mod->offset_reg));
    return 0;
}

static long clk_cpu_mod_round_rate(struct aic_clk_comm_cfg *comm_cfg,
                                    unsigned long rate,
                                    unsigned long *prate)
{
    u32 rrate, parent_rate, parent_index;
    u32 div0 = 0;

    struct aic_clk_cpu_cfg *mod = to_clk_cpu_mod(comm_cfg);

    parent_rate = *prate;
    parent_index =
        (readl(cmu_reg(mod->offset_reg)) >> mod->mux_bit) & mod->mux_mask;

    if (parent_index == 1)
        try_best_divider(rate, parent_rate, mod->div0_mask + 1, &div0);
    else
        div0 = 1;

    rrate = parent_rate / div0;

#ifdef FPGA_BOARD_ARTINCHIP
    rrate = fpga_board_rate[mod->id];
#endif
    return rrate;
}


const struct aic_clk_ops aic_clk_cpu_ops = {
    .enable      = clk_cpu_mod_enable,
    .disable     = clk_cpu_mod_disable,
    .is_enabled  = clk_cpu_mod_is_enabled,
    .recalc_rate = clk_cpu_mod_recalc_rate,
    .round_rate  = clk_cpu_mod_round_rate,
    .get_parent  = clk_cpu_mod_get_parent,
    .set_parent  = clk_cpu_mod_set_parent,
    .set_rate    = clk_cpu_mod_set_rate,
};
