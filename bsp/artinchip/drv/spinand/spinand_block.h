/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#ifndef __SPI_NAND_BLOCK_H__
#define __SPI_NAND_BLOCK_H__

#include <rtthread.h>
#include <rtdevice.h>

int rt_blk_nand_register_device(const char *name, struct rt_mtd_nand_device *mtd_device);

#endif /* __SPI_NAND_BLOCK_H__ */
