/*
 * Copyright (c) 2006-2022, ArtInChip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ARCH_CC_H__
#define __ARCH_CC_H__

#include <sys/types.h>
#include <stdio.h>
#include <sys/_timeval.h>

#define LWIP_TIMEVAL_PRIVATE  0

#ifndef BYTE_ORDER
#ifdef ARCH_CPU_BIG_ENDIAN
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#endif /* ARCH_CPU_BIG_ENDIAN */
#endif /* BYTE_ORDER */

typedef unsigned int sys_prot_t;

#if LPKG_USING_LWIP_VER_NUM < 0x20000
#include <stdint.h>
typedef uint8_t   u8_t;
typedef int8_t    s8_t;
typedef uint16_t  u16_t;
typedef int16_t   s16_t;
typedef uint32_t  u32_t;
typedef int32_t   s32_t;
typedef uintptr_t mem_ptr_t;

#endif /* RT_USING_LWIP_VER_NUM < 0x20000 */

#if defined(__CC_ARM)   /* ARMCC compiler */
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050) /*Arm Compiler 6*/
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
#elif defined(__IAR_SYSTEMS_ICC__)   /* IAR Compiler */
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_USE_INCLUDES
#elif defined(__GNUC__)     /* GNU GCC Compiler */
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
#elif defined(_MSC_VER)
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
#define PACK_STRUCT_USE_INCLUDES
#endif

void sys_arch_assert(const char* file, int line);
#define LWIP_PLATFORM_DIAG(x)   do {printf x;} while(0)
#define LWIP_PLATFORM_ASSERT(x) do {printf(x); sys_arch_assert(__FILE__, __LINE__);}while(0)

#endif /* __ARCH_CC_H__ */
