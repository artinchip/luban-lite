/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aic_hal_can.h"
#include "aic_hal_clk.h"

#define abs(x)                  ((x) >= 0 ? (x):-(x))

bus_err_msg_t bus_err_code[] = {
    {0x03, "SOF"},
    {0x02, "ID28~21"},
    {0x06, "ID20~18"},
    {0x04, "SRTR"},
    {0x05, "IDE"},
    {0x07, "ID17~13"},
    {0x0f, "ID12~5"},
    {0x0e, "ID4~0"},
    {0x0c, "RTR"},
    {0x0d, "R1"},
    {0x09, "R0"},
    {0x0b, "DLC"},
    {0x0A, "DATA Field"},
    {0x08, "CRC Sequence"},
    {0x18, "CRC Delimiter"},
    {0x19, "ACK Slot"},
    {0x1B, "ACK Delimiter"},
    {0x1A, "EOF"},
    {0x12, "Intermission"},
    {0x11, "Active error"},
    {0x16, "Passive error"},
    {0x13, "Tolerate dominant bits"},
    {0x17, "Error delimiter"},
    {0x1c, "Overload"}
};

/* CAN Bus Error Type */
bus_err_msg_t bus_err_type[] = {
    {0x0, "Bit Error"},
    {0x1, "Format Error"},
    {0x2, "Stuff Error"},
    {0x3, "Other Error"}
};

/* CAN Bus Error Direction */
bus_err_msg_t bus_err_dir[] = {
    {0x0, "error in TX direction"},
    {0x1, "error in RX direction"},
};

/* CAN Bus Arbitration Lost */
bus_err_msg_t bus_arb_lost[] = {
    {0x00, "ID28 ArbLost"},
    {0x01, "ID27 ArbLost"},
    {0x02, "ID26 ArbLost"},
    {0x03, "ID25 ArbLost"},
    {0x04, "ID24 ArbLost"},
    {0x05, "ID23 ArbLost"},
    {0x06, "ID22 ArbLost"},
    {0x07, "ID21 ArbLost"},
    {0x08, "ID20 ArbLost"},
    {0x09, "ID19 ArbLost"},
    {0x0A, "ID18 ArbLost"},
    {0x0B, "SRTR ArbLost"},
    {0x0C, "IDE  ArbLost"},
    {0x0D, "ID17 ArbLost"},
    {0x0E, "ID16 ArbLost"},
    {0x0F, "ID15 ArbLost"},
    {0x10, "ID14 ArbLost"},
    {0x11, "ID13 ArbLost"},
    {0x12, "ID12 ArbLost"},
    {0x13, "ID11 ArbLost"},
    {0x14, "ID10 ArbLost"},
    {0x15, "ID9 ArbLost"},
    {0x16, "ID8 ArbLost"},
    {0x17, "ID7 ArbLost"},
    {0x18, "ID6 ArbLost"},
    {0x19, "ID5 ArbLost"},
    {0x1A, "ID4 ArbLost"},
    {0x1B, "ID3 ArbLost"},
    {0x1C, "ID2 ArbLost"},
    {0x1D, "ID1 ArbLost"},
    {0x1E, "ID0 ArbLost"},
    {0x1F, "RTR ArbLost"},
};

bus_err_msg_t bus_state[] = {
    {0, "active status"},
    {1, "warning status"},
    {2, "passive status"},
    {3, "bus off"},
};

static const struct can_bittiming_const btc = {
    .sync_seg = 1,
    .tseg1_min = 1,
    .tseg1_max = 16,
    .tseg2_min = 1,
    .tseg2_max = 8,
    .sjw_max = 4,
    .brp_min = 1,
    .brp_max = 64,
    .brp_inc = 1,
};

void hal_can_set_mode(can_handle *phandle, u32 mode)
{
    u32 reg_val;

    reg_val = readl(phandle->can_base + CAN_MODE_REG);
    reg_val &= ~CAN_MODE_MASK;
    reg_val |= mode;
    writel(reg_val, phandle->can_base + CAN_MODE_REG);
}

