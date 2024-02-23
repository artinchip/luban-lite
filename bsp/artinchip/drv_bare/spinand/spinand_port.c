/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xuan.Wen <xuan.wen@artinchip.com>
 */

#include <rtconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <aic_common.h>
#include <string.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <driver.h>
#include <hal_dma.h>
#include <hal_qspi.h>
#include <aic_dma_id.h>
#include <aic_clk_id.h>
#include "spinand_port.h"

//#define AIC_BOOT_QSPI_DRV_DEBUG

#if defined(AIC_QSPI_DRV_V10)
#if defined(AIC_BOOTLOADER)
#define DMA_SLAVE_MAXBURST_DEFAULT 8
#else
#define DMA_SLAVE_MAXBURST_DEFAULT 1
#endif
#elif defined(AIC_QSPI_DRV_V20)
#define DMA_SLAVE_MAXBURST_DEFAULT 1
#else
#define DMA_SLAVE_MAXBURST_DEFAULT 8
#endif

static struct aic_qspi qspi_controller[] = {
#if defined(AIC_USING_QSPI0)
    {
        .name = "qspi0",
        .idx = 0,
        .clk_id = CLK_QSPI0,
        .clk_in_hz = AIC_DEV_QSPI0_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_QSPI0_DEVICE_SPINAND_FREQ,
        .dma_port_id = DMA_ID_SPI0,
        .irq_num = QSPI0_IRQn,
    },
#endif
#if defined(AIC_USING_QSPI1)
    {
        .name = "qspi1",
        .idx = 1,
        .clk_id = CLK_QSPI1,
        .clk_in_hz = AIC_DEV_QSPI1_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_QSPI1_DEVICE_SPINAND_FREQ,
        .dma_port_id = DMA_ID_SPI1,
        .irq_num = QSPI1_IRQn,
    },
#endif
#if defined(AIC_USING_QSPI2)
    {
        .name = "qspi2",
        .idx = 2,
        .clk_id = CLK_QSPI2,
        .clk_in_hz = AIC_DEV_QSPI2_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_QSPI2_DEVICE_SPINAND_FREQ,
        .dma_port_id = DMA_ID_SPI2,
        .irq_num = QSPI2_IRQn,
    },
#endif
#if defined(AIC_USING_QSPI3)
    {
        .name = "qspi3",
        .idx = 3,
        .clk_id = CLK_QSPI3,
        .clk_in_hz = AIC_DEV_QSPI3_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_QSPI3_DEVICE_SPINAND_FREQ,
        .dma_port_id = DMA_ID_SPI3,
        .irq_num = QSPI3_IRQn,
    },
#endif
};

struct aic_qspi *get_qspi_by_index(u32 idx)
{
    struct aic_qspi *qspi;
    u32 i;

    qspi = NULL;
    for (i = 0; i < ARRAY_SIZE(qspi_controller); i++) {
        if (i == idx) {
            qspi = &qspi_controller[i];
            break;
        }
    }

    return qspi;
}

static u32 address_copy(u32 addr, u32 size, u8 *dst)
{
    u32 i;

    i = 0;
    while (size) {
        dst[i++] = (addr >> (8 * (size - 1))) & 0xFF;
        size--;
    }
    return i;
}

#ifdef AIC_BOOT_QSPI_DRV_DEBUG
void dump_cmd(uint8_t *data, uint32_t len)
{
    uint32_t i;

    printf("CMD: (%u) ", len);
    for (i = 0; i < len; i++)
        printf("%02X ", data[i]);
    printf("\n");
}

void dump_rxdata(uint8_t *data, uint32_t len)
{
    uint32_t i;

    printf("rxdata: (%u) ", len);
    for (i = 0; i < len; i++)
        printf("%02X ", data[i]);
    printf("\n");
}

void dump_txdata(const u8 *data, uint32_t len)
{
    uint32_t i;

    printf("txdata: (%u) ", len);
    for (i = 0; i < len; i++)
        printf("%02X ", data[i]);
    printf("\n");
}
#endif

static int qspi_send_command(struct aic_qspi *qspi,
                             struct aic_qspi_message *qspi_message)
{
    u32 single_cnt, rest_cnt, cmd_rest_bw, dmycnt, dmybw;
    u8 cmdbuf1[16], cmdbuf2[16];
    struct qspi_transfer t;
    int ret = 0;

