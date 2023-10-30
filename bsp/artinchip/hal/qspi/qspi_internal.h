/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_HAL_QSPI_INTERNAL_H_
#define _AIC_HAL_QSPI_INTERNAL_H_
#include <aic_common.h>
#include <aic_soc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QSPI_WORK_MODE_SYNC_RX_CPU  0
#define QSPI_WORK_MODE_SYNC_TX_CPU  1
#define QSPI_WORK_MODE_ASYNC_RX_CPU 2
#define QSPI_WORK_MODE_ASYNC_TX_CPU 3
#define QSPI_WORK_MODE_ASYNC_RX_DMA 4
#define QSPI_WORK_MODE_ASYNC_TX_DMA 5

#define HAL_QSPI_STATUS_INTERNAL_MSK        (0xFFFFUL << 16)
#define HAL_QSPI_STATUS_ASYNC_TDONE         (0x1UL << 16)
#define HAL_QSPI_STATUS_ASYNC_DMA_DONE      (0x1UL << 17)
#define HAL_QSPI_STATUS_ASYNC_ALL_DONE      (HAL_QSPI_STATUS_ASYNC_TDONE | HAL_QSPI_STATUS_ASYNC_DMA_DONE)
#define QSPI_IS_ASYNC_ALL_DONE(sts, msk)    ((sts & msk) == msk)
#define PTR2U32(p) ((u32)(unsigned long)(p))

void show_freq(char *msg, u32 id, u32 hz);
void hal_qspi_fifo_reset(u32 base, u32 fifo);
void hal_qspi_show_ists(u32 id, u32 sts);
int qspi_fifo_write_data(u32 base, u8 *data, u32 len, u32 tmo);
int qspi_fifo_read_data(u32 base, u8 *data, u32 len, u32 tmo_us);
int qspi_wait_transfer_done(u32 base, u32 tmo);
u32 qspi_calc_timeout(u32 bus_hz, u32 bw, u32 len);

#ifdef __cplusplus
}
#endif

#endif
