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

#include "hal_rtp.h"

/* Register definition for RTP */
#define RTP_MCR             (RTP_BASE + 0x000)
#define RTP_INTR            (RTP_BASE + 0x004)
#define RTP_PDEB            (RTP_BASE + 0x008)
#define RTP_PCTL            (RTP_BASE + 0x00C)
#define RTP_CHCFG           (RTP_BASE + 0x010)
#define RTP_MMSC            (RTP_BASE + 0x014)
#define RTP_FIL             (RTP_BASE + 0x018)
#define RTP_AMSC            (RTP_BASE + 0x01C)
#define RTP_FCR             (RTP_BASE + 0x020)
#define RTP_DATA            (RTP_BASE + 0x024)
#define RTP_DLY             (RTP_BASE + 0x028)
#define RTP_VERSION         (RTP_BASE + 0xFFC)

#define RTP_MCR_PRES_DET_BYPASS BIT(16)
#define RTP_MCR_RISE_STS        BIT(12)
#define RTP_MCR_PRES_DET_EN     BIT(8)
#define RTR_MCR_MODE_SHIFT      4
#define RTR_MCR_MODE_MASK       GENMASK(7, 4)
#define RTP_MCR_EN              BIT(0)

#define RTP_INTR_SCI_FLG        BIT(21)
#define RTP_INTR_DOUR_FLG       BIT(20)
#define RTP_INTR_FIFO_FLG       BIT(19)
#define RTP_INTR_DRDY_FLG       BIT(18)
#define RTP_INTR_RISE_DET_FLG   BIT(17)
#define RTP_INTR_PRES_DET_FLG   BIT(16)
#define RTP_INTR_SCI_IE         BIT(5)
#define RTP_INTR_DOUR_INTEN     BIT(4)
#define RTP_INTR_FIFO_ERR_IE    BIT(3)
#define RTP_INTR_DAT_RDY_IE     BIT(2)
#define RTP_INTR_RISE_DET_IE    BIT(1)
#define RTP_INTR_PRES_DET_IE    BIT(0)

#define RTP_PDEB_SLRDET_DEBDC_SHIFT     16
#define RTP_PDEB_SLRDET_DEBDC_MASK      GENMASK(23, 16)

#define RTP_PCTL_PRES_DET_BYPASS        BIT(16)

#define RTP_MMSC_VREF_MINUS_SEL_SHIFT   22
#define RTP_MMSC_VREF_PLUS_SEL_SHIFT    20
#define RTP_MMSC_XY_DRV_X_PLUS          BIT(19)
#define RTP_MMSC_XY_DRV_Y_PLUS          BIT(18)
#define RTP_MMSC_XY_DRV_X_MINUS         BIT(17)
#define RTP_MMSC_XY_DRV_Y_MINUS         BIT(16)
#define RTP_MMSC_XY_DRV_SHIFT           16
#define RTP_MMSC_SMP_CNT_PER_TRIG_SHIFT 8
#define RTP_MMSC_SMP_CH_SEL_SHIFT       4
#define RTP_MMSC_SMP_TRIG               BIT(0)

#define RTP_FIL_Z_REL_RANGE_SHIFT       28
#define RTP_FIL_X_ABS_RANGE_SHIFT       24
#define RTP_FIL_XY_REL_RANGE_SHIFT      20
#define RTP_FIL_XY_ABS_RANGE_SHIFT      16

#define RTP_AMSC_PERIOD_SAMPLE_INT_SHIFT    12
#define RTP_AMSC_PERIOD_SAMPLE_EN           BIT(1)
#define RTP_AMSC_SINGLE_SAMPLE_EN           BIT(0)

#define RTP_FCR_DAT_CNT_SHIFT           24
#define RTP_FCR_DAT_CNT_MASK            GENMASK(28, 24)
#define RTP_FCR_UF_FLAG                 BIT(18)
#define RTP_FCR_OF_FLAG                 BIT(17)
#define RTP_FCR_DAT_RDY_THD_SHIFT       8
#define RTP_FCR_DAT_RDY_THD_MASK        GENMASK(12, 8)
#define RTP_FCR_UF_IE                   BIT(2)
#define RTP_FCR_OF_IE                   BIT(1)
#define RTP_FCR_FLUSH                   BIT(0)

#define RTP_DATA_CH_NUM_SHIFT           12
#define RTP_DATA_CH_NUM_MASK            GENMASK(13, 12)
#define RTP_DATA_DATA_MASK              GENMASK(11, 0)

