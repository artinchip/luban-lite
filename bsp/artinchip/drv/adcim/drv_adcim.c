/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aic_core.h"
#include "hal_adcim.h"

int drv_adcim_init(void)
{
    if (hal_adcim_probe())
        return -RT_ERROR;
    else
        return RT_EOK;
}
INIT_BOARD_EXPORT(drv_adcim_init);
