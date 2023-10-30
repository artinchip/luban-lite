/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef __CONFIG_PARSE_H__
#define __CONFIG_PARSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>

#define IMG_NAME_MAX_SIZ 128
#define PROTECTION_PARTITION_LEN 128

int boot_cfg_parse_file(char *cfgtxt, u32 clen, char *key, char *fname,
                        u32 nsiz, ulong *offset, ulong *flen);
int boot_cfg_get_boot1(char *cfgtxt, u32 clen, char *name, u32 nsiz,
                       ulong *offset, ulong *boot1len);
int boot_cfg_get_image(char *cfgtxt, u32 clen, char *name, u32 nsiz);
int boot_cfg_get_protection(char *cfgtxt, u32 clen, char *name, u32 nsiz);

#ifdef __cplusplus
}
#endif

#endif/* __CONFIG_PARSE_H__ */
