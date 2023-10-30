/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef _AIC_HAL_CE_H_
#define _AIC_HAL_CE_H_

#include <aic_common.h>

#define RSA2048_SIGN_LEN 256
#define RSA2048_KEY_LEN  256
#define SHA256_BYTE_LEN  32
#define MD5_BYTE_LEN     16
#define AES128_KEY_LEN   16
#define AES192_KEY_LEN   24
#define AES256_KEY_LEN   32
#define DES64_KEY_LEN    8

#define MD5_DIGEST_SIZE     16
#define SHA1_DIGEST_SIZE    20
#define SHA224_DIGEST_SIZE  28
#define SHA256_DIGEST_SIZE  32
#define SHA384_DIGEST_SIZE  48
#define SHA512_DIGEST_SIZE  64

#define MD5_BLOCK_SIZE      16
#define SHA1_BLOCK_SIZE     64
#define SHA224_BLOCK_SIZE	64
#define SHA256_BLOCK_SIZE   64
#define SHA384_BLOCK_SIZE   128
#define SHA512_BLOCK_SIZE   128

#define MD5_CE_OUT_LEN      16
#define SHA1_CE_OUT_LEN     20
#define SHA224_CE_OUT_LEN   32
#define SHA256_CE_OUT_LEN   32
#define SHA384_CE_OUT_LEN   64
#define SHA512_CE_OUT_LEN   64

#define MD5_H0 0x67452301UL
#define MD5_H1 0xefcdab89UL
#define MD5_H2 0x98badcfeUL
#define MD5_H3 0x10325476UL

#define BE_SHA1_H0   0x01234567UL
#define BE_SHA1_H1   0x89abcdefUL
#define BE_SHA1_H2   0xfedcba98UL
#define BE_SHA1_H3   0x76543210UL
#define BE_SHA1_H4   0xf0e1d2c3UL
#define BE_SHA224_H0 0xd89e05c1UL
#define BE_SHA224_H1 0x07d57c36UL
#define BE_SHA224_H2 0x17dd7030UL
#define BE_SHA224_H3 0x39590ef7UL
#define BE_SHA224_H4 0x310bc0ffUL
#define BE_SHA224_H5 0x11155868UL
#define BE_SHA224_H6 0xa78ff964UL
#define BE_SHA224_H7 0xa44ffabeUL
#define BE_SHA256_H0 0x67e6096aUL
#define BE_SHA256_H1 0x85ae67bbUL
#define BE_SHA256_H2 0x72f36e3cUL
#define BE_SHA256_H3 0x3af54fa5UL
#define BE_SHA256_H4 0x7f520e51UL
#define BE_SHA256_H5 0x8c68059bUL
#define BE_SHA256_H6 0xabd9831fUL
#define BE_SHA256_H7 0x19cde05bUL
#define BE_SHA384_H0 0xd89e05c15d9dbbcbULL
#define BE_SHA384_H1 0x07d57c362a299a62ULL
#define BE_SHA384_H2 0x17dd70305a015991ULL
#define BE_SHA384_H3 0x39590ef7d8ec2f15ULL
#define BE_SHA384_H4 0x310bc0ff67263367ULL
#define BE_SHA384_H5 0x11155868874ab48eULL
#define BE_SHA384_H6 0xa78ff9640d2e0cdbULL
#define BE_SHA384_H7 0xa44ffabe1d48b547ULL
#define BE_SHA512_H0 0x08c9bcf367e6096aULL
#define BE_SHA512_H1 0x3ba7ca8485ae67bbULL
#define BE_SHA512_H2 0x2bf894fe72f36e3cULL
#define BE_SHA512_H3 0xf1361d5f3af54fa5ULL
#define BE_SHA512_H4 0xd182e6ad7f520e51ULL
#define BE_SHA512_H5 0x1f6c3e2b8c68059bULL
#define BE_SHA512_H6 0x6bbd41fbabd9831fULL
#define BE_SHA512_H7 0x79217e1319cde05bULL

#define ALG_UNIT_SYMM         (0)
#define ALG_UNIT_HASH         (1)
#define ALG_UNIT_ASYM         (2)

#define ALG_DIR_ENCRYPT        (0)
#define ALG_DIR_DECRYPT        (1)

