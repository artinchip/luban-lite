#ifndef HASH_KDF_H
#define HASH_KDF_H

#include <hmac.h>

#ifdef __cplusplus
extern "C" {
#endif

//APIs
#ifdef AIC_HASH_PBKDF2
void pbkdf2_hmac_backup(HMAC_CTX *ctx_bak, HMAC_CTX *ctx);
void pbkdf2_hmac_recover(HMAC_CTX *ctx, HMAC_CTX *ctx_bak);
u32 pbkdf2_hmac(HASH_ALG hash_alg, u8 *pwd, u16 sp_key_idx, u32 pwd_bytes, 
        u8 *salt, u32 salt_bytes, u32 iter, u8 *out, u32 out_bytes);
#endif

#ifdef AIC_HASH_ANSI_X9_63_KDF
#ifdef AIC_HASH_NODE
u32 ansi_x9_63_kdf_node_with_xor_in(HASH_ALG hash_alg, HASH_NODE *hash_node, u32 node_num, 
        u32 counter_idx, u8 *in, u8 *out, u32 out_bytes, u32 check_whether_zero);
u32 ansi_x9_63_kdf_internal(HASH_ALG hash_alg, HASH_NODE *hash_node, u32 node_num, 
        u32 counter_idx, u8 *key, u32 key_bytes);
u32 ansi_x9_63_kdf_node(HASH_ALG hash_alg, HASH_NODE *hash_node, u32 node_num, u32 counter_idx, 
        u8 *k1, u32 k1_bytes, u8 *k2, u32 k2_bytes);
u32 ansi_x9_63_kdf(HASH_ALG hash_alg, u8 *Z, u32 Z_bytes, u8 *shared_info, 
        u32 shared_info_bytes, u8 *k1, u32 k1_bytes, u8 *k2, u32 k2_bytes);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif

