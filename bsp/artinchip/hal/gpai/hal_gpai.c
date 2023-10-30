/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "hal_gpai.h"
#include "aic_hal_clk.h"

/* Register definition of GPAI Controller */
#define GPAI_MCR            0x000

#ifdef AIC_GPAI_DRV_V10
#define GPAI_INTR           0x004
#endif
#ifdef AIC_GPAI_DRV_V11
#define GPAI_INTE           0x004
#define GPAI_INTS           0x008
#endif

#define GPAI_DRQ            0x00C
#define GPAI_CHnCR(n)       (0x100 + (((n) & 0x7) << 6) + 0x00)
#define GPAI_CHnINT(n)      (0x100 + (((n) & 0x7) << 6) + 0x04)
#define GPAI_CHnPSI(n)      (0x100 + (((n) & 0x7) << 6) + 0x08)
#define GPAI_CHnHLAT(n)     (0x100 + (((n) & 0x7) << 6) + 0x10)
#define GPAI_CHnLLAT(n)     (0x100 + (((n) & 0x7) << 6) + 0x14)
#define GPAI_CHnACR(n)      (0x100 + (((n) & 0x7) << 6) + 0x18)
#define GPAI_CHnFCR(n)      (0x100 + (((n) & 0x7) << 6) + 0x20)
#define GPAI_CHnDATA(n)     (0x100 + (((n) & 0x7) << 6) + 0x24)
#define GPAI_VERSION        0xFFC

#define GPAI_MCR_CH0_EN             BIT(8)
#define GPAI_MCR_CH_EN(n)           (GPAI_MCR_CH0_EN << (n))
#define GPAI_MCR_EN                 BIT(0)

#ifdef AIC_GPAI_DRV_V10
#define GPAI_INTR_CH0_INT_FLAG      BIT(16)
#define GPAI_INTR_CH_INT_FLAG(n)    (GPAI_INTR_CH0_INT_FLAG << (n))
#define GPAI_INTR_CH0_INT_EN        BIT(0)
#define GPAI_INTR_CH_INT_EN(n)      (GPAI_INTR_CH0_INT_EN << (n))
#endif
#ifdef AIC_GPAI_DRV_V11
#define GPAI_INTS_CH0_INT_FLAG      BIT(0)
#define GPAI_INTS_CH_INT_FLAG(n)    (GPAI_INTS_CH0_INT_FLAG << (n))
#define GPAI_INTE_CH0_INT_EN        BIT(0)
#define GPAI_INTE_CH_INT_EN(n)      (GPAI_INTE_CH0_INT_EN << (n))
#endif

#define GPAI_CHnCR_SBC_SHIFT        24
#define GPAI_CHnCR_SBC_2_POINTS     1
#define GPAI_CHnCR_SBC_4_POINTS     2
#define GPAI_CHnCR_SBC_8_POINTS     3

#define GPAI_CHnCR_SBC_SHIFT            24
#define GPAI_CHnCR_SBC_MASK             GENMASK(25, 24)
#define GPAI_CHnCR_HIGH_ADC_PRIORITY    BIT(4)
#define GPAI_CHnCR_PERIOD_SAMPLE_EN     BIT(1)
#define GPAI_CHnCR_SINGLE_SAMPLE_EN     BIT(0)

#define GPAI_CHnINT_LLA_RM_FLAG     BIT(23)
#define GPAI_CHnINT_LLA_VALID_FLAG  BIT(22)
#define GPAI_CHnINT_HLA_RM_FLAG     BIT(21)
#define GPAI_CHnINT_HLA_VALID_FLAG  BIT(20)
#define GPAI_CHnINT_FIFO_ERR_FLAG   BIT(17)
#define GPAI_CHnINT_DRDY_FLG        BIT(16)
#define GPAI_CHnINT_LLA_RM_IE       BIT(7)
#define GPAI_CHnINT_LLA_VALID_IE    BIT(6)
#define GPAI_CHnINT_HLA_RM_IE       BIT(5)
#define GPAI_CHnINT_HLA_VALID_IE    BIT(4)
#define GPAI_CHnINT_FIFO_ERR_IE     BIT(1)
#define GPAI_CHnINT_DAT_RDY_IE      BIT(0)

#define GPAI_CHnLAT_HLLA_RM_THD_SHIFT   16
#define GPAI_CHnLAT_HLLA_RM_THD_MASK    GENMASK(27, 16)
#define GPAI_CHnLAT_HLLA_THD_MASK       GENMASK(11, 0)
#define GPAI_CHnLAT_HLA_RM_THD(n)       ((n) - 30)
#define GPAI_CHnLAT_LLA_RM_THD(n)       ((n) + 30)

