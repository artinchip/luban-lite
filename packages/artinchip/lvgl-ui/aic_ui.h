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
#include <stdlib.h>

#ifndef LVGL_STORAGE_PATH
#define LVGL_STORAGE_PATH "/rodata/lvgl_data"
#endif
#define LVGL_DIR "L:"LVGL_STORAGE_PATH"/"
#define FILE_LIST_PATH LVGL_STORAGE_PATH"/video/"

#define CONN(x, y) x#y
#define LVGL_PATH(y) CONN(LVGL_DIR, y)
#define LVGL_FILE_LIST_PATH(y) CONN(FILE_LIST_PATH, y)

/* use fake image to fill color */
#define FAKE_IMAGE_DECLARE(name) char fake_##name[128];
#define FAKE_IMAGE_INIT(name, w, h, blend, color) \
                snprintf(fake_##name, 127, "L:/%dx%d_%d_%08x.fake",\
                 w, h, blend, color);
#define FAKE_IMAGE_NAME(name) (fake_##name)

#define FAKE_IMAGE_PARSE(fake_name, pwidth, pheight, pblend, pcolor) \
        fake_image_parse(fake_name, pwidth, pheight, pblend, pcolor)

static inline void fake_image_parse(char *fake_name, int *width,
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
    return;
}

#define ui_snprintf(fmt, arg...) snprintf(fmt, 127, ##arg)

void aic_ui_init();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //AIC_UI_H