    single_cnt = 0;
    rest_cnt = 0;
    cmd_rest_bw = HAL_QSPI_BUS_WIDTH_SINGLE;
    dmybw = HAL_QSPI_BUS_WIDTH_SINGLE;
    if (qspi_message->instruction.qspi_lines == 1) {
        cmdbuf1[single_cnt++] = qspi_message->instruction.content;
    } else if (qspi_message->instruction.qspi_lines > 1) {
        cmdbuf2[rest_cnt++] = qspi_message->instruction.content;
        cmd_rest_bw = qspi_message->instruction.qspi_lines;
        dmybw = qspi_message->instruction.qspi_lines;
    }

    if ((qspi_message->address.size != 0) &&
        (qspi_message->address.qspi_lines == 1)) {
        single_cnt +=
            address_copy(qspi_message->address.content,
                         qspi_message->address.size, &cmdbuf1[single_cnt]);
        dmybw = qspi_message->address.qspi_lines;
    } else if ((qspi_message->address.size != 0) &&
               (qspi_message->address.qspi_lines > 1)) {
        rest_cnt +=
            address_copy(qspi_message->address.content,
                         qspi_message->address.size, &cmdbuf2[rest_cnt]);
        cmd_rest_bw = qspi_message->address.qspi_lines;
        dmybw = qspi_message->address.qspi_lines;
    }

    dmycnt = (dmybw * qspi_message->dummy_cycles) / 8;
    if ((dmycnt != 0) && (dmybw == 1)) {
        memset(&cmdbuf1[single_cnt], 0, dmycnt);
        single_cnt += dmycnt;
    } else if (dmycnt != 0) {
        memset(&cmdbuf2[rest_cnt], 0, dmycnt);
        rest_cnt += dmycnt;
    }

#ifdef AIC_BOOT_QSPI_DRV_DEBUG
    if (single_cnt)
        dump_cmd(cmdbuf1, single_cnt);
    if (rest_cnt)
        dump_cmd(cmdbuf2, rest_cnt);
#endif

    /* Command + Address + Dummy stage */
    if (single_cnt > 0) {
        hal_qspi_master_set_bus_width(&qspi->handle, HAL_QSPI_BUS_WIDTH_SINGLE);
        t.rx_data = NULL;
        t.tx_data = cmdbuf1;
        t.data_len = single_cnt;
        /* Data is short, use sync API to send */
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
        if (ret) {
            pr_err("Send command failed.\n");
            return -1;
        }
    }
    if (rest_cnt > 0) {
        hal_qspi_master_set_bus_width(&qspi->handle, cmd_rest_bw);
        t.rx_data = NULL;
        t.tx_data = cmdbuf2;
        t.data_len = rest_cnt;
        /* Data is short, use sync API to send */
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
        if (ret) {
            pr_err("Send command failed.\n");
            return -1;
        }
#ifdef AIC_BOOT_QSPI_DRV_DEBUG
        if (rest_cnt)
            dump_cmd(cmdbuf2, rest_cnt);
#endif

    }

    return 0;
}

static u32 aic_qspi_send(struct aic_qspi *qspi,
                         struct aic_qspi_message *qspi_message, const u8 *tx,
                         u32 size)
{
    struct qspi_transfer t;
    u32 ret = 0;

    /* Command + Address + Dummy stage */
    ret = qspi_send_command(qspi, qspi_message);
    if (ret)
        goto out;

    /* Data stage */
    if (size > 0) {
        hal_qspi_master_set_bus_width(&qspi->handle,
                                      qspi_message->qspi_data_lines);
        t.rx_data = NULL;
        t.tx_data = (u8 *)tx;
        t.data_len = size;
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
        if (ret) {
            pr_err("Send data failed.\n");
            goto out;
        }
#ifdef AIC_BOOT_QSPI_DRV_DEBUG
        dump_txdata(tx, size);
#endif
    }

    return size;
out:
    return ret;
}

static u32 aic_qspi_receive(struct aic_qspi *qspi,
                            struct aic_qspi_message *qspi_message, u8 *rx,
                            u32 size)
{
    struct qspi_transfer t;
    u32 ret = 0;

    if (!qspi)
        return -1;

    /* Command + Address + Dummy stage */
    ret = qspi_send_command(qspi, qspi_message);
    if (ret)
        goto out;

    /* Data stage */
    if (size > 0) {
        hal_qspi_master_set_bus_width(&qspi->handle,
                                      qspi_message->qspi_data_lines);
        t.tx_data = NULL;
        t.rx_data = rx;
        t.data_len = size;
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
        if (ret) {
            pr_err("Recv data failed.\n");
            goto out;
        }
#ifdef AIC_BOOT_QSPI_DRV_DEBUG
        dump_rxdata(rx, size);
#endif
    }

    return size;
out:
    return ret;
}