#define GPAI_CHnACR_DISCARD_NOR_DAT     BIT(6)
#define GPAI_CHnACR_DISCARD_LL_DAT      BIT(5)
#define GPAI_CHnACR_DISCARD_HL_DAT      BIT(4)
#define GPAI_CHnACR_LLA_EN              BIT(1)
#define GPAI_CHnACR_HLA_EN              BIT(0)

#define GPAI_CHnFCR_DAT_CNT_MAX(ch)     ((ch) > 1 ? 0x8 : 0x40)
#define GPAI_CHnFCR_DAT_CNT_SHIFT       24
#define GPAI_CHnFCR_DAT_CNT_MASK        GENMASK(30, 24)
#define GPAI_CHnFCR_UF_STS              BIT(18)
#define GPAI_CHnFCR_OF_STS              BIT(17)
#define GPAI_CHnFCR_DAT_RDY_THD_SHIFT   8
#define GPAI_CHnFCR_DAT_RDY_THD_MASK    GENMASK(15, 8)
#define GPAI_CHnFCR_FLUSH               BIT(0)

// TODO: irq_handle() should get 'struct aic_gpai_ch *' from 'void *arg'
extern struct aic_gpai_ch aic_gpai_chs[];
static u32 aic_gpai_ch_num = 0; // the number of available channel

static inline void gpai_writel(u32 val, int reg)
{
    writel(val, GPAI_BASE + reg);
}

static inline u32 gpai_readl(int reg)
{
    return readl(GPAI_BASE + reg);
}

// TODO: Add the transform algorithm, offered by SD later
static s32 gpai_data2vol(u16 data)
{
    return data;
}

static u16 gpai_vol2data(s32 vol)
{
    return vol;
}

static u32 gpai_ms2itv(u32 pclk_rate, u32 ms)
{
    u32 tmp = 0;

    tmp = pclk_rate / 1000;
    tmp *= ms;
    return tmp;
}

static void gpai_reg_enable(int offset, int bit, int enable)
{
    int tmp = gpai_readl(offset);

    if (enable)
        tmp |= bit;
    else
        tmp &= ~bit;

    gpai_writel(tmp, offset);
}

void aich_gpai_enable(int enable)
{
    gpai_reg_enable(GPAI_MCR, GPAI_MCR_EN, enable);
}

void aich_gpai_ch_enable(u32 ch, int enable)
{
    gpai_reg_enable(GPAI_MCR, GPAI_MCR_CH_EN(ch), enable);
}

static void gpai_int_enable(u32 ch, u32 enable, u32 detail)
{
    u32 val = 0;

#ifdef AIC_GPAI_DRV_V10
    val = gpai_readl(GPAI_INTR);
    if (enable) {
        val |= GPAI_INTR_CH_INT_EN(ch);
        gpai_writel(detail, GPAI_CHnINT(ch));
    } else {
        val &= ~GPAI_INTR_CH_INT_EN(ch);
        gpai_writel(0, GPAI_CHnINT(ch));
    }
    gpai_writel(val, GPAI_INTR);
#endif

#ifdef AIC_GPAI_DRV_V11
    val = gpai_readl(GPAI_INTE);
    if (enable) {
        val |= GPAI_INTE_CH_INT_EN(ch);
        gpai_writel(detail, GPAI_CHnINT(ch));
    } else {
        val &= ~GPAI_INTE_CH_INT_EN(ch);
        gpai_writel(0, GPAI_CHnINT(ch));
    }
    gpai_writel(val, GPAI_INTE);
#endif
}

static void gpai_fifo_init(u32 ch)
{
    u32 val = 0;

    val = 1 << GPAI_CHnFCR_DAT_RDY_THD_SHIFT;
    gpai_writel(val, GPAI_CHnFCR(ch));
}

static void gpai_fifo_flush(u32 ch)
{
    u32 val = gpai_readl(GPAI_CHnFCR(ch));

    if (val & GPAI_CHnFCR_UF_STS)
        pr_err("ch%d FIFO is Underflow!%#x\n", ch, val);
    if (val & GPAI_CHnFCR_OF_STS)
        pr_err("ch%d FIFO is Overflow!%#x\n", ch, val);

    gpai_writel(val | GPAI_CHnFCR_FLUSH, GPAI_CHnFCR(ch));
}

