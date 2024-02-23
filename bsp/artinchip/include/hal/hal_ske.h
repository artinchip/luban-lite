/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef _AIC_HAL_SKE_H_
#define _AIC_HAL_SKE_H_

#include <aic_core.h>
#include <aic_common.h>

#define SKE_BASE      (CE_BASE + 0x1000)
#define SKE_CTRL      (0x000)            /* SKE Control Register */
#define SKE_CFG       (0x004)            /* SKE Config Register */
#define SKE_SR1       (0x008)            /* SKE Status Register 1 */
#define SKE_SR2       (0x00C)            /* SKE Status Register 2 */
#define SKE_AES_CFG   (0x060)            /* SKE AES key length config Register */
#define SKE_IV(i)     (0x070 + (i << 2)) /* Initial Vector */
#define SKE_KEY(i)    (0x080 + (i << 2)) /* Key*/
#define SKE_RNG_SD(i) (0x100 + (i << 2)) /* pseudo-random seed */
#define SKE_RNG_CTRL  (0x130)            /* pseudo-random Control Register */
#define SKE_VERSION   (0x140)            /* SKE version Register */
#define SKE_IN(i)     (0x200 + (i << 2)) /* SKE Input Register */
#define SKE_OUT(i)    (0x210 + (i << 2)) /* SKE Output Register */

// Some register offset
#define SKE_REVERSE_BYTE_ORDER_IN_WORD_OFFSET (12)
#define SKE_RESET_OFFSET                      (16)
#define SKE_MODE_OFFSET                       (9)
#define SKE_CRYPTO_OFFSET                     (8)
#define SKE_UPDATE_KEY_OFFSET                 (16)
#define SKE_SECURE_PORT_OFFSET                (17)
#define SKE_UPDATE_IV_OFFSET                  (18)
#define SKE_ERR_CFG_OFFSET                    (8)

#define SKE_SET_SEED_BYTE_LEN (36)

// SKE return code
#define SKE_SUCCESS        (0)
#define SKE_BUFFER_NULL    (1)
#define SKE_CONFIG_INVALID (2)
#define SKE_INPUT_INVALID  (3)
#define SKE_RUNTIME_ALARM  (4)
#define SKE_PADDING_ERROR  (5)
#define SKE_ERROR          (6)

// SKE Operation Mode
typedef enum ske_mode{
    SKE_MODE_ECB = 0,     // ECB Mode
    SKE_MODE_CBC = 1,     // CBC Mode
    SKE_MODE_CFB = 2,     // CFB Mode
    SKE_MODE_OFB = 3,     // OFB Mode
    SKE_MODE_CTR = 4,     // CTR Mode
    SKE_MODE_XTS = 5,     // XTS Mode
    SKE_MODE_CMAC = 6,    // CMAC Mode
    SKE_MODE_CBC_MAC = 7, // CBC-MAC Mode
    SKE_MODE_GCM = 8,     // GCM Mode
    SKE_MODE_CCM = 9,     // CCM Mode
    SKE_MODE_GMAC = 10,   // GMAC Mode
} SKE_MODE;

// SKE Crypto Action
typedef enum {
    SKE_CRYPTO_ENCRYPT = 0, // encrypt
    SKE_CRYPTO_DECRYPT,     // decrypt
} SKE_CRYPTO;

// SKE MAC Action
typedef enum {
    SKE_GENERATE_MAC = SKE_CRYPTO_ENCRYPT,
    SKE_VERIFY_MAC = SKE_CRYPTO_DECRYPT,
} SKE_MAC;

// SKE Algorithm
typedef enum ske_alg{
    SKE_ALG_DES            = 0,      // DES
    SKE_ALG_TDES_128       = 1,      // TDES 128 bits key
    SKE_ALG_TDES_192       = 2,      // TDES 192 bits key
    SKE_ALG_TDES_EEE_128   = 3,      // TDES_EEE 128 bits key
    SKE_ALG_TDES_EEE_192   = 4,      // TDES_EEE 192 bits key
    SKE_ALG_AES_128        = 5,      // AES 128 bits key
    SKE_ALG_AES_192        = 6,      // AES 192 bits key
    SKE_ALG_AES_256        = 7,      // AES 256 bits key
    SKE_ALG_SM4            = 8,      // SM4
} SKE_ALG;

// SKE padding scheme
typedef enum {
    SKE_NO_PADDING = 0,
    SKE_ANSI_X923_PADDING,
    SKE_PKCS_5_7_PADDING,
    SKE_ISO_7816_4_PADDING,
    SKE_ZERO_PADDING, //hardware does not support, just for CBC-MAC
} SKE_PADDING;

// SKE block length
typedef struct {
#if (defined(CONFIG_SKE_SUPPORT_MUL_THREAD))
    SKE_ALG alg;
    u8 *key;
    u16 sp_key_idx;
    u32 key_buf[32 / 4]; //hw not support xts directly
#endif
    u32 iv[4];
    SKE_MODE mode;
    SKE_CRYPTO crypto;
    u8 block_bytes;
    u8 block_words;
    SKE_PADDING padding;
} SKE_CTX;

//APIs
s32 ske_init(void);
u32 ske_get_version(void);
void ske_reset(void);
void ske_set_endian_uint32(void);
void ske_set_crypto(SKE_CRYPTO crypto);
void ske_set_alg(SKE_ALG ske_alg);
void ske_set_mode(SKE_MODE mode);
u32 ske_set_seed(void);
u32 ske_check_runtime_alarm(void);
u32 ske_check_config(void);
void ske_start(void);
u32 ske_wait_till_done(void);
void ske_set_key_uint32(u32 *key, u32 idx, u32 key_words);
void ske_set_iv_uint32(u32 mode, u32 *iv, u32 block_words);
void ske_simple_set_input_block(u32 *in, u32 block_words);
void ske_simple_get_output_block(u32 *out, u32 block_words);

#endif
