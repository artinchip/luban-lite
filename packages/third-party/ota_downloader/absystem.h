/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#ifndef __ABSYSTEM_H__
#define __ABSYSTEM_H__

#ifdef __cplusplus
extern "C" {
#endif

int aic_upgrade_start(void);
char *aic_upgrade_get_partname(unsigned char index);
int aic_upgrade_end(void);
int aic_ota_status_update(void);
int aic_get_rodata_to_mount(char *target_rodata);

#ifdef __cplusplus
}
#endif

#endif
