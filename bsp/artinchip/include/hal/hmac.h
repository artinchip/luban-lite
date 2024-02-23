#ifndef HMAC_H
#define HMAC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hash.h>

#define HMAC_IPAD                 (0x36363636U)
#define HMAC_OPAD                 (0x5c5c5c5cU)
#define HMAC_IPAD_XOR_OPAD        (HMAC_IPAD ^ HMAC_OPAD)

//HMAC context
typedef struct
{
    u32 K0[HASH_BLOCK_MAX_WORD_LEN];
    HASH_CTX hash_ctx[1];
} HMAC_CTX;


//HMAC DMA context
#ifdef AIC_HASH_DMA
typedef struct
{
    u32 K0[HASH_BLOCK_MAX_WORD_LEN];
    HASH_DMA_CTX hash_dma_ctx[1];
} HMAC_DMA_CTX;
#endif

//APIs
u32 hmac_init(HMAC_CTX *ctx, HASH_ALG hash_alg, const u8 *key, u16 sp_key_idx, u32 key_bytes);
u32 hmac_update(HMAC_CTX *ctx, const u8 *msg, u32 msg_bytes);
u32 hmac_final(HMAC_CTX *ctx, u8 *mac);
u32 hmac(HASH_ALG hash_alg, u8 *key, u16 sp_key_idx, u32 key_bytes, u8 *msg, 
        u32 msg_bytes, u8 *mac);
#ifdef AIC_HASH_NODE
u32 hmac_node_steps(HASH_ALG hash_alg, u8 *key, u16 sp_key_idx, u32 key_bytes,
        HASH_NODE *node, u32 node_num, u8 *mac);
#endif

#ifdef AIC_HASH_DMA
u32 hmac_dma_init(HMAC_DMA_CTX *ctx, HASH_ALG hash_alg, const u8 *key, u16 sp_key_idx, 
        u32 key_bytes, HASH_CALLBACK callback);

#ifdef AIC_HASH_ADDRESS_HIGH_LOW
u32 hmac_dma_update_blocks(HMAC_DMA_CTX *ctx, u32 msg_h, u32 msg_l, u32 msg_bytes);
u32 hmac_dma_final(HMAC_DMA_CTX *ctx, u32 remainder_msg_h, u32 remainder_msg_l, 
        u32 remainder_bytes, u32 mac_h, u32 mac_l);
u32 hmac_dma(HASH_ALG hash_alg, u8 *key, u16 sp_key_idx, u32 key_bytes, u32 msg_h, 
        u32 msg_l, u32 msg_bytes, u32 mac_h, u32 mac_l, HASH_CALLBACK callback);

#ifdef AIC_HASH_DMA_NODE
u32 hmac_dma_node_steps(HASH_ALG hash_alg, u8 *key, u16 sp_key_idx, u32 key_bytes, 
        HASH_DMA_NODE *node, u32 node_num, u32 mac_h, u32 mac_l, HASH_CALLBACK callback);
#endif
#else
u32 hmac_dma_update_blocks(HMAC_DMA_CTX *ctx, u32 *msg, u32 msg_bytes);
u32 hmac_dma_final(HMAC_DMA_CTX *ctx, u32 *remainder_msg, u32 remainder_bytes, u32 *mac);
u32 hmac_dma(HASH_ALG hash_alg, u8 *key, u16 sp_key_idx, u32 key_bytes, u32 *msg, 
        u32 msg_bytes, u32 *mac, HASH_CALLBACK callback);

#ifdef AIC_HASH_DMA_NODE
u32 hmac_dma_node_steps(HASH_ALG hash_alg, u8 *key, u16 sp_key_idx, u32 key_bytes, 
        HASH_DMA_NODE *node, u32 node_num, u32 *mac, HASH_CALLBACK callback);
#endif
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif

