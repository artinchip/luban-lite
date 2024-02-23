#ifndef RSA_H
#define RSA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_pke.h>

#if defined(AIC_PKE_RSASSA_PSS_SUPPORT)
#include <hash.h>
#endif

//RSA return code
#define RSA_SUCCESS                           PKE_SUCCESS
#define RSA_BUFFER_NULL                       (PKE_SUCCESS+0x30U)
#define RSA_INPUT_TOO_LONG                    (PKE_SUCCESS+0x31U)
#define RSA_INPUT_INVALID                     (PKE_SUCCESS+0x32U)

//APIs
u32 RSA_ModExp(u32 *a, u32 *e, u32 *n, u32 *out, u32 eBitLen, u32 nBitLen);
u32 RSA_CRTModExp(u32 *a, u32 *p, u32 *q, u32 *dp, u32*dq,
        u32 *u, u32 *out,  u32 nBitLen);
u32 RSA_GetKey(u32 *e, u32 *d, u32 *n, u32 eBitLen, u32 nBitLen);
u32 RSA_GetCRTKey(u32 *e, u32 *p, u32 *q, u32 *dp, u32 *dq, u32 *u,
        u32 *n, u32 eBitLen, u32 nBitLen);

#ifdef RSA_SEC

//RSA return code(secure version)
#define RSA_SUCCESS_S                         (0x3AEBA318U)
#define RSA_ERROR_S                           (0x45DF3DAEU)

u32 RSA_ModExp_with_pub(u32 *a, u32 *e, u32 *d, u32 *n, u32 *out, u32 eBitLen, u32 nBitLen);
u32 RSA_ModExp_without_pub(u32 *a, u32 *d, u32 *n, u32 *out, u32 nBitLen);
u32 RSA_CRTModExp_with_pub(u32 *a, u32 *p, u32 *q, u32 *dp, u32*dq, u32 *u, u32 *e,
        u32 *out, u32 eBitLen, u32 nBitLen);
u32 RSA_CRTModExp_without_pub(u32 *a, u32 *p, u32 *q, u32 *dp, u32*dq, u32 *u, 
        u32 *out, u32 nBitLen);
#endif

typedef struct {
    u8 *p;
    u8 *q;
    u8 *dp;
    u8 *dq;
    u8 *u;//qinv
} RSA_CRT_PRIVATE_KEY;

#if (defined(AIC_PKE_RSASSA_PSS_SUPPORT) || defined(SUPPORT_RSAES_OAEP))
void rsa_pkcs1_mgf1_counter_add(u8 *counter, u32 bytes, u8 b);
u32 rsa_pkcs1_mgf1_with_xor_in(HASH_ALG hash_alg, u8 *seed, u32 seed_bytes, u8 *in,
        u8 *out, u32 mask_bytes);
#endif

#ifdef AIC_PKE_RSASSA_PSS_SUPPORT
u32 rsa_ssa_pss_sign_by_msg_digest(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, u8 *salt, u32 salt_bytes,
        u8 *msg_digest, u8 *d, u8 *n, u32 n_bits, u8 *signature);
u32 rsa_ssa_pss_sign(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, u8 *salt, u32 salt_bytes, u8 *msg,
        u32 msg_bytes, u8 *d, u8 *n, u32 n_bits, u8 *signature);
u32 rsa_ssa_pss_crt_sign_by_msg_digest(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, u8 *salt, u32 salt_bytes,
        u8 *msg_digest, RSA_CRT_PRIVATE_KEY *d, u32 n_bits, u8 *signature);
u32 rsa_ssa_pss_crt_sign(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, u8 *salt, u32 salt_bytes, u8 *msg,
        u32 msg_bytes, RSA_CRT_PRIVATE_KEY *d, u32 n_bits, u8 *signature);
u32 rsa_ssa_pss_verify_by_msg_digest(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, s32 salt_bytes,
        u8 *msg_digest, u8 *e, u32 e_bits, u8 *n, u32 n_bits, u8 *signature);
u32 rsa_ssa_pss_verify(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, s32 salt_bytes, u8 *msg,
        u32 msg_bytes, u8 *e, u32 e_bits, u8 *n, u32 n_bits, u8 *signature);
#endif

#ifdef __cplusplus
}
#endif

#endif

