/*
 * Copyright (C) 2022-2023 ArtinChip Technology Co., Ltd.
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef AIC_UI_H
#define AIC_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define USE_DATA_PATH     0
#define USE_SDCARD_PATH   1
#define USE_UDISK_PATH    2

#define LVGL_DATA_PATH USE_DATA_PATH

#ifdef RT_USING_DFS_ROMFS
#if LVGL_DATA_PATH == USE_DATA_PATH
#define STORAGE_PATH "/data/"
#elif  LVGL_DATA_PATH == USE_SDCARD_PATH
#define STORAGE_PATH "/sdcard/"
#elif  LVGL_DATA_PATH == USE_UDISK_PATH
#define STORAGE_PATH "/udisk/"
#else
#define STORAGE_PATH "/"
#endif // LVGL_DATA_PATH
#else
#define STORAGE_PATH "/rodata/"
#endif // RT_USING_DFS_ROMFS

#ifdef LPKG_USING_RAMDISK
#define LVGL_DIR "L:/ram/lvgl_data/"
#else
#ifdef AIC_CHIP_D13X
#define LVGL_DIR "L:/rodata/lvgl_data/"
#else
#define LVGL_DIR "L:"STORAGE_PATH"lvgl_data/"
#endif // AIC_CHIP_D13X
#endif // LPKG_USING_RAMDISK
#define FILE_LIST_PATH STORAGE_PATH"lvgl_video/"

#define CONN(x, y) x#y
#define LVGL_PATH(y) CONN(LVGL_DIR, y)
#define LVGL_FILE_LIST_PATH(y) CONN(FILE_LIST_PATH, y)

/* use fake image to fill color */
#define FAKE_IMAGE_DECLARE(name) char fake_##name[128];
#define FAKE_IMAGE_INIT(name, w, h, blend, color) \
                snprintf(fake_##name, 127, "L:/%dx%d_%d_%08x.fake",\
                 w, h, blend, color);
#define FAKE_IMAGE_NAME(name) (fake_##name)

/* avoid using sscanf function now */
#if 0
static inline void FAKE_IMAGE_PARSE(char *fake_name, int *width,
                                    int *height, int *blend,
                                    unsigned int *color)
{
        sscanf(fake_name + 3, "%dx%d_%d_%08x", width, height, blend, color);
}

#else
#include <stdlib.h>
static inline void FAKE_IMAGE_PARSE(char *fake_name, int *width,
                                    int *height, int *blend,
                                    unsigned int *color)
{
    char *cur_ptr;
    char *pos_ptr;
    cur_ptr = fake_name + 3;
    *width = strtol(cur_ptr, &pos_ptr, 10);
    cur_ptr = pos_ptr + 1;
    *height = strtol(cur_ptr, &pos_ptr, 10);
    cur_ptr = pos_ptr + 1;
    *blend = strtol(cur_ptr, &pos_ptr, 10);
    cur_ptr = pos_ptr + 1;
    *color = strtol(cur_ptr, &pos_ptr, 16);
}
#endif

#define ui_snprintf(fmt, arg...) snprintf(fmt, 127, ##arg)

void aic_ui_init();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //AIC_UI_H
