#ifndef SM3_H
#define SM3_H

#include <hash.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AIC_HASH_SM3_SUPPORT
typedef HASH_CTX SM3_CTX;

#ifdef AIC_HASH_DMA
typedef HASH_DMA_CTX SM3_DMA_CTX;
#endif

//APIs
u32 sm3_init(SM3_CTX *ctx);
u32 sm3_update(SM3_CTX *ctx, const u8 *msg, u32 msg_bytes);
u32 sm3_final(SM3_CTX *ctx, u8 *digest);
u32 sm3(u8 *msg, u32 msg_bytes, u8 *digest);

#ifdef AIC_HASH_NODE
u32 sm3_node_steps(HASH_NODE *node, u32 node_num, u8 *digest);
#endif

#ifdef AIC_HASH_DMA
u32 sm3_dma_init(SM3_DMA_CTX *ctx, HASH_CALLBACK callback);

#ifdef AIC_HASH_ADDRESS_HIGH_LOW
u32 sm3_dma_update_blocks(SM3_DMA_CTX *ctx,  u32 msg_h, u32 msg_l, u32 msg_bytes);
u32 sm3_dma_final(SM3_DMA_CTX *ctx, u32 remainder_msg_h, u32 remainder_msg_l, 
        u32 remainder_bytes, u32 digest_h, u32 digest_l);
u32 sm3_dma(u32 msg_h, u32 msg_l, u32 msg_bytes, u32 digest_h, u32 digest_l, 
        HASH_CALLBACK callback);

#ifdef AIC_HASH_DMA_NODE
u32 sm3_dma_node_steps(HASH_DMA_NODE *node, u32 node_num, u32 digest_h, u32 digest_l, 
        HASH_CALLBACK callback);
#endif
#else
u32 sm3_dma_update_blocks(SM3_DMA_CTX *ctx, u32 *msg, u32 msg_bytes);
u32 sm3_dma_final(SM3_DMA_CTX *ctx, u32 *remainder_msg, u32 remainder_bytes, u32 *digest);
u32 sm3_dma(u32 *msg, u32 msg_bytes, u32 *digest, HASH_CALLBACK callback);

#ifdef AIC_HASH_DMA_NODE
u32 sm3_dma_node_steps(HASH_DMA_NODE *node, u32 node_num, u32 *digest, 
        HASH_CALLBACK callback);
#endif
#endif
#endif

#endif

#ifdef __cplusplus
}
#endif


#endif

