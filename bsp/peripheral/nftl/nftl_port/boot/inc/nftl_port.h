/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: mingfeng.li <mingfeng.li@artinchip.com>
 */

#ifndef _NFTL_PORT_H
#define _NFTL_PORT_H

#include <aic_common.h>
#include <aic_core.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "nftl_api.h"

#define NFTL_DBG_MESSAGE_ON 0
#define NFTL_ERR_MESSAGE_ON 1

#if NFTL_DBG_MESSAGE_ON
#define NFTL_DBG PRINT
#else
#define NFTL_DBG(...)
#endif

#if NFTL_ERR_MESSAGE_ON
#define NFTL_ERR PRINT
#else
#define NFTL_ERR(...)
#endif

#define NFTL_INFO PRINT

//define the message print interface
#define PRINT     printf //NAND_Print(__VA_ARGS__)
#define PRINT_DBG printf //NAND_Print_DBG(__VA_ARGS__)

void *nftl_memcpy(void *str1, const void *str2, int size);
void nftl_memset(void *str, int c, int size);
void *nftl_malloc(u32 size);
void nftl_free(const void *ptr);
int _nftl_port_hw_erase_block(void *device, struct physical_op_info *p);
int _nftl_port_hw_read_page(void *device, struct physical_op_info *p);
int _nftl_port_hw_write_page(void *device, struct physical_op_info *p);
int _nftl_port_hw_is_block_good(void *device, struct physical_op_info *p);
int _nftl_port_hw_mark_bad_block(void *device, struct physical_op_info *p);
#endif /*_NFTL_PORT_H*/
