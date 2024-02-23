/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include <aic_iopoll.h>

#include "aic_hal_dbi.h"

#define DBI_TIMEOUT_US  1000000

void i8080_cmd_ctl(void *base, u32 first_line, u32 other_line)
{
    reg_set_bits(base + DBI_I8080_COMMAND_CTL,
            FIRST_LINE_COMMAND_MASK,
            FIRST_LINE_COMMAND(first_line));
    reg_set_bit(base + DBI_I8080_COMMAND_CTL, FIRST_LINE_COMMAND_CTL);

    reg_set_bits(base + DBI_I8080_COMMAND_CTL,
            OTHER_LINE_COMMAND_MASK,
            OTHER_LINE_COMMAND(other_line));
    reg_set_bit(base + DBI_I8080_COMMAND_CTL, OTHER_LINE_COMMAND_CTL);
}

void i8080_wr_cmd(void *base, u32 cmd)
{
    reg_write(base + DBI_I8080_WR_CMD, cmd);
}

void i8080_wr_data(void *base, u32 data)
{
    reg_write(base + DBI_I8080_WR_DATA, data);
}

void i8080_wr_ctl(void *base, u32 count, u32 start)
{
    reg_write(base + DBI_I8080_WR_CTL, count << 8 | start);
}

void i8080_rd_ctl(void *base, u32 count, u32 start)
{
    reg_write(base + DBI_I8080_RD_CTL, count << 8 | start);
}

u32 i8080_rd_data(void *base)
{
    return reg_read(base + DBI_I8080_RD_DATA);
}

u32 i8080_rd_fifo_depth(void *base)
{
    return reg_rd_bits(base + DBI_I8080_FIFO_DEPTH,
            DBI_I8080_RD_FIFO_DEPTH_MASK,
            DBI_I8080_RD_FIFO_DEPTH_SHIFT);
}

u32 i8080_wr_fifo_depth(void *base)
{
    return reg_rd_bits(base + DBI_I8080_FIFO_DEPTH,
            DBI_I8080_WR_FIFO_DEPTH_MASK,
            DBI_I8080_WR_FIFO_DEPTH_SHIFT);
}

void i8080_rd_fifo_flush(void *base)
{
    reg_set_bit(base + DBI_I8080_FIFO_DEPTH, DBI_I8080_RD_FIFO_FLUSH);
}

void i8080_wr_fifo_flush(void *base)
{
    reg_set_bit(base + DBI_I8080_FIFO_DEPTH, DBI_I8080_RD_FIFO_FLUSH);
}

void i8080_cmd_wr(void *base, u32 code, u32 count, const u8 *data)
{
    int i, ret;
    u32 val;

    i8080_wr_cmd(base, code);

    for (i = 0; i < count; i++)
        i8080_wr_data(base, *(data + i));

    i8080_wr_ctl(base, count, 1);

#ifdef AIC_DISP_MIPI_DBI_DEBUG
    printf("command: %#x, ", code);
    printf("data:");
    for (i = 0; i < count; i++)
        printf(" %#x", *(data + i));

    printf("\n");
#endif

    ret = readl_poll_timeout(base + DBI_I8080_STATUS, val,
            ((val & (DBI_I8080_IDEL | DBI_I8080_TX_FIFO_EMPTY)) == 0x2),
            DBI_TIMEOUT_US);
    if (ret)
        pr_err("Timeout during i8080 write command\n");
}

void qspi_code_cfg(void *base, u32 code1, u32 code2, u32 code3)
{
    reg_write(base + DBI_QSPI_CODE, code1 << 16 | code2 << 8 | code3);
}

void qspi_mode_cfg(void *base, u32 code1_cfg, u32 vbp_num, u32 qspi_mode)
{
    reg_set_bits(base + DBI_QSPI_MODE,
            DBI_SPI_CODE1_CFG_MASK,
            DBI_SPI_CODE1_CFG(code1_cfg));

    reg_set_bits(base + DBI_QSPI_MODE,
            DBI_SPI_VBP_NUM_MASK,
            DBI_SPI_VBP_NUM(vbp_num));
    if (qspi_mode)
        reg_set_bit(base + DBI_QSPI_MODE, DBI_QSPI_MODE_MASK);
    else
        reg_clr_bit(base + DBI_QSPI_MODE, DBI_QSPI_MODE_MASK);
}

