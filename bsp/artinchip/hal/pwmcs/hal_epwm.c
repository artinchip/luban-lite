/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#include <string.h>

#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_epwm.h"

/* Register definition of PWMCS Controller */
#define EPWM_BASE               (PWMCS_BASE)
#define GLB_BASE_E              (PWMCS_BASE + 0xF000)

#ifdef AIC_EPWM_DRV_V11
#define GLB_CLK_CTL_E            0x020
#else
#define GLB_CLK_CTL_E            0x000
#endif
#define GLB_PWM_INT_STS          0x004
#define GLB_PWM_EN               0x014

#define EPWM_CNT_PRDV(n)         ((((n) & 0xF) << 8) + 0x000)
#define EPWM_CNT_V(n)            ((((n) & 0xF) << 8) + 0x008)
#define EPWM_CNT_CONF(n)         ((((n) & 0xF) << 8) + 0x00C)
#define EPWM_CNT_AV(n)           ((((n) & 0xF) << 8) + 0x014)
#define EPWM_CNT_BV(n)           ((((n) & 0xF) << 8) + 0x018)
#define EPWMA_ACT(n)             ((((n) & 0xF) << 8) + 0x020)
#define EPWMB_ACT(n)             ((((n) & 0xF) << 8) + 0x024)
#define EPWM_SW_ACT(n)           ((((n) & 0xF) << 8) + 0x028)
#define EPWM_ACT_SW_CT(n)        ((((n) & 0xF) << 8) + 0x02C)
#define EPWM_ADC_INT_CTL(n)      ((((n) & 0xF) << 8) + 0x058)
#define EPWM_ADC_INT_PRE(n)      ((((n) & 0xF) << 8) + 0x05C)
#define EPWM_EVNT_FLAG(n)        ((((n) & 0xF) << 8) + 0x060)
#define EPWM_EVENT_CLR(n)        ((((n) & 0xF) << 8) + 0x064)
#define EPWM_VERSION(n)          ((((n) & 0xF) << 8) + 0x0FC)

#define GLB_EPWM_EN_B            BIT(0)
#define EPWM_S0_CLK_EN           BIT(0)
#define EPMW_SX_CLK_EN(n)        (EPWM_S0_CLK_EN << (n))
#define EPWMA_ACT_CNTDBV_SHIFT   10
#define EPWMA_ACT_CNTUBV_SHIFT   8
#define EPWMA_ACT_CNTDAV_SHIFT   6
#define EPWMA_ACT_CNTUAV_SHIFT   4
#define EPWMA_ACT_CNTPRD_SHIFT   2
#define EPWM_TBPRD_MAX           0xFFFF
#define EPWM_A_INIT              BIT(16)
#define EPWM_B_INIT              BIT(18)
#define EPWM_CLK_DIV1_MAX        0x7
#define EPWM_CLK_DIV2_SHIFT      10
#define EPWM_CLK_DIV1_SHIFT      7
#define EPWM_CNT_MOD_MASK        GENMASK(1, 0)
#define EPWM_CNT_MOD_SHIFT       0
#define EPWM_INT_EN_SHITF        3
#define EPWM_INT_SEL_SHIFT       0
#define EPWM_INT_CLR             BIT(0)
#define EPWM_ACT_SW_CT_UPDT      6
#define EPWM_SWACT_UPDT          3 << EPWM_ACT_SW_CT_UPDT
#define EPWM_ACT_SW_NONE         0x0
#define EPWM_ACT_SW_HIGH         0xA
#define EPWM_ACT_SW_LOW          0x5

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC            1000000000
#endif

static struct aic_epwm_arg g_epwm_args[AIC_EPWM_CH_NUM] = {{0}};

static inline void epwm_writel(u32 val, int reg)
{
    writel(val, EPWM_BASE + reg);
}

static inline u32 epwm_readl(int reg)
{
    return readl(EPWM_BASE + reg);
}


