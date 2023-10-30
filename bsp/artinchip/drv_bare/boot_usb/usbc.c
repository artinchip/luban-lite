/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <usb_reg.h>
#include <usb_drv.h>
#include "usbc.h"

void usbc_soft_disconnect(void)
{
    u32 val;

    /*
     * Set Soft Disconnect bit to signal the DWC_otg core to do a soft
     * disconnect.
     *
     * Bit[1]: Default is 1, disconnect.
     */
    val = readl(USB_DEV_DCTL);
    val |= (0x1U << 1);
    writel(val, USB_DEV_DCTL);

    /*
     * Delay 3 ms to ensure HOST can detect device disconnect
     */
    aicos_mdelay(3);
}

void usbc_soft_connect(void)
{
    u32 val;

    /*
     * Clear Soft Disconnect bit to signal the DWC_otg core to do a soft
     * connect.
     *
     * Bit[1]: Default is 1, disconnect.
     */
    val = readl(USB_DEV_DCTL);
    val &= ~(0x1U << 1);
    writel(val, USB_DEV_DCTL);
}

static void in_ep_init(u32 idx)
{
    u32 val;

    val = readl(USB_DEV_DIEPCTL(idx));

    /*
     * Bit[31] EP Enable
     */
    if ((val & (0x1U << 31))) {
        /*
         * Bit[30] EP Disable
         * Bit[27] Set NAK
         */
        val = (0x1U << 30) | (0x1U << 27);
        writel(val, USB_DEV_DIEPCTL(idx));
    } else {
        writel(0, USB_DEV_DIEPCTL(idx));
    }

    /*
     * For EP0, this set MPS = 0, means MPS = 64 Bytes.
     */
    writel(0, USB_DEV_DIEPTSIZ(idx));

    /*
     * Clear pending interrupts
     */
    writel(0xFB7F, USB_DEV_DIEPINT(idx));
}

void usbc_in_ep_reset(u32 idx)
{
    u32 val;

    /*
     * Clear pending interrupts
     */
    writel(0xFB7F, USB_DEV_DIEPINT(idx));

    val = readl(USB_DEV_DIEPCTL(idx));

    /*
     * Bit[27] Set NAK
     * Bit[21] STALL
     */
    val &= ~(0x1U << 21);
    val |= (0x1U << 27);
    writel(val, USB_DEV_DIEPCTL(idx));
}

static void out_ep_init(u32 idx)
{
    u32 val;

    val = readl(USB_DEV_DOEPCTL(idx));

    /*
     * Bit[31] EP Enable
     */
    if ((val & (0x1U << 31))) {
        /*
         * Bit[30] EP Disable
         * Bit[27] Set NAK
         */
        val = (0x1U << 30) | (0x1U << 27);
        writel(val, USB_DEV_DOEPCTL(idx));
    } else {
        writel(0, USB_DEV_DOEPCTL(idx));
    }

    writel(0, USB_DEV_DOEPTSIZ(idx));

    /*
     * Clear pending interrupts
     */
    writel(0xFB7F, USB_DEV_DOEPINT(idx));
}

void usbc_out_ep_reset(u32 idx)
{
    u32 val;

    /*
     * Clear pending interrupts
     */
    writel(0xFB7F, USB_DEV_DOEPINT(idx));

    val = readl(USB_DEV_DOEPCTL(idx));

    /*
     * Bit[27] Set NAK
     * Bit[21] STALL
     */
    val &= ~(0x1U << 21);
    val |= (0x1U << 27);
    writel(val, USB_DEV_DOEPCTL(idx));
}

static void fifo_init(void)
{
    u32 depth, start;

    /* Set RX FIFO Size 512 */
    depth = 0x200;
    writel(depth, USB_DEV_GRXFSIZ);

    /*
     * Set NP TX FIFO size 512, start from 0x200
     * but when internal DMA is enabled, need to reserve 10 words, internal
     * using for EP information.
     */
#if defined(AICUPG_USB_DMA_ENALBE)
    depth = 512 - 10;
#else
    depth = 0x200;
#endif
    start = 0x200; /* TX FIFO start address in Transmit RAM */
    writel((depth << 16) | (start << 0), USB_DEV_GNPTXFSIZ);
}