static void gpai_single_mode(u32 ch)
{
    u32 val = 0;

    val = gpai_readl(GPAI_CHnCR(ch));
    val |= GPAI_CHnCR_SBC_8_POINTS << GPAI_CHnCR_SBC_SHIFT
        | GPAI_CHnCR_SINGLE_SAMPLE_EN;
    gpai_writel(val, GPAI_CHnCR(ch));

    gpai_int_enable(ch, 1,
            GPAI_CHnINT_DAT_RDY_IE | GPAI_CHnINT_FIFO_ERR_IE);
}

/* Only in period mode, HLA and LLA are available */
static void gpai_period_mode(struct aic_gpai_ch *chan, u32 pclk)
{
    u32 val, acr = 0, ch = chan->id;
    u32 detail = GPAI_CHnINT_DAT_RDY_IE | GPAI_CHnINT_FIFO_ERR_IE;

    if (chan->hla_enable) {
        detail |= GPAI_CHnINT_HLA_RM_IE | GPAI_CHnINT_HLA_VALID_IE;
        val = ((gpai_vol2data(chan->hla_rm_thd) << GPAI_CHnLAT_HLLA_RM_THD_SHIFT)
            & GPAI_CHnLAT_HLLA_RM_THD_MASK)
            | (gpai_vol2data(chan->hla_thd) & GPAI_CHnLAT_HLLA_THD_MASK);
        gpai_writel(val, GPAI_CHnHLAT(ch));
        acr |= GPAI_CHnACR_HLA_EN;
    }

    if (chan->lla_enable) {
        detail |= GPAI_CHnINT_LLA_VALID_IE | GPAI_CHnINT_LLA_RM_IE;
        val = ((gpai_vol2data(chan->lla_rm_thd) << GPAI_CHnLAT_HLLA_RM_THD_SHIFT)
            & GPAI_CHnLAT_HLLA_RM_THD_MASK)
            | (gpai_vol2data(chan->lla_thd) & GPAI_CHnLAT_HLLA_THD_MASK);
        gpai_writel(val, GPAI_CHnLLAT(ch));
        acr |= GPAI_CHnACR_LLA_EN;
    }

    gpai_int_enable(ch, 1, detail);

    gpai_writel(acr, GPAI_CHnACR(ch));

    val = gpai_ms2itv(pclk, chan->smp_period);
    gpai_writel(val, GPAI_CHnPSI(ch));

    val = gpai_readl(GPAI_CHnCR(ch));
    val |= GPAI_CHnCR_SBC_8_POINTS << GPAI_CHnCR_SBC_SHIFT
        | GPAI_CHnCR_PERIOD_SAMPLE_EN;
    gpai_writel(val, GPAI_CHnCR(ch));
}

int aich_gpai_ch_init(struct aic_gpai_ch *chan, u32 pclk)
{
    aich_gpai_ch_enable(chan->id, 1);
    gpai_fifo_init(chan->id);
    if (chan->mode == AIC_GPAI_MODE_PERIOD)
        gpai_period_mode(chan, pclk);

    /* For single mode, should init the channel in .read() */
    return 0;
}

int aich_gpai_read(struct aic_gpai_ch *chan, u32 *val, u32 timeout)
{
    int ret = 0;
    u32 ch = chan->id;

    if (!chan->available) {
        hal_log_err("Ch%d is unavailable!\n", chan->id);
        return -ENODATA;
    }

#ifndef CONFIG_ARTINCHIP_ADCIM_DM
    if (chan->mode == AIC_GPAI_MODE_PERIOD) {
        *val = gpai_data2vol(chan->latest_data);
        return 0;
    }
#endif

    aich_gpai_ch_enable(ch, 1);
    gpai_single_mode(ch);

    ret = aicos_sem_take(chan->complete, timeout);
    if (ret < 0) {
        hal_log_err("Ch%d read timeout!\n", ch);
        aich_gpai_ch_enable(ch, 0);
        return -ETIMEDOUT;
    }
    // aich_gpai_ch_enable(ch, 0);

    if (val)
        *val = gpai_data2vol(chan->latest_data);

    return 0;
}

void aich_gpai_status_show(struct aic_gpai_ch *chan)
{
    int mcr = gpai_readl(GPAI_MCR);
    int version = gpai_readl(GPAI_VERSION);

    printf("In GPAI V%d.%02d:\n"
               "Ch Mode Enable Value  LTA  HTA\n"
               "%2d %4s %6d %5d %4d %4d\n",
               version >> 8, version & 0xff,
               chan->id, chan->mode ? "P" : "S",
               mcr & GPAI_MCR_CH_EN(chan->id) ? 1 : 0,
               chan->latest_data, chan->lla_thd, chan->hla_thd);
}