static void aic_epwm_ch_info(char *buf, u32 ch, u32 en, struct aic_epwm_arg *arg)
{
    const static char *mode[] = {"Up", "Down", "UpDw"};
    const static char *act[] = {"-", "Low", "Hgh", "Inv"};

    sprintf(buf, "%2d %2d %4s %11d %3d %3s %3s %3s %3s %3s %3s\n"
        "%30s %3s %3s %3s %3s %3s\n",
        ch, en & EPMW_SX_CLK_EN(ch) ? 1 : 0,
        mode[arg->mode], EPWM_TB_CLK_RATE, arg->def_level,
        act[arg->action0.CBD], act[arg->action0.CBU],
        act[arg->action0.CAD], act[arg->action0.CAU],
        act[arg->action0.PRD], act[arg->action0.ZRO],
        act[arg->action1.CBD], act[arg->action1.CBU],
        act[arg->action1.CAD], act[arg->action1.CAU],
        act[arg->action1.PRD], act[arg->action1.ZRO]);
}

void hal_epwm_status_show(void)
{
    int enable = readl(GLB_BASE_E + GLB_CLK_CTL_E);
    char info[AIC_EPWM_CH_NUM][128] = {{0}};
    u32 i;

    printf("Ch En Mode Tb-clk-rate Def CBD CBU CAD CAU PRD ZRO\n");

    for (i = 0; i < AIC_EPWM_CH_NUM; i++) {
        aic_epwm_ch_info(info[i], i, enable, &g_epwm_args[i]);
        printf("%s", info[i]);
    }
}

static void epwm_reg_enable(int addr, int bit, int enable)
{
    int tmp;

    tmp = readl((ulong)addr);
    if (enable)
        tmp |= bit;
    else
        tmp &= ~bit;

    writel(tmp, (ulong)addr);
}

u32 hal_epwm_int_sts(u32 ch)
{
    return epwm_readl(EPWM_EVNT_FLAG(ch));
}

void hal_epwm_clr_int(u32 stat, u32 ch)
{
    epwm_writel(stat, EPWM_EVENT_CLR(ch));
}

void hal_epwm_int_config(u32 ch, u8 irq_mode, u8 enable)
{
    epwm_writel(enable, EPWM_ADC_INT_PRE(ch));
    epwm_writel((enable << EPWM_INT_EN_SHITF) | ((irq_mode + 2) << EPWM_INT_SEL_SHIFT), EPWM_ADC_INT_CTL(ch));
}

void hal_epwm_ch_init(u32 ch, enum aic_epwm_mode mode, u32 default_level,
                     struct aic_epwm_action *a0, struct aic_epwm_action *a1)
{
    struct aic_epwm_arg *arg = &g_epwm_args[ch];

    arg->mode = mode;
    arg->available = 1;
    arg->def_level = default_level;
    memcpy(&arg->action0, a0, sizeof(struct aic_epwm_action));
    memcpy(&arg->action1, a1, sizeof(struct aic_epwm_action));

    epwm_reg_enable(GLB_BASE_E + GLB_CLK_CTL_E, EPMW_SX_CLK_EN(ch), 1);
    epwm_writel(EPWM_SWACT_UPDT, EPWM_SW_ACT(ch));
}

int hal_epwm_set(u32 ch, u32 duty_ns, u32 period_ns)
{
    struct aic_epwm_arg *arg = &g_epwm_args[ch];
    u32 prd = 0;
    u64 duty = 0;

    if ((period_ns < 1) || (period_ns > NSEC_PER_SEC)) {
        hal_log_err("ch%d invalid period %d\n", ch, period_ns);
        return -ERANGE;
    }

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    arg->freq = NSEC_PER_SEC / period_ns;
    prd = EPWM_TB_CLK_RATE / arg->freq;
    if (arg->mode == EPWM_MODE_UP_DOWN_COUNT)
        prd >>= 1;
    else
        prd--;

    if (prd > EPWM_TBPRD_MAX) {
        hal_log_err("ch%d period %d is too big\n", ch, prd);
        return -ERANGE;
    }
    arg->period = prd;
    epwm_writel(prd, EPWM_CNT_PRDV(ch));

    duty = (u64)duty_ns * (u64)prd;
    do_div(duty, period_ns);
    if (duty == prd)
        duty++;

    arg->duty = (u32)duty;
    hal_log_debug("Set CMP %u/%u\n", (u32)duty, prd);
    epwm_writel((u32)duty, EPWM_CNT_AV(ch));
    epwm_writel((u32)duty, EPWM_CNT_BV(ch));
    return 0;
}