int hal_can_init(can_handle *phandle, u32 can_idx)
{
    int ret = 0;

    if (!can_idx) {
        phandle->can_base = CAN0_BASE;
        phandle->irq_num = CAN0_IRQn;
        phandle->clk_id = CLK_CAN0;
    } else if (can_idx == 1) {
        phandle->can_base = CAN1_BASE;
        phandle->irq_num = CAN1_IRQn;
        phandle->clk_id = CLK_CAN1;
    } else {
        hal_log_err("CAN index error\n");
        return -EINVAL;
    }

    phandle->idx = can_idx;

    ret = hal_clk_enable_deassertrst(phandle->clk_id);
    if (ret < 0) {
        hal_log_err("CAN clock and reset init error\n");
        return ret;
    }

    hal_can_set_mode(phandle, CAN_RESET_MODE);
    /* accept all frame */
    writel(0, phandle->can_base + CAN_RXCODE0_REG);
    writel(0, phandle->can_base + CAN_RXCODE1_REG);
    writel(0, phandle->can_base + CAN_RXCODE2_REG);
    writel(0, phandle->can_base + CAN_RXCODE3_REG);

    writel(0xFF, phandle->can_base + CAN_RXMASK0_REG);
    writel(0xFF, phandle->can_base + CAN_RXMASK1_REG);
    writel(0xFF, phandle->can_base + CAN_RXMASK2_REG);
    writel(0xFF, phandle->can_base + CAN_RXMASK3_REG);
    hal_can_set_mode(phandle, CAN_NORMAL_MODE);

    return ret;
}

void hal_can_uninit(can_handle *phandle)
{
    hal_clk_disable_assertrst(phandle->clk_id);
}

static void hal_can_update_sample_point(u32 sample_point_nominal,
                                        u32 tseg, u32 *tseg1_ptr,
                                        u32 *tseg2_ptr,
                                        u32 *sample_point_error_ptr)
{
    u32 sample_point_error, best_sample_point_error = UINT32_MAX;
    u32 sample_point, tseg1, tseg2;
    int i;

    for (i = 0; i <= 1; i++) {
        tseg2 = tseg + btc.sync_seg -
                (sample_point_nominal * (tseg + btc.sync_seg)) / 1000 - i;
        if (tseg2 < btc.tseg2_min)
            tseg2 = btc.tseg2_min;
        else if (tseg2 > btc.tseg2_max)
            tseg2 = btc.tseg2_max;

        tseg1 = tseg - tseg2;
        if (tseg1 > btc.tseg1_max) {
            tseg1 = btc.tseg1_max;
            tseg2 = tseg - tseg1;
        }

        sample_point = 1000 * (tseg + btc.sync_seg - tseg2) /
                        (tseg + btc.sync_seg);
        sample_point_error = abs(sample_point_nominal - sample_point);

        if (sample_point <= sample_point_nominal &&
            sample_point_error < best_sample_point_error) {
            best_sample_point_error = sample_point_error;
            *tseg1_ptr = tseg1;
            *tseg2_ptr = tseg2;
        }
    }

    if (sample_point_error_ptr)
        *sample_point_error_ptr = best_sample_point_error;
}

