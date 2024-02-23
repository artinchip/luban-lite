#ifndef SKE_H
#define SKE_H

#include <hal_ske.h>

#ifdef __cplusplus
extern "C" {
#endif

//APIs
void ske_big_endian_add_uint8(u8 *a, u32 a_bytes, u8 b);
//void ske_little_endian_add_uint32(u32 *a, u32 a_words, u32 b);
u8 ske_sec_get_key_byte_len(SKE_ALG ske_alg);
u8 ske_sec_get_block_byte_len(SKE_ALG ske_alg);
void ske_set_key(SKE_ALG alg, u8 *key, u16 key_bytes, u16 key_idx);
u32 keep_alg_key_iv(SKE_CTX *ctx, SKE_ALG alg, SKE_MODE mode, u8 *key, u16 sp_key_idx, u8 *iv);
u32 ske_sec_init_internal(SKE_CTX *ctx, SKE_ALG alg, SKE_MODE mode, SKE_CRYPTO crypto, u8 *key, 
        u16 sp_key_idx, u8 *iv);
u32 ske_sec_init(SKE_CTX *ctx, SKE_ALG alg, SKE_MODE mode, SKE_CRYPTO crypto, u8 *key, 
        u16 sp_key_idx, u8 *iv, SKE_PADDING padding);
u32 ske_sec_update_blocks(SKE_CTX *ctx, u8 *in, u8 *out, u32 bytes);
u32 ske_sec_update_including_last_block(SKE_CTX *ctx, u8 *in, u8 *out, u32 in_bytes, 
        u32 *out_bytes);
u32 ske_sec_final(SKE_CTX *ctx);
u32 ske_sec_crypto(SKE_ALG alg, SKE_MODE mode, SKE_CRYPTO crypto, u8 *key, u16 sp_key_idx, u8 *iv,
        SKE_PADDING padding, u8 *in, u8 *out, u32 in_bytes, u32 *out_bytes);

#ifdef __cplusplus
}
#endif

#endif

