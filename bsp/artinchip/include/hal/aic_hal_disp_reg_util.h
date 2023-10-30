/*
 * Copyright (C) 2020-2022 ArtInChip Technology Co., Ltd.
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _REG_UTIL_H_
#define _REG_UTIL_H_

#include <stdio.h>
#include <aic_common.h>
#include <aic_io.h>

static inline u32 reg_read(void *reg_addr)
{
    return readl(reg_addr);
}

static inline void reg_write(void *reg_addr, u32 reg_value)
{
    writel(reg_value, reg_addr);
}

static inline u32 reg_rd_bits(void *reg_addr, u32 mask, u32 shift)
{
    return (reg_read(reg_addr) & mask) >> shift;
}

static inline u32 reg_rd_bit(void *reg_addr, u32 mask, u32 shift)
{
    return reg_rd_bits(reg_addr, mask, shift);
}

static inline void reg_clr_bits(void *reg_addr, u32 clr_mask)
{
    u32 value = reg_read(reg_addr);

    reg_write(reg_addr, value & (~clr_mask));
}

static inline void reg_clr_bit(void *reg_addr, u32 clr_mask)
{
    reg_clr_bits(reg_addr, clr_mask);
}

/* Notice: Must set only one bit to 1 by this function. */
static inline void reg_set_bit(void *reg_addr, u32 set)
{
    u32 value = reg_read(reg_addr);

    if (unlikely(set == 0))
        BUG();
    else
        reg_write(reg_addr, value | set);
}

static inline void reg_set_bits(void *reg_addr, u32 clr_mask, u32 set)
{
    u32 value = reg_read(reg_addr);

    reg_write(reg_addr, (value & (~clr_mask)) | (set & clr_mask));
}

#endif /* _REG_UTIL_H_ */

