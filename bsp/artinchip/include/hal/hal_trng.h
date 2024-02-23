/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef _AIC_HAL_TRNG_H_
#define _AIC_HAL_TRNG_H_

#include <aic_common.h>

// TRNG register address
#define TRNG_BASE    (CE_BASE + 0x5000)
#define TRNG_CR      (0x000)
#define TRNG_MSEL    (0x004)
#define TRNG_SR      (0x008)
#define TRNG_DR      (0x00C)
#define TRNG_RESEED  (0x010)
#define RO_CLK_EN    (0x014)
#define RO_SRC_EN1   (0x018)
#define RO_SRC_EN2   (0x01C)
#define TRNG_HT_CR   (0x020)
#define TRNG_HT_SR   (0x024)
#define TRNG_VERSION (0x030)

// TRNG freq config
#define TRNG_RO_FREQ_4  (0)
#define TRNG_RO_FREQ_8  (1)
#define TRNG_RO_FREQ_16 (2)
#define TRNG_RO_FREQ_32 (3) //default

// TRNG action offset
#define TRNG_GLOBAL_INT_OFFSET     (24)
#define TRNG_READ_EMPTY_INT_OFFSET (17)
#define TRNG_DATA_INT_OFFSET       (16)
#define TRNG_FREQ_OFFSET           (16)

// TRNG return code
#define TRNG_SUCCESS        (0)
#define TRNG_BUFFER_NULL    (1)
#define TRNG_INVALID_INPUT  (2)
#define TRNG_INVALID_CONFIG (3)
#define TRNG_HT_ERROR       (4)
#define TRNG_TIMEOUT_ERROR  (5)
#define TRNG_ERROR          (6)

typedef u32 (*GET_RAND_WORDS)(u32 *a, u32 words);

//API

u32 trng_get_version(void);
void trng_global_int_enable(void);
void trng_global_int_disable(void);
void trng_empty_read_int_enable(void);
void trng_empty_read_int_disable(void);
void trng_data_int_enable(void);
void trng_data_int_disable(void);
void trng_enable(void);
void trng_disable(void);

#ifdef AIC_TRNG_RO_ENTROPY
u32 trng_ro_entropy_config(u8 cfg);
u32 trng_ro_sub_entropy_config(u8 sn, u16 cfg);
void trng_set_mode(u8 with_post_processing);
void trng_reseed(void);
u32 trng_set_freq(u8 freq);
u32 get_rand_uint32(u32 *a, u32 words);
u32 get_rand_uint32_without_reseed(u32 *a, u32 words);
u32 get_rand_uint32_with_reseed(u32 *a, u32 words);
u32 get_rand_buffer(u8 *rand, u32 bytes, GET_RAND_WORDS get_rand_words);
#endif

#endif
