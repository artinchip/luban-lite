/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ARTINCHIP_AIC_HAL_CLK_H__
#define __ARTINCHIP_AIC_HAL_CLK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "aic_hal_clk_cmu.h"
#include "aic_clk_id.h"

int hal_clk_enable(uint32_t clk_id);
int hal_clk_disable(uint32_t clk_id);
int hal_clk_is_enabled(uint32_t clk_id);
int hal_clk_set_rate(uint32_t clk_id, unsigned long rate,
                     unsigned long parent_rate);
unsigned long hal_clk_recalc_rate(uint32_t clk_id, unsigned long parent_rate);
long hal_clk_round_rate(uint32_t clk_id, unsigned long rate,
                        unsigned long parent_rate);
int hal_clk_set_parent(uint32_t clk_id, unsigned int parent_clk_id);
unsigned int hal_clk_get_parent(uint32_t clk_id);
unsigned long hal_clk_get_freq(uint32_t clk_id);
int hal_clk_set_freq(uint32_t clk_id, unsigned long freq);
int hal_clk_enable_iter(uint32_t clk_id);

int hal_clk_enable_deassertrst(uint32_t clk_id);
int hal_clk_disable_assertrst(uint32_t clk_id);
int hal_clk_enable_deassertrst_iter(uint32_t clk_id);
void hal_clk_pll_lowpower(void);

#ifdef __cplusplus
}
#endif

#endif /* __ARTINCHIP_AIC_HAL_CLK_H__ */
