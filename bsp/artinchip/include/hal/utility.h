#ifndef UTILITY_H
#define UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_common.h>

#define CAST2UINT32(a)           ((u32)(a))

#define GET_MAX_LEN(a,b)         (((a)>(b))?(a):(b))
#define GET_MIN_LEN(a,b)         (((a)>(b))?(b):(a))
#define GET_WORD_LEN(bitLen)     (((bitLen)+31)/32)
#define GET_BYTE_LEN(bitLen)     (((bitLen)+7)/8)


//APIs
#ifdef AIC_UTILITY_PRINT_BUF
extern void print_buf_U8(u8 *buf, u32 byteLen, char *name);
extern void print_buf_U32(u32 *buf, u32 wordLen, char *name);
extern void print_BN_buf_U32(u32 *buf, u32 wordLen, char *name);
#endif

void memcpy_(u8 *dst, u8 *src, u32 size);
void memset_(u8 *dst, u8 value, u32 size);
u8 memcmp_(u8 *m1, u8 *m2, u32 size);
void uint32_set(u32 *a, u32 value, u32 wordLen);
void uint32_copy(u32 *dst, u32 *src, u32 wordLen);
void uint32_clear(u32 *a, u32 wordLen);
void uint32_sleep(u32 count, u8 rand);
void uint32_endian_reverse(u8 *in, u8 *out, u32 wordLen);
void reverse_byte_array(u8 *in, u8 *out, u32 byteLen);
//void reverse_word(u8 *in, u8 *out, u32 bytelen);
//void dma_reverse_word_array(u32 *in, u32 *out, u32 wordlen, u32 reverse_word);
void uint8_XOR(u8 *A, u8 *B, u8 *C, u32 byteLen);
void uint32_XOR(u32 *A, u32 *B, u32 *C, u32 wordLen);
u32 get_bit_value_by_index(const u32 *a, u32 bit_index);
u32 get_valid_bits(const u32 *a, u32 wordLen);
u32 get_valid_words(u32 *a, u32 max_words);
u8 uint8_BigNum_Check_Zero(u8 *a, u32 aByteLen);
u32 uint32_BigNum_Check_Zero(u32 *a, u32 aWordLen);
u32 uint8_big_num_big_endian_add_little(u8 *a, u32 a_bytes, u8 b, u8 is_secure);
u32 uint32_big_num_little_endian_add_little(u32 *a, u32 a_words, u32 b, u8 is_secure);
int uint32_BigNumCmp(u32 *a, u32 aWordLen, u32 *b, u32 bWordLen);
u32 Get_Multiple2_Number(u32 *a);
u32 Big_Div2n(u32 *a, u32 aWordLen, u32 n);
u8 Bigint_Check_1(u32 *a, u32 aWordLen);
u8 Bigint_Check_p_1(u32 *a, u32 *p, u32 wordLen);
u32 uint32_integer_check(u32 *k, u32 *n, u32 wordLen, u32 ret_zero, u32 ret_big, u32 ret_success);

#ifdef __cplusplus
}
#endif

#endif
