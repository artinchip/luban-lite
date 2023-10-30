/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <aic_log.h>
#include <hal_ce.h>

#define SECURE_SRAM_ADDR    (0x1000)
#define CE_REG_ICR          (0x000)
#define CE_REG_ISR          (0x004)
#define CE_REG_TAR          (0x008)
#define CE_REG_TCR          (0x00C)
#define CE_REG_TSR          (0x010)
#define CE_REG_TER          (0x014)
#define CE_REG_VER          (0xFFC)

int hal_crypto_init(void)
{
    int ret = 0;

    ret = hal_clk_enable(CLK_CE);
    if (ret < 0) {
        hal_log_err("Failed to enable CE clk.\n");
        return -EFAULT;
    }

    ret = hal_clk_enable_deassertrst(CLK_CE);
    if (ret < 0) {
        hal_log_err("Failed to reset CE deassert.\n");
        return -EFAULT;
    }

    writel(0x7, CE_BASE + CE_REG_ICR);
    /* Clear interrupt status */
    writel(0xf, CE_BASE + CE_REG_ISR);
    writel(0xffffffff, CE_BASE + CE_REG_TER);

    return 0;
}

int hal_crypto_deinit(void)
{
    hal_clk_disable(CLK_CE);
    hal_clk_disable_assertrst(CLK_CE);

    return 0;
}

static inline s32 hal_crypto_start(struct crypto_task *task)
{
    writel((unsigned long)task, CE_BASE + CE_REG_TAR);
    writel((task->alg.alg_tag) | (1UL << 31), CE_BASE + CE_REG_TCR);
    return 0;
}

s32 hal_crypto_start_symm(struct crypto_task *task)
{
    return hal_crypto_start(task);
}

s32 hal_crypto_start_asym(struct crypto_task *task)
{
    return hal_crypto_start(task);
}

s32 hal_crypto_start_hash(struct crypto_task *task)
{
    return hal_crypto_start(task);
}

u32 hal_crypto_poll_finish(u32 alg_unit)
{
    /* Interrupt should be disabled, so here check and wait tmo */
    return ((readl(CE_BASE + CE_REG_ISR) & (0x01 << alg_unit)));
}

void hal_crypto_pending_clear(u32 alg_unit)
{
    u32 reg_val;

    reg_val = readl(CE_BASE + CE_REG_ISR);
    if ((reg_val & (0x01 << alg_unit)) == (0x01 << alg_unit)) {
        reg_val &= ~(0x0f);
        reg_val |= (0x01 << alg_unit);
    }
    writel(reg_val, CE_BASE + CE_REG_ISR);
}

u32 hal_crypto_get_err(u32 alg_unit)
{
    return ((readl(CE_BASE + CE_REG_TER) >> (8 * alg_unit)) & 0xFF);
}

s32 hal_crypto_bignum_byteswap(u8 *bn, u32 len)
{
    u32 i, j;
    u8 val;

    if (len == 0)
        return (-1);

    i = 0;
    j = len - 1;

    while (i < j) {
        val = bn[i];
        bn[i] = bn[j];
        bn[j] = val;
        i++;
        j--;
    }
    return 0;
}

/* Big Number from little-endian to big-endian */
s32 hal_crypto_bignum_le2be(u8 *src, u32 slen, u8 *dst, u32 dlen)
{
    int i;

    if (dlen < slen)
        return (-1);

    for (i = 0; i < slen; i++)
        dst[dlen - 1 - i] = src[i];

    for (; i < dlen; i++)
        dst[dlen - 1 - i] = 0;

    return 0;
}

/* Big Number from big-endian to litte-endian */
s32 hal_crypto_bignum_be2le(u8 *src, u32 slen, u8 *dst, u32 dlen)
{
    int i;

    if (dlen < slen)
        return (-1);

    for (i = 0; i < slen; i++)
        dst[i] = src[slen - 1 - i];

    for (; i < dlen; i++)
        dst[i] = 0;

    return 0;
}

