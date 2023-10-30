/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <rtconfig.h>
#include <stdio.h>
#include <aic_core.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <absystem.h>
#include <env.h>

#define APPLICATION_PART           "os"
#define APPLICATION_PART_REDUNDAND "os_r"

int aic_ota_version_fallback(void)
{
    int ret = 0;
    char *next = NULL;

    next = fw_getenv("osAB_next");
#ifdef AIC_ENV_DEBUG
    printf("osAB_next = %s\n", next);
#endif

    /*Version fallback*/
    if (strncmp(next, "A", 2) == 0) {
        ret = fw_env_write("osAB_next", "B");
    } else if (strncmp(next, "B", 2) == 0) {
        ret = fw_env_write("osAB_next", "A");
    } else {
        pr_err("Invalid osAB_next\n");
        return -1;
    }

    if (ret) {
        pr_err("Env write fail\n");
        return ret;
    }

    ret = fw_env_write("bootcount", "0");
    if (ret) {
        pr_err("Env write fail\n");
        return ret;
    }

    ret = fw_env_write("upgrade_available", "0");
    if (ret) {
        pr_err("Env write fail\n");
        return ret;
    }

    return 0;
}

int aic_ota_check(void)
{
    char *str = NULL;
    char *status = NULL;
    int count = 0;
    int limit = 0;
    char string[32] = { 0 };
    int ret = 0;

    if (fw_env_open()) {
        pr_err("Open env failed\n");
        return -1;
    }

    status = fw_getenv("upgrade_available");
#ifdef AIC_ENV_DEBUG
    printf("upgrade_available = %s\n", status);
#endif
    if (strncmp(status, "1", 2) == 0) {
        str = fw_getenv("bootlimit");
        limit = strtol(str, NULL, 10);

        str = fw_getenv("bootcount");
        count = strtol(str, NULL, 10);
#ifdef AIC_ENV_DEBUG
        printf("limit = %d, count =%d\n", limit, count);
#endif
        if (limit < count) {
            pr_warn(
                "The number of boot failed exceeds the limit, Version fallback now\n");

            ret = aic_ota_version_fallback();
            if (ret) {
                goto aic_set_upgrade_status_err;
            }

        } else {
            count++;
            str = itoa(count, string, 10);

            ret = fw_env_write("bootcount", str);
            if (ret) {
                pr_err("Env write fail\n");
                goto aic_set_upgrade_status_err;
            }
        }

        fw_env_flush();
    } else if (strncmp(status, "0", 2) == 0) {
        ret = 0;
    } else {
        pr_err("Invalid upgrade_available\n");
        ret = -1;
    }

aic_set_upgrade_status_err:
    fw_env_close();
    return ret;
}

int aic_get_os_to_startup(char *target_os)
{
    char *next = NULL;
    int ret = 0;

    if (fw_env_open()) {
        pr_err("Open env failed\n");
        return -1;
    }

    next = fw_getenv("osAB_next");
#ifdef AIC_ENV_DEBUG
    printf("osAB_next = %s\n", next);
#endif
    if (strncmp(next, "A", 2) == 0) {
        memcpy(target_os, APPLICATION_PART, strlen(APPLICATION_PART));
    } else if (strncmp(next, "B", 2) == 0) {
        memcpy(target_os, APPLICATION_PART_REDUNDAND,
               strlen(APPLICATION_PART_REDUNDAND));
    } else {
        ret = -1;
        pr_err("Invalid osAB_next\n");
    }

    fw_env_close();
    return ret;
}
