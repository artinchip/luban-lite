/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

#include "aic_core.h"
#include "hal_psadc.h"

/* Register definition of PSADC Controller */
#define PSADC_MCR               0x000
#define PSADC_TCR               0x004
#define PSADC_NODE1             0x008
#define PSADC_NODE2             0x00C
#define PSADC_MSR               0x010
#define PSADC_CALCSR            0x014
#define PSADC_FILTER            0x01C
#define PSADC_Q1FCR             0x020
#define PSADC_Q2FCR             0x024
#define PSADC_Q1FDR             0x040
#define PSADC_Q2FDR             0x080
#define PSADC_VERSION           0xFFC

#define PSADC_MCR_Q1_TRIGS      BIT(22)
#define PSADC_MCR_Q1_INTE       BIT(18)
#define PSADC_MCR_QUE_COMB      BIT(1)
#define PSADC_MCR_EN            BIT(0)

#define PSADC_MSR_Q1_FERR           BIT(2)
#define PSADC_MSR_Q1_INT            BIT(0)

#define PSADC_Q1FCR_FIFO_DRTH_SHIFT 11
#define PSADC_Q1FCR_UF_STS          BIT(17)
#define PSADC_Q1FCR_OF_STS          BIT(16)
#define PSADC_Q1FCR_FIFO_ERRIE      BIT(3)
#define PSADC_Q1FCR_FIFO_FLUSH      BIT(0)

#define PSADC_Q1FDR_CHNUM_SHIFT     12
#define PSADC_Q1FDR_DATA_MASK       GENMASK(11, 0)
#define PSADC_Q1FDR_DATA            BIT(0)

extern struct aic_psadc_ch aic_psadc_chs[];
static u32 aic_psadc_ch_num = 0; // the number of available channel
static u16 aic_psadc_ch_data = 0;

static inline void psadc_writel(u32 val, int reg)
{
    writel(val, PSADC_BASE + reg);
}

static inline u32 psadc_readl(int reg)
{
    return readl(PSADC_BASE + reg);
}

static s32 psadc_data2vol(u16 data)
{
    return data;
}

static void psadc_reg_enable(int offset, int bit, int enable)
{
    int tmp = psadc_readl(offset);

    if (enable)
        tmp |= bit;
    else
        tmp &= ~bit;

    psadc_writel(tmp, offset);
}

void aich_psadc_enable(int enable)
{
    psadc_reg_enable(PSADC_MCR, PSADC_MCR_EN, enable);
}

void aic_psadc_single_queue_mode(int enable)
{
    psadc_reg_enable(PSADC_MCR, PSADC_MCR_QUE_COMB, enable);
}

void aich_psadc_qc_irq_enable(int enable)
{
    psadc_reg_enable(PSADC_MCR, PSADC_MCR_Q1_INTE, enable);
}

static void psadc_fifo_flush(u32 ch)
{
    u32 val = psadc_readl(PSADC_Q1FCR);

    if (val & PSADC_Q1FCR_UF_STS)
        pr_err("ch%d FIFO is Underflow!%#x\n", ch, val);
    if (val & PSADC_Q1FCR_OF_STS)
        pr_err("ch%d FIFO is Overflow!%#x\n", ch, val);

    psadc_writel(val | PSADC_Q1FCR_FIFO_FLUSH, PSADC_Q1FCR);
}

static void psadc_fifo_init(void)
{
    u32 val = 0;

    val = 1 << PSADC_Q1FCR_FIFO_DRTH_SHIFT;
    psadc_writel(val, PSADC_Q1FCR);
    psadc_writel(val, PSADC_Q2FCR);
}

int  aich_psadc_ch_init(struct aic_psadc_ch *chan, u32 pclk)
{
    psadc_fifo_init();
    psadc_writel(chan->id, PSADC_NODE1);

    psadc_reg_enable(PSADC_MCR, PSADC_MCR_Q1_TRIGS, 1);
    psadc_reg_enable(PSADC_MCR, PSADC_MCR_Q1_INTE, 1);

    return 0;
}



void aich_psadc_status_show(struct aic_psadc_ch *chan)
{
    int version = psadc_readl(PSADC_VERSION);

    printf("In PSADC V%d.%02d:\n"
               "Ch Mode Enable\n"
               "%2d %4s %6d \n",
               version >> 8, version & 0xff,
               chan->id, chan->mode ? "P" : "S",
               chan->available ? 1 : 0);
}

static void aic_psadc_read_ch(u32 ch)
{
    u32 data = psadc_readl(PSADC_Q1FDR) & PSADC_Q1FDR_DATA_MASK;
    aic_psadc_ch_data = data;
}

struct aic_psadc_ch *hal_psadc_ch_is_valid(u32 ch)
{
    s32 i;
    if (ch >= AIC_PSADC_CH_NUM) {
        pr_err("Invalid channel %d\n", ch);
        return NULL;
    }

    for (i = 0; i < aic_psadc_ch_num; i++) {
        if (aic_psadc_chs[i].id != ch)
            continue;

        if (aic_psadc_chs[i].available)
            return &aic_psadc_chs[i];
        else
            break;
    }
    pr_warn("Ch%d is unavailable!\n", ch);
    return NULL;
}

int aich_psadc_read(struct aic_psadc_ch *chan, u32 *val, u32 timeout)
{
    int ret = 0;
    u32 ch = chan->id;

    if (!chan->available) {
        hal_log_err("Ch%d is unavailable!\n", chan->id);
        return -ENODATA;
    }

    ret = aicos_sem_take(chan->complete, timeout);
    if (ret < 0) {
        hal_log_err("Ch%d read timeout!\n", ch);
        aich_psadc_qc_irq_enable(0);
        return -ETIMEDOUT;
    }

    if (val)
        *val = psadc_data2vol(aic_psadc_ch_data);

    return 0;
}

irqreturn_t aich_psadc_isr(int irq, void *arg)
{
    u32 q_flag = 0;
    u32 chan = 0;
    chan = psadc_readl(PSADC_NODE1);
    q_flag = psadc_readl(PSADC_MSR);
    psadc_writel(q_flag, PSADC_MSR);

    if (q_flag | PSADC_MSR_Q1_INT)
        aic_psadc_read_ch(chan);

    if (q_flag | PSADC_MSR_Q1_FERR)
        psadc_fifo_flush(chan);

    return IRQ_HANDLED;
}

void hal_psadc_set_ch_num(u32 num)
{
    aic_psadc_ch_num = num;
}
