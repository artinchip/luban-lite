/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BL_HEXDUMP_H_
#define __BL_HEXDUMP_H_

#ifdef __cplusplus
extern "C" {
#endif

void hexdump(unsigned char *buf, unsigned long len, int groupsize);
void show_speed(char *msg, u32 len, u32 us);

#ifdef __cplusplus
}
#endif

#endif /* __BL_HEXDUMP_H_ */
