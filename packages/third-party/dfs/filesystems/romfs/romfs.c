/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include <dfs_bare.h>
#include <dfs_romfs.h>

const struct romfs_dirent _root_dirent[] =
{
    {ROMFS_DIRENT_DIR, "ram", NULL, 0},
    {ROMFS_DIRENT_DIR, "data", NULL, 0},
    {ROMFS_DIRENT_DIR, "rodata", NULL, 0},
    {ROMFS_DIRENT_DIR, "sdcard", NULL, 0},
    {ROMFS_DIRENT_DIR, "udisk", NULL, 0},
};

const struct romfs_dirent romfs_root =
{
    ROMFS_DIRENT_DIR, "/", (unsigned char *)_root_dirent, sizeof(_root_dirent) / sizeof(_root_dirent[0])
};

