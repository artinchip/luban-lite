/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef _AIC_HAL_HASH_H_
#define _AIC_HAL_HASH_H_

#include <aic_core.h>
#include <aic_common.h>

#define HASH_BASE       (CE_BASE + 0x4000)
#define HASH_CTRL       (0x000)              /* Control register */
#define HASH_CFG        (0x004)              /* Config register */
#define HASH_SR1        (0x008)              /* Status register 1 */
#define HASH_SR2        (0x00C)              /* Status register 2 */
#define HASH_PCR_LEN(i) (0x020 + ((i) << 2)) /* message length register */
#define HASH_OUT(i)     (0x030 + ((i) << 2)) /* Output register */
#define HASH_IN(i)      (0x070 + ((i) << 2)) /* Hash iterator Input register */
#define HASH_VERSION    (0x0B0)              /* Version register */
#define HASH_M_DIN(i)   (0x100 + ((i) << 2)) /* Hash message Input register */
#define HASH_DMA_SA     (0x180)              /* DMA Source Address register */
#define HASH_DMA_DA     (0x184)              /* DMA Destination Address register */
#define HASH_DMA_RLEN   (0x188)              /* DMA Input Length register */
#define HASH_DMA_WLEN   (0x18C)              /* DMA Output Length register */

#ifdef HMAC_SECURE_PORT_FUNCTION
// If key is from secure port, the max key index(or the number of keys)
#define HMAC_MAX_KEY_IDX     (8)
#define HMAC_MAX_SP_KEY_SIZE (64) // For secure port key, max bytes of one key
#endif

// Some register offset
#define HASH_LAST_OFFSET         (24)
#define HASH_DMA_OFFSET          (17)
#define HASH_INTERRUPTION_OFFSET (16)

// HASH max length
#if (defined(SUPPORT_HASH_SHA384) || defined(SUPPORT_HASH_SHA512) || \
     defined(SUPPORT_HASH_SHA512_224) || defined(SUPPORT_HASH_SHA512_256))
#define HASH_DIGEST_MAX_WORD_LEN    (16)
#define HASH_BLOCK_MAX_WORD_LEN     (32)
#define HASH_TOTAL_LEN_MAX_WORD_LEN (4)
#else
#define HASH_DIGEST_MAX_WORD_LEN    (8)
#define HASH_BLOCK_MAX_WORD_LEN     (16)
#define HASH_TOTAL_LEN_MAX_WORD_LEN (2)
#endif

#define HASH_ITERATOR_MAX_WORD_LEN HASH_DIGEST_MAX_WORD_LEN
#define HASH_BLOCK_MAX_BYTE_LEN    (HASH_BLOCK_MAX_WORD_LEN << 2)

// HASH return code
#define HASH_SUCCESS         (0)
#define HASH_BUFFER_NULL     (1)
#define HASH_CONFIG_INVALID  (2)
#define HASH_INPUT_INVALID   (3)
#define HASH_LEN_OVERFLOW    (4)
#define HASH_OUTPUT_ZERO_ALL (5)
#define HASH_ERROR           (6)

// HASH callback function type
typedef void (*HASH_CALLBACK)(void);

// HASH algorithm definition
typedef enum hash_alg {
    HASH_SM3 = 0,
    HASH_MD5 = 1,
    HASH_SHA256 = 2,
    HASH_SHA384 = 3,
    HASH_SHA512 = 4,
    HASH_SHA1 = 5,
    HASH_SHA224 = 6,
    HASH_SHA512_224 = 7,
    HASH_SHA512_256 = 8,
    HASH_SHA3_224 = 9,
    HASH_SHA3_256 = 10,
    HASH_SHA3_384 = 11,
    HASH_SHA3_512 = 12,
} HASH_ALG;

// APIs
u32 hash_get_version(void);
void hash_set_cpu_mode(void);
void hash_set_dma_mode(void);
void hash_set_alg(enum hash_alg alg);
void hash_enable_interruption(void);
void hash_disable_interruption(void);
void hash_set_last_block(u32 tag);
void hash_get_iterator(u8 *iterator, u32 hash_iterator_words);
void hash_set_iterator(u32 *iterator, u32 hash_iterator_words);
void hash_clear_msg_len(void);
void hash_set_msg_total_byte_len(u32 *msg_total_bytes, u32 words);
void hash_set_dma_output_len(u32 bytes);
void hash_start(void);
void hash_wait_till_done(void);
void hash_dma_wait_till_done(HASH_CALLBACK callback);
void hash_input_msg_u8(u8 *msg, u32 msg_bytes);
#ifdef AIC_HASH_DMA
#ifdef AIC_HASH_ADDRESS_HIGH_LOW
void hash_dma_operate(u32 in_h, u32 in_l, u32 out_h, u32 out_l, u32 inByteLen, 
        HASH_CALLBACK callback);
#else
void hash_dma_operate(u32 *in, u32 *out, u32 inByteLen, HASH_CALLBACK callback);
#endif
#endif

#endif
