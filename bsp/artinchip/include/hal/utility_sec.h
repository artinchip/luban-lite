#ifndef UTILITY_SEC_H
#define UTILITY_SEC_H

#include "utility.h"

#ifdef __cplusplus
extern "C" {
#endif

u16 crc16_calc(u8 *in, u32 bytelen, u16 crc);
u32 uint32_get_rand_big_number_msb_0(u32 *a, u32 aBitLen);
u8 uint8_BigNum_Check_Zero_sec(u8 a[], u32 aByteLen);
u32 uint32_BigNum_Check_Zero_sec(u32 a[], u32 aWordLen);
int uint32_BigNumCmp_sec(u32 *a, u32 aWordLen, u32 *b, u32 bWordLen);
u32 uint32_cmp_sec(u32 *a, u32 *b, u32 wordLen, u8 rand);
u32 uint32_integer_check_sec(u32 *k, u32 *n, u32 wordLen, u32 ret_zero, u32 ret_big,
        u32 ret_success);


#ifdef __cplusplus
}
#endif

#endif
