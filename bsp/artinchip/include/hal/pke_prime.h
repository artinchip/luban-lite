#ifndef PKE_PRIME_H
#define PKE_PRIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//1:use hardware;  2:use software
#define BIGINT_DIV_CHOICE     (2)

#if (BIGINT_DIV_CHOICE == 1)
typedef struct {
    u32 low;
    u32 high;
}double_u32;
#elif (BIGINT_DIV_CHOICE == 2)
typedef u32 double_u32;
//#define BIGINT_DIV_UINT32
#endif

//1:use Fermat primality test;  2:use Miller–Rabin primality test
#define PRIMALITY_TEST_CHOICE (1)

#if (PRIMALITY_TEST_CHOICE == 1)
#define FERMAT_ROUND          (3)
#elif (PRIMALITY_TEST_CHOICE == 2)
#define MILLER_RABIN_ROUND    (3)
#endif

//prime table level(total number of small prime numbers)
#define PTL_MAX               (400)   //the max PTL value
#define PTL_512               (400)   //the best PTL value for prime bit length 512 (RSA1024)
#define PTL_1024              (400)   //the best PTL value for prime bit length 1024 (RSA2048)

#define NOT_PRIME             (0xFFFFFFFF)
#define MAYBE_PRIME           (0)

u32 get_prime(u32 p[], u32 pBitLen);

#ifdef __cplusplus
}
#endif

#endif

