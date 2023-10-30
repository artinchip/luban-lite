/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef _AIC_USBC_H_
#define _AIC_USBC_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CTL_EP0_MPS    (64)
#define BULK_EP_HS_MPS (512)

s32 usbc_init(void);
s32 usbc_dev_core_reset(void);
void usbc_soft_connect(void);
void usbc_soft_disconnect(void);
void usbc_in_ep_reset(u32 idx);
void usbc_out_ep_reset(u32 idx);
void usbc_intr_reset(void);
void usbc_flush_rxfifo(void);
void usbc_flush_all_txfifo(void);
void usbc_flush_np_txfifo(void);
void usbc_flush_periodic_txfifo(u32 num);
void usbc_clock_gating_enable(void);
void usbc_clock_gating_disable(void);
void usbc_set_address(u32 dev_addr);
void usbc_in_ctrl_ep_activate(void);
void usbc_out_ctrl_ep_activate(void);
void usbc_in_ctrl_ep_enable(void);
void usbc_out_ctrl_ep_enable(void);
void usbc_in_ctrl_ep_xfer_cfg(u32 pkt_cnt, u32 xfersiz);
void usbc_out_ctrl_ep_xfer_cfg(u32 setup_pkt_max, u32 pkt_cnt, u32 xfersiz);
void usbc_in_ctrl_ep_set_stall(void);
void usbc_out_ctrl_ep_set_stall(void);
void usbc_set_ctrl_ep_mps(u8 mps);
u32 usbc_ctrl_ep_send_data(u8 *buf, u32 len);
u32 usbc_intr_get_gintsts(void);
void usbc_intr_clear_gintsts_pending(u32 mask);
void usbc_intr_gintmsk_enable(u32 intr);
void usbc_intr_gintmsk_disable(u32 intr);
u32 usbc_get_status_pop(void);
u32 usbc_get_daint_out(void);
u32 usbc_get_daint_in(void);
u32 usbc_intr_get_doepint(u32 ep);
void usbc_intr_clear_doepint(u32 ep, u32 msk);
u32 usbc_get_gnptxsts(void);
u32 usbc_ep_recv_data_cpu(u8 *buf, u32 len);
u32 usbc_get_dev_speed(void);
u32 usbc_intr_get_diepint(u32 ep);
void usbc_intr_clear_diepint(u32 ep, u32 msk);
void usbc_in_bulk_ep_activate(u32 ep);
void usbc_out_bulk_ep_activate(u32 ep);
void usbc_in_bulk_ep_enable(u32 ep);
void usbc_out_bulk_ep_enable(u32 ep);
void usbc_in_ep_set_stall(u32 ep);
void usbc_out_ep_set_stall(u32 ep);
void usbc_in_ep_clr_stall(u32 ep);
void usbc_out_ep_clr_stall(u32 ep);
void usbc_in_bulk_ep_xfer_cfg(u32 ep, u32 pkt_cnt, u32 xfersiz);
void usbc_out_bulk_ep_xfer_cfg(u32 ep, u32 pkt_cnt, u32 xfersiz);
u32 usbc_bulk_ep_send_data(u32 ep, u8 *buf, u32 len);

#ifdef __cplusplus
}
#endif

#endif
