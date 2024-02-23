#ifndef ECDSA_H
#define ECDSA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_pke.h"

//ECDSA return code
#define ECDSA_SUCCESS                         PKE_SUCCESS
#define ECDSA_POINTOR_NULL                    (PKE_SUCCESS+0x50U)
#define ECDSA_INVALID_INPUT                   (PKE_SUCCESS+0x51U)
#define ECDSA_ZERO_ALL                        (PKE_SUCCESS+0x52U)
#define ECDSA_INTEGER_TOO_BIG                 (PKE_SUCCESS+0x53U)
#define ECDSA_VERIFY_FAILED                   (PKE_SUCCESS+0x54U)

//APIs
u32 ecdsa_sign(eccp_curve_t *curve, u8 *E, u32 EByteLen, u8 *rand_k, u8 *priKey,
        u8 *signature);
u32 ecdsa_verify(eccp_curve_t *curve, u8 *E, u32 EByteLen, u8 *pubKey, u8 *signature);

#ifdef ECDSA_SEC

//ECDSA return code(secure version)
#define ECDSA_SUCCESS_S                       (0x7D5FEB14U)
#define ECDSA_ERROR_S                         (0xB4C0BC5AU)

u32 ecdsa_sign_s(eccp_curve_t *curve, u8 *E, u32 EByteLen, u8 *rand_k, u8 *priKey,
        u8 *signature);
u32 ecdsa_verify_s(eccp_curve_t *curve, u8 *E, u32 EByteLen, u8 *pubKey, u8 *signature);

#endif

#ifdef __cplusplus
}
#endif

#endif