#define ALG_AES_ECB           (0x00)
#define ALG_AES_CBC           (0x01)
#define ALG_AES_CTR           (0x02)
#define ALG_AES_XTS           (0x03)
#define ALG_AES_CTS           (0x04)
#define ALG_DES_ECB           (0x10)
#define ALG_DES_CBC           (0x11)
#define ALG_TDES_ECB          (0x20)
#define ALG_TDES_CBC          (0x21)
#define ALG_RSA               (0x30)
#define ALG_SHA1              (0x40)
#define ALG_SHA224            (0x41)
#define ALG_SHA256            (0x42)
#define ALG_SHA384            (0x43)
#define ALG_SHA512            (0x44)
#define ALG_MD5               (0x45)
#define ALG_HMAC_SHA1         (0x46)
#define ALG_HMAC_SHA256       (0x47)
#define ALG_TRNG              (0x50)

#define CTR_BIT_WIDTH_16       (0)
#define CTR_BIT_WIDTH_32       (1)
#define CTR_BIT_WIDTH_64       (2)
#define CTR_BIT_WIDTH_128      (3)

#define KEY_SIZE_64            (0x00)
#define KEY_SIZE_128           (0x01)
#define KEY_SIZE_192           (0x02)
#define KEY_SIZE_256           (0x03)
#define KEY_SIZE_512           (0x04)
#define KEY_SIZE_1024          (0x05)
#define KEY_SIZE_2048          (0x06)

/*
 * Key source
 *   - User input key(RAM or Secure SRAM)
 *   - eFuse key
 */
#define CE_KEY_SRC_USER        (0)
#define CE_KEY_SRC_SSK         (1)
#define CE_KEY_SRC_HUK         (2)
#define CE_KEY_SRC_PNK         (3)
#define CE_KEY_SRC_PSK0       (4)
#define CE_KEY_SRC_PSK1       (5)
#define CE_KEY_SRC_PSK2       (6)
#define CE_KEY_SRC_PSK3       (7)

#define uaddr u64
#define PTR2U32(ptr) ((u32)(uaddr)(ptr))

struct aes_ecb_desc {
    u32 alg_tag   : 8; /* bit[7:0] */
    u32 direction : 1; /* bit[8] */
    u32 r0        : 7; /* bit[15:9] */
    u32 key_src   : 4; /* bit[19:16] */
    u32 key_siz   : 4; /* bit[23:20] */
    u32 r1        : 8; /* bit[31:24] */
    u32 key_addr;
    u8 r2[28]; /* Pad to 36 bytes */
};

struct aes_cbc_desc {
    u32 alg_tag   : 8; /* bit[7:0] */
    u32 direction : 1; /* bit[8] */
    u32 r0        : 7; /* bit[15:9] */
    u32 key_src   : 4; /* bit[19:16] */
    u32 key_siz   : 4; /* bit[23:20] */
    u32 r1        : 8; /* bit[31:24] */
    u32 key_addr;
    u32 iv_addr;
    u8 r2[24]; /* Pad to 36 bytes */
};

/*
 * CTS-CBC-CS3(Kerberos)
 */
struct aes_cts_desc {
    u32 alg_tag   : 8; /* bit[7:0] */
    u32 direction : 1; /* bit[8] */
    u32 r0        : 7; /* bit[15:9] */
    u32 key_src   : 4; /* bit[19:16] */
    u32 key_siz   : 4; /* bit[23:20] */
    u32 r1        : 8; /* bit[31:24] */
    u32 key_addr;
    u32 iv_addr;
    u8 r2[24]; /* Pad to 36 bytes */
};

struct aes_ctr_desc {
    u32 alg_tag   : 8; /* bit[7:0] */
    u32 direction : 1; /* bit[8] */
    u32 r0        : 5; /* bit[13:9] */
    u32 ctr_width : 2; /* bit[15:14] */
    u32 key_src   : 4; /* bit[19:16] */
    u32 key_siz   : 4; /* bit[23:20] */
    u32 r1        : 8; /* bit[31:24] */
    u32 key_addr;
    u32 ctr_in_addr;
    u32 ctr_out_addr;
    u8 r2[20]; /* Pad to 36 bytes */
};