static void intr_init(void)
{
    u32 val = 0;

    /*
     * Write bits 0 to disable all relative interrupts.
     */
    val = 0;
    writel(val, USB_DEV_DIEPMSK);  /* IN EP common interrupts, DIEPINTn */
    writel(val, USB_DEV_DOEPMSK);  /* OUT EP common interrupts, DOEPINTn */
    writel(val, USB_DEV_DAINTMSK); /* EP interrupt */
    writel(val, USB_DEV_GINTMSK);  /* Global interrupt */

    /*
     * Clear all endpoints interrupt status
     */
    val = 0xFFFFFFFF;
    writel(val, USB_DEV_DAINT);
    writel(val, USB_DEV_GINTSTS);

    /*
     * Enable some interrupts
     */
    val = readl(USB_DEV_GINTMSK);
    val |= (USB_DEV_GINTMSK_USBRST | USB_DEV_GINTMSK_ENUMDNE |
            USB_DEV_GINTMSK_RXFLVL | USB_DEV_GINTMSK_OEPINT |
            USB_DEV_GINTMSK_IEPINT);
    writel(val, USB_DEV_GINTMSK);

    /*
     * Ummask/Enable the global interrupt
     */
    val = readl(USB_DEV_INIT);
    val |= (0x1U << 9);
    writel(val, USB_DEV_INIT);
}

void usbc_intr_reset(void)
{
    u32 val;

    pr_debug("%s\n", __func__);
    /*
     * Enable IN/OUT EP interrupt
     */
    val = readl(USB_DEV_DAINTMSK);
    val |= USB_DEV_DAINTMSK_IEP;
    val |= USB_DEV_DAINTMSK_OEP;
    writel(val, USB_DEV_DAINTMSK);

    val = readl(USB_DEV_DOEPMSK);
    val |= (USB_DEV_DOEPMSK_SETUP | USB_DEV_DOEPMSK_XFRC | USB_DEV_DOEPMSK_EPD |
            USB_DEV_DOEPMSK_OTEPSPR | USB_DEV_DOEPMSK_NAK);
    writel(val, USB_DEV_DOEPMSK);

    val = readl(USB_DEV_DIEPMSK);
    val |= (USB_DEV_DIEPMSK_TMO | USB_DEV_DIEPMSK_XFRC |
            USB_DEV_DIEPMSK_ITTXFE | USB_DEV_DIEPMSK_EPD);
    writel(val, USB_DEV_DIEPMSK);

    /*
     * Clear Global Non-Periodic IN NAK
     */
    val = readl(USB_DEV_DCTL);
    val |= USB_DEV_DCTL_CGINAK;
    writel(val, USB_DEV_DCTL);
}

s32 usbc_init(void)
{
    u32 val = 0, i = 0;

    /*
     * cmu_mod_enable(CMU_MOD_AHB0_USB_DEV);
     * aic_usb_phy_init();
     * Enable Device and Phy by the databook's guide
     *
     * 1. Enable device and phy's clock
     * 2. Delay more than 40us
     * 3. Release the reset signal
     */
    hal_clk_enable_deassertrst_iter(CLK_USBD);
    hal_clk_enable_deassertrst_iter(CLK_USB_PHY0);
    aicos_udelay(100);
    hal_reset_deassert(RESET_USBPHY0);
    aicos_udelay(1);
    hal_reset_deassert(RESET_USBD);
    aicos_udelay(1);

    /*
     * Flush FIFO and Configure size
     */
    usbc_flush_rxfifo();
    usbc_flush_all_txfifo();
    fifo_init();

    for (i = 0; i < USB_DEV_EP_CNT; i++) {
        in_ep_init(i);
        out_ep_init(i);
    }

    intr_init();

    /*
     * Set DCFG Register bit[1:0], Device Speed to High speed
     */
    val = readl(USB_DEV_DCFG);
    val &= ~(0x3 << 0);
    val |= USB_DEV_SPEED_HIGH;
    writel(val, USB_DEV_DCFG);

    pr_debug("%s done\n", __func__);
    return 0;
}

