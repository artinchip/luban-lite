/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#define LOG_TAG "SID"
#include <string.h>
#include <hal_efuse.h>
#include <drv_efuse.h>
#include <aic_core.h>


int drv_efuse_init(void)
{
    int ret;

    ret = hal_efuse_init();
    if (ret) {
        LOG_E("Failed to initialize efuse.\n");
        return RT_FALSE;
    }

    return RT_TRUE;
}

int drv_efuse_read(u32 addr, void *data, u32 size)
{
    u32 wid, wval, rest, cnt;
    u8 *pd, *pw;
    int ret;

    if (hal_efuse_wait_ready()) {
        LOG_E("eFuse is not ready.\n");
        return RT_FALSE;
    }

    pd = data;
    rest = size;
    while (rest > 0) {
        wid = addr >> 2;
        ret = hal_efuse_read(wid, &wval);
        if (ret)
            break;
        pw = (u8 *)&wval;
        cnt = rest;
        if (addr % 4) {
            if (rest > (4 - (addr % 4)))
                cnt = (4 - (addr % 4));
            memcpy(pd, pw + (addr % 4), cnt);
        } else {
            if (rest > 4)
                cnt = 4;
            memcpy(pd, pw, cnt);
        }
        pd += cnt;
        addr += cnt;
        rest -= cnt;
    }

    return (int)(size - rest);
}

int drv_efuse_program(u32 addr, const void *data, u32 size)
{
    u32 wid, wval, rest, cnt;
    const u8 *pd;
    u8 *pw;
    int ret;

    if (hal_efuse_wait_ready()) {
        LOG_E("eFuse is not ready.\n");
        return RT_FALSE;
    }

    pd = data;
    rest = size;
    while (rest > 0) {
        cnt = rest;
        wval = 0;
        pw = (u8 *)&wval;
        if (addr % 4) {
            if (rest > (4 - (addr % 4)))
                cnt = (4 - (addr % 4));
            memcpy(pw + (addr % 4), pd, cnt);
        } else {
            if (rest > 4)
                cnt = 4;
            memcpy(pw, pd, cnt);
        }

        wid = addr >> 2;
        ret = hal_efuse_write(wid, wval);
        if (ret)
            break;
        pd += cnt;
        addr += cnt;
        rest -= cnt;
    }

    return (int)(size - rest);
}