void hal_can_set_baudrate(can_handle *phandle, unsigned long baudrate)
{
    u32 mod_freq;
    u32 brp, tsegall, tseg, tseg1 = 0, tseg2 = 0;
    u32 sample_point_nominal;
    u32 current_baudrate, baudrate_error;
    u32 best_baudrate_error = UINT32_MAX;
    u32 sample_point_error;
    u32 best_sample_point_error = UINT32_MAX;
    u32 best_tseg = 0, best_brp = 0;
    u32 sjw = 1, sample_time = 0;
    u32 reg_val;

    if (baudrate > 800000)
        sample_point_nominal = 750;
    else if (baudrate > 500000)
        sample_point_nominal = 800;
    else
        sample_point_nominal = 875;

    mod_freq = hal_clk_get_freq(phandle->clk_id);
    hal_log_debug("mod_freq: %d\n", mod_freq);

    /* tseg even = round down, odd = round up */
    for (tseg = (btc.tseg1_max + btc.tseg2_max) * 2 + 1;
         tseg > CAN_TSEG_MIN * 2; tseg--) {
        tsegall = btc.sync_seg + tseg / 2;

        /* Compute all possible tseg choices (tseg=tseg1+tseg2) */
        brp = mod_freq / (2 * tsegall * baudrate) + tseg % 2;
        /* choose brp step which is possible in system */
        brp = (brp / btc.brp_inc) * btc.brp_inc;
        if (brp < btc.brp_min || brp > btc.brp_max)
            continue;

        current_baudrate = mod_freq / (2 * brp * tsegall);
        baudrate_error = abs(current_baudrate - baudrate);

        if (baudrate_error > best_baudrate_error)
            continue;
        /* reset sample point error if we have a better baudrate */
        if (baudrate_error < best_baudrate_error)
            best_sample_point_error = UINT32_MAX;

        hal_can_update_sample_point(sample_point_nominal, tseg / 2, &tseg1,
                                    &tseg2, &sample_point_error);
        if (sample_point_error > best_sample_point_error)
            continue;

        best_sample_point_error = sample_point_error;
        best_baudrate_error = baudrate_error;
        best_tseg = tseg / 2;
        best_brp = brp;

        if (baudrate_error == 0 && sample_point_error == 0)
            break;
    }

    hal_can_update_sample_point(sample_point_nominal, best_tseg,
                                &tseg1, &tseg2, NULL);

    if (sjw > tseg2)
        sjw = tseg2;

    if (baudrate < 125000)
        sample_time = 1;

    hal_log_debug("brp: %d, best_tseg: %d, tseg1: %d, tseg2: %d, sjw: %d, "
                  "sample_point = %.2f\n", best_brp, best_tseg, tseg1,
                  tseg2, sjw, (float)(1 + tseg1) / (1 + tseg1 + tseg2));

    hal_can_set_mode(phandle, CAN_MODE_RST);
    /* write value to register */
    reg_val = ((sjw - 1) << 6) | (best_brp - 1);
    writel(reg_val, phandle->can_base + CAN_BTR0_REG);
    hal_log_debug("BTR0: %02x\n", reg_val);
    reg_val = (sample_time << 7) | ((tseg2 - 1) << 4) | (tseg1 - 1);
    hal_log_debug("BTR1: %02x\n", reg_val);
    writel(reg_val, phandle->can_base + CAN_BTR1_REG);
    hal_can_set_mode(phandle, CAN_MODE_NORMAL);
}

static void hal_can_bus_error_msg(can_handle *phandle)
{
    u8 i;
    u8 errinfo = readb(phandle->can_base + CAN_ERRCODE_REG);
    u8 errtype = (errinfo & CAN_ERRCODE_ERRTYPE_MASK) >> 6;
    u8 errdir = (errinfo & CAN_ERRCODE_DIR) >> 5;
    u8 errcode = errinfo & CAN_ERRCODE_SEGCODE_MASK;

    for (i = 0; i < ARRAY_SIZE(bus_err_dir); i++) {
        if (errdir == bus_err_dir[i].code) {
            hal_log_err("%s, ", bus_err_dir[i].msg);
            if (i)
                phandle->status.recverrcnt++;
            else
                phandle->status.snderrcnt++;
            break;
        }
    }

    for (i = 0; i < ARRAY_SIZE(bus_err_type); i++) {
        if (errtype == bus_err_type[i].code) {
            hal_log_err("%s, ", bus_err_type[i].msg);
            switch (i) {
            case 0:
                phandle->status.biterrcnt++;
                break;
            case 1:
                phandle->status.formaterrcnt++;
                break;
            case 2:
                phandle->status.stufferrcnt++;
                break;
            default:
                phandle->status.othererrcnt++;
                break;
            }
            break;
        }
    }

    for (i = 0; i < ARRAY_SIZE(bus_err_code); i++) {
        if (errcode == bus_err_code[i].code) {
            hal_log_err("%s\n", bus_err_code[i].msg);
            break;
        }
    }
}

