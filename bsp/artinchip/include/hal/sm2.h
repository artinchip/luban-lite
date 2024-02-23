#ifndef SM2_H
#define SM2_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_pke.h>
#include <hash.h>

#if (defined(PKE_HP) || defined(PKE_UHP))
#define SM2_HIGH_SPEED        //only available for PKE_HP, PKE_UHP
#endif

//some sm2 length
#define SM3_DIGEST_BYTE_LEN                   SM2_BYTE_LEN
#define SM2_MAX_ID_BYTE_LEN                   (1<<13)

//SM2 return code
#define SM2_SUCCESS                           PKE_SUCCESS
#define SM2_BUFFER_NULL                       (PKE_SUCCESS+0x40U)
#define SM2_NOT_ON_CURVE                      (PKE_SUCCESS+0x41U)
#define SM2_EXCHANGE_ROLE_INVALID             (PKE_SUCCESS+0x42U)
#define SM2_INPUT_INVALID                     (PKE_SUCCESS+0x43U)
#define SM2_ZERO_ALL                          (PKE_SUCCESS+0x44U)
#define SM2_INTEGER_TOO_BIG                   (PKE_SUCCESS+0x45U)
#define SM2_VERIFY_FAILED                     (PKE_SUCCESS+0x46U)
#define SM2_DECRYPT_VERIFY_FAILED             (PKE_SUCCESS+0x47U)

//SM2 key exchange role
typedef enum {
    SM2_Role_Sponsor = 0,
    SM2_Role_Responsor
} sm2_exchange_role_e;

// SM2 ciphertext order
typedef enum {
    SM2_C1C3C2   = 0,
    SM2_C1C2C3,
} sm2_cipher_order_e;

//APIs
u32 sm2_getZ(u8 *ID, u32 byteLenofID, u8 pubKey[65], u8 Z[32]);
u32 sm2_getE(u8 *M, u32 byteLen, u8 Z[32], u8 E[32]);
//#define SM2_GETE_BY_STEPS
#ifdef SM2_GETE_BY_STEPS
u32 sm2_getE_init(HASH_CTX *ctx, u8 Z[32]);
u32 sm2_getE_update(HASH_CTX *ctx, u8 *msg, u32 msg_bytes);
u32 sm2_getE_final(HASH_CTX *ctx, u8 E[32]);
#endif

u32 sm2_get_pubkey_from_prikey(u8 priKey[32], u8 pubKey[65]);
u32 sm2_getkey(u8 priKey[32], u8 pubKey[65]);
u32 sm2_sign(u8 E[32], u8 rand_k[32], u8 priKey[32], u8 signature[64]);
u32 sm2_verify(u8 E[32], u8 pubKey[65], u8 signature[64]);
u32 sm2_encrypt(u8 *M, u32 MByteLen, u8 rand_k[32], u8 pubKey[65],
        sm2_cipher_order_e order, u8 *C, u32 *CByteLen);
u32 sm2_decrypt(u8 *C, u32 CByteLen, u8 priKey[32],
        sm2_cipher_order_e order, u8 *M, u32 *MByteLen);
u32 sm2_exchangekey(sm2_exchange_role_e role,
                        u8 *dA, u8 *PB,
                        u8 *rA, u8 *RA,
                        u8 *RB,
                        u8 *ZA, u8 *ZB,
                        u32 kByteLen,
                        u8 *KA, u8 *S1, u8 *SA);

#ifdef SM2_SEC

//SM2 return code(secure version)
#define SM2_SUCCESS_S                         (0x3E2FDB1AU)
#define SM2_ERROR_S                           (0xCBAD735EU)

u32 sm2_sign_s(u8 E[32], u8 rand_k[32], u8 priKey[32], u8 signature[64]);
u32 sm2_verify_s(u8 E[32], u8 pubKey[65], u8 signature[64]);
u32 sm2_encrypt_s(u8 *M, u32 MByteLen, u8 rand_k[32], u8 pubKey[65],
        sm2_cipher_order_e order, u8 *C, u32 *CByteLen);
u32 sm2_decrypt_s(u8 *C, u32 CByteLen, u8 priKey[32],
        sm2_cipher_order_e order, u8 *M, u32 *MByteLen);
u32 sm2_exchangekey_s(sm2_exchange_role_e role,
                        u8 *dA, u8 *PB,
                        u8 *rA, u8 *RA,
                        u8 *RB,
                        u8 *ZA, u8 *ZB,
                        u32 kByteLen,
                        u8 *KA, u8 *S1, u8 *SA);

#endif

#ifdef __cplusplus
}
#endif

#endif
