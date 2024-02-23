#ifndef PKE_COMMON_H
#define PKE_COMMON_H

#include <aic_common.h>

//ECC point conversion form
#define POINT_COMPRESSED          (0x02)   //pc||x, pc = 0x02|LSB(y)
#define POINT_UNCOMPRESSED        (0x04)   //pc||x||y, pc=0x04
typedef u8 EC_POINT_FORM;

//define KDF
typedef void *(*KDF_FUNC)(const void *input, u32 byteLen, u8 *key, u32 keyByteLen);

//APIs
void pke_load_operand(u32 *baseaddr, u32 *data, u32 wordLen);
void pke_read_operand(u32 *baseaddr, u32 *data, u32 wordLen);
void pke_load_operand_U8(u32 *baseaddr, u8 *data, u32 byteLen);
void pke_read_operand_U8(u32 *baseaddr, u8 *data, u32 byteLen);
void pke_set_operand_uint32_value(u32 *baseaddr, u32 wordLen, u32 b);

#endif
