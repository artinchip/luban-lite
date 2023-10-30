/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "aic_hal_clk.h"
#include "aic_reboot_reason.h"

#include "hal_rtc.h"
#include "hal_wri.h"

#define AIC_RTC_NAME                    "aic-rtc"

#define RTC_REG_CTL                     (0x0000)
#define RTC_REG_INIT                    (0x0004)
#define RTC_REG_IRQ_EN                  (0x0008)
#define RTC_REG_IRQ_STA                 (0x000C)
#define RTC_REG_TIME0                   (0x0020)
#define RTC_REG_TIME1                   (0x0024)
#define RTC_REG_TIME2                   (0x0028)
#define RTC_REG_TIME3                   (0x002C)
#define RTC_REG_ALARM0                  (0x0030)
#define RTC_REG_ALARM1                  (0x0034)
#define RTC_REG_ALARM2                  (0x0038)
#define RTC_REG_ALARM3                  (0x003C)
#define RTC_REG_CALI0                   (0x0040)
#define RTC_REG_CALI1                   (0x0044)
#define RTC_REG_ANALOG0                 (0x0050)
#define RTC_REG_ANALOG1                 (0x0054)
#define RTC_REG_ANALOG2                 (0x0058)
#define RTC_REG_ANALOG3                 (0x005C)
#define RTC_REG_WR_EN                   (0x00FC)
#define RTC_REG_SYSBAK                  (0x0100)
#define RTC_REG_TCNT                    (0x0800)
#define RTC_REG_32K_DET                 (0x0804)
#define RTC_REG_VER                     (0x08FC)

#define RTC_CTL_IO_OUTPUT_ENABLE        1
#define RTC_CTL_IO_ALARM_OUTPUT         2
#define RTC_CTL_IO_32K_CLK_OUTPUT       3
#define RTC_CTL_IO_SEL_SHIFT            4
#define RTC_CTL_IO_SEL_MASK             GENMASK(5, 4)
#define RTC_CTL_IO_SEL(x)               (((x) & 0x3) << RTC_CTL_IO_SEL_SHIFT)
#define RTC_CTL_ALARM_EN                BIT(2)
#define RTC_CTL_TCNT_EN                 BIT(0)

#define RTC_IRQ_32K_ERR_EN              BIT(2)
#define RTC_IRQ_ALARM_EN                BIT(0)

#define RTC_IRQ_STA_32K_ERR             BIT(2)
#define RTC_IRQ_STA_ALARM_IO            BIT(1)
#define RTC_IRQ_STA_ALARM               BIT(0)

#define RTC_CAL1_FAST_DIR               BIT(7)

#define RTC_ANA2_XTAL32K_DRV_MASK       GENMASK(3, 0)

#define RTC_32K_DET_EN                  BIT(0)

#define RTC_WR_EN_KEY                   0xAC

#define RTC_DRV_TIMEOUT                 msecs_to_jiffies(2000)
#define RTC_32K_FREQ                    (32 * 1024 * 100)

#define RTC_WRITE_ENABLE    writeb(RTC_WR_EN_KEY, RTC_BASE + RTC_REG_WR_EN)
#define RTC_WRITE_DISABLE   writeb(0, RTC_BASE + RTC_REG_WR_EN)
#define RTC_WRITEB(val, reg) \
        do { \
                RTC_WRITE_ENABLE; \
                writeb((val) & 0xFF, RTC_BASE + (reg)); \
                RTC_WRITE_DISABLE; \
        } while (0)
#define RTC_WRITEL(val, reg) \
        do { \
                RTC_WRITE_ENABLE; \
                writeb((val) & 0xFF, RTC_BASE + (reg)); \
                writeb(((val) >> 8) & 0xFF, RTC_BASE + (reg) + 0x4); \
                writeb(((val) >> 16) & 0xFF, RTC_BASE + (reg) + 0x8); \
                writeb(((val) >> 24) & 0xFF, RTC_BASE + (reg) + 0xC); \
                RTC_WRITE_DISABLE; \
        } while (0)
#define RTC_READB(reg)  readb(RTC_BASE + reg)
#define RTC_READL(reg)  (readb(RTC_BASE + reg) \
                        | (readb((RTC_BASE + reg) + 0x4) << 8) \
                        | (readb((RTC_BASE + reg) + 0x8) << 16) \
                        | (readb((RTC_BASE + reg) + 0xC) << 24))