static void hal_can_arblost_msg(can_handle *phandle)
{
    u8 i;
    u8 arbinfo = readb(phandle->can_base + CAN_ARBLOST_REG) &
                            CAN_ARBLOST_CAP_MASK;

    for (i = 0; i < ARRAY_SIZE(bus_arb_lost); i++) {
        if (arbinfo == bus_arb_lost[i].code) {
            hal_log_err("%s, ", bus_arb_lost[i].msg);
            phandle->status.arblostcnt++;
            phandle->status.snderrcnt++;
            break;
        }
    }
}

static void hal_can_error_handle(can_handle *phandle, u32 err_status)
{
    u32 can_status;
    u8 errcode;

    errcode = readb(phandle->can_base + CAN_ERRCODE_REG) &
              CAN_ERRCODE_SEGCODE_MASK;

    can_status = readl(phandle->can_base + CAN_STAT_REG);
    phandle->status.rxerr = readl(phandle->can_base + CAN_RXERR_REG);
    phandle->status.txerr = readl(phandle->can_base + CAN_TXERR_REG);

    if (err_status & CAN_INTR_ERRB) {
        hal_can_bus_error_msg(phandle);
    }

    if (err_status & CAN_INTR_ARBLOST) {
        hal_can_arblost_msg(phandle);
    }

    if (err_status & CAN_INTR_ERRP) {
        if (phandle->status.current_state == PASSIVE_STATUS)
            phandle->status.current_state = WARNING_STATUS;
        else
            phandle->status.current_state = PASSIVE_STATUS;
    }

    if (err_status & CAN_INTR_OVF) {
        phandle->status.othererrcnt++;
        phandle->status.recverrcnt++;
        hal_can_set_mode(phandle, CAN_MODE_RST);
        hal_can_set_mode(phandle, CAN_MODE_NORMAL);
        if (phandle->callback)
            phandle->callback(phandle, (void *)CAN_EVENT_RXOF_IND);
        /* clear bit */
        writel(CAN_MCR_CLR_OVF, phandle->can_base + CAN_MCR_REG);
    }

    if (err_status & CAN_INTR_ERRW) {
        if (can_status & CAN_STAT_BUS)
            phandle->status.current_state = BUS_OFF;
        else if (can_status & CAN_STAT_ERR)
            phandle->status.current_state = WARNING_STATUS;
        else
            phandle->status.current_state = ACTIVE_STATUS;
    }

    if (phandle->status.current_state == BUS_OFF)
        hal_can_set_mode(phandle, CAN_MODE_NORMAL);

    if (phandle->status.txerr > CAN_ERRP_THRESHOLD &&
        errcode == CAN_ERRCODE_ACK_SLOT)
    {
        writel(CAN_MCR_ABORTREQ, phandle->can_base + CAN_MCR_REG);
        hal_can_set_mode(phandle, CAN_MODE_RST);
        hal_can_set_mode(phandle, CAN_MODE_NORMAL);
    }
}

int hal_can_attach_callback(can_handle *phandle, void *callback, void *arg)
{
    CHECK_PARAM(phandle != NULL, -EINVAL);

    phandle->callback = callback;
    phandle->arg = arg;
    return 0;
}

void hal_can_detach_callback(can_handle *phandle)
{
    CHECK_PARAM_RET(phandle);

    phandle->callback = NULL;
    phandle->arg = NULL;
}

