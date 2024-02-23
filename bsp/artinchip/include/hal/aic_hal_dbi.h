/*
 * Copyright (C) 2020-2022 ArtInChip Technology Co., Ltd.
 * Authors:  matteo <duanmt@artinchip.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HAL_DBI_H_
#define _HAL_DBI_H_

#include <aic_common.h>
#include <aic_io.h>
#include "aic_hal_disp_reg_util.h"

/* DBI type */
#define I8080   0x2
#define SPI     0x3

/* I8080 format */
#define I8080_RGB565_8BIT           0x0
#define I8080_RGB666_8BIT           0x1
#define I8080_RGB666_9BIT           0x2
#define I8080_RGB666_16BIT_3CYCLE   0x3
#define I8080_RGB666_16BIT_2CYCLE   0x4
#define I8080_RGB565_16BIT          0x5
#define I8080_RGB666_18BIT          0x6
#define I8080_RGB888_24BIT          0x7

/* SPI format */
#define SPI_3LINE_RGB565            0x0
#define SPI_3LINE_RGB666            0x1
#define SPI_3LINE_RGB888            0x2
#define SPI_4LINE_RGB565            0x3
#define SPI_4LINE_RGB666            0x4
#define SPI_4LINE_RGB888            0x5
#define SPI_4SDA_RGB565             0x6
#define SPI_4SDA_RGB666             0x7
#define SPI_4SDA_RGB888             0x8

#define SPI_MODE_NUM                0x3

#define DBI_CTL_TYPE_MASK               GENMASK(5, 4)
#define DBI_CTL_TYPE(x)                 (((x)&0x3) << 4)
#define DBI_CTL_EN                      BIT(0)
#define DBI_CTL_I8080_TYPE_MASK         GENMASK(19, 16)
#define DBI_CTL_I8080_TYPE(x)           (((x) & 0xF) << 16)
#define DBI_CTL_SPI_TYPE_MASK           GENMASK(21, 20)
#define DBI_CTL_SPI_TYPE(x)             (((x) & 0x3)<<20)
#define DBI_CTL_SPI_FORMAT_MASK         GENMASK(27, 24)
#define DBI_CTL_SPI_FORMAT(x)           (((x) & 0xF)<<24)

#define DBI_I8080_IDEL                  BIT(4)
#define DBI_I8080_IDEL_SHIFT            4

#define DBI_I8080_TX_FIFO_EMPTY         BIT(1)
#define DBI_I8080_TX_FIFO_EMPTY_SHIFT   1
#define DBI_I8080_RD_FIFO_FLUSH         BIT(21)
#define DBI_I8080_WR_FIFO_FLUSH         BIT(20)
#define DBI_I8080_RD_FIFO_DEPTH_MASK    GENMASK(14, 8)
#define DBI_I8080_RD_FIFO_DEPTH_SHIFT   8
#define DBI_I8080_WR_FIFO_DEPTH_MASK    GENMASK(6, 0)
#define DBI_I8080_WR_FIFO_DEPTH_SHIFT   0

#define DBI_SPI_IDEL                    BIT(4)
#define DBI_SPI_IDEL_SHIFT              4

#define DBI_SPI_TX_FIFO_EMPTY           BIT(1)
#define DBI_SPI_TX_FIFO_EMPTY_SHIFT     1
#define DBI_SPI_RD_FIFO_FLUSH           BIT(21)
#define DBI_SPI_WR_FIFO_FLUSH           BIT(20)
#define DBI_SPI_RD_FIFO_DEPTH_MASK      GENMASK(14, 8)
#define DBI_SPI_RD_FIFO_DEPTH_SHIFT     8
#define DBI_SPI_WR_FIFO_DEPTH_MASK      GENMASK(6, 0)
#define DBI_SPI_WR_FIFO_DEPTH_SHIFT     0

#define FIRST_LINE_COMMAND_MASK         GENMASK(15, 8)
#define OTHER_LINE_COMMAND_MASK         GENMASK(31, 24)
#define FIRST_LINE_COMMAND(x)           (((x) & 0xff) << 8)
#define OTHER_LINE_COMMAND(x)           (((x) & 0xff) << 24)
#define FIRST_LINE_COMMAND_CTL          BIT(0)
#define OTHER_LINE_COMMAND_CTL          BIT(16)

#define DBI_SPI_CODE1_CFG_MASK          GENMASK(23, 16)
#define DBI_SPI_CODE1_CFG(x)            (((x) & 0xff) << 16)
#define DBI_SPI_VBP_NUM_MASK            GENMASK(15, 8)
#define DBI_SPI_VBP_NUM(x)              (((x) & 0xff) << 8)
#define DBI_QSPI_MODE_MASK              BIT(0)

#define DBI_SCL_CTL                     BIT(4)
#define DBI_SCL_PHASE_CFG               BIT(1)
#define DBI_SCL_POL                     BIT(0)

#define RESET_DBI                       RESET_RGB
#define CLK_DBI                         CLK_RGB
#define DBI_BASE                        LCD_BASE
#define DBI_CTL                         0x00

#define DBI_I8080_COMMAND_CTL           0x100
#define DBI_I8080_WR_CMD                0x104
#define DBI_I8080_WR_DATA               0x108
#define DBI_I8080_WR_CTL                0x10C
#define DBI_I8080_RD_CTL                0x110
#define DBI_I8080_RD_DATA               0x114
#define DBI_I8080_FIFO_DEPTH            0x118
#define DBI_I8080_INT_ENABLE            0x11C
#define DBI_I8080_INT_CLR               0x120
#define DBI_I8080_STATUS                0x124
#define DBI_I8080_CLK_CTL               0x128

#define DBI_SPI_SCL_CFG                 0x200
#define DBI_QSPI_CODE                   0x204
#define DBI_SPI_COMMAND_CTL             0x208
#define DBI_SPI_WR_CMD                  0x20C
#define DBI_SPI_WR_DATA                 0x210
#define DBI_SPI_WR_CTL                  0x214
#define DBI_SPI_RD_CTL                  0x218
#define DBI_SPI_RD_DATA                 0x21C
#define DBI_SPI_FIFO_DEPTH              0x220
#define DBI_SPI_INT_ENABLE              0x224
#define DBI_SPI_INT_CLR                 0x228
#define DBI_SPI_STATUS                  0x22C
#define DBI_QSPI_MODE                   0x234

void i8080_cmd_wr(void *base, u32 code, u32 count, const u8 *data);
void i8080_cmd_ctl(void *base, u32 first_line, u32 other_line);

void i8080_rd_ctl(void *base, u32 count, u32 start);
u32 i8080_rd_data(void *base);

u32 i8080_rd_fifo_depth(void *base);
u32 i8080_wr_fifo_depth(void *base);
void i8080_rd_fifo_flush(void *base);
void i8080_wr_fifo_flush(void *base);

void spi_cmd_wr(void *base, u32 code, u32 count, const u8 *data);
void spi_cmd_ctl(void *base, u32 first_line, u32 other_line);
void spi_scl_cfg(void *base, u32 phase, u32 pol);

void spi_rd_ctl(void *base, u32 count, u32 start);
u32 spi_rd_data(void *base);

u32 spi_rd_fifo_depth(void *base);
u32 spi_wr_fifo_depth(void *base);
void spi_rd_fifo_flush(void *base);
void spi_wr_fifo_flush(void *base);

void qspi_code_cfg(void *base, u32 code1, u32 code2, u32 code3);
void qspi_mode_cfg(void *base, u32 code1_cfg, u32 vbp_num, u32 qspi_mode);

#endif // end of _HAL_DBI_H_
