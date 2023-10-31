/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019/01/13     Bernard      code cleanup
 */

#ifndef __DFS_ROMFS_H__
#define __DFS_ROMFS_H__

#include <dfs_bare.h>

#define ROMFS_DIRENT_FILE   0x00
#define ROMFS_DIRENT_DIR    0x01

struct romfs_dirent
{
    rt_uint32_t      type;  /* dirent type */

    const char       *name; /* dirent name */
    const unsigned char *data; /* file date ptr */
    rt_size_t        size;  /* file size */
};

int dfs_romfs_init(void);
extern const struct romfs_dirent romfs_root;

#endif
