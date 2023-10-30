/* 
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef __AIC_DRV_HRTIMER_H__
#define __AIC_DRV_HRTIMER_H__

#include "aic_common.h"

struct hrtimer_info {
    char name[12];
    u32 id;
    rt_hwtimer_t hrtimer;
};

#endif
