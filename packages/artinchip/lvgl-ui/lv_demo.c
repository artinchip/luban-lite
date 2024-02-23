/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2021-10-17     Meco Man      First version
 */
#include <lvgl.h>
#ifdef KERNEL_RTTHREAD
#include <rtthread.h>
#endif
#include "aic_core.h"
#include "aic_osal.h"
#include "aic_ui.h"
#include "aic_dec.h"
#include <dfs_fs.h>
#include "aic_time.h"

#ifndef LV_CACHE_IMG_NUM
#define LV_CACHE_IMG_NUM 1
#endif

void lv_user_gui_init(void)
{
    // wait sdcard mounted
    if (!strcmp(LVGL_STORAGE_PATH, "/sdcard")) {
        aicos_msleep(1000);
    }

    lv_img_cache_set_size(LV_CACHE_IMG_NUM);
    aic_dec_create();
    aic_ui_init();
}

#ifdef KERNEL_RTTHREAD
extern int lvgl_thread_init(void);

INIT_APP_EXPORT(lvgl_thread_init);
#endif
