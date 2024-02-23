#ifndef TRNG_H
#define TRNG_H

#include <hal_trng.h>

#ifdef __cplusplus
extern "C" {
#endif

u32 get_rand_internal(u8 *a, u32 bytes);
u32 get_rand_fast(u8 *rand, u32 bytes);
#ifndef AIC_TRNG_GENERATE_BY_HARDWARE
u32 get_rand_register(void);
#endif
u32 get_rand(u8 *rand, u32 bytes);

#ifdef __cplusplus
}
#endif

#endif
