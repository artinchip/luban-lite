/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_adcim.h"

/* Register of ADCIM */
#define ADCIM_MCSR       0x000
#define ADCIM_CALCSR     0x004
#define ADCIM_FIFOSTS    0x008
#define ADCIM_VERSION    0xFFC

#define ADCIM_MCSR_BUSY                 BIT(16)
#define ADCIM_MCSR_SEMFLAG_SHIFT        8
#define ADCIM_MCSR_SEMFLAG_MASK         GENMASK(15, 8)
#define ADCIM_MCSR_CHN_MASK             GENMASK(3, 0)
#define ADCIM_CALCSR_CALVAL_UPD         BIT(31)
#define ADCIM_CALCSR_CALVAL_SHIFT       16
#define ADCIM_CALCSR_CALVAL_MASK        GENMASK(27, 16)
#define ADCIM_CALCSR_ADC_ACQ_SHIFT      8
#define ADCIM_CALCSR_ADC_ACQ_MASK       GENMASK(15, 8)
#define ADCIM_CALCSR_DCAL_MASK          BIT(1)
#define ADCIM_CALCSR_CAL_ENABLE         BIT(0)
#define ADCIM_FIFOSTS_ADC_ARBITER_IDLE  BIT(6)
#define ADCIM_FIFOSTS_FIFO_ERR          BIT(5)
#define ADCIM_FIFOSTS_CTR_MASK          GENMASK(4, 0)
#define ADCDM_CAL_ADC_STANDARD_VAL      0x800
#define ADCIM_CAL_ADC_VAL_OFFSET        0X32

#ifdef AIC_ADCIM_DM_DRV
#define ADCDM_RTP_CFG   0x03F0
#define ADCDM_RTP_STS   0x03F4
#define ADCDM_SRAM_CTL  0x03F8
#define ADCDM_SRAM_BASE 0x0400

#define ADCDM_RTP_CAL_VAL_SHIFT     16
#define ADCDM_RTP_CAL_VAL_MASK      GENMASK(27, 16)
#define ADCDM_RTP_PDET              BIT(0)

#define ADCDM_RTP_DRV_SHIFT         4
#define ADCDM_RTP_DRV_MASK          GENMASK(7, 4)
#define ADCDM_RTP_VPSEL_SHIFT       2
#define ADCDM_RTP_VPSEL_MASK        GENMASK(3, 2)
#define ADCDM_RTP_VNSEL_MASK        GENMASK(1, 0)

#define ADCDM_SRAM_CLR_SHIFT        16
#define ADCDM_SRAM_CLR(n)           (1 << ((n) + ADCDM_SRAM_CLR_SHIFT))
#define ADCDM_SRAM_MODE_SHIFT       8
#define ADCDM_SRAM_MODE             BIT(8)
#define ADCDM_SRAM_SEL_MASK         GENMASK(3, 0)
#define ADCDM_SRAM_SEL(n)           (n)

#define ADCDM_SRAM_SIZE             (512 * 4)
#define ADCDM_CHAN_NUM              16

enum adcdm_sram_mode {
    ADCDM_NORMAL_MODE = 0,
    ADCDM_DEBUG_MODE = 1
};
#endif

struct adcim_dev {
#ifdef AIC_ADCIM_DM_DRV
    u32 dm_cur_chan;
#endif
    int usr_cnt;
};

#ifdef AIC_ADCIM_DM_DRV
static struct adcim_dev g_adcim_dev = {0};
#endif

static inline void adcim_writel(u32 val, int reg)
{
    writel(val, ADCIM_BASE + reg);
}

static inline u32 adcim_readl(int reg)
{
    return readl(ADCIM_BASE + reg);
}

int hal_adcim_calibration_set(unsigned int val)
{
    int cal;

    if (val > 2048) {
        pr_err("The calibration value %d is too big\n", val);
        return -EINVAL;
    }

    cal = adcim_readl(ADCIM_CALCSR);
    cal = (cal & ~ADCIM_CALCSR_CALVAL_MASK)
        | (val << ADCIM_CALCSR_CALVAL_SHIFT);
    cal = cal | ADCIM_CALCSR_CALVAL_UPD;
    adcim_writel(cal, ADCIM_CALCSR);

    return 0;
}

