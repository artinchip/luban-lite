/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: weilin.peng@artinchip.com
 */

#include <rtthread.h>

#ifdef AIC_AB_SYSTEM_INTERFACE
#include <absystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <dfs.h>
#include <dfs_fs.h>
#endif

int main(void)
{
#ifdef AIC_AB_SYSTEM_INTERFACE
    char target[32] = { 0 };

    aic_ota_status_update();
    aic_get_rodata_to_mount(target);
    printf("Mount APP in blk %s\n", target);

    if (dfs_mount(target, "/rodata", "elm", 0, 0) < 0)
        printf("Failed to mount elm\n");
#endif
    return 0;
}