void usbc_set_ctrl_ep_mps(u8 mps)
{
    u32 val;

    val = readl(USB_DEV_DIEPCTL0);
    val &= ~USB_DEV_DIEPCTL0_MPSIZ;
    val |= (mps & USB_DEV_DIEPCTL0_MPSIZ) << 0;
    writel(val, USB_DEV_DIEPCTL0);
}

u32 usbc_get_dev_speed(void)
{
    u32 val;

    val = readl(USB_DEV_DSTS);
    val &= USB_DEV_DSTS_ENUMSPD;
    return val >> 1;
}

void usbc_clock_gating_enable(void)
{
    /* USB IP clock?
     * USB Phy clock?
     */
}

void usbc_clock_gating_disable(void)
{
}

void usbc_set_address(u32 dev_addr)
{
    u32 val;

    val = readl(USB_DEV_DCFG);
    val &= (~USB_DEV_DCFG_DAD);
    val |= ((dev_addr << 4) & USB_DEV_DCFG_DAD);
    writel(val, USB_DEV_DCFG);
}

void usbc_in_bulk_ep_enable(u32 ep)
{
    u32 val;
#if defined(AICUPG_USB_DMA_ENALBE)
    /*
     * When enumeration in ctrlep is finished, controller need to switch
     * to IN EP, here set the IN EP number to CTRL EP.
     *
     * When USB DMA is enabled, it is required.
     */
    val = readl(USB_DEV_DIEPCTL0);
    val &= ~USB_DEV_DIEPCTL_NEXTEP;
    val |= (ep << 11);
    writel(val, USB_DEV_DIEPCTL0);
#endif

    val = readl(USB_DEV_DIEPCTL(ep));
    val |= (USB_DEV_DIEPCTL_CNAK | USB_DEV_DIEPCTL_EPENA);
    writel(val, USB_DEV_DIEPCTL(ep));
}

void usbc_out_bulk_ep_enable(u32 ep)
{
    u32 val;

    val = readl(USB_DEV_DOEPCTL(ep));
    val |= (USB_DEV_DOEPCTL_CNAK | USB_DEV_DOEPCTL_EPENA);
    writel(val, USB_DEV_DOEPCTL(ep));
}

void usbc_in_bulk_ep_activate(u32 ep)
{
    u32 val = 0, mps = BULK_EP_HS_MPS;

    val = readl(USB_DEV_DAINTMSK);
    val |= (USB_DEV_DAINTMSK_IEP & (0x1 << ep));
    writel(val, USB_DEV_DAINTMSK);

    val = readl(USB_DEV_DIEPCTL(ep));
    if ((val & USB_DEV_DIEPCTL_USBAEP) == 0) {
        val &= ~USB_DEV_DIEPCTL_MPSIZ;
        val |= (USB_DEV_DIEPCTL_MPSIZ & mps);
        val |= (0x2 << 18);             // EP Type is Bulk
        val &= ~USB_DEV_DIEPCTL_TXFNUM; // Set TxFIFO is NP FIFO
        val |= (0x1 << 28);             // Initial data PID is DATA0
        val |= USB_DEV_DIEPCTL_USBAEP;
        writel(val, USB_DEV_DIEPCTL(ep));
    }
}

void usbc_out_bulk_ep_activate(u32 ep)
{
    u32 val = 0, mps = BULK_EP_HS_MPS;

    val = readl(USB_DEV_DAINTMSK);
    val |= (USB_DEV_DAINTMSK_OEP & (0x1 << (ep + 16)));
    writel(val, USB_DEV_DAINTMSK);

    val = readl(USB_DEV_DOEPCTL(ep));
    if ((val & USB_DEV_DOEPCTL_USBAEP) == 0) {
        val &= ~USB_DEV_DOEPCTL_MPSIZ;
        val |= (USB_DEV_DOEPCTL_MPSIZ & mps);
        val |= (0x2 << 18); // EP Type is Bulk
        val |= (0x1 << 28); // Initial data PID is DATA0
        val |= USB_DEV_DOEPCTL_USBAEP;
        writel(val, USB_DEV_DOEPCTL(ep));
    }
}

