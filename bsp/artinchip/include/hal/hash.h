#ifndef HASH_H
#define HASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_hash.h>

//HASH status
typedef struct {
    u32 busy             : 1;        // calculate busy flag
} hash_status_t;

//HASH context
typedef struct
{
#ifdef AIC_HASH_MUL_THREAD
    u32 iterator[HASH_ITERATOR_MAX_WORD_LEN];    //keep current hash iterator value for multiple thread
#endif

    u8 hash_buffer[HASH_BLOCK_MAX_BYTE_LEN];     //block buffer
    u32 total[HASH_TOTAL_LEN_MAX_WORD_LEN];      //total byte length of the whole message
    hash_status_t status;                             //hash update status, .busy=1 means doing, .busy=0 means idle
    HASH_ALG hash_alg;                                //current hash algorithm
    u8 block_byte_len;
    u8 iterator_word_len;
    u8 digest_byte_len;
    u8 first_update_flag;                        //whether first time to update message(1:yes, 0:no)
    u8 finish_flag;                              //whether the whole message has been inputted(1:yes, 0:no)
} HASH_CTX;

#ifdef AIC_HASH_DMA
//HASH DMA context
typedef struct
{
#ifdef AIC_HASH_MUL_THREAD
    u32 iterator[HASH_ITERATOR_MAX_WORD_LEN];    //keep current hash iterator value for multiple thread
#endif

    u32 total[HASH_TOTAL_LEN_MAX_WORD_LEN];      //total byte length of the whole message
    HASH_CALLBACK callback;
    HASH_ALG hash_alg;                                //current hash algorithm
    u8 block_word_len;

#ifdef AIC_HASH_MUL_THREAD
    u8 iterator_word_len;
    u8 first_update_flag;                        //whether first time to update message(1:yes, 0:no)
#endif
    u8 digest_byte_len;                          //just for hmac
} HASH_DMA_CTX;
#endif


#ifdef AIC_HASH_NODE
typedef struct {
    u8 *msg_addr;
    u32 msg_bytes;
} HASH_NODE;
#endif


#ifdef AIC_HASH_DMA_NODE
typedef struct {
#ifdef AIC_HASH_ADDRESS_HIGH_LOW
    u32 msg_addr_h;
    u32 msg_addr_l;
#else
    u32 *msg_addr;
#endif
    u32 msg_bytes;
} HASH_DMA_NODE;
#endif

//APIs
u32 check_hash_alg(HASH_ALG hash_alg);
u8 hash_get_block_word_len(HASH_ALG hash_alg);
u8 hash_get_iterator_word_len(HASH_ALG hash_alg);
u8 hash_get_digest_word_len(HASH_ALG hash_alg);
u32 *hash_get_IV(HASH_ALG hash_alg);
void hash_set_IV(HASH_ALG hash_alg, u32 hash_iterator_words);
u32 hash_total_byte_len_add_uint32(u32 *a, u32 a_words, u32 b);
#ifdef AIC_HASH_ADDRESS_HIGH_LOW
u32 hash_addr64_add_uint32(u32 *addr_h, u32 *addr_l, u32 offset);
#endif

//void hash_total_bytelen_2_bitlen(u32 *a, u32 a_words);

void hash_start_calculate(HASH_CTX *ctx);
void hash_calc_blocks(HASH_CTX *ctx, const u8 *msg, u32 block_count);
void hash_calc_rand_len_msg(HASH_CTX *ctx, const u8 *msg, u32 msg_bytes);

u32 hash_init_with_iv_and_updated_length(HASH_CTX *ctx, HASH_ALG hash_alg, u32 *iv, 
        u32 byte_length_h, u32 byte_length_l);
u32 hash_init(HASH_CTX *ctx, HASH_ALG hash_alg);
u32 hash_update(HASH_CTX *ctx, const u8 *msg, u32 msg_bytes);
u32 hash_final(HASH_CTX *ctx, u8 *digest);
u32 hash(HASH_ALG hash_alg, u8 *msg, u32 msg_bytes, u8 *digest);

#ifdef AIC_HASH_NODE
u32 hash_node_steps(HASH_ALG hash_alg, HASH_NODE *node, u32 node_num, u8 *digest);
#endif

#ifdef AIC_HASH_DMA
u32 hash_dma_init_with_iv_and_updated_length(HASH_DMA_CTX *ctx, HASH_ALG hash_alg, u32 *iv, 
        u32 byte_length_h, u32 byte_length_l, HASH_CALLBACK callback);
u32 hash_dma_init(HASH_DMA_CTX *ctx, HASH_ALG hash_alg, HASH_CALLBACK callback);

#ifdef AIC_HASH_ADDRESS_HIGH_LOW
u32 hash_dma_update_blocks(HASH_DMA_CTX *ctx, u32 msg_h, u32 msg_l, u32 msg_bytes);
u32 hash_dma_final(HASH_DMA_CTX *ctx, u32 remainder_msg_h, u32 remainder_msg_l, 
        u32 remainder_bytes, u32 digest_h, u32 digest_l);
u32 hash_dma(HASH_ALG hash_alg, u32 msg_h, u32 msg_l, u32 msg_bytes, u32 digest_h, 
        u32 digest_l, HASH_CALLBACK callback);

#ifdef AIC_HASH_DMA_NODE
u32 hash_dma_node_steps(HASH_ALG hash_alg, HASH_DMA_NODE *node, u32 node_num, u32 digest_h, 
        u32 digest_l, HASH_CALLBACK callback);
#endif
#else
u32 hash_dma_update_blocks(HASH_DMA_CTX *ctx, u32 *msg, u32 msg_bytes);
u32 hash_dma_final(HASH_DMA_CTX *ctx, u32 *remainder_msg, u32 remainder_bytes, u32 *digest);
u32 hash_dma(HASH_ALG hash_alg, u32 *msg, u32 msg_bytes, u32 *digest, HASH_CALLBACK callback);

#ifdef AIC_HASH_DMA_NODE
u32 hash_dma_node_steps(HASH_ALG hash_alg, HASH_DMA_NODE *node, u32 node_num, u32 *digest, 
        HASH_CALLBACK callback);
#endif
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif

