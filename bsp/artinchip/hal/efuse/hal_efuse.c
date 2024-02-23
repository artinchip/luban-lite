/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <hal_efuse.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <aic_log.h>

#define EFUSE_REG_CTL       (SID_BASE + 0x0000)
#define EFUSE_REG_ADDR      (SID_BASE + 0x0004)
#define EFUSE_REG_WDATA     (SID_BASE + 0x0008)
#define EFUSE_REG_RDATA     (SID_BASE + 0x000C)
#define EFUSE_REG_TIMING    (SID_BASE + 0x0010)
#define EFUSE_REG_PWRCFG    (SID_BASE + 0x0014)
#define EFUSE_REG_WFRID     (SID_BASE + 0x0018)
#define EFUSE_REG_PKGID     (SID_BASE + 0x001C)
#define EFUSE_REG_JTAG      (SID_BASE + 0x0080)
#define EFUSE_REG_SRAM      (SID_BASE + 0x200)

#define EFUSE_CTL_BROM_PRIV_LOCK (0x1 << 28)

#define EFUSE_OP_CODE       0xA1C

#define EFUSE_STS_INITIALIZING 1
#define EFUSE_STS_IDLE         2
#define EFUSE_STS_WRITING      3
#define EFUSE_STS_READING      4

int hal_efuse_init(void)
{
    int ret = 0, val = EFUSE_TIMING_VALUE;

    ret = hal_clk_enable(CLK_SID);
    if (ret < 0) {
        hal_log_err("Failed to enable SID clk.\n");
        return -EFAULT;
    }

    ret = hal_clk_enable_deassertrst(CLK_SID);
    if (ret < 0) {
        hal_log_err("Failed to reset SID deassert.\n");
        return -EFAULT;
    }

    writel(val, EFUSE_REG_TIMING);

    return 0;
}

int hal_efuse_deinit(void)
{
    hal_clk_disable_assertrst(CLK_SID);
    hal_clk_disable(CLK_SID);

    return 0;
}

int hal_efuse_wait_ready(void)
{
    u32 val, msk;
    int i;

    msk = (0xF << 8);
    for (i = 1000; i > 0; i--) {
        val = readl(EFUSE_REG_CTL);
        if ((val & msk) == (EFUSE_STS_IDLE << 8))
            return 0;
    }

    return -1;
}

int hal_efuse_read(u32 wid, u32 *wval)
{
    u32 i, addr, rval = 0, val = 0;

    if (wid >= EFUSE_MAX_WORD) {
        hal_log_err("Error, word id is too large.\n");
        return -EINVAL;
    }

    for (i = 0; i < 2; i++) {
        addr = (wid + EFUSE_MAX_WORD * i) << 2;
        writel(addr, EFUSE_REG_ADDR);

        /*
         * bit[27:16] OP CODE
         * bit[4] read start
         */
        val = readl(EFUSE_REG_CTL);
        val &= ~((0xFFF << 16) | (1 << 4));
        val |= ((EFUSE_OP_CODE << 16) | (1 << 4));
        writel(val, EFUSE_REG_CTL);

        /* Wait read finish */
        while(readl(EFUSE_REG_CTL) & (1 << 4)) {
            continue;
        }

        rval |= readl(EFUSE_REG_RDATA);
    }
    *wval = rval;

    return 0;
}

int hal_efuse_write(u32 wid, u32 wval)
{
    u32 addr, val, i;

    if (wid >= EFUSE_MAX_WORD) {
        hal_log_err("Error, word id is too large.\n");
        return -EINVAL;
    }
    
    for (i = 0; i < 2; i++) {
        addr = (wid + EFUSE_MAX_WORD * i) << 2;
        writel(addr, EFUSE_REG_ADDR);
        writel(wval, EFUSE_REG_WDATA);

        /*
        * bit[27:16] OP CODE
        * bit[0] read start
        */
        val = readl(EFUSE_REG_CTL);
        val &= ~((0xFFF << 16) | (1 << 0));
        val |= ((EFUSE_OP_CODE << 16) | (1 << 0));
        writel(val, EFUSE_REG_CTL);

        /* Wait write finish */
        while(readl(EFUSE_REG_CTL) & (1 << 0)) {
            continue;
        }
    }

    return 0;
}
