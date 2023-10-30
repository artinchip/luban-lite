/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xuan.Wen <xuan.wen@artinchip.com>
 */

#ifndef __BL_SPINAND_H_
#define __BL_SPINAND_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <hal_qspi.h>
#include <spinand.h>

struct aic_qspi {
    char *name;
    u32 idx;
    u32 clk_id;
    u32 clk_in_hz;
    u32 bus_hz;
    u32 dma_port_id;
    u32 irq_num;
    qspi_master_handle handle;
    struct aic_spinand *attached_flash;
    bool inited;
};

/**
 * SPI message structure
 */
struct aic_spi_message {
    const void *send_buf;
    void *recv_buf;
    u32 length;

    unsigned cs_take    : 1;
    unsigned cs_release : 1;
};

struct aic_qspi_message {
    struct aic_spi_message parent;
    struct aic_qspi_message *next;

    /* instruction stage */
    struct {
        u8 content;
        u8 qspi_lines;
    } instruction;

    /* address and alternate_bytes stage */
    struct {
        u32 content;
        u8 size;
        u8 qspi_lines;
    } address, alternate_bytes;

    /* dummy_cycles stage */
    u32 dummy_cycles;

    /* number of lines in qspi data stage, the other configuration items are in parent */
    u8 qspi_data_lines;
};

/**
 * SPI configuration structure
 */
struct aic_spi_configuration {
    u8 mode;
    u8 data_width;
    u16 reserved;
    u32 max_hz;
};

u32 qspi_xfer(struct aic_qspi *qspi, struct aic_qspi_message *qspi_message);
u32 qspi_configure(struct aic_qspi *qspi,
                   struct aic_spi_configuration *configuration);
struct aic_qspi *get_qspi_by_index(u32 idx);

struct aic_spinand *spinand_probe(u32 spi_bus);

#ifdef __cplusplus
}
#endif

#endif /* __BL_SPINAND_H_ */
