/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#include "usbh_core.h"
#include "usbh_msc.h"
#ifdef KERNEL_RTTHREAD
#include <dfs_fs.h>

struct rt_device udisk_dev;
struct usbh_msc *active_msc_class;
struct dfs_partition part0;

static rt_err_t rt_udisk_init(rt_device_t dev)
{
#if 0
    active_msc_class = (struct usbh_msc *)usbh_find_class_instance("/dev/sda");
    if (active_msc_class == NULL) {
        printf("do not find /dev/sda\r\n");
        return -1;
    }
#endif
    return RT_EOK;
}

static rt_size_t rt_udisk_read(rt_device_t dev, rt_off_t pos, void* buffer,
    rt_size_t size)
{
    rt_err_t ret;

    ret = usbh_msc_scsi_read10(active_msc_class, part0.offset + pos, buffer, size);

    if (ret != RT_EOK)
    {
        rt_kprintf("usb mass_storage read failed\n");
        return 0;
    }

    return size;
}

static rt_size_t rt_udisk_write (rt_device_t dev, rt_off_t pos, const void* buffer,
    rt_size_t size)
{
    rt_err_t ret;

    ret = usbh_msc_scsi_write10(active_msc_class, part0.offset + pos, buffer, size);
    if (ret != RT_EOK)
    {
        rt_kprintf("usb mass_storage write %d sector failed\n", size);
        return 0;
    }

    return size;
}

static rt_err_t rt_udisk_control(rt_device_t dev, int cmd, void *args)
{
    /* check parameter */
    RT_ASSERT(dev != RT_NULL);

    if (cmd == RT_DEVICE_CTRL_BLK_GETGEOME)
    {
        struct rt_device_blk_geometry *geometry;

        geometry = (struct rt_device_blk_geometry *)args;
        if (geometry == RT_NULL) return -RT_ERROR;

        geometry->bytes_per_sector = active_msc_class->blocksize;
        geometry->block_size = 1 * active_msc_class->blocksize;
        if (part0.offset) {
            geometry->sector_count = part0.size;
        } else {
            geometry->sector_count = active_msc_class->blocknum;
        }
    }

    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops udisk_device_ops =
{
    rt_udisk_init,
    RT_NULL,
    RT_NULL,
    rt_udisk_read,
    rt_udisk_write,
    rt_udisk_control
};
#endif

int udisk_init(void)
{
    rt_uint8_t *sector = NULL;
    rt_err_t ret = 0;
    rt_uint8_t i;

    /* get the first sector to read partition table */
    sector = (rt_uint8_t *)rt_malloc(512);
    if (sector == RT_NULL)
    {
        pr_err("allocate partition sector buffer failed!");

        return -RT_ENOMEM;
    }

    ret = usbh_msc_scsi_read10(active_msc_class, 0, sector, 1);
    if (ret != RT_EOK)
    {
        rt_kprintf("usb mass_storage read failed\n");
        goto free_res;;
    }

    for (i=0; i<16; i++) {
        /* Get the first partition */
        ret = dfs_filesystem_get_partition(&part0, sector, i);
        if (ret == RT_EOK) {
            pr_info("Found partition %d: type = %d, offet=0x%x, size=0x%x\n",
                     i, part0.type, part0.offset, part0.size);
        } else {
            break;
        }
    }

    udisk_dev.type    = RT_Device_Class_Block;
#ifdef RT_USING_DEVICE_OPS
    udisk_dev.ops     = &udisk_device_ops;
#else
    udisk_dev.init    = rt_udisk_init;
    udisk_dev.read    = rt_udisk_read;
    udisk_dev.write   = rt_udisk_write;
    udisk_dev.control = rt_udisk_control;
#endif
    udisk_dev.user_data = NULL;

    rt_device_register(&udisk_dev, "udisk", RT_DEVICE_FLAG_RDWR |
        RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);

#ifdef RT_USING_DFS_MNTTABLE
    LOG_I("try to mount file system!");
    /* try to mount file system on this block device */
    dfs_mount_device(&udisk_dev);
#else
    int ret = 0;
    ret = dfs_mount(udisk_dev.parent.name, "/", "elm", 0, 0);
    if (ret == 0)
    {
        printf("udisk mount successfully\n");
    }
    else
    {
        printf("udisk mount failed, ret = %d\n", ret);
    }
#endif

free_res:
    if (sector)
        rt_free(sector);
    return ret;
}
#endif

void usbh_msc_run(struct usbh_msc *msc_class)
{
    active_msc_class = msc_class;

    /* Mount fatfs usb massstorage */
#ifdef KERNEL_RTTHREAD
    udisk_init();
#endif
}