void usbc_in_ctrl_ep_enable(void)
{
    u32 val;

    val = readl(USB_DEV_DIEPCTL0);
    val |= (USB_DEV_DIEPCTL_CNAK | USB_DEV_DIEPCTL_EPENA);
    writel(val, USB_DEV_DIEPCTL0);
}

void usbc_in_ctrl_ep_activate(void)
{
    u32 val = 0;

    val = readl(USB_DEV_DAINTMSK);
    val |= 0x1;
    writel(val, USB_DEV_DAINTMSK);

    val = readl(USB_DEV_DIEPCTL0);
    if ((val & USB_DEV_DIEPCTL_USBAEP) == 0) {
        val &= 0xFFFFFFFC; //maxpacket = 64byte
        val |= USB_DEV_DIEPCTL_USBAEP;
        writel(val, USB_DEV_DIEPCTL0);
    }
}

void usbc_out_ctrl_ep_enable(void)
{
    u32 val;

    val = readl(USB_DEV_DOEPCTL0);
    val |= (USB_DEV_DOEPCTL_CNAK | USB_DEV_DOEPCTL_EPENA);
    writel(val, USB_DEV_DOEPCTL0);
}

void usbc_out_ctrl_ep_activate(void)
{
    u32 val = 0;

    val = readl(USB_DEV_DAINTMSK);
    val |= (USB_DEV_DAINTMSK_OEP & (0x1 << 16));
    writel(val, USB_DEV_DAINTMSK);

    val = readl(USB_DEV_DOEPCTL0);
    if ((val & USB_DEV_DOEPCTL_USBAEP) == 0) {
        val &= 0xFFFFFFFC; //maxpacket = 64byte
        val |= USB_DEV_DOEPCTL_USBAEP;
        writel(val, USB_DEV_DOEPCTL0);
    }
}

void usbc_out_ctrl_ep_xfer_cfg(u32 setup_pkt_max, u32 pkt_cnt, u32 xfersiz)
{
    u32 val = 0;

    pr_debug("%s, setup %X, pkt %x, siz %X\n", __func__, setup_pkt_max, pkt_cnt,
             xfersiz);
    /*
     * USB_DEV_DOEPTSIZ0:
     * Bit[6:0] Transfer size going to recieve
     * Bit[19] Packet count going to recieve
     * Bit[30:29] Max back-to-back Setup Packet count
     */
    val |= ((pkt_cnt & 0x1) << 19);
    val |= ((xfersiz & 0x7f) << 0);
    val |= ((setup_pkt_max & 0x3) << 29);
    ;
    writel(val, USB_DEV_DOEPTSIZ0);

    usbc_out_ctrl_ep_enable();
}

void usbc_in_ctrl_ep_xfer_cfg(u32 pkt_cnt, u32 xfersiz)
{
    u32 val = 0;

    /*
     * USB_DEV_DIEPTSIZ0:
     * Bit[6:0] Transfer size going to be send
     * Bit[20:19] Packet count going to be send
     */

    val |= ((pkt_cnt & 0x3) << 19);
    val |= ((xfersiz & 0x7f) << 0);
    writel(val, USB_DEV_DIEPTSIZ0);

    /*
     * Enable EP0 after configure xfer pkt and size
     */
    usbc_in_ctrl_ep_enable();

    /*
     * Enable TX FIFO Empty Interrupt for this EP
     */
    if (xfersiz > 0)
        usbc_intr_gintmsk_enable(USB_DEV_GINTMSK_NPTXFE);
}

