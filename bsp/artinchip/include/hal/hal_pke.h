/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef _AIC_HAL_PKE_H_
#define _AIC_HAL_PKE_H_

#include <aic_core.h>
#include <aic_common.h>
#include <hal_pke_eccp_curve.h>

extern const eccp_curve_t sm2_curve[1];
extern const eccp_curve_t sm9_curve[1];
extern const edward_curve_t ed25519[1];

/***************** PKE register *******************/
#define PKE_BASE       (CE_BASE + 0x2000)
#define PKE_CTRL       (0x000)
#define PKE_CFG        (0x004)
#define PKE_MC_PTR     (0x008)
#define PKE_RISR       (0x00C)
#define PKE_IMCR       (0x010)
#define PKE_MISR       (0x014)
#define PKE_RT_CODE    (0x024)
#define PKE_RAND_SEED  (0x040)
#define PKE_EXE_CONF   (0x050)
#define PKE_RC_EN      (0x060)
#define PKE_RC_KEY     (0x064)
#define PKE_RC_D_NONCE (0x068)
#define PKE_RC_A_NONCE (0x06C)
#define PKE_VERSION    (0x0FC)
#define PKE_A(a, step) (PKE_BASE + 0x0400 + (a) * (step))
#define PKE_B(a, step) (PKE_BASE + 0x1000 + (a) * (step))

/*********** PKE register action offset ************/
#define PKE_START_CALC (1)

#define PKE_EXE_OUTPUT_AFFINE       (0x10)
#define PKE_EXE_R1_MONT_R0_AFFINE   (0x09)
#define PKE_EXE_R1_MONT_R0_MONT     (0x0A)
#define PKE_EXE_R1_AFFINE_R0_AFFINE (0x05)
#define PKE_EXE_R1_AFFINE_R0_MONT   (0x06)
#define PKE_EXE_ECCP_POINT_MUL \
    (PKE_EXE_OUTPUT_AFFINE + PKE_EXE_R1_AFFINE_R0_MONT)
#define PKE_EXE_ECCP_POINT_ADD \
    (PKE_EXE_OUTPUT_AFFINE + PKE_EXE_R1_AFFINE_R0_AFFINE)
#define PKE_EXE_ECCP_POINT_DBL \
    (PKE_EXE_OUTPUT_AFFINE + PKE_EXE_R1_MONT_R0_AFFINE)
#define PKE_EXE_ECCP_POINT_VER \
    (PKE_EXE_OUTPUT_AFFINE + PKE_EXE_R1_AFFINE_R0_MONT)

#define PKE_EXE_CFG_ALL_MONT           (0x002A)
#define PKE_EXE_CFG_ALL_NON_MONT       (0x0000)
#define PKE_EXE_CFG_MODEXP_WITH_PUB    (0x0016)
#define PKE_EXE_CFG_MODEXP_WITHOUT_PUB (0x0116)
#define PKE_EXE_CFG_MODEXP_MONT_LADDER (0x0216)
#define PKE_EXE_CFG_MODEXP             (0x0316)

/***************** PKE microcode ******************/
#define MICROCODE_PDBL         (0x04)
#define MICROCODE_PADD         (0x08)
#define MICROCODE_PVER         (0x0C)
#define MICROCODE_PMUL         (0x10)
#define MICROCODE_MODEXP       (0x14)
#define MICROCODE_MODMUL       (0x18)
#define MICROCODE_MODINV       (0x1C)
#define MICROCODE_MODADD       (0x20)
#define MICROCODE_MODSUB       (0x24)
#define MICROCODE_MGMR_PRE     (0x28)
#define MICROCODE_INTMUL       (0x2C)
#define MICROCODE_Ed25519_PMUL (0x30)
#define MICROCODE_Ed25519_PADD (0x34)
#define MICROCODE_C25519_PMUL  (0x38)

