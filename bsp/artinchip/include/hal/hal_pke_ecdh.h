#ifndef ECDH_H
#define ECDH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_pke.h"

//ECDH return code
#define ECDH_SUCCESS                          PKE_SUCCESS
#define ECDH_POINTOR_NULL                     (PKE_SUCCESS+0x60U)
#define ECDH_INVALID_INPUT                    (PKE_SUCCESS+0x61U)
#define ECDH_ZERO_ALL                         (PKE_SUCCESS+0x62U)
#define ECDH_INTEGER_TOO_BIG                  (PKE_SUCCESS+0x63U)

//APIs
u32 ecdh_compute_key(eccp_curve_t *curve, u8 *local_prikey, u8 *peer_pubkey, u8 *key,
        u32 keyByteLen, KDF_FUNC kdf);

#ifdef ECDH_SEC

//ECDH return code(secure version)
#define ECDH_SUCCESS_S                        (0x8B9BC1E1U)
#define ECDH_ERROR_S                          (0xCBC192A3U)

u32 ecdh_compute_key_s(eccp_curve_t *curve, u8 *local_prikey, u8 *peer_pubkey, u8 *key,
        u32 keyByteLen, KDF_FUNC kdf);

#endif

#ifdef __cplusplus
}
#endif

#endif
