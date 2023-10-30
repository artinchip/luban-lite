/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2010-02-06     Bernard      Add elm_init function declaration
 */

#ifndef __DFS_ELM_H__
#define __DFS_ELM_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE     64
#endif

/* Definitions of physical drive number for each drive */
#define SDMC_DISK        0
#define SPINAND_DISK     1       /* spinand disk */
#define USB_DISK         2       /* usb disk */
#define SPINOR_DISK      3       /* spinor disk */
#define RAM_DISK         4       /* Example: ram disk */

struct dev_info {
    char *dev_name;
    int dev_type;
};

int elm_init(void);

#ifdef __cplusplus
}
#endif

#endif