#define RTC_READ_CNT()  readl(RTC_BASE + RTC_REG_TCNT)

#define RTC_REBOOT_REASON_MASK          GENMASK(7, 4)
#define RTC_REBOOT_REASON_SHIFT         4

struct aic_rtc_dev {
    rtc_callback_t callback;
    u32 clk_rate;
    u32 clk_drv;
    u32 alarm_io;
    u32 cal_fast;
    s32 cal_val;
    s32 inited;
};

static struct aic_rtc_dev aich_rtc = {0};
#if defined(AIC_RTC_DRV_V10) || defined(AIC_RTC_DRV_V11)
static enum aic_reboot_reason g_prev_reason = REBOOT_REASON_INVALID;
#endif

#if 0
static void aic_rtc_set_32k_drv(u8 drv)
{
    u8 val = 0;

    val = RTC_READB(RTC_REG_ANALOG2);
    val &= ~RTC_ANA2_XTAL32K_DRV_MASK;
    val |= drv;
    RTC_WRITEB(val, RTC_REG_ANALOG2);
}
#endif

/* calibrate: (100 * 1024 * 1024 + cval)/( freq / 32) = 1024
 * freq = 100 * actual OSC frequency
 */
void hal_rtc_cali(s32 clk_rate)
{
    /* calibrate: (1024*1024+cval)/(freq/32) = 1024 */
    s32 cval = 0;

    if (clk_rate == RTC_32K_FREQ)
        return; /* It's perfect, need not calibrate */

    cval = 100 * 1024 * 1024 - clk_rate * 32;
    if (clk_rate > RTC_32K_FREQ)
        cval -= 50;
    else
        cval += 50;

    cval /= 100;
    hal_log_debug("RTC Calibration %d(clk %d)\n", cval, clk_rate);
    if (cval > 0)
        cval |= RTC_CAL1_FAST_DIR << 8;
    else
        cval = -cval;

    RTC_WRITEB(cval >> 8, RTC_REG_CALI1);
    RTC_WRITEB(cval & 0xFF, RTC_REG_CALI0);
}

#if defined(AIC_RTC_DRV_V10) || defined(AIC_RTC_DRV_V11)

void aic_set_reboot_reason(enum aic_reboot_reason r)
{
    u32 cur = 0;
    u8 reason_num = RTC_REBOOT_REASON_MASK >> RTC_REBOOT_REASON_SHIFT;

    cur = RTC_READB(RTC_REG_SYSBAK);
     /* If it's valid already, so ignore the current request */
    if (cur & RTC_REBOOT_REASON_MASK)
        return;

    setbits(r, RTC_REBOOT_REASON_MASK, RTC_REBOOT_REASON_SHIFT, cur);
    RTC_WRITEB(cur, RTC_REG_SYSBAK);

    if (r <= reason_num)
        pr_debug("Set reboot reason %d\n", r);
}

enum aic_reboot_reason aic_get_reboot_reason(void)
{
    u32 cur = 0;
    enum aic_warm_reset_type wr = aic_wr_type_get();

    if (g_prev_reason == REBOOT_REASON_INVALID) {
        cur = RTC_READB(RTC_REG_SYSBAK);
        g_prev_reason = getbits(RTC_REBOOT_REASON_MASK,
                                RTC_REBOOT_REASON_SHIFT, cur);
        if (g_prev_reason) {
            /* Clear the previous state */
            clrbits(RTC_REBOOT_REASON_MASK, cur);
            RTC_WRITEB(cur, RTC_REG_SYSBAK);
        }
    }

    return aic_judge_reboot_reason(wr, g_prev_reason);
}

/* Convert and print the time info from a GTC counter.
   It's useful when print the time before console is ready. */
void aic_show_gtc_time(char *tag, u32 val)
{
    u32 sec, msec;
    u32 cnt = val;

    if (!cnt)
        cnt = readl(GTC_BASE + 0x8);

    sec = cnt / 4000000;
    msec = (cnt % 4000000) / 4 / 1000;
    printf("\n%s time: %d.%03d sec\n", tag ? tag : "GTC", sec, msec);
}

void aic_show_startup_time(void)
{
    u32 cnt = readl(GTC_BASE + 0x8);

    aic_show_gtc_time("Startup", cnt);
}

#endif