u32 qspi_xfer(struct aic_qspi *qspi, struct aic_qspi_message *qspi_message)
{
    const u8 *sndb = qspi_message->parent.send_buf;
    u8 *rcvb = qspi_message->parent.recv_buf;
    u32 ret = 0;

    if (qspi_message->parent.cs_take)
        hal_qspi_master_set_cs(&qspi->handle, 0, true);

    if (rcvb)
        ret = aic_qspi_receive(qspi, qspi_message, rcvb,
                               qspi_message->parent.length);
    else if (qspi_message->instruction.qspi_lines || sndb)
        ret = aic_qspi_send(qspi, qspi_message, sndb,
                            qspi_message->parent.length);

    if (qspi_message->parent.cs_release)
        hal_qspi_master_set_cs(&qspi->handle, 0, false);

    return ret;
}

u32 qspi_configure(struct aic_qspi *qspi,
                   struct aic_spi_configuration *configuration)
{
    u32 ret = 0;
    struct qspi_master_config cfg = {0};

    if (qspi->inited)
        return 0;

    memset(&cfg, 0, sizeof(cfg));
    cfg.idx = qspi->idx;
    cfg.clk_in_hz = qspi->clk_in_hz;
    cfg.clk_id = qspi->clk_id;
    cfg.cpol = HAL_QSPI_CPOL_ACTIVE_HIGH;
    cfg.cpha = HAL_QSPI_CPHA_FIRST_EDGE;
    cfg.cs_polarity = HAL_QSPI_CS_POL_VALID_LOW;

    ret = hal_qspi_master_init(&qspi->handle, &cfg);
    if (ret) {
        pr_err("qspi init failed.\n");
        return ret;
    }
#ifdef AIC_DMA_DRV
    struct qspi_master_dma_config dmacfg;
    memset(&dmacfg, 0, sizeof(dmacfg));
    dmacfg.port_id = qspi->dma_port_id;

    dmacfg.tx_bus_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;
    dmacfg.rx_bus_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;
    dmacfg.tx_max_burst = DMA_SLAVE_MAXBURST_DEFAULT;
    dmacfg.rx_max_burst = DMA_SLAVE_MAXBURST_DEFAULT;

    ret = hal_qspi_master_dma_config(&qspi->handle, &dmacfg);
    if (ret) {
        pr_err("qspi dma config failed.\n");
        return ret;
    }
#endif
    hal_qspi_master_set_bus_freq(&qspi->handle, qspi->bus_hz);
    qspi->inited = true;
    return ret;
}

void qspi_messages_init(struct aic_qspi_message *qspi_messages,
                        struct spi_nand_cmd_cfg *cfg, u32 addr, u8 *sendbuff,
                        u8 *recvbuff, u32 datacount)
{
    pr_debug(
        "%s %d opcode = 0x%x addr_bytes = 0x%x addr_bits = 0x%x dummy_bytes = 0x%x data_bits = 0x%x\n",
        __func__, __LINE__, cfg->opcode, cfg->addr_bytes, cfg->addr_bits,
        cfg->dummy_bytes, cfg->data_bits);

    /* 1-bit mode */
    qspi_messages->instruction.content = cfg->opcode;
    qspi_messages->instruction.qspi_lines = cfg->opcode_bits;

    qspi_messages->address.content = addr;
    qspi_messages->address.size = cfg->addr_bytes;
    qspi_messages->address.qspi_lines = cfg->addr_bits;

    if (cfg->addr_bits)
        qspi_messages->dummy_cycles =
            (cfg->dummy_bytes * 8) / qspi_messages->address.qspi_lines;
    else
        qspi_messages->dummy_cycles =
            (cfg->dummy_bytes * 8) / qspi_messages->instruction.qspi_lines;

    /* 4-bit mode */
    qspi_messages->qspi_data_lines = cfg->data_bits;
    qspi_messages->parent.cs_take = 1;
    qspi_messages->parent.cs_release = 1;
    qspi_messages->parent.send_buf = sendbuff;
    qspi_messages->parent.recv_buf = recvbuff;
    qspi_messages->parent.length = datacount;
}

int aic_spinand_transfer_message(struct aic_spinand *flash,
                                 struct spi_nand_cmd_cfg *cfg, u32 addr,
                                 u8 *sendbuff, u8 *recvbuff, u32 datacount)
{
    int result;
    struct aic_qspi *qspi = flash->user_data;
    struct aic_qspi_message qspi_message = { 0 };

    qspi_messages_init(&qspi_message, cfg, addr, sendbuff, recvbuff, datacount);

    /* transmit each QSPI message */
    if (qspi_xfer(qspi, &qspi_message) < 0) {
        printf("Xfer SPI bus failed\n");
        result = -1;
        goto __exit;
    }

    result = 0;
__exit:
    return result;
}