enum aic_rtp_vref_minus_sel {
    RTP_VREF_MINUS_2_GND = 0,
    RTP_VREF_MINUS_2_X_MINUS,
    RTP_VREF_MINUS_2_Y_MINUS
};

enum aic_rtp_vref_plus_sel {
    RTP_VREF_PLUS_2_VCC = 0,
    RTP_VREF_PLUS_2_X_PLUS,
    RTP_VREF_PLUS_2_Y_PLUS
};

enum aic_rtp_relative_range {
    RTP_REL_RANGE_DISABLE = 0,
    RTP_REL_RANGE_MAX_1_8,
    RTP_REL_RANGE_MAX_1_16,
    RTP_REL_RANGE_MAX_1_32,
    RTP_REL_RANGE_MAX_1_64,
    RTP_REL_RANGE_MAX_1_128,
    RTP_REL_RANGE_MAX_1_256,
    RTP_REL_RANGE_MAX_1_512
};

enum aic_rtp_absolute_range {
    RTP_ABS_RANGE_DISABLE = 0,
    RTP_ABS_RANGE_MAX_2_9,
    RTP_ABS_RANGE_MAX_2_8,
    RTP_ABS_RANGE_MAX_2_7,
    RTP_ABS_RANGE_MAX_2_6,
    RTP_ABS_RANGE_MAX_2_5,
    RTP_ABS_RANGE_MAX_2_4,
    RTP_ABS_RANGE_MAX_2_3
};

/* n_m - Pick out n samples from m continuous samples,
 * and drop (m-n)/2 max and (m-n)/2 min samples.
 */
enum aic_rtp_filter_type {
    RTP_FILTER_NONE = 0,
    RTP_FILTER_2_4,
    RTP_FILTER_4_6,
    RTP_FILTER_4_8
};

enum aic_rtp_ch {
    RTP_CH_Y_MINUS = 0,
    RTP_CH_X_MINUS,
    RTP_CH_Y_PLUS,
    RTP_CH_X_PLUS,
    RTP_CH_Z_A,
    RTP_CH_Z_B,
    RTP_CH_Z_C,
    RTP_CH_Z_D,

    RTP_CH_MAX
};

enum aic_rtp_manual_mode_status {
    RTP_MMS_X_MINUS = 0,
    RTP_MMS_Y_MINUS,
    RTP_MMS_Z_A,
    RTP_MMS_Z_B,
    RTP_MMS_DOWN,
    RTP_MMS_IDLE,
    RTP_MMS_FINISH
};

void hal_rtp_status_show(struct aic_rtp_dev *rtp)
{
    int mcr, version;

    mcr = readl(RTP_MCR);
    version = readl(RTP_VERSION);

    printf("In RTP controller V%d.%02d:\n"
               "Mode hw %d/ sw %d, RTP enale %d, Press detect enable %d\n"
               "Pressure enable %d, max %d, x-plate %d, y-plate %d\n"
               "Sample period: %d ms, Fuzz: %d\n",
               version >> 8, version & 0xff,
               (u32)((mcr & RTR_MCR_MODE_MASK) >> RTR_MCR_MODE_SHIFT),
               rtp->mode, (u32)(mcr & RTP_MCR_EN),
               (mcr & RTP_MCR_PRES_DET_EN) ? 1 : 0,
               rtp->pressure_det, rtp->max_press,
               rtp->x_plate, rtp->y_plate,
               rtp->smp_period, rtp->fuzz);
}

static struct aic_rtp_dev *g_rtp_dev_of_user = NULL;

static u32 rtp_ms2itv(u32 pclk_rate, u32 ms)
{
    u32 tmp = pclk_rate / 1000;

    tmp = (tmp * ms) >> 12;
    return tmp;
}

static s32 rtp_is_rise(void)
{
    return readl(RTP_MCR) & RTP_MCR_RISE_STS ? 1 : 0;
}

static void rtp_reg_enable(int offset, int bit, int enable)
{
    int tmp;

    tmp = readl((long)offset);
    tmp &= ~bit;
    if (enable)
        tmp |= bit;

    writel(tmp, (long)offset);
}

static void rtp_fifo_flush(void)
{
    u32 sta = readl(RTP_FCR);

    if (sta & RTP_FCR_UF_FLAG)
        pr_err("FIFO is Underflow!%#x\n", sta);
    if (sta & RTP_FCR_OF_FLAG)
        pr_err("FIFO is Overflow!%#x\n", sta);

    writel(sta | RTP_FCR_FLUSH, RTP_FCR);
}

