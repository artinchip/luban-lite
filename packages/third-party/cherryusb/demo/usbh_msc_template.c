/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#include "usbh_core.h"
#include "usbh_msc.h"

struct usbh_msc *active_msc_class;
struct dfs_partition part0;

#ifdef KERNEL_RTTHREAD
#include <dfs_fs.h>

struct rt_device udisk_dev;

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

    memset(&part0, 0, sizeof(part0));

    for (i=0; i<16; i++) {
        /* Get the first partition */
        ret = dfs_filesystem_get_partition(&part0, sector, i);
        if (ret == RT_EOK) {
            pr_info("Found partition %d: type = %d, offet=0x%x, size=0x%x\n",
                     i, part0.type, part0.offset, part0.size);
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

int udisk_exit(void)
{
    dfs_unmount_device(&udisk_dev);
    rt_device_unregister(&udisk_dev);
    return 0;
}

#else
#include <ff.h>
#include <diskio.h>
#ifdef LPKG_USING_DFS
#define HAVE_DIR_STRUCTURE
#include <dfs.h>
#include <dfs_fs.h>
#ifdef LPKG_USING_DFS_ELMFAT
#include <dfs_elm.h>
#endif
#endif

int USB_disk_status(void)
{
    return 0;
}
int USB_disk_initialize(void)
{
    return 0;
}
int USB_disk_read(BYTE *buff, LBA_t sector, UINT count)
{
    return usbh_msc_scsi_read10(active_msc_class, part0.offset + sector, buff, count);
}
int USB_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
    return usbh_msc_scsi_write10(active_msc_class, part0.offset + sector, buff, count);
}
int USB_disk_ioctl(BYTE cmd, void *buff)
{
    int result = 0;

    switch (cmd) {
        case CTRL_SYNC:
            result = RES_OK;
            break;

        case GET_SECTOR_SIZE:
            *(WORD *)buff = active_msc_class->blocksize;
            result = RES_OK;
            break;

        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1;
            result = RES_OK;
            break;

        case GET_SECTOR_COUNT:
            if (part0.offset)
                *(DWORD *)buff = part0.size;
            else
                *(DWORD *)buff = active_msc_class->blocknum;
            result = RES_OK;
            break;

        default:
            result = RES_PARERR;
            break;
    }

    return result;
}

#define UDISK_TEST
#ifdef UDISK_TEST
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#define UDISK_TEST_FILE "/udisk/test.txt"
void udisk_test()
{
    int fd = -1;
    uint8_t buff[64];
    int rlen = 0;
    int i = 0;

    fd = open(UDISK_TEST_FILE, O_RDWR);
    if (fd < 0) {
        pr_err("Open %s failed!", UDISK_TEST_FILE);
        return;
    }

    rlen = read(fd, buff, 64);
    printf("Read %s %d bytes data: \n", UDISK_TEST_FILE, rlen);
    for (i=0; i<rlen; i++) {
        if (i%16 == 0)
            printf("\r\n");
        printf("%02x ", buff[i]);
    }
    printf("\r\n");
    close(fd);
}
#endif

int udisk_init(void)
{
    uint8_t *sector = NULL;
    int ret = 0;
    int i;

    /* get the first sector to read partition table */
    sector = (uint8_t *)aicos_malloc(0, 512);
    if (sector == NULL)
    {
        pr_err("allocate partition sector buffer failed!");

        return -ENOMEM;
    }

    ret = usbh_msc_scsi_read10(active_msc_class, 0, sector, 1);
    if (ret != EOK)
    {
        pr_err("usb mass_storage read failed\n");
        goto free_res;;
    }

    memset(&part0, 0, sizeof(part0));

    for (i=0; i<16; i++) {
        /* Get the first partition */
        ret = dfs_filesystem_get_partition(&part0, sector, i);
        if (ret == EOK) {
            pr_info("Found partition %d: type = %d, offet=0x%lx, size=0x%lx\n",
                     i, part0.type, part0.offset, (unsigned long)part0.size);
            break;
        }
    }

    if (dfs_mount("udisk", "/udisk", "elm", 0, DEVICE_TYPE_USB_DISK) < 0) {
        pr_err("Failed to mount udisk with FatFS\n");
    } else {
        pr_info("Succeed to mount udisk with FatFS\n");
        #ifdef UDISK_TEST
        udisk_test();
        #endif
    }

free_res:
    if (sector)
        aicos_free(0, sector);

    return 0;
}

int udisk_exit(void)
{
    dfs_unmount("/udisk");
    return 0;
}
#endif

void usbh_msc_run(struct usbh_msc *msc_class)
{
    active_msc_class = msc_class;

    /* Mount fatfs usb massstorage */
    udisk_init();
}

void usbh_msc_stop(struct usbh_msc *msc_class)
{
    udisk_exit();
}

