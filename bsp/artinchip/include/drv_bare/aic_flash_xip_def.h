/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#ifndef __BL_AIC_FLASH_XIP_DEF_H_
#define __BL_AIC_FLASH_XIP_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_qspi.h>
#include <mtd.h>
#ifdef AIC_QSPI_DRV_V11

#ifdef AIC_SPINOR_DRV
#include <sfud.h>
struct aic_qspi_bus {
    char *name;
    u32 idx;
    u32 clk_id;
    u32 clk_in_hz;
    u32 bus_hz;
    u32 dma_port_id;
    u32 irq_num;
    qspi_master_handle handle;
    int probe_flag;
    sfud_flash attached_flash;
};
#endif

#define CMD_PROTO_QIO                       (0x1U << 0)
#define CMD_PROTO_QPI                       (0x1U << 1)
#define CMD_PROTO_CACHE_EN                  (0x1U << 2)
#define CMD_PROTO_CACHE_DIS                 (0x1U << 2)
#define XIP_C0_BITS_WIDTH_4                 0
#define XIP_C0_BITS_WIDTH_1                 1
#define XIP_BURST_WRAPPED_DISABLED          0x00
#define XIP_BURST_WRAPPED_WITH_FIXED_LEN    0x01
#define XIP_BURST_WRAPPED_WITH_AUTO_SEL_LEN 0x02
#define SPINOR_ADDR_MODE_3BYTE              0
#define SPINOR_ADDR_MODE_4BYTE              1
#define QSPI_CS_CTL_BY_HW                   0
#define QSPI_CS_CTL_BY_SW                   1

struct xip_device {
    const char *name;
    u32 flash_id;
    u32 proto_support;
    struct qspi_xip_burst_cfg burst_cfg;
    struct qspi_xip_read_cfg read_cfg;
};

struct xip_device *get_xip_device_cfg(u32 flash_id, u32 support_msk, u32
        support_val);
#endif // AIC_QSPI_DRV_V11
#ifdef __cplusplus
}
#endif

#endif /* __BL_AIC_FLASH_XIP_DEF_H_ */