static void hal_can_rx_frame(u32 reg_base, can_msg_t *msg)
{
    u32 dreg, i;
    u32 buf0_val;
    unsigned long can_base = reg_base;

    CHECK_PARAM_RET(msg);

    buf0_val = readl(can_base + CAN_BUF0_REG);
    msg->ide = (buf0_val >> CAN_BUF0_MSG_EFF_SHIFT) & 1;
    msg->rtr = (buf0_val >> CAN_BUF0_MSG_RTR_SHIFT) & 1;
    msg->dlc = buf0_val & 0xf;

    if (msg->ide) {
        dreg = CAN_BUF5_REG;
        msg->id = (readl(can_base + CAN_BUF1_REG) << 21) |
                  (readl(can_base + CAN_BUF2_REG) << 13) |
                  (readl(can_base + CAN_BUF3_REG) << 5)  |
                  ((readl(can_base + CAN_BUF4_REG) >> 3) & 0x1f);
    } else {
        dreg = CAN_BUF3_REG;
        msg->id = (readl(can_base + CAN_BUF1_REG) << 3) |
                  ((readl(can_base + CAN_BUF2_REG) >> 5) & 0x7);
    }

    if (!msg->rtr)
        for (i = 0; i < msg->dlc; i++)
            msg->data[i] = readl(can_base + dreg + i * 4);
    /* release rx buffer */
    writel(RXB_REL_REQ, can_base + CAN_MCR_REG);
}

void hal_can_send_frame(can_handle *phandle, can_msg_t * msg, can_op_req_t req)
{
    CHECK_PARAM_RET(phandle);
    CHECK_PARAM_RET(msg);
    u32 dreg, i;
    unsigned long can_base = phandle->can_base;
    u8 ide = msg->ide;
    u8 rtr = msg->rtr;
    u8 dlc = msg->dlc;
    u32 buf0_val = dlc;

    if (req == CLR_OF_REQ || req == RXB_REL_REQ) {
        hal_log_err("invalid parameter: req\n");
        return;
    }

    if (rtr)
        buf0_val |= CAN_BUF0_MSG_RTR_FLAG;

    if (ide) {
        /* extended frame */
        buf0_val |= CAN_BUF0_MSG_EFF_FLAG;
        dreg = CAN_BUF5_REG;
        writel((msg->id >> 21) & 0xff, can_base + CAN_BUF1_REG);
        writel((msg->id >> 13) & 0xff, can_base + CAN_BUF2_REG);
        writel((msg->id >> 5) & 0xff, can_base + CAN_BUF3_REG);
        writel((msg->id & 0x1f) << 3, can_base + CAN_BUF4_REG);
    } else {
        /* standard frame */
        dreg = CAN_BUF3_REG;
        writel((msg->id >> 3) & 0xff, can_base + CAN_BUF1_REG);
        writel((msg->id & 0x7) << 5, can_base + CAN_BUF2_REG);
    }

    for (i = 0; i < dlc; i++)
        writel(msg->data[i], can_base + dreg + i * 4);

    writel(buf0_val, can_base + CAN_BUF0_REG);

    writel(req, can_base + CAN_MCR_REG);
}

void hal_can_receive_frame(can_handle *phandle, can_msg_t * msg)
{
    int i;

    msg->id = phandle->msg.id;
    msg->rtr = phandle->msg.rtr;
    msg->ide = phandle->msg.ide;
    msg->dlc = phandle->msg.dlc;

    for (i = 0; i < msg->dlc; i++)
        msg->data[i] = phandle->msg.data[i];
}

irqreturn_t hal_can_isr_handler(int irq_num, void *arg)
{
    u32 int_status;
    can_handle *phandle = (can_handle *)arg;

    int_status = readl(phandle->can_base + CAN_INTR_REG);

    if (int_status & (CAN_INTR_ERRB | CAN_INTR_ARBLOST | CAN_INTR_ERRP |
                CAN_INTR_WAKEUP | CAN_INTR_OVF | CAN_INTR_ERRW))
        hal_can_error_handle(phandle, int_status);

    if (int_status & CAN_INTR_TX) {
        phandle->status.sndpkgcnt++;
        if (phandle->callback)
            phandle->callback(phandle, (void *)CAN_EVENT_TX_DONE);
    }

    if ((int_status & CAN_INTR_RX) && !(int_status & CAN_INTR_OVF)) {
        hal_can_rx_frame(phandle->can_base, &phandle->msg);
        if (phandle->callback)
            phandle->callback(phandle, (void *)CAN_EVENT_RX_IND);
    }

    writel(int_status, phandle->can_base + CAN_INTR_REG);
    return IRQ_HANDLED;
}

