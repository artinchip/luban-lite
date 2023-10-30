/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_CRC_
#define _AIC_CRC_

#ifdef __cplusplus
extern "C" {
#endif

uint32_t crc32(uint32_t crc, const uint8_t *p, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