int hal_adcim_auto_calibration(int adc_val, int st_voltage, int scale,
                               int adc_max_val)
{
    u32 flag = 1;
    u32 data = 0;
    int new_voltage = 0;

    adcim_writel(0x083F2f03, ADCIM_CALCSR);//auto cal
    do {
        flag = adcim_readl(ADCIM_CALCSR) & 0x00000001;
    } while (flag);

    data = (adcim_readl(ADCIM_CALCSR) >> 16) & 0xfff;
    if (adc_val) {
        new_voltage = (adc_val + ADCDM_CAL_ADC_STANDARD_VAL - data + ADCIM_CAL_ADC_VAL_OFFSET) * st_voltage * scale / adc_max_val;
    }

    return new_voltage;
}

void adcim_status_show(void)
{
    int mcsr;
    int fifo;
    int version;
#ifdef AIC_ADCIM_DM_DRV
    int rtp_cfg, rtp_sts, sram_ctl;
#endif

    mcsr = adcim_readl(ADCIM_MCSR);
    fifo = adcim_readl(ADCIM_FIFOSTS);
    version = adcim_readl(ADCIM_VERSION);

#ifdef AIC_ADCIM_DM_DRV
    rtp_cfg = adcim_readl(ADCDM_RTP_CFG);
    rtp_sts = adcim_readl(ADCDM_RTP_STS);
    sram_ctl = adcim_readl(ADCDM_SRAM_CTL);
#endif

    hal_log_info("In ADCIM V%d.%02d:\n"
               "Busy state: %d\n"
               "Semflag: %d\n"
               "Current Channel: %d\n"
               "ADC Arbiter Idel: %d\n"
               "FIFO Error: %d\n"
               "FIFO Counter: %d\n"
#ifdef AIC_ADCIM_DM_DRV
               "\nIn DM:\nMode: %s, Current channel: %d/%ld\n"
               "Calibration val: %ld\n"
               "RTP PDET: %ld, DRV: %ld, VPSEL: %ld, VNSEL: %ld\n"
#endif
               , version >> 8, version & 0xff,
               (mcsr & ADCIM_MCSR_BUSY) ? 1 : 0,
               (mcsr & ADCIM_MCSR_SEMFLAG_MASK)
                >> ADCIM_MCSR_SEMFLAG_SHIFT,
               mcsr & ADCIM_MCSR_CHN_MASK,
               fifo & ADCIM_FIFOSTS_ADC_ARBITER_IDLE ? 1 : 0,
               fifo & ADCIM_FIFOSTS_FIFO_ERR ? 1 : 0,
               fifo & ADCIM_FIFOSTS_CTR_MASK
#ifdef AIC_ADCIM_DM_DRV
               , sram_ctl & ADCDM_SRAM_MODE ? "Debug" : "Normal",
               g_adcim_dev.dm_cur_chan, sram_ctl & ADCDM_SRAM_SEL_MASK,
               (rtp_cfg & ADCDM_RTP_CAL_VAL_MASK)
                >> ADCDM_RTP_CAL_VAL_SHIFT,
               rtp_cfg & ADCDM_RTP_PDET,
               (rtp_sts & ADCDM_RTP_DRV_MASK) >> ADCDM_RTP_DRV_SHIFT,
               (rtp_sts & ADCDM_RTP_VPSEL_MASK)
                >> ADCDM_RTP_VPSEL_SHIFT,
               rtp_sts & ADCDM_RTP_VNSEL_MASK
#endif
               );
}

void adcim_calibration_show(void)
{
    int cal;

    cal = adcim_readl(ADCIM_CALCSR);

    pr_info("Calibration Enable: %d\n Current value: %d\nADC ACQ: %d\n",
           (cal & ADCIM_CALCSR_CAL_ENABLE) ? 0 : 1,
           (cal & ADCIM_CALCSR_CALVAL_MASK) >> ADCIM_CALCSR_CALVAL_SHIFT,
           (cal & ADCIM_CALCSR_ADC_ACQ_MASK) >> ADCIM_CALCSR_ADC_ACQ_SHIFT);
}