void hal_rtp_enable(struct aic_rtp_dev *rtp, int en)
{
    if (!en) {
        rtp_fifo_flush();
        pr_info("clean fifo\n");
    }

    rtp_reg_enable(RTP_MCR,
        rtp->mode << RTR_MCR_MODE_SHIFT | RTP_MCR_PRES_DET_EN | RTP_MCR_EN,
        en);

#if defined(CONFIG_ARTINCHIP_ADCIM_DM)
    writel(0, RTP_PDEB);
#else
    writel(rtp->pdeb, RTP_PDEB);
#endif

    if (rtp->mode != RTP_MODE_MANUAL) {
        writel(rtp->delay, RTP_DLY);
        rtp_reg_enable(RTP_MCR, RTP_MCR_PRES_DET_BYPASS, en);
    }

    rtp->pclk_rate = hal_clk_get_freq(hal_clk_get_parent(CLK_RTP));
    g_rtp_dev_of_user = rtp;
}

void hal_rtp_int_enable(struct aic_rtp_dev *rtp, int en)
{
    u32 val = RTP_INTR_FIFO_ERR_IE | RTP_INTR_DAT_RDY_IE
            | RTP_INTR_RISE_DET_IE | RTP_INTR_SCI_IE;

    if (rtp->mode == RTP_MODE_MANUAL)
        val |= RTP_INTR_PRES_DET_IE;

    rtp_reg_enable(RTP_INTR, val, en);
}

static void rtp_fifo_init(enum aic_rtp_mode mode, u32 smp_period)
{
    u32 thd = 0;

    switch (mode) {
    case RTP_MODE_AUTO1:
        if (smp_period)
            thd = 8;
        else
            thd = 2;
        break;
    case RTP_MODE_AUTO2:
        if (smp_period)
            thd = 12;
        else
            thd = 4;
        break;
    case RTP_MODE_AUTO3:
        if (smp_period)
            thd = 12;
        else
            thd = 6;
        break;
    case RTP_MODE_AUTO4:
    default:
        thd = 8;
        break;
    }
    thd <<= RTP_FCR_DAT_RDY_THD_SHIFT;

    writel(thd | RTP_FCR_UF_IE | RTP_FCR_OF_IE, RTP_FCR);
}

static u32 rtp_press_calc(struct aic_rtp_dev *rtp)
{
    struct aic_rtp_dat *dat = &rtp->latest;
    u32 pressure = rtp->x_plate * dat->x_minus / AIC_RTP_VAL_RANGE;

    if (rtp->y_plate) {
        pressure = pressure * (AIC_RTP_VAL_RANGE - dat->z_a) / dat->z_a;
        pressure -= rtp->y_plate * (AIC_RTP_VAL_RANGE - dat->y_minus)
                / AIC_RTP_VAL_RANGE;
    } else {
        pressure = pressure * (dat->z_b - dat->z_a) / dat->z_a;
    }
    pr_debug("Current pressure: %d\n", pressure);

    if (pressure > rtp->max_press) {
        pr_debug("Invalid pressure %d\n", pressure);
        pressure = AIC_RTP_INVALID_VAL;
    }
#if defined(CONFIG_ARTINCHIP_ADCIM_DM)
    return (dat->z_a + dat->z_b) / 2;
#else
    return pressure;
#endif
}

static void rtp_report_abs(struct aic_rtp_dev *rtp, u16 down)
{
    struct aic_rtp_dat *dat = &rtp->latest;
    struct aic_rtp_event e = {0};

    if (dat->x_minus == AIC_RTP_INVALID_VAL || dat->y_minus == AIC_RTP_INVALID_VAL)
        e.down = 0;

    if (rtp->pressure_det) {
        int pressure = rtp_press_calc(rtp);

        if (pressure == AIC_RTP_INVALID_VAL)
            e.down = 0;

        e.pressure = pressure;
    }

    e.x = dat->x_minus;
    e.y = dat->y_minus;
    e.down = down;
    e.timestamp = dat->timestamp;

    hal_rtp_ebuf_write(&rtp->ebuf, &e);

    if (rtp->callback)
        rtp->callback();
}

