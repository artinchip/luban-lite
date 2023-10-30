/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-04-17    Jianjia Ma   first version
 */

#ifndef __DRV_RAMDISK_H_
#define __DRV_RAMDISK_H_

#include <rtthread.h>
#include "rtdevice.h"

/* ramdisk */
struct ramdisk_device
{
    struct rt_device                parent;     /**< RT-Thread device struct */
    struct rt_device_blk_geometry   geometry;   /**< sector size, sector count */

    rt_uint8_t* disk;                           /**< ramdisk start address */
    rt_size_t size;                             /**< size of the ramdisk */
    rt_uint8_t is_allocated;                    /**< whether the disk buffer is allocated by us or user*/
};

rt_err_t ramdisk_init(const char *dev_name, rt_uint8_t* disk_addr, rt_size_t block_size, rt_size_t num_block);

#endif /*__DRV_RAMDISK_H_ */