/*********** some PKE algorithm operand length ************/
#define RSA_MAX_BIT_LEN                       AIC_PKE_OPERAND_MAX_BIT_LEN
#define DH_MAX_BIT_LEN                        AIC_PKE_OPERAND_MAX_BIT_LEN

#ifdef AIC_PKE_SEC

#ifdef AIC_PKE_RSA_SUPPORT
#define RSA_SEC
#endif

#ifdef SUPPORT_DH
#define DH_SEC
#endif

#ifdef AIC_PKE_ECDH_SUPPORT
#define ECDH_SEC
#endif

#ifdef AIC_PKE_ECDSA_SUPPORT
#define ECDSA_SEC
#endif

#ifdef AIC_PKE_SM2_SUPPORT
#define SM2_SEC
#endif

#endif

#define OPERAND_MAX_WORD_LEN (GET_WORD_LEN(AIC_PKE_OPERAND_MAX_BIT_LEN))

#define ECCP_MAX_BYTE_LEN (GET_BYTE_LEN(AIC_PKE_ECCP_MAX_BIT_LEN))
#define ECCP_MAX_WORD_LEN (GET_WORD_LEN(AIC_PKE_ECCP_MAX_BIT_LEN))

#define C25519_BYTE_LEN (256 / 8)
#define C25519_WORD_LEN (256 / 32)

#define Ed25519_BYTE_LEN C25519_BYTE_LEN
#define Ed25519_WORD_LEN C25519_WORD_LEN

#define RSA_MAX_WORD_LEN (GET_WORD_LEN(RSA_MAX_BIT_LEN))
#define RSA_MAX_BYTE_LEN (GET_BYTE_LEN(RSA_MAX_BIT_LEN))
#define RSA_MIN_BIT_LEN  (512)

#define DH_MAX_WORD_LEN (GET_WORD_LEN(DH_MAX_BIT_LEN))
#define DH_MAX_BYTE_LEN (GET_BYTE_LEN(DH_MAX_BIT_LEN))
#define DH_MIN_BIT_LEN  (512)

#define SM2_BIT_LEN  (256)
#define SM2_BYTE_LEN (32)
#define SM2_STEPS    (SM2_BYTE_LEN + 4)
#define SM2_WORD_LEN (8)

#define SM9_BASE_BIT_LEN  (256)
#define SM9_BASE_BYTE_LEN (SM9_BASE_BIT_LEN / 8)
#define SM9_STEPS         (SM9_BASE_BYTE_LEN + 4)
#define SM9_BASE_WORD_LEN (SM9_BASE_BIT_LEN / 32)

/******************* PKE return code ********************/
#define PKE_SUCCESS         (0)
#define PKE_STOP            (1)
#define PKE_NO_MODINV       (2)
#define PKE_NOT_ON_CURVE    (3)
#define PKE_INVALID_MC      (4)
#define PKE_ZERO_ALL        (5) //for ECCP input check
#define PKE_INTEGER_TOO_BIG (6) //for ECCP input check
#define PKE_INVALID_INPUT   (7)
#define PKE_FINISHED        (8)
#define PKE_ERROR           (9)

//APIs
s32 pke_init(void);
u32 pke_get_version(void);
u32 pke_set_operand_width(u32 bitLen);
u32 pke_get_operand_bytes(void);
void pke_set_exe_cfg(u32 cfg);
u32 pke_modinv(const u32 *modulus, const u32 *a, u32 *ainv, u32 modWordLen,
               u32 aWordLen);
u32 pke_modadd(const u32 *modulus, const u32 *a, const u32 *b, u32 *out,
               u32 wordLen);
u32 pke_modsub(const u32 *modulus, const u32 *a, const u32 *b, u32 *out,
               u32 wordLen);
u32 pke_add(const u32 *a, const u32 *b, u32 *out, u32 wordLen);
u32 pke_sub(const u32 *a, const u32 *b, u32 *out, u32 wordLen);
u32 pke_mul_internal(const u32 *a, const u32 *b, u32 *out, u32 a_wordLen,
                     u32 b_wordLen, u32 out_wordLen);