void hal_rtc_enable(u32 enable)
{
    u8 val = 0;

    val = RTC_READB(RTC_REG_CTL);
    if (enable)
        val |= RTC_CTL_TCNT_EN;
    else
        val &= ~RTC_CTL_TCNT_EN;
    RTC_WRITEB(val, RTC_REG_CTL);
}

void hal_rtc_read_time(u32 *sec)
{
    *sec = RTC_READ_CNT();
}

void hal_rtc_set_time(u32 sec)
{
    hal_rtc_enable(0);
    RTC_WRITEL(sec, RTC_REG_TIME0);
    RTC_WRITEB(1, RTC_REG_INIT);
    hal_rtc_enable(1);
}

s32 hal_rtc_read_alarm(u32 *sec)
{
    /* get alarm target time */
    *sec = RTC_READL(RTC_REG_ALARM0);

    return RTC_READB(RTC_REG_IRQ_EN) & RTC_IRQ_ALARM_EN;
}

static int hal_rtc_alarm_irq_enable(unsigned int enabled)
{
    RTC_WRITEB(enabled ? (RTC_IRQ_32K_ERR_EN | RTC_IRQ_ALARM_EN) : 0,
           RTC_REG_IRQ_EN);
    return 0;
}

void hal_rtc_alarm_io_output(void)
{
    u32 val = 0;

    val = RTC_READB(RTC_REG_CTL);
    val |= RTC_CTL_IO_SEL(RTC_CTL_IO_ALARM_OUTPUT);
    RTC_WRITEB(val, RTC_REG_CTL);
}

void hal_rtc_32k_clk_output(void)
{
    u32 val = 0;

    val = RTC_READB(RTC_REG_CTL);
    val |= RTC_CTL_IO_SEL(RTC_CTL_IO_32K_CLK_OUTPUT);
    RTC_WRITEB(val, RTC_REG_CTL);
}

void hal_rtc_set_alarm(u32 sec)
{
    u32 value;

    /* First, disable alarm */
    value = RTC_READB(RTC_REG_CTL);
    value &= ~RTC_CTL_ALARM_EN;
    RTC_WRITEB(value, RTC_REG_CTL);

    /* set alarm time value */
    RTC_WRITEL(sec, RTC_REG_ALARM0);

    /* Then, enable alarm */
    value |= RTC_CTL_ALARM_EN;
    RTC_WRITEB(value, RTC_REG_CTL);

    /* enable alarm irq */
    hal_rtc_alarm_irq_enable(1);
}

s32 hal_rtc_register_callback(rtc_callback_t callback)
{
    if (callback == NULL) {
        hal_log_err("Invalid callback function!\n");
        return -1;
    }

    aich_rtc.callback = callback;
    return 0;
}

irqreturn_t hal_rtc_irq(int irq, void *arg)
{
    u8 val = 0;

    val = RTC_READB(RTC_REG_IRQ_STA);
    RTC_WRITEB(val, RTC_REG_IRQ_STA);
    hal_log_debug("IRQ status %#x\n", val);

    if (val & RTC_IRQ_STA_32K_ERR)
        hal_log_err("The 32K clk is not fast enough\n");

    if (val & RTC_IRQ_STA_ALARM) {
        if (aich_rtc.callback)
            aich_rtc.callback();
    }
    return IRQ_HANDLED;
}

s32 hal_rtc_init(void)
{
    s32 ret = 0;

    if (aich_rtc.inited) {
        hal_log_info("RTC is already inited.\n");
        return 0;
    }

    ret = hal_clk_enable(CLK_RTC);
    if (ret < 0) {
        hal_log_err("RTC clk enable failed!\n");
        return -1;
    }

    /* Check & clean poweroff alarm status */
    ret = RTC_READB(RTC_REG_IRQ_STA);
    if (ret) {
        hal_log_debug("IRQ_STA is %#x\n", ret);
        RTC_WRITEB(ret, RTC_REG_IRQ_STA);
        if (ret & RTC_IRQ_STA_ALARM_IO)
            hal_log_info("Powered by RTC alarm.\n");
    }

    aich_rtc.inited = 1;
    hal_rtc_enable(1);
    return 0;
}

s32 hal_rtc_deinit(void)
{
    if (!aich_rtc.inited) {
        hal_log_warn("RTC is not inited.\n");
        return -1;
    }

    hal_clk_disable(CLK_RTC);

    aich_rtc.inited = 0;
    return 0;
}