static int aic_gpai_read_ch(struct aic_gpai_ch *chan)
{
    u32 i, ch = chan->id;
    u32 cnt = (gpai_readl(GPAI_CHnFCR(ch)) & GPAI_CHnFCR_DAT_CNT_MASK)
            >> GPAI_CHnFCR_DAT_CNT_SHIFT;

    if (unlikely(cnt == 0 || cnt > GPAI_CHnFCR_DAT_CNT_MAX(ch))) {
        pr_err("ch%d invalid data count %d\n", ch, cnt);
        return -1;
    }

    /* Just record the last data as to now */
    for (i = 0; i < cnt; i++) {
        chan->latest_data = gpai_readl(GPAI_CHnDATA(ch));
        // pr_debug("ch%d data%d %d\n", ch, i, chan->latest_data);
    }
    pr_debug("There are %d data ready in ch%d, last %d\n", cnt,
        ch, chan->latest_data);

    return 0;
}

struct aic_gpai_ch *hal_gpai_ch_is_valid(u32 ch)
{
    s32 i;

    if (ch >= AIC_GPAI_CH_NUM) {
        pr_err("Invalid channel %d\n", ch);
        return NULL;
    }

    for (i = 0; i < aic_gpai_ch_num; i++) {
        if (aic_gpai_chs[i].id != ch)
            continue;

        if (aic_gpai_chs[i].available)
            return &aic_gpai_chs[i];
        else
            break;
    }
    pr_warn("Ch%d is unavailable!\n", ch);
    return NULL;
}

irqreturn_t aich_gpai_isr(int irq, void *arg)
{
    u32 ch_flag = 0, ch_int = 0;
    int i;
    struct aic_gpai_ch *chan = NULL;

#ifdef AIC_GPAI_DRV_V10
    ch_flag = gpai_readl(GPAI_INTR);
#endif
#ifdef AIC_GPAI_DRV_V11
    ch_flag = gpai_readl(GPAI_INTS);
#endif

    for (i = 0; i < AIC_GPAI_CH_NUM; i++) {
#ifdef AIC_GPAI_DRV_V10
        if (!(ch_flag & GPAI_INTR_CH_INT_FLAG(i)))
            continue;
#endif
#ifdef AIC_GPAI_DRV_V11
        if (!(ch_flag & GPAI_INTS_CH_INT_FLAG(i)))
            continue;
#endif

        chan = hal_gpai_ch_is_valid(i);
        if (!chan)
            return IRQ_NONE;

        ch_int = gpai_readl(GPAI_CHnINT(i));
        gpai_writel(ch_int, GPAI_CHnINT(i));
        if (ch_int & GPAI_CHnINT_DRDY_FLG) {
            aic_gpai_read_ch(chan);
            chan->irq_count++;
            if (chan->mode == AIC_GPAI_MODE_SINGLE)
                aicos_sem_give(chan->complete);
        }

        if (ch_int & GPAI_CHnINT_LLA_VALID_FLAG)
            pr_warn("LLA: ch%d %d!\n", i, chan->latest_data);
        if (ch_int & GPAI_CHnINT_LLA_RM_FLAG)
            pr_warn("LLA removed: ch%d %d\n", i,
                 chan->latest_data);
        if (ch_int & GPAI_CHnINT_HLA_VALID_FLAG)
            pr_warn("HLA: ch%d %d!\n", i, chan->latest_data);
        if (ch_int & GPAI_CHnINT_HLA_RM_FLAG)
            pr_warn("HLA removed: ch%d %d\n", i,
                 chan->latest_data);
        if (ch_int & GPAI_CHnINT_FIFO_ERR_FLAG)
            gpai_fifo_flush(i);
    }
    pr_debug("IRQ flag %#x, detail %#x\n", ch_flag, ch_int);

    return IRQ_HANDLED;
}

s32 hal_gpai_clk_init(void)
{
    s32 ret = 0;

    ret = hal_clk_enable(CLK_GPAI);
    if (ret < 0) {
        pr_err("GPAI clk enable failed!");
        return -1;
    }

    ret = hal_clk_enable_deassertrst(CLK_GPAI);
    if (ret < 0) {
        pr_err("GPAI reset deassert failed!");
        return -1;
    }

    return ret;
}

void hal_gpai_clk_get(struct aic_gpai_ch *chan)
{
    chan->pclk_rate = hal_clk_get_freq(hal_clk_get_parent(CLK_GPAI));
}

void hal_gpai_set_ch_num(u32 num)
{
    aic_gpai_ch_num = num;
}