int hal_epwm_get(u32 ch, u32 *duty_ns, u32 *period_ns)
{
    struct aic_epwm_arg *arg = &g_epwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    *duty_ns   = arg->duty;
    *period_ns = arg->period;
    return 0;
}

static void epwm_action_set(u32 ch, struct aic_epwm_action *act, char *name)
{
    u32 offset;
    u32 action = 0;

    if (strcmp(name, "action0") == 0)
        offset = EPWMA_ACT(ch);
    else
        offset = EPWMB_ACT(ch);

    action |= (act->CBD << EPWMA_ACT_CNTDBV_SHIFT) |
          (act->CBU << EPWMA_ACT_CNTUBV_SHIFT) |
          (act->CAD << EPWMA_ACT_CNTDAV_SHIFT) |
          (act->CAU << EPWMA_ACT_CNTUAV_SHIFT) |
          (act->PRD << EPWMA_ACT_CNTPRD_SHIFT) | act->ZRO;
    epwm_writel(action, offset);
}

int hal_epwm_enable(u32 ch)
{
    u32 div1 = 0;
    struct aic_epwm_arg *arg = &g_epwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    hal_log_debug("ch%d enable\n", ch);
    div1 = EPWM_CLK_RATE / EPWM_TB_CLK_RATE / 2;
    if (div1 > EPWM_CLK_DIV1_MAX) {
        hal_log_err("ch%d clkdiv %d is too big\n", ch, div1);
        return -ERANGE;
    }

    epwm_writel(EPWM_ACT_SW_NONE, EPWM_ACT_SW_CT(ch));

    epwm_action_set(ch, &arg->action0, "action0");
    epwm_action_set(ch, &arg->action1, "action1");

    epwm_writel((div1 << EPWM_CLK_DIV1_SHIFT) | arg->mode, EPWM_CNT_CONF(ch));

    return 0;
}

int hal_epwm_disable(u32 ch)
{
    struct aic_epwm_arg *arg = &g_epwm_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    hal_log_debug("ch%d disable\n", ch);

    if (arg->def_level)
        epwm_writel(EPWM_ACT_SW_HIGH, EPWM_ACT_SW_CT(ch));
    else
        epwm_writel(EPWM_ACT_SW_LOW, EPWM_ACT_SW_CT(ch));

    epwm_writel((u32)EPWM_MODE_STOP_COUNT, EPWM_CNT_CONF(ch));

    epwm_writel(0, EPWM_CNT_V(ch));

    return 0;
}

int hal_epwm_init(void)
{
    int i, ret = 0;
    u32 clk_id = 0;

#ifdef AIC_EPWM_DRV_V11
    clk_id = CLK_PWMCS_SDFM;
#else
    clk_id = CLK_PWMCS;
#endif

    ret = hal_clk_set_freq(clk_id, EPWM_CLK_RATE);
    if (ret < 0) {
        hal_log_err("Failed to set EPWM clk %d\n", EPWM_CLK_RATE);
        return -1;
    }
    ret = hal_clk_enable(CLK_PWMCS);
    if (ret < 0) {
        hal_log_err("Failed to enable EPWM clk\n");
        return -1;
    }

    ret = hal_clk_enable_deassertrst(CLK_PWMCS);
    if (ret < 0) {
        hal_log_err("Failed to reset EPWM deassert\n");
        return -1;
    }

    epwm_reg_enable(GLB_BASE_E + GLB_PWM_EN, GLB_EPWM_EN_B, 1);

    /* default configuration */
    for (i = 0; i < AIC_EPWM_CH_NUM; i++) {
        g_epwm_args[i].id = i;
        g_epwm_args[i].mode = EPWM_MODE_UP_COUNT;
        g_epwm_args[i].tb_clk_rate = EPWM_TB_CLK_RATE;
    }

    return 0;
}

int hal_epwm_deinit(void)
{
    hal_clk_disable_assertrst(CLK_PWMCS);
    hal_clk_disable(CLK_PWMCS);
    return 0;
}