void usbc_in_bulk_ep_xfer_cfg(u32 ep, u32 pkt_cnt, u32 xfersiz)
{
    u32 val;

    /*
     * Bit[28:19] PktCnt
     * Bit[18:0]  XferSize
     */
    val = readl(USB_DEV_DIEPTSIZ(ep));
    val &= ~(0x3FF << 19);
    val |= ((pkt_cnt & 0x3FF) << 19);
    val &= ~(0x7FFFF << 0);
    val |= ((xfersiz & 0x7FFFF) << 0);
    writel(val, USB_DEV_DIEPTSIZ(ep));

    /*
     * Enable Bulk EP after configure xfer pkt and size
     */
    usbc_in_bulk_ep_enable(ep);

    /*
     * We need NPTXFE interrupt, enable here
     */
    if (xfersiz > 0)
        usbc_intr_gintmsk_enable(USB_DEV_GINTMSK_NPTXFE);
}

void usbc_out_bulk_ep_xfer_cfg(u32 ep, u32 pkt_cnt, u32 xfersiz)
{
    u32 val;

    /*
     * Bit[28:19] PktCnt
     * Bit[18:0]  XferSize
     */
    val = readl(USB_DEV_DOEPTSIZ(ep));
    val &= ~(0x3FF << 19);
    val |= ((pkt_cnt & 0x3FF) << 19);
    val &= ~(0x7FFFF << 0);
    val |= ((xfersiz & 0x7FFFF) << 0);
    writel(val, USB_DEV_DOEPTSIZ(ep));

    /*
     * Enable Bulk EP after configure xfer pkt and size
     */
    usbc_out_bulk_ep_enable(ep);
}

/*
 * Flush RX FIFO, need to ensure there is no reading transaction before call
 * this API.
 */
void usbc_flush_rxfifo(void)
{
    u32 val, maxtry = 3000;

    /*
     * Set bit[4] RxFifo flush
     */
    val = readl(USB_DEV_INIT);
    val |= USB_DEV_INIT_RXFFLSH;
    writel(val, USB_DEV_INIT);

    /*
     * HW will clear this bit when flush is done.
     * Wait here.
     */
    do {
        val = readl(USB_DEV_INIT);
        maxtry--;
    } while ((val & USB_DEV_INIT_RXFFLSH) && maxtry > 0);
}

static void txfifo_flush(u32 num)
{
    u32 val, maxtry = 3000;

    /*
     * bit[5] TxFifo flush
     * bit[10:6] TxFifo Num
     */
    val = readl(USB_DEV_INIT);
    val &= ~(USB_DEV_INIT_TXFFLSH | USB_DEV_INIT_TXFNUM);
    val |= (USB_DEV_INIT_TXFFLSH | (num << 6));
    writel(val, USB_DEV_INIT);

    /*
     * HW will clear this bit when flush is done.
     * Wait here.
     */
    do {
        val = readl(USB_DEV_INIT);
        maxtry--;
    } while ((val & USB_DEV_INIT_TXFFLSH) && maxtry > 0);
}

void usbc_flush_all_txfifo(void)
{
    txfifo_flush(0x10);
}

void usbc_flush_np_txfifo(void)
{
    txfifo_flush(0);
}

void usbc_flush_periodic_txfifo(u32 num)
{
    txfifo_flush(num);
}

u32 usbc_intr_get_gintsts(void)
{
    u32 val, msk;

    val = readl(USB_DEV_GINTSTS);
    msk = readl(USB_DEV_GINTMSK);

    return val & msk;
}

void usbc_intr_clear_gintsts_pending(u32 mask)
{
    u32 val;

    val = readl(USB_DEV_GINTSTS);
    val |= mask;
    writel(val, USB_DEV_GINTSTS);
}

void usbc_intr_gintmsk_enable(u32 intr)
{
    u32 val;

    val = readl(USB_DEV_GINTMSK);
    val |= intr;
    writel(val, USB_DEV_GINTMSK);
}

void usbc_intr_gintmsk_disable(u32 intr)
{
    u32 val;

    val = readl(USB_DEV_GINTMSK);
    val &= (~intr);
    writel(val, USB_DEV_GINTMSK);
}

