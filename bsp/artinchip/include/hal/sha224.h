#ifndef SHA224_H
#define SHA224_H

#include <hash.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AIC_HASH_SHA224_SUPPORT
typedef HASH_CTX SHA224_CTX;

#ifdef AIC_HASH_DMA
typedef HASH_DMA_CTX SHA224_DMA_CTX;
#endif

//APIs
u32 sha224_init(SHA224_CTX *ctx);
u32 sha224_update(SHA224_CTX *ctx, const u8 *msg, u32 msg_bytes);
u32 sha224_final(SHA224_CTX *ctx, u8 *digest);
u32 sha224(u8 *msg, u32 msg_bytes, u8 *digest);

#ifdef AIC_HASH_NODE
u32 sha224_node_steps(HASH_NODE *node, u32 node_num, u8 *digest);
#endif

#ifdef AIC_HASH_DMA
u32 sha224_dma_init(SHA224_DMA_CTX *ctx, HASH_CALLBACK callback);

#ifdef AIC_HASH_ADDRESS_HIGH_LOW
u32 sha224_dma_update_blocks(SHA224_DMA_CTX *ctx, u32 msg_h, u32 msg_l, u32 msg_bytes);
u32 sha224_dma_final(SHA224_DMA_CTX *ctx, u32 remainder_msg_h, u32 remainder_msg_l, 
        u32 remainder_bytes, u32 digest_h, u32 digest_l);
u32 sha224_dma(u32 msg_h, u32 msg_l, u32 msg_bytes, u32 digest_h, u32 digest_l, 
        HASH_CALLBACK callback);

#ifdef AIC_HASH_DMA_NODE
u32 sha224_dma_node_steps(HASH_DMA_NODE *node, u32 node_num, u32 digest_h, 
        u32 digest_l, HASH_CALLBACK callback);
#endif
#else
u32 sha224_dma_update_blocks(SHA224_DMA_CTX *ctx, u32 *msg, u32 msg_bytes);
u32 sha224_dma_final(SHA224_DMA_CTX *ctx, u32 *remainder_msg, u32 remainder_bytes, u32 *digest);
u32 sha224_dma(u32 *msg, u32 msg_bytes, u32 *digest, HASH_CALLBACK callback);

#ifdef AIC_HASH_DMA_NODE
u32 sha224_dma_node_steps(HASH_DMA_NODE *node, u32 node_num, u32 *digest, 
        HASH_CALLBACK callback);
#endif
#endif
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
