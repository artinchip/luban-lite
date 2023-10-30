/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_HAL_MTOP_H_
#define _AIC_HAL_MTOP_H_

#include <aic_common.h>
#include <aic_io.h>
#include <aic_mtop_id.h>


#define MTOP_CTL                        0x0000
#define MTOP_TIME_CNT                   0x0004
#define MTOP_IRQ_CTL                    0x0008
#define MTOP_IRQ_STA                    0x000C
#define MTOP_AXI_WCNT(p)                (0x0100 + p * 0x20)
#define MTOP_AXI_RCNT(p)                (0x0104 + p * 0x20)
#define MTOP_AHB_WCNT                   0x0200
#define MTOP_AHB_RCNT                   0x0204

#define MTOP_TRIG                       BIT(29)
#define MTOP_MODE                       BIT(28)
#define MTOP_EN                         BIT(0)

typedef struct port_bandwidth_t {
    u32 wcnt;
    u32 rcnt;
}port_bandwidth;

struct aic_mtop_dev {
        unsigned long reg_base;
        IRQn_Type irq_num;
        uint32_t clk_id;
        uint8_t grp;
        uint8_t prt;
        port_bandwidth port_bw[MTOP_GROUP_MAX * MTOP_PORT_MAX];
        void (*callback)(struct aic_mtop_dev *phandle, void *arg);
        void *arg;
};

int hal_mtop_init(struct aic_mtop_dev *phandle);
int hal_mtop_deinit(struct aic_mtop_dev *phandle);
void hal_mtop_enable(struct aic_mtop_dev *phandle);
void hal_mtop_irq_enable(struct aic_mtop_dev *phandle);
irqreturn_t hal_mtop_irq_handler(int irq_num, void *can_handle);
void hal_mtop_set_period_cnt(struct aic_mtop_dev *phandle, uint32_t period_cnt);
void hal_mtop_attach_callback(struct aic_mtop_dev *phandle, void *callback, void *arg);
void hal_mtop_detach_callback(struct aic_mtop_dev *phandle);






#endif