u32 usbc_get_status_pop(void)
{
    u32 val;

    val = readl(USB_DEV_GRXSTSP);

    return val;
}

/*
 * Get DAINT Register OUT EP part
 */
u32 usbc_get_daint_out(void)
{
    u32 val;

    val = readl(USB_DEV_DAINT);
    val &= readl(USB_DEV_DAINTMSK);
    val = (val & 0xFFFF0000) >> 16;

    return val;
}

/*
 * Get DAINT Register IN EP part
 */
u32 usbc_get_daint_in(void)
{
    u32 val;

    val = readl(USB_DEV_DAINT);
    val &= readl(USB_DEV_DAINTMSK);
    val = (val & 0xFFFF);

    return val;
}

u32 usbc_intr_get_doepint(u32 ep)
{
    u32 val;

    val = readl(USB_DEV_DOEPINT(ep));
    return val;
}

void usbc_intr_clear_doepint(u32 ep, u32 msk)
{
    writel(msk, USB_DEV_DOEPINT(ep));
}

u32 usbc_intr_get_diepint(u32 ep)
{
    u32 val;

    val = readl(USB_DEV_DIEPINT(ep));
    return val;
}

void usbc_intr_clear_diepint(u32 ep, u32 msk)
{
    writel(msk, USB_DEV_DIEPINT(ep));
}

u32 usbc_get_gnptxsts(void)
{
    u32 val;

    val = readl(USB_DEV_GNPTXSTS);

    return val;
}

void usbc_in_ctrl_ep_set_stall(void)
{
    u32 val;

    val = readl(USB_DEV_DIEPCTL0);
    val |= USB_DEV_DIEPCTL_STALL;
    writel(val, USB_DEV_DIEPCTL0);
}

void usbc_out_ctrl_ep_set_stall(void)
{
    u32 val;

    val = readl(USB_DEV_DOEPCTL0);
    val |= USB_DEV_DOEPCTL_STALL;
    writel(val, USB_DEV_DOEPCTL0);
}

void usbc_in_ep_set_stall(u32 ep)
{
    u32 val;

    val = readl(USB_DEV_DIEPCTL(ep));
    val |= USB_DEV_DIEPCTL_STALL;
    if ((val & USB_DEV_DIEPCTL_EPENA) == 0 && ep != 0)
        val &= ~(USB_DEV_DIEPCTL_EPDIS);
    writel(val, USB_DEV_DIEPCTL(ep));
}

void usbc_out_ep_set_stall(u32 ep)
{
    u32 val;

    val = readl(USB_DEV_DOEPCTL(ep));
    val |= USB_DEV_DOEPCTL_STALL;
    if ((val & USB_DEV_DOEPCTL_EPENA) == 0 && ep != 0)
        val &= ~(USB_DEV_DOEPCTL_EPDIS);
    writel(val, USB_DEV_DOEPCTL(ep));
}

void usbc_in_ep_clr_stall(u32 ep)
{
    u32 val;

    val = readl(USB_DEV_DIEPCTL(ep));
    val &= ~USB_DEV_DIEPCTL_STALL;
    writel(val, USB_DEV_DIEPCTL(ep));
}

void usbc_out_ep_clr_stall(u32 ep)
{
    u32 val;

    val = readl(USB_DEV_DOEPCTL(ep));
    val &= ~USB_DEV_DOEPCTL_STALL;
    writel(val, USB_DEV_DOEPCTL(ep));
}