static void rtp_smp_period(u32 period)
{
    u32 val = 0;

    if (period) {
        val = period << RTP_AMSC_PERIOD_SAMPLE_INT_SHIFT
            | RTP_AMSC_PERIOD_SAMPLE_EN;
    } else {
        val = RTP_AMSC_SINGLE_SAMPLE_EN;
        writel(0, RTP_AMSC);
    }
    writel(val, RTP_AMSC);
}

void hal_rtp_auto_mode(struct aic_rtp_dev *rtp)
{
    writel(RTP_FILTER_4_8, RTP_FIL);
    rtp_smp_period(rtp_ms2itv(rtp->pclk_rate, rtp->smp_period));
    rtp_fifo_init(rtp->mode, rtp->smp_period);
}

/* Data format: XN, YN */
static void rtp_report_abs_auto1(struct aic_rtp_dev *rtp, u16 *ori, u32 cnt)
{
    u32 i = 0;
    struct aic_rtp_dat *latest = &rtp->latest;

    for (i = 0; i < cnt; ) {
        latest->x_minus = ori[i];
        latest->y_minus = ori[i + 1];
        rtp_report_abs(rtp, 1);

        i += 2;
        pr_debug("X %d, Y %d\n", latest->x_minus, latest->y_minus);
    }
}

/* Data format: XN, YN, ZA, ZB */
static void rtp_report_abs_auto2(struct aic_rtp_dev *rtp, u16 *ori, u32 cnt)
{
    u32 i = 0;
    struct aic_rtp_dat *latest = &rtp->latest;

    for (i = 0; i < cnt; ) {
        latest->x_minus = ori[i];
        latest->y_minus = ori[i + 1];
        latest->z_a = ori[i + 2];
        latest->z_b = ori[i + 3];
        rtp_report_abs(rtp, 1);

        i += 4;
        pr_debug("X %d, Y %d, ZA %d ZB %d\n", latest->x_minus, latest->y_minus,
                 latest->z_a, latest->z_b);
    }
}

static s32 rtp_distance_is_far(struct aic_rtp_dat *latest)
{
    s32 ret = 0;

    if (latest->x_minus != latest->x_plus) {
        if (latest->x_plus > latest->x_minus)
            ret = latest->x_plus > latest->x_minus +
                        AIC_RTP_SCATTER_THD;
        else
            ret = latest->x_minus > latest->x_plus +
                        AIC_RTP_SCATTER_THD;
    }
    if (ret)
        return 1;

    if (latest->y_minus != latest->y_plus) {
        if (latest->y_plus > latest->y_minus)
            ret = latest->y_plus > latest->y_minus +
                        AIC_RTP_SCATTER_THD;
        else
            ret = latest->y_minus > latest->y_plus +
                        AIC_RTP_SCATTER_THD;
    }
    return ret;
}

/* Data format: XN, XP, YN, YP, ZA, ZB */
static void rtp_report_abs_auto3(struct aic_rtp_dev *rtp, u16 *ori, u32 cnt)
{
    u32 i = 0;
    struct aic_rtp_dat *latest = &rtp->latest;

    for (i = 0; i < cnt; ) {
        latest->x_minus = ori[i];
        latest->x_plus = ori[i + 1];
        latest->y_minus = ori[i + 2];
        latest->y_plus = ori[i + 3];
        latest->z_a = ori[i + 4];
        latest->z_b = ori[i + 5];
        pr_debug("X %u-%u, Y %u-%u, ZA %u, ZB %u\n",
                 latest->x_minus, latest->x_plus,
                 latest->y_minus, latest->y_plus,
                 latest->z_a, latest->z_b);

        if (!rtp_distance_is_far(latest)) {
            latest->x_minus += latest->x_plus;
            latest->x_minus >>= 1;
            latest->y_minus += latest->y_plus;
            latest->y_minus >>= 1;
            rtp_report_abs(rtp, 1);
        } else {
            pr_debug("Distance is so far\n");
        }

        i += 6;
        if (i + 6 > cnt)
            break;
    }
}

