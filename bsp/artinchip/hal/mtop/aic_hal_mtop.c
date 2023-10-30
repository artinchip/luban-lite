/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#include <stdbool.h>
#include <string.h>
#include <aic_core.h>
#include <stdint.h>

#include "aic_hal_mtop.h"
#include "aic_hal_clk.h"

void hal_mtop_irq_enable(struct aic_mtop_dev *phandle)
{
    writel(0x1, phandle->reg_base + MTOP_IRQ_CTL);
}

int hal_mtop_init(struct aic_mtop_dev *phandle)
{
    int ret;

    phandle->reg_base = MTOP_BASE;
    phandle->irq_num = MTOP_IRQn;
    phandle->clk_id = CLK_MTOP;
    phandle->grp = MTOP_GROUP_MAX;
    phandle->prt = MTOP_PORT_MAX;

    ret = hal_clk_enable_deassertrst(phandle->clk_id);
    if (ret) {
        hal_log_err("mtop enable clock deassert failed\n");
        return ret;
    }

    return 0;
}

int hal_mtop_deinit(struct aic_mtop_dev *phandle)
{
    int ret;

    ret = hal_clk_disable_assertrst(phandle->clk_id);
    if (ret) {
        hal_log_err("mtop disable clock assert failed\n");
        return ret;
    }

    return 0;
}

void hal_mtop_enable(struct aic_mtop_dev *phandle)
{
    uint32_t reg_value;

    reg_value = readl(phandle->reg_base + MTOP_CTL);
    reg_value |= MTOP_EN;
    writel(reg_value, phandle->reg_base + MTOP_CTL);
}

void hal_mtop_set_period_cnt(struct aic_mtop_dev *phandle, uint32_t period_cnt)
{
    writel(period_cnt, phandle->reg_base + MTOP_TIME_CNT);
}

void hal_mtop_attach_callback(struct aic_mtop_dev *phandle, void *callback, void *arg)
{
    phandle->callback = callback;
    phandle->arg = arg;
}

void hal_mtop_detach_callback(struct aic_mtop_dev *phandle)
{
    phandle->callback = NULL;
    phandle->arg = NULL;
}

irqreturn_t hal_mtop_irq_handler(int irq_num, void *can_handle)
{
    uint32_t i, j, pos, offset;
    struct aic_mtop_dev *phandle = (struct aic_mtop_dev *)can_handle;

    if (readl(MTOP_BASE + MTOP_IRQ_STA)) {
        for (i = 0; i < MTOP_GROUP_MAX; i++)
            for (j = 0; j < MTOP_PORT_MAX; j++) {
                /* calculate the corresponding position of the port in BITMAP */
                pos = group_id[i] * 8 + j;
                if ((1 << pos) & PORT_BITMAP) {
                    offset = 0x100 + 0x100 * group_id[i] + 0x20 * j;
                    phandle->port_bw[i * phandle->prt + j].wcnt =
                        readl(phandle->reg_base + offset);
                    phandle->port_bw[i * phandle->prt + j].rcnt =
                        readl(phandle->reg_base + offset + 4);
                }
            }

        if (phandle->callback)
            phandle->callback(phandle, phandle->arg);

        writel(1, MTOP_BASE + MTOP_IRQ_STA);
        return 0;
    }

    return IRQ_HANDLED;
}