static int hal_can_get_baudrate(can_handle *phandle, u32 *baudrate)
{
    u8 brp, tseg1, tseg2;
    u32 mod_freq;

    CHECK_PARAM(phandle, -EINVAL);
    CHECK_PARAM(baudrate, -EINVAL);

    mod_freq = hal_clk_get_freq(phandle->clk_id);
    brp = readb(phandle->can_base + CAN_BTR0_REG) & CAN_BTR0_BRP_MASK;
    tseg1 = readb(phandle->can_base + CAN_BTR1_REG) & CAN_BTR1_TS1_MASK;
    tseg2 = (readb(phandle->can_base + CAN_BTR1_REG) & CAN_BTR1_TS2_MASK) >> 4;

    *baudrate = mod_freq / 2 / (3 + tseg1 + tseg2) / (brp + 1);
    return 0;
}

static int hal_can_set_filter(can_handle *phandle, can_filter_mode_t mode, can_filter_t *rxcode, can_filter_t *rxmask, u8 filter_ext)
{
    u32 rxcode0, rxcode1, rxcode2, rxcode3;
    u32 rxmask0, rxmask1, rxmask2, rxmask3;
    u32 reg_val;

    CHECK_PARAM(phandle, -EINVAL);
    CHECK_PARAM(rxcode, -EINVAL);
    CHECK_PARAM(rxmask, -EINVAL);

    reg_val = readl(phandle->can_base + CAN_MODE_REG);

    hal_can_set_mode(phandle, CAN_RESET_MODE);

    switch (mode) {
    case SINGLE_FILTER_MODE:
        reg_val |= CAN_MODE_FILTER_SINGLE;
        if (filter_ext) {
            rxcode0 = (rxcode->sfe.id_filter >> 21) & 0xff;
            rxcode1 = (rxcode->sfe.id_filter >> 13) & 0xff;
            rxcode2 = (rxcode->sfe.id_filter >> 5) & 0xff;
            rxcode3 = ((rxcode->sfe.id_filter & 0x1f) << 3) |
                      ((rxcode->sfe.rtr_filter & 0x1) << 2);

            rxmask0 = (rxmask->sfe.id_filter >> 21) & 0xff;
            rxmask1 = (rxmask->sfe.id_filter >> 13) & 0xff;
            rxmask2 = (rxmask->sfe.id_filter >> 5) & 0xff;
            rxmask3 = ((rxmask->sfe.id_filter & 0x1f) << 3) |
                      ((rxmask->sfe.rtr_filter & 0x1) << 2);
        } else {
            rxcode0 = (rxcode->sfs.id_filter >> 3) & 0xff;
            rxcode1 = ((rxcode->sfs.id_filter & 0x7) << 5) |
                      ((rxcode->sfs.rtr_filter & 0x1) << 4);
            rxcode2 = rxcode->sfs.data0_filter;
            rxcode3 = rxcode->sfs.data1_filter;

            rxmask0 = (rxmask->sfs.id_filter >> 3) & 0xff;
            rxmask1 = ((rxmask->sfs.id_filter & 0x7) << 5) |
                      ((rxmask->sfs.rtr_filter & 0x1) << 4);
            rxmask2 = rxmask->sfs.data0_filter;
            rxmask3 = rxmask->sfs.data1_filter;
        }
        break;
    case DUAL_FILTER_MODE:
        reg_val &= ~CAN_MODE_FILTER_SINGLE;
        if (filter_ext) {
            rxcode0 = rxcode->dfe.id_filter0 >> 8;
            rxcode1 = rxcode->dfe.id_filter0 & 0xff;
            rxcode2 = rxcode->dfe.id_filter1 >> 8;
            rxcode3 = rxcode->dfe.id_filter1 & 0xff;

            rxmask0 = rxmask->dfe.id_filter0 >> 8;
            rxmask1 = rxmask->dfe.id_filter0 & 0xff;
            rxmask2 = rxmask->dfe.id_filter1 >> 8;
            rxmask3 = rxmask->dfe.id_filter1 & 0xff;
        } else {
            rxcode0 = (rxcode->dfs.id_filter0 >> 3) & 0xff;
            rxcode1 = ((rxcode->dfs.id_filter0 & 0x7) << 5) |
                      ((rxcode->dfs.rtr_filter0 & 0x1) << 4) |
                      ((rxcode->dfs.data0_filter0 >> 4) & 0xf);
            rxcode2 = (rxcode->dfs.id_filter1 >> 3) & 0xff;
            rxcode3 = ((rxcode->dfs.id_filter1 & 0x7) << 5) |
                      ((rxcode->dfs.rtr_filter1 & 0x1) << 4) |
                      (rxcode->dfs.id_filter0 & 0xf);

            rxmask0 = (rxmask->dfs.id_filter0 >> 3) & 0xff;
            rxmask1 = ((rxmask->dfs.id_filter0 & 0x7) << 5) |
                      ((rxmask->dfs.rtr_filter0 & 0x1) << 4) |
                      ((rxmask->dfs.data0_filter0 >> 4) & 0xf);
            rxmask2 = (rxmask->dfs.id_filter1 >> 3) & 0xff;
            rxmask3 = ((rxmask->dfs.id_filter1 & 0x7) << 5) |
                      ((rxmask->dfs.rtr_filter1 & 0x1) << 4) |
                      (rxmask->dfs.id_filter0 & 0xf);
        }
        break;
    case FILTER_CLOSE:
    default:
        reg_val &= ~CAN_MODE_FILTER_SINGLE;

        /* Set filters, accept all data */
        rxcode0 = 0;
        rxcode1 = 0;
        rxcode2 = 0;
        rxcode3 = 0;
        rxmask0 = 0xff;
        rxmask1 = 0xff;
        rxmask2 = 0xff;
        rxmask3 = 0xff;
        break;
    }

    writel(rxcode0, phandle->can_base + CAN_RXCODE0_REG);
    writel(rxcode1, phandle->can_base + CAN_RXCODE1_REG);
    writel(rxcode2, phandle->can_base + CAN_RXCODE2_REG);
    writel(rxcode3, phandle->can_base + CAN_RXCODE3_REG);

    writel(rxmask0, phandle->can_base + CAN_RXMASK0_REG);
    writel(rxmask1, phandle->can_base + CAN_RXMASK1_REG);
    writel(rxmask2, phandle->can_base + CAN_RXMASK2_REG);
    writel(rxmask3, phandle->can_base + CAN_RXMASK3_REG);

    writel(reg_val, phandle->can_base + CAN_MODE_REG);

    hal_can_set_mode(phandle, CAN_NORMAL_MODE);
    return 0;
}

int hal_can_ioctl(can_handle *phandle, int cmd, void *arg)
{
    int ret = 0;
    can_filter_config_t *cfg;

    switch (cmd) {
    case CAN_IOCTL_SET_MODE:
        hal_can_set_mode(phandle, (can_mode_t)arg);
        break;
    case CAN_IOCTL_SET_BAUDRATE:
        hal_can_set_baudrate(phandle, (unsigned long)arg);
        break;
    case CAN_IOCTL_GET_BAUDRATE:
        ret = hal_can_get_baudrate(phandle, (u32 *)arg);
        break;
    case CAN_IOCTL_SET_FILTER:
        cfg = (can_filter_config_t *)arg;

        ret = hal_can_set_filter(phandle, cfg->filter_mode, &cfg->rxcode,
                                 &cfg->rxmask, cfg->is_eff);
        break;
    default:
        return -EOPNOTSUPP;
    }

    return ret;
}