/* Data format: XN, XP, YN, YP, ZA, ZB, ZC, ZD */
static void rtp_report_abs_auto4(struct aic_rtp_dev *rtp, u16 *ori, u32 cnt)
{
    u32 i = 0;
    struct aic_rtp_dat *latest = &rtp->latest;

    for (i = 0; i < cnt; ) {
        latest->x_minus = ori[i];
        latest->x_plus = ori[i + 1];
        latest->y_minus = ori[i + 2];
        latest->y_plus = ori[i + 3];
        latest->z_a = ori[i + 4];
        latest->z_b = ori[i + 5];
        latest->z_c = ori[i + 6];
        latest->z_d = ori[i + 7];

        pr_debug("X %u-%u, Y %u-%u, ZA %u-%u, ZB %u-%u\n",
                 latest->x_minus, latest->x_plus,
                 latest->y_minus, latest->y_plus,
                 latest->z_a, latest->z_c,
                 latest->z_b, latest->z_d);

#if defined(CONFIG_ARTINCHIP_ADCIM_DM)
        rtp_report_abs(rtp, 1);
        latest->x_minus = latest->x_plus;
        latest->y_minus = latest->y_plus;
        latest->z_a = latest->z_c;
        latest->z_b = latest->z_d;
        rtp_report_abs(rtp, 1);
#else
        if (!rtp_distance_is_far(latest)) {
            latest->x_minus += latest->x_plus;
            latest->x_minus >>= 1;
            latest->y_minus += latest->y_plus;
            latest->y_minus >>= 1;
            rtp_report_abs(rtp, 1);
        } else {
            pr_debug("Distance is so far\n");
        }
#endif

        i += 8;
        if (i + 8 > cnt)
            break;
    }
}

static void aic_rtp_read_fifo(struct aic_rtp_dev *rtp, u32 cnt)
{
    int i;
    u32 tmp;
    u16 data[AIC_RTP_FIFO_DEPTH] = {0};

    tmp = (readl(RTP_FCR) & RTP_FCR_DAT_CNT_MASK) >> RTP_FCR_DAT_CNT_SHIFT;
    if (tmp != cnt) {
        if (rtp->mode == RTP_MODE_MANUAL)
            pr_err("FIFO did changed %d/%d\n", tmp, cnt);
        else
            cnt = tmp;
    }

    for (i = 0; i < cnt; i++) {
        if (!(readl(RTP_FCR) & RTP_FCR_DAT_CNT_MASK)) {
            pr_err("FIFO is empty %d/%d\n", i, cnt);
            return;
        }
        tmp = readl(RTP_DATA);
        data[i] = tmp & RTP_DATA_DATA_MASK;
        // pr_debug("%d/%d - Read chan %d, data %d \n", i, 12 + (tmp >> 12), tmp >> 12, data[i]);
    }

    rtp->latest.timestamp = aic_get_time_ms();

    tmp = readl(RTP_FCR) & RTP_FCR_DAT_CNT_MASK;
    if (tmp) {
        pr_err("FIFO is not empty! %d\n", tmp >> RTP_FCR_DAT_CNT_SHIFT);
        rtp_fifo_flush();
    }
    tmp = readl(RTP_INTR);
    if (tmp & (RTP_INTR_DOUR_FLG | RTP_INTR_SCI_FLG))
        pr_debug("After read FIFO, INTR %#x, FCR %#x\n", tmp, readl(RTP_FCR));

    switch (rtp->mode) {
    case RTP_MODE_AUTO1:
        rtp_report_abs_auto1(rtp, data, cnt);
        break;
    case RTP_MODE_AUTO2:
        rtp_report_abs_auto2(rtp, data, cnt);
        break;
    case RTP_MODE_AUTO3:
        rtp_report_abs_auto3(rtp, data, cnt);
        break;
    case RTP_MODE_AUTO4:
        rtp_report_abs_auto4(rtp, data, cnt);
        break;
    default:
        return;
    }
}

s32 hal_rtp_register_callback(rtp_callback_t callback)
{
    struct aic_rtp_dev *rtp = g_rtp_dev_of_user;
    if (callback == NULL) {
        hal_log_err("Invalid callback function!\n");
        return -1;
    }
    rtp->callback = callback;
    return 0;
}

