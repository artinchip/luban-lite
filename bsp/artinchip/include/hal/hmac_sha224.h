#ifndef HMAC_SHA224_H
#define HMAC_SHA224_H

#include <hmac.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AIC_HASH_SHA224_SUPPORT
typedef HMAC_CTX HMAC_SHA224_CTX;

#ifdef AIC_HASH_DMA
typedef HMAC_DMA_CTX HMAC_SHA224_DMA_CTX;
#endif

//APIs
u32 hmac_sha224_init(HMAC_SHA224_CTX *ctx, const u8 *key, u16 sp_key_idx, u32 key_bytes);
u32 hmac_sha224_update(HMAC_SHA224_CTX *ctx, const u8 *msg, u32 msg_bytes);
u32 hmac_sha224_final(HMAC_SHA224_CTX *ctx, u8 *mac);
u32 hmac_sha224(u8 *key, u16 sp_key_idx, u32 key_bytes, u8 *msg, 
        u32 msg_bytes, u8 *mac);

#ifdef AIC_HASH_NODE
u32 hmac_sha224_node_steps(u8 *key, u16 sp_key_idx, u32 key_bytes, 
        HASH_NODE *node, u32 node_num, u8 *mac);
#endif

#ifdef AIC_HASH_DMA
u32 hmac_sha224_dma_init(HMAC_SHA224_DMA_CTX *ctx, const u8 *key, u16 sp_key_idx, 
        u32 key_bytes, HASH_CALLBACK callback);

#ifdef AIC_HASH_ADDRESS_HIGH_LOW
u32 hmac_sha224_dma_update_blocks(HMAC_SHA224_DMA_CTX *ctx, u32 msg_h, u32 msg_l, 
        u32 msg_bytes);
u32 hmac_sha224_dma_final(HMAC_SHA224_DMA_CTX *ctx, u32 remainder_msg_h, u32 remainder_msg_l, 
        u32 remainder_bytes, u32 mac_h, u32 mac_l);
u32 hmac_sha224_dma(u8 *key, u16 sp_key_idx, u32 key_bytes, u32 msg_h, 
        u32 msg_l, u32 msg_bytes, u32 mac_h, u32 mac_l, HASH_CALLBACK callback);

#ifdef AIC_HASH_DMA_NODE
u32 hmac_sha224_dma_node_steps(u8 *key, u16 sp_key_idx, u32 key_bytes, 
        HASH_DMA_NODE *node, u32 node_num, u32 mac_h, u32 mac_l, HASH_CALLBACK callback);
#endif
#else
u32 hmac_sha224_dma_update_blocks(HMAC_SHA224_DMA_CTX *ctx, u32 *msg, u32 msg_bytes);
u32 hmac_sha224_dma_final(HMAC_SHA224_DMA_CTX *ctx, u32 *remainder_msg, u32 remainder_bytes, 
        u32 *mac);
u32 hmac_sha224_dma(u8 *key, u16 sp_key_idx, u32 key_bytes, u32 *msg, 
        u32 msg_bytes, u32 *mac, HASH_CALLBACK callback);

#ifdef AIC_HASH_DMA_NODE
u32 hmac_sha224_dma_node_steps(u8 *key, u16 sp_key_idx, u32 key_bytes, 
        HASH_DMA_NODE *node, u32 node_num, u32 *mac, HASH_CALLBACK callback);
#endif
#endif
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif

