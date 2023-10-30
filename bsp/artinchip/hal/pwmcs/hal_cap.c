/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <string.h>

#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_cap.h"

#define AIC_CAP_CH_NUM          AIC_HRTIMER_CH_NUM

/* Register definition of PWMCS Controller */

#define CAP_BASE(n)             (PWMCS_BASE + 0x1000 + 0x100 * (n))
#define GLB_BASE                (PWMCS_BASE + 0xF000)

#define CAP_CNT_V(n)            (CAP_BASE(n) + 0x00)
#define CAP_CNT_PHV(n)          (CAP_BASE(n) + 0x04)
#define CAP_CNT_PRDV(n)         (CAP_BASE(n) + 0x08)
#define CAP_CNT_CMPV(n)         (CAP_BASE(n) + 0x0c)
#define CAP_CNT_PRDV_SH(n)      (CAP_BASE(n) + 0x10)
#define CAP_CNT_CMPV_SH(n)      (CAP_BASE(n) + 0x14)
#define CAP_CONF1(n)            (CAP_BASE(n) + 0x18)
#define CAP_CONF2(n)            (CAP_BASE(n) + 0x1c)
#define CAP_INT_EN(n)           (CAP_BASE(n) + 0x20)
#define CAP_FLG(n)              (CAP_BASE(n) + 0x24)
#define CAP_FLG_CLR(n)          (CAP_BASE(n) + 0x28)
#define CAP_SW_FRC(n)           (CAP_BASE(n) + 0x2c)
#define CAP_IN_FLT(n)           (CAP_BASE(n) + 0x30)
#define CAP_IN_SRC(n)           (CAP_BASE(n) + 0x34)
#define CAP_VER(n)              (CAP_BASE(n) + 0xfc)

#ifdef AIC_HRTIMER_DRV_V10
#define GLB_CLK_CTL             (GLB_BASE + 0x00)
#define GLB_CLK_CTL_CAP_EN(n)   (BIT(16) << (n))
#endif
#ifdef AIC_HRTIMER_DRV_V11
#define GLB_CLK_CTL             (GLB_BASE + 0x24)
#define GLB_CLK_CTL_CAP_EN(n)   BIT(n)
#endif

#define GLB_CAP_INT_STS         (GLB_BASE + 0x0C)

#define GLB_CAP_INT_STS_MASK(n) (1 << (n))

#define CAP_CONF2_PWM_MODE      BIT(9)
#define CAP_CONF2_CNT_EN        BIT(4)

#define CAP_INT_EN_CNT_CMP      BIT(7)
#define CAP_INT_EN_CNT_PRD      BIT(6)

#define CAP_FLG_CLR_CMP         BIT(7)
#define CAP_FLG_CLR_PRD         BIT(6)

// #define CAP_USE_CMP_INT

struct aic_cap_arg {
    u16 available;
    u16 id;
    u32 freq;
    u32 prd;
    u32 irq_cnt;
};

static struct aic_cap_arg g_cap_args[AIC_CAP_CH_NUM] = {{0}};
static int g_cap_inited = 0;

static void _cap_ch_info(char *buf, u32 ch, u32 en, struct aic_cap_arg *arg)
{
    sprintf(buf, "%2d %2d %9d %10d/%-10d %10d\n",
            ch, en & GLB_CLK_CTL_CAP_EN(ch) ? 1 : 0,
            arg->freq, readl(CAP_CNT_PRDV(ch)), arg->prd, arg->irq_cnt);
}

void hal_cap_status_show(void)
{
    int ver = readl(CAP_VER(0));
    int enable = readl(GLB_CLK_CTL);
    char info[AIC_CAP_CH_NUM][128] = {{0}};
    u32 i;

    printf("In CAP V%d.%02d:\n"
               "CAP Enable: 0x%x, IRQ Enable: 0x%x\n"
               "Ch En %9s %15s %16s\n",
               ver >> 8, ver & 0xFF, enable, readl(GLB_CAP_INT_STS),
               "Frequency", "Timeout cnt", "IRQ cnt");

    for (i = 0; i < AIC_CAP_CH_NUM; i++) {
        _cap_ch_info(info[i], i, enable, &g_cap_args[i]);
        printf("%s", info[i]);
    }
}

static void _cap_reg_enable(int addr, int bit, int enable)
{
    int tmp;

    tmp = readl((ulong)addr);
    if (enable)
        tmp |= bit;
    else
        tmp &= ~bit;

    writel(tmp, (ulong)addr);
}

void hal_cap_ch_init(u32 ch)
{
    struct aic_cap_arg *arg = &g_cap_args[ch];

    arg->available = 1;
}

void hal_cap_ch_deinit(u32 ch)
{
    struct aic_cap_arg *arg = &g_cap_args[ch];

    arg->available = 0;
}

