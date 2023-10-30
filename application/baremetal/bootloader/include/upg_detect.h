/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BL_UPG_DETECT_H_
#define __BL_UPG_DETECT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define UPG_DETECT_REASON_SOFT 1
#define UPG_DETECT_REASON_PIN  2

s32 upg_mode_detect(void);
s32 upg_reg_flag_check(void);
void upg_reg_flag_clear(void);
s32 upg_boot_pin_check(void);

#ifdef __cplusplus
}
#endif

#endif /* __BL_UPG_DETECT_H_ */