#ifdef AIC_ADCIM_DM_DRV
void hal_dm_chan_show(void)
{
    pr_info("Current chan: %d/%ld\n", g_adcim_dev.dm_cur_chan,
               adcim_readl(ADCDM_SRAM_CTL) & ADCDM_SRAM_SEL_MASK);
}

s32 hal_dm_chan_store(u32 val)
{
    int ret;

    if (val >= ADCDM_CHAN_NUM) {
        hal_log_err("Invalid channel number %u\n", val);
        return 0;
    }

    g_adcim_dev.dm_cur_chan = val;
    ret = adcim_readl(ADCDM_SRAM_CTL);
    ret &= ~ADCDM_SRAM_SEL_MASK;
    ret |= val;
    adcim_writel(ret, ADCDM_SRAM_CTL);

    return 0;
}

void hal_adcdm_rtp_down_store(u32 val)
{
    int ret;

    ret = adcim_readl(ADCDM_RTP_CFG);
    if (val)
        ret &= ~ADCDM_RTP_PDET;
    else
        ret |= ADCDM_RTP_PDET;
    adcim_writel(ret, ADCDM_RTP_CFG);
}

static void adcdm_sram_clear(u32 chan)
{
    u32 val = 0;

    val = adcim_readl(ADCDM_SRAM_CTL);
    val |= ADCDM_SRAM_CLR(chan);
    adcim_writel(val, ADCDM_SRAM_CTL);

    val &= ~ADCDM_SRAM_CLR(chan);
    adcim_writel(val, ADCDM_SRAM_CTL);
}

static void adcdm_sram_mode(enum adcdm_sram_mode mode)
{
    u32 val = 0;

    val = adcim_readl(ADCDM_SRAM_CTL);
    if (mode)
        val |= mode << ADCDM_SRAM_MODE_SHIFT;
    else
        val &= ~ADCDM_SRAM_MODE;
    adcim_writel(val, ADCDM_SRAM_CTL);
}

static void adcdm_sram_select(u32 chan)
{
    u32 val = 0;

    val = adcim_readl(ADCDM_SRAM_CTL);
    val &= ~ADCDM_SRAM_SEL_MASK;
    val |= chan;
    adcim_writel(val, ADCDM_SRAM_CTL);
}

ssize_t hal_adcdm_sram_write(int *buf, u32 offset, size_t count)
{
    int i;

    if (count + offset > ADCDM_SRAM_SIZE)
        return 0;
    hal_adcim_probe();
    adcdm_sram_mode(ADCDM_DEBUG_MODE);
    adcdm_sram_select(g_adcim_dev.dm_cur_chan);

    for (i = 0; i < count; i++) {
        adcim_writel(buf[i], ADCDM_SRAM_BASE + i * 4);
    }

    adcdm_sram_mode(ADCDM_NORMAL_MODE);
    for (i = 0; i < ADCDM_CHAN_NUM; i++)
        adcdm_sram_clear(i);

    return count;
}

#endif

s32 hal_adcim_probe(void)
{
    s32 ret = 0;
    static s32 inited = 0;

    if (inited) {
        hal_log_info("ADCIM is already inited\n");
        return 0;
    }

#ifdef AIC_ADCIM_DRV_V11
    ret = hal_clk_set_freq(CLK_ADCIM, 48000000);
    if (ret < 0) {
        hal_log_err("Failed to set ADCIM clk\n");
        return -1;
    }
#endif

    ret = hal_clk_enable(CLK_ADCIM);
    if (ret < 0) {
        hal_log_err("ADCIM clk enable failed!\n");
        return -1;
    }

    ret = hal_clk_enable_deassertrst(CLK_ADCIM);
    if (ret < 0) {
        hal_log_err("ADCIM reset deassert failed!\n");
        return -1;
    }

    inited = 1;
    return 0;
}
