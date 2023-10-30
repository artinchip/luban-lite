/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-04-17    Jianjia Ma   first version
 * 2023-03-31    Vandoul      fix bug and add test cmd.
 */

#include "board.h"
#include <string.h>
#include <dfs_fs.h>
#include <sys/stat.h>
#include "drv_ramdisk.h"

//#define DRV_DEBUG

#define DBG_TAG              "drv.ramdisk"
#ifdef DRV_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif /* DRV_DEBUG */
#include <rtdbg.h>

static rt_err_t  rt_ramdisk_init(rt_device_t dev);
static rt_err_t  rt_ramdisk_open(rt_device_t dev, rt_uint16_t oflag);
static rt_err_t  rt_ramdisk_close(rt_device_t dev);
static rt_size_t rt_ramdisk_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size);
static rt_size_t rt_ramdisk_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size);
static rt_err_t  rt_ramdisk_control(rt_device_t dev, int cmd, void *args);


#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops ramdisk_ops =
{
    rt_ramdisk_init,
    rt_ramdisk_open,
    rt_ramdisk_close,
    rt_ramdisk_read,
    rt_ramdisk_write,
    rt_ramdisk_control
};
#endif

/* RT-Thread Device Driver Interface */
static rt_err_t rt_ramdisk_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t rt_ramdisk_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rt_ramdisk_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t rt_ramdisk_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    struct ramdisk_device *ramdisk = (struct ramdisk_device *)dev;

    rt_memcpy(buffer,
            ramdisk->disk + pos * ramdisk->geometry.block_size,
            size*ramdisk->geometry.block_size);

    return size;
}

static rt_size_t rt_ramdisk_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    struct ramdisk_device *ramdisk = (struct ramdisk_device *)dev;
    rt_memcpy(ramdisk->disk + pos * ramdisk->geometry.block_size,
            buffer,
            size*ramdisk->geometry.block_size);
    return size;
}


static rt_err_t rt_ramdisk_control(rt_device_t dev, int cmd, void *args)
{
    struct ramdisk_device *ramdisk = (struct ramdisk_device *)dev;

    RT_ASSERT(dev != RT_NULL);

    if (cmd == RT_DEVICE_CTRL_BLK_GETGEOME)
    {
        struct rt_device_blk_geometry *geometry;

        geometry = (struct rt_device_blk_geometry *)args;
        if (geometry == RT_NULL) return -RT_ERROR;

        geometry->bytes_per_sector = ramdisk->geometry.bytes_per_sector;
        geometry->block_size = ramdisk->geometry.block_size;
        geometry->sector_count = ramdisk->geometry.sector_count;
    }

    return RT_EOK;
}

rt_err_t ramdisk_init(const char *dev_name, rt_uint8_t* disk_addr, rt_size_t block_size, rt_size_t num_block)
{
    rt_err_t result = RT_EOK;
    struct ramdisk_device *ramdisk_dev;

    RT_ASSERT(num_block*block_size > 0)

    ramdisk_dev = rt_malloc(sizeof(struct ramdisk_device));
    if (ramdisk_dev == RT_NULL)
    {
        LOG_E("no memory for ramdisk control block: %s", dev_name);
        return -RT_ENOMEM;
    }
    rt_memset(ramdisk_dev, 0, sizeof(struct ramdisk_device));

    /* allocate memory for disk if user hasn't done it */
    if(disk_addr == RT_NULL)
    {
        disk_addr = rt_malloc(num_block*block_size);
        if (disk_addr == RT_NULL)
        {
            LOG_E("no memory for ramdisk %s, require %d bytes", dev_name, num_block*block_size);
            return -RT_ENOMEM;
        }
        ramdisk_dev->is_allocated = 1;
    }

    /* device type */
    ramdisk_dev->parent.type = RT_Device_Class_Block;

    /* set up ops */
#ifdef RT_USING_DEVICE_OPS
    ramdisk_dev->parent.ops  = &ramdisk_ops;
#else
    ramdisk_dev->parent.init   = rt_ramdisk_init;
    ramdisk_dev->parent.open   = rt_ramdisk_open;
    ramdisk_dev->parent.read   = rt_ramdisk_read;
    ramdisk_dev->parent.write  = rt_ramdisk_write;
    ramdisk_dev->parent.close  = rt_ramdisk_close;
    ramdisk_dev->parent.control= rt_ramdisk_control;
#endif

    /* no callback no private data */
    ramdisk_dev->parent.user_data = RT_NULL;
    ramdisk_dev->parent.rx_indicate = RT_NULL;
    ramdisk_dev->parent.tx_complete = RT_NULL;

    /* set up geometry */
    ramdisk_dev->geometry.sector_count = num_block;
    ramdisk_dev->geometry.block_size = block_size;
    ramdisk_dev->geometry.bytes_per_sector = block_size;

    /* set up disk */
    ramdisk_dev->size = num_block*block_size;
    ramdisk_dev->disk = disk_addr;

    /* register ramdisk as a block device */
    result = rt_device_register((rt_device_t)ramdisk_dev, dev_name,
                                RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_REMOVABLE);

    return result;
}

#include <finsh.h>

static int str2num(char *str)
{
    if(str == RT_NULL || str[0] == '\0')
    {
        return 0;
    }
    if(str[0] == '0' && ((str[1] == 'x') || (str[1] == 'X')))
    {
        int value = 0;
        for(int i=2; str[i] && i < 10; i++)
        {
            value <<= 4;
            if(str[i] >= '0' && str[i] <= '9')
            {
                value += str[i] - '0';
            }
            else if(str[i] >= 'a' && str[i] <= 'f')
            {
                value += str[i] - 'a';
            }
            else if(str[i] >= 'A' && str[i] <= 'F')
            {
                value += str[i] - 'A';
            }
            else
            {
                return value >> 4;
            }
        }
        return value;
    }
    else
    {
        return atoi(str);
    }
}
int ramdisk_cmd(int argc, char *argv[])
{
    if(argc != 4)
    {
        rt_kprintf("usage: %s <name> <block_size> <block_num>\n", argv[0]);
        return 0;
    }
    const char *name = argv[1];
    int bsize = str2num(argv[2]);
    int bnum = str2num(argv[3]);
    return ramdisk_init(name, RT_NULL, bsize, bnum);
}
MSH_CMD_EXPORT_ALIAS(ramdisk_cmd, ramdisk, create ramdisk device);


#ifdef LPKG_RAMDISK_TYPE_EMPTY
int ramdisk_device_init(void)
{
   ramdisk_init("ramdisk0", NULL, LPKG_RAMDISK_BLK_SIZE, LPKG_RAMDISK_NUM_BLK);
   return RT_EOK;
}
INIT_DEVICE_EXPORT(ramdisk_device_init);
#endif