irqreturn_t hal_rtp_isr(int irq, void *arg)
{
    // TODO: struct aic_rtp_dev *rtp = (struct aic_rtp_dev *)arg;
    struct aic_rtp_dev *rtp = g_rtp_dev_of_user;
    u32 intr, fcr;

    intr = readl(RTP_INTR);
    fcr = readl(RTP_FCR);
    writel(fcr, RTP_FCR);
    writel(intr, RTP_INTR);

    pr_debug("INTS %#x, FCR %#x, Pressed %d\n", intr, fcr, !rtp_is_rise());
    if ((intr & RTP_INTR_PRES_DET_FLG) && (intr & RTP_INTR_RISE_DET_FLG)) {
        pr_debug("Press&rise happened at the same time!\n");
        if (rtp_is_rise())
            intr &= ~RTP_INTR_PRES_DET_FLG;
        else
            intr &= ~RTP_INTR_RISE_DET_FLG;
    }

    if (intr & RTP_INTR_SCI_FLG) {
        pr_err("SCI error, flush the FIFO ...\n");
        goto irq_clean_fifo;
    }

    if (intr & RTP_INTR_DOUR_FLG) {
        pr_err("DOUR error, flush the FIFO ...\n");
        goto irq_clean_fifo;
    }

    if (intr & RTP_INTR_FIFO_FLG) {
        /* When FIFO is overflow, the FIFO data is valid, so read it */
        if (!(fcr & RTP_FCR_OF_FLAG) || !(intr & RTP_INTR_RISE_DET_FLG)) {
            pr_err("FIFO error, flush the FIFO ...\n");
            goto irq_clean_fifo;
        }
    }

    if (intr & RTP_INTR_RISE_DET_FLG)
        rtp_report_abs(rtp, 0);

    if (intr & RTP_INTR_DRDY_FLG)
        aic_rtp_read_fifo(rtp, (fcr & RTP_FCR_DAT_CNT_MASK)
                          >> RTP_FCR_DAT_CNT_SHIFT);

    goto irq_done;

irq_clean_fifo:
    rtp_fifo_flush();

irq_done:
    return IRQ_HANDLED;
}

u32 hal_rtp_ebuf_read_space(struct aic_rtp_ebuf *ebuf)
{
    u16 rd = ebuf->rd_pos;
    u16 wr = ebuf->wr_pos;

    if (rd == wr) {
        return 0;
    } else if (rd < wr) {
        return wr - rd;
    } else {
        return AIC_RTP_EVT_BUF_SIZE - (rd - wr - 1);
    }
}

s32 hal_rtp_ebuf_write(struct aic_rtp_ebuf *ebuf, struct aic_rtp_event *e)
{
    if (hal_rtp_ebuf_full(ebuf)) {
        pr_debug("ebuf is full! Will drop a event\n");
        return -1;
    }

    memcpy(&ebuf->event[ebuf->wr_pos], e, sizeof(struct aic_rtp_event));
    ebuf->wr_pos++;

    if (ebuf->wr_pos >= AIC_RTP_EVT_BUF_SIZE)
        ebuf->wr_pos = 0;

    return 0;
}

s32 hal_rtp_ebuf_read(struct aic_rtp_ebuf *ebuf, struct aic_rtp_event *e)
{
    if (hal_rtp_ebuf_empty(ebuf)) {
        pr_debug("There is no RTP event in ebuf\n");
        return -1;
    }

    memcpy(e, &ebuf->event[ebuf->rd_pos], sizeof(struct aic_rtp_event));
    ebuf->rd_pos++;
    if (ebuf->rd_pos >= AIC_RTP_EVT_BUF_SIZE)
            ebuf->rd_pos = 0;

    return 0;
}

s32 hal_rtp_ebuf_sync(struct aic_rtp_ebuf *ebuf)
{
    if (ebuf->wr_pos != 0)
        ebuf->rd_pos = ebuf->wr_pos - 1;
    else
        ebuf->rd_pos = AIC_RTP_EVT_BUF_SIZE - 1;
    return 0;
}

s32 hal_rtp_clk_init(void)
{
    int ret = 0;

    ret = hal_clk_enable(CLK_RTP);
    if (ret < 0) {
        pr_err("RTP clk enable failed!");
        return -1;
    }

    ret = hal_clk_enable_deassertrst(CLK_RTP);
    if (ret < 0) {
        pr_err("RTP reset deassert failed!");
        return -1;
    }
    return ret;
}

u32 hal_rtp_pdeb_valid_check(struct aic_rtp_dev *rtp)
{
    int psi, debdc, debdc_max;

    debdc_max = RTP_PDEB_SLRDET_DEBDC_MASK >> RTP_PDEB_SLRDET_DEBDC_SHIFT;
    debdc = rtp->pdeb & debdc_max;
    psi = (rtp->smp_period * rtp->pclk_rate) / (4096 * 1000);

    /* When psi >=debdc, the current configuration of psi and pdeb is valid */
    if (psi >= debdc)
        return 0;

    /* When psi < debdc,return the recommended value of debdc */
    if (psi > debdc_max)
        debdc = debdc_max;
    else
        debdc = psi;
    return debdc;
}