struct aes_xts_desc {
    u32 alg_tag   : 8; /* bit[7:0] */
    u32 direction : 1; /* bit[8] */
    u32 r0        : 7; /* bit[15:9] */
    u32 key_src   : 4; /* bit[19:16] */
    u32 key_siz   : 4; /* bit[23:20] */
    u32 r1        : 8; /* bit[31:24] */
    u32 key_addr;
    u32 tweak_addr;
    u8 r2[24]; /* Pad to 36 bytes */
};

struct des_ecb_desc {
    u32 alg_tag   : 8; /* bit[7:0] */
    u32 direction : 1; /* bit[8] */
    u32 r0        : 7; /* bit[15:9] */
    u32 key_src   : 4; /* bit[19:16] */
    u32 key_siz   : 4; /* bit[23:20] */
    u32 r1        : 8; /* bit[31:24] */
    u32 key_addr;
    u8 r2[28]; /* Pad to 36 bytes */
};

struct des_cbc_desc {
    u32 alg_tag   : 8; /* bit[7:0] */
    u32 direction : 1; /* bit[8] */
    u32 r0        : 7; /* bit[15:9] */
    u32 key_src   : 4; /* bit[19:16] */
    u32 key_siz   : 4; /* bit[23:20] */
    u32 r1        : 8; /* bit[31:24] */
    u32 key_addr;
    u32 iv_addr;
    u8 r2[24]; /* Pad to 36 bytes */
};

struct rsa_alg_desc {
    u32 alg_tag : 8;  /* bit[7:0] */
    u32 r0      : 12; /* bit[19:8] */
    u32 op_siz  : 4;  /* bit[23:20] */
    u32 r1      : 8;  /* bit[31:24] */
    u32 m_addr;
    u32 d_e_addr;
    u8 r2[24]; /* Pad to 36 bytes */
};

struct hash_alg_desc {
    u32 alg_tag : 8;  /* bit[7:0] */
    u32 r0      : 1;  /* bit[8] */
    u32 iv_mode : 1;  /* bit[9] */
    u32 r1      : 22; /* bit[31:10] */
    u32 hmac_key_addr;
    u32 iv_addr;
    u8 r2[24]; /* Pad to 36 bytes */
};

struct trng_alg_desc {
    u32 alg_tag : 8;  /* bit[7:0] */
    u32 r1      : 24; /* bit[31:8] */
    u8 r2[32];        /* Pad to 36 bytes */
};

union alg_desc {
    u8 alg_tag;
    struct aes_ecb_desc aes_ecb;
    struct aes_cbc_desc aes_cbc;
    struct aes_ctr_desc aes_ctr;
    struct aes_cts_desc aes_cts;
    struct aes_xts_desc aes_xts;
    struct des_ecb_desc des_ecb;
    struct des_cbc_desc des_cbc;
    struct rsa_alg_desc rsa;
    struct hash_alg_desc hash;
    struct hash_alg_desc hmac;
    struct trng_alg_desc trng;
};

struct data_desc {
    u32 first_flag : 1;  /* bit[0] */
    u32 last_flag  : 1;  /* bit[1] */
    u32 r1         : 30; /* bit[31:2] */
    u32 total_bytelen;
    u32 in_addr;
    u32 in_len;
    u32 out_addr;
    u32 out_len;
};

/*
 * size of crypto task: 64 bytes
 */
struct crypto_task {
    union alg_desc alg;
    struct data_desc data;
    u32 next;
};

s32 hal_crypto_init(void);
s32 hal_crypto_deinit(void);
void hal_crypto_irq_handler();
s32 hal_crypto_start_symm(struct crypto_task *task);
s32 hal_crypto_start_asym(struct crypto_task *task);
s32 hal_crypto_start_hash(struct crypto_task *task);
u32 hal_crypto_poll_finish(u32 alg_unit);
void hal_crypto_pending_clear(u32 alg_unit);
u32 hal_crypto_get_err(u32 alg_unit);
s32 hal_crypto_bignum_byteswap(u8 *bn, u32 len);
s32 hal_crypto_bignum_le2be(u8 *src, u32 slen, u8 *dst, u32 dlen);
s32 hal_crypto_bignum_be2le(u8 *src, u32 slen, u8 *dst, u32 dlen);

#endif