void hal_cap_int_enable(u32 ch, int enable)
{
    if (enable) {
#ifdef CAP_USE_CMP_INT
        writel(CAP_INT_EN_CNT_CMP, CAP_INT_EN(ch));
#else
        writel(CAP_INT_EN_CNT_PRD, CAP_INT_EN(ch));
#endif
    } else {
        writel(0, CAP_INT_EN(ch));
    }
}

u32 hal_cap_int_sta(void)
{
    return readl(GLB_CAP_INT_STS);
}

u32 hal_cap_is_pending(u32 ch)
{
    u32 pending = readl_bit(GLB_CAP_INT_STS_MASK(ch), GLB_CAP_INT_STS);

    g_cap_args[ch].irq_cnt += pending;
    if (pending)
#ifdef CAP_USE_CMP_INT
        writel(CAP_FLG_CLR_CMP, CAP_FLG_CLR(ch));
#else
        writel(CAP_FLG_CLR_PRD, CAP_FLG_CLR(ch));
#endif

    return pending;
}

int hal_cap_enable(u32 ch)
{
    struct aic_cap_arg *arg = &g_cap_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    _cap_reg_enable(GLB_CLK_CTL, GLB_CLK_CTL_CAP_EN(ch), 1);
    _cap_reg_enable(CAP_CONF2(ch), CAP_CONF2_PWM_MODE, 1);
    writel(readl(CAP_FLG(ch)), CAP_FLG_CLR(ch));
    return 0;
}

int hal_cap_disable(u32 ch)
{
    struct aic_cap_arg *arg = &g_cap_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    _cap_reg_enable(GLB_CLK_CTL, GLB_CLK_CTL_CAP_EN(ch), 0);
    return 0;
}

void hal_cap_cnt_start(u32 ch)
{
    _cap_reg_enable(CAP_CONF2(ch), CAP_CONF2_CNT_EN, 1);
}

void hal_cap_cnt_stop(u32 ch)
{
    _cap_reg_enable(CAP_CONF2(ch), CAP_CONF2_CNT_EN, 0);
}

int hal_cap_set_freq(u32 ch, u32 freq)
{
    struct aic_cap_arg *arg = &g_cap_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    if (freq > CAP_MAX_FREQ) {
        hal_log_err("ch%d freq %d is out of range\n", ch, freq);
        return -ERANGE;
    }
    arg->freq = freq;
    arg->prd  = PWMCS_CLK_RATE / freq;
    return 0;
}

int hal_cap_set_cnt(u32 ch, u32 cnt)
{
    struct aic_cap_arg *arg = &g_cap_args[ch];

    if (!arg->available) {
        hal_log_err("ch%d is unavailable\n", ch);
        return -EINVAL;
    }

    arg->prd = (PWMCS_CLK_RATE / arg->freq) * cnt;
    writel(arg->prd, CAP_CNT_PRDV(ch));
#ifdef CAP_USE_CMP_INT
    writel(arg->prd - (arg->prd >> 4), CAP_CNT_CMPV(ch));
#endif
    writel(0, CAP_CNT_V(ch));

    return 0;
}

int hal_cap_get(u32 ch)
{
    return readl(CAP_CNT_V(ch));
}

int hal_cap_init(void)
{
    int i, ret = 0;
    u32 clk_id = 0;

    if (g_cap_inited) {
        hal_log_debug("PWMCS was already inited\n");
        return 0;
    }

#ifdef AIC_HRTIMER_DRV_V10
    clk_id = CLK_PWMCS;
#endif
#ifdef AIC_HRTIMER_DRV_V11
    clk_id = CLK_PWMCS_SDFM;
#endif
    ret = hal_clk_set_freq(clk_id, PWMCS_CLK_RATE);
    if (ret < 0) {
        hal_log_err("Failed to set PWMCS clk %d\n", PWMCS_CLK_RATE);
        return -1;
    }
    ret = hal_clk_enable(CLK_PWMCS);
    if (ret < 0) {
        hal_log_err("Failed to enable PWMCS clk\n");
        return -1;
    }

    ret = hal_clk_enable_deassertrst(CLK_PWMCS);
    if (ret < 0) {
        hal_log_err("Failed to reset PWMCS deassert\n");
        return -1;
    }

    /* default configuration */
    for (i = 0; i < AIC_CAP_CH_NUM; i++) {
        g_cap_args[i].id = i;
        g_cap_args[i].freq = CAP_MAX_FREQ;
    }

    g_cap_inited = 1;
    return 0;
}

int hal_cap_deinit(void)
{
    u32 i;

    for (i = 0; i < AIC_CAP_CH_NUM; i++)
        if (g_cap_args[i].available)
            hal_cap_disable(i);

    hal_clk_disable_assertrst(CLK_PWMCS);
    hal_clk_disable(CLK_PWMCS);
    g_cap_inited = 0;
    return 0;
}