u32 pke_mul(const u32 *a, const u32 *b, u32 *out, u32 ab_wordLen);
u32 pke_pre_calc_mont(const u32 *modulus, u32 bitLen, u32 *H, u32 *n0);
u32 pke_pre_calc_mont_no_output(const u32 *modulus, u32 wordLen);
u32 pke_load_modulus_and_pre_monts(u32 *modulus, u32 *modulus_h,
                                   u32 *modulus_n0, u32 bitLen);
u32 pke_set_modulus_and_pre_monts(u32 *modulus, u32 *modulus_h, u32 *modulus_n0,
                                  u32 bitLen);
u32 pke_modmul_internal(const u32 *a, const u32 *b, u32 *out, u32 wordLen);
u32 pke_modmul(const u32 *modulus, const u32 *a, const u32 *b, u32 *out,
               u32 wordLen);
u32 pke_modexp(const u32 *modulus, const u32 *exponent, const u32 *base,
               u32 *out, u32 mod_wordLen, u32 exp_wordLen);
u32 pke_modexp_check_input(const u32 *modulus, const u32 *exponent,
                           const u32 *base, u32 *out, u32 mod_wordLen,
                           u32 exp_wordLen);
u32 pke_modexp_U8(const u8 *modulus, const u8 *exponent, const u8 *base,
                  u8 *out, u32 mod_bitLen, u32 exp_bitLen, u32 calc_pre_monts);
u32 pke_mod(u32 *a, u32 aWordLen, u32 *b, u32 *b_h, u32 *b_n0, u32 bWordLen,
            u32 *c);
u32 eccp_pointMul(eccp_curve_t *curve, u32 *k, u32 *Px, u32 *Py, u32 *Qx,
                  u32 *Qy);
u32 eccp_pointAdd(eccp_curve_t *curve, u32 *P1x, u32 *P1y, u32 *P2x, u32 *P2y,
                  u32 *Qx, u32 *Qy);
//#define ECCP_POINT_DOUBLE   //not recommended to define

#ifdef ECCP_POINT_DOUBLE
u32 eccp_pointDouble(eccp_curve_t *curve, u32 *Px, u32 *Py, u32 *Qx, u32 *Qy);
#endif

u32 eccp_pointVerify(eccp_curve_t *curve, u32 *Px, u32 *Py);
u32 eccp_get_pubkey_from_prikey(eccp_curve_t *curve, u8 *priKey, u8 *pubKey);
u32 eccp_getkey(eccp_curve_t *curve, u8 *priKey, u8 *pubKey);

#ifdef SUPPORT_C25519
u32 x25519_pointMul(mont_curve_t *curve, u32 *k, u32 *Pu, u32 *Qu);
u32 ed25519_pointMul(edward_curve_t *curve, u32 *k, u32 *Px, u32 *Py, u32 *Qx,
                     u32 *Qy);
u32 ed25519_pointAdd(edward_curve_t *curve, u32 *P1x, u32 *P1y, u32 *P2x,
                     u32 *P2y, u32 *Qx, u32 *Qy);
#endif

#ifdef AIC_PKE_SEC
u32 pke_sec_init(void);
u32 pke_sec_uninit(void);
u32 pke_modexp_ladder(const u32 *modulus, const u32 *exponent, const u32 *base,
                      u32 *out, u32 mod_wordLen, u32 exp_wordLen);
u32 pke_modexp_with_pub(const u32 *modulus, const u32 *exponent, const u32 *pub,
                        const u32 *base, u32 *out, u32 mod_wordLen,
                        u32 exp_wordLen, u32 pub_wordLen);
u32 pke_modexp_without_pub(const u32 *modulus, const u32 *exponent,
                           const u32 *base, u32 *out, u32 mod_wordLen,
                           u32 exp_wordLen);
u32 eccp_pointMul_sec(eccp_curve_t *curve, u32 *k, u32 *Px, u32 *Py, u32 *Qx,
                      u32 *Qy);
#endif

#endif