u32 usbc_ctrl_ep_send_data(u8 *buf, u32 len)
{
    u32 gintsts, gnptxsts, len32b, restbyte, i, val;

    gintsts = usbc_intr_get_gintsts();

    /*
     * Wait NPTXFE interrupt
     */
    if ((gintsts & USB_DEV_GINTSTS_NPTXFE) == 0) {
        pr_info("TXFIFO Not ready\n");
        return 0;
    }

    len32b = len >> 2;
    gnptxsts = usbc_get_gnptxsts();
    if ((gnptxsts & USB_DEV_GNPTXSTS_QUEUE_SPC_AVAIL) == 0) {
        pr_err("NPTX FIFO queue not available\n");
        return 0;
    }
    if ((gnptxsts & USB_DEV_GNPTXSTS_FIFO_SPC_AVAIL) < len32b) {
        pr_err("NPTX FIFO space not available\n");
        return 0;
    }
    /*
     * FIFO has enough space
     */
    for (i = 0; i < len32b; i++) {
        val = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
        writel(val, USB_DEV_DFIFO0);
        buf += 4;
    }
    /*
     * Still need to write rest bytes
     * 1~3 bytes
     */
    val = 0;
    restbyte = (len - (len32b << 2));
    for (i = 0; i < restbyte; i++)
        val |= (buf[i] << (i * 8));
    if (restbyte > 0)
        writel(val, USB_DEV_DFIFO0);

    usbc_intr_clear_gintsts_pending(USB_DEV_GINTSTS_NPTXFE);
    pr_debug("%s, len = %X\n", __func__, len);
    return len;
}

u32 usbc_ep_recv_data_cpu(u8 *buf, u32 len)
{
    u32 val, len32b, restbyte, i;

    len32b = len >> 2;
    for (i = 0; i < len32b; i++) {
        val = readl(USB_DEV_DFIFO0);
        memcpy(buf, &val, 4);
        buf += 4;
    }

    val = 0;
    restbyte = (len - (len32b << 2));
    if (restbyte > 0) {
        val = readl(USB_DEV_DFIFO0);
        for (i = 0; i < restbyte; i++)
            buf[i] = (val >> (i * 8)) & 0xFF;
    }

    return len;
}

u32 usbc_bulk_ep_send_data(u32 ep, u8 *buf, u32 len)
{
    u32 ep_intr, gnptxsts, len32b, restbyte, i, val, gintsts;

    ep_intr = usbc_intr_get_diepint(ep);

    /*
     * Wait ITTXFE interrupt
     * IN Token received when TXFIFO is Empty
     * Got IN Token, so it is time to send one MPS DATA packet.
     */
    if ((ep_intr & USB_DEV_DIEPINT_ITTXFE) == 0) {
        pr_info("TXFIFO Not ready\n");
        return 0;
    }

    /*
     * ITTXFE, so NPTXFE already issued, clear it.
     */
    gintsts = usbc_intr_get_gintsts();
    if (gintsts & USB_DEV_GINTSTS_NPTXFE)
        usbc_intr_clear_gintsts_pending(USB_DEV_GINTSTS_NPTXFE);

    /*
     * One time only can send BULK_EP_HS_MPS byte
     */
    if (len > BULK_EP_HS_MPS)
        len = BULK_EP_HS_MPS;

    len32b = len >> 2;
    gnptxsts = usbc_get_gnptxsts();
    if ((gnptxsts & USB_DEV_GNPTXSTS_QUEUE_SPC_AVAIL) == 0) {
        pr_err("NPTX FIFO queue not available\n");
        return 0;
    }
    if ((gnptxsts & USB_DEV_GNPTXSTS_FIFO_SPC_AVAIL) < len32b) {
        pr_err("NPTX FIFO space not available\n");
        return 0;
    }
    /*
     * FIFO has enough space
     */
    for (i = 0; i < len32b; i++) {
        val = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
        writel(val, USB_DEV_DFIFO1);
        buf += 4;
    }
    /*
     * Still need to write rest bytes
     * 1~3 bytes
     */
    val = 0;
    restbyte = (len - (len32b << 2));
    for (i = 0; i < restbyte; i++)
        val |= (buf[i] << (i * 8));
    if (restbyte > 0)
        writel(val, USB_DEV_DFIFO1);

    usbc_intr_clear_diepint(ep, USB_DEV_DIEPINT_ITTXFE);
    pr_debug("%s, len = %X\n", __func__, len);
    return len;
}