void spi_cmd_ctl(void *base, u32 first_line, u32 other_line)
{
    reg_set_bits(base + DBI_SPI_COMMAND_CTL,
            FIRST_LINE_COMMAND_MASK,
            FIRST_LINE_COMMAND(first_line));
    reg_set_bit(base + DBI_SPI_COMMAND_CTL, FIRST_LINE_COMMAND_CTL);

    reg_set_bits(base + DBI_SPI_COMMAND_CTL,
            OTHER_LINE_COMMAND_MASK,
            OTHER_LINE_COMMAND(other_line));
    reg_set_bit(base + DBI_SPI_COMMAND_CTL, OTHER_LINE_COMMAND_CTL);
}

void spi_scl_cfg(void *base, u32 phase, u32 pol)
{
    reg_clr_bit(base + DBI_SPI_SCL_CFG, DBI_SCL_PHASE_CFG);

    if (phase)
        reg_set_bit(base + DBI_SPI_SCL_CFG, DBI_SCL_PHASE_CFG);
    else
        reg_clr_bit(base + DBI_SPI_SCL_CFG, DBI_SCL_PHASE_CFG);

    if (pol)
        reg_set_bit(base + DBI_SPI_SCL_CFG, DBI_SCL_POL);
    else
        reg_clr_bit(base + DBI_SPI_SCL_CFG, DBI_SCL_POL);
}

void spi_wr_cmd(void *base, u32 cmd)
{
    reg_write(base + DBI_SPI_WR_CMD, cmd);
}

void spi_wr_data(void *base, u32 data)
{
    reg_write(base + DBI_SPI_WR_DATA, data);
}

void spi_wr_ctl(void *base, u32 count, u32 start)
{
    reg_write(base + DBI_SPI_WR_CTL, count << 8 | start);
}

void spi_rd_ctl(void *base, u32 count, u32 start)
{
    reg_write(base + DBI_SPI_RD_CTL, count << 8 | start);
}

u32 spi_rd_data(void *base)
{
    return reg_read(base + DBI_SPI_RD_DATA);
}

u32 spi_rd_fifo_depth(void *base)
{
    return reg_rd_bits(base + DBI_SPI_FIFO_DEPTH,
            DBI_SPI_RD_FIFO_DEPTH_MASK,
            DBI_SPI_RD_FIFO_DEPTH_SHIFT);
}

u32 spi_wr_fifo_depth(void *base)
{
    return reg_rd_bits(base + DBI_SPI_FIFO_DEPTH,
            DBI_SPI_WR_FIFO_DEPTH_MASK,
            DBI_SPI_WR_FIFO_DEPTH_SHIFT);
}

void spi_rd_fifo_flush(void *base)
{
    reg_set_bit(base + DBI_SPI_FIFO_DEPTH, DBI_SPI_RD_FIFO_FLUSH);
}

void spi_wr_fifo_flush(void *base)
{
    reg_set_bit(base + DBI_SPI_FIFO_DEPTH, DBI_SPI_WR_FIFO_FLUSH);
}

void spi_cmd_wr(void *base, u32 code, u32 count, const u8 *data)
{
    int i, ret;
    u32 val;

    spi_wr_cmd(base, code);

    for (i = 0; i < count; i++)
        spi_wr_data(base, *(data + i));

    spi_wr_ctl(base, count, 1);

#ifdef AIC_DISP_MIPI_DBI_DEBUG
    printf("command: %#x, ", code);
    printf("data:");
    for (i = 0; i < count; i++)
        printf(" %#x", *(data + i));

    printf("\n");
#endif

    ret = readl_poll_timeout(base + DBI_SPI_STATUS, val,
            ((val & (DBI_SPI_IDEL | DBI_SPI_TX_FIFO_EMPTY)) == 0x2),
            DBI_TIMEOUT_US);
    if (ret)
        pr_err("Timeout during spi write command\n");
}


