/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <rtthread.h>
#include "rtdevice.h"
#include <aic_core.h>

#define CAN_DEV_NAME                "can0"

static struct rt_semaphore rx_sem;
static rt_device_t can_dev;

static rt_err_t can_rx_call(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&rx_sem);

    return RT_EOK;
}

static void can_tx_thread(void *parameter)
{
    struct rt_can_msg msg = {0};
    uint8_t num = 0;
    rt_size_t  size;

    msg.id = 0x123;
    msg.ide = RT_CAN_STDID;
    msg.rtr = RT_CAN_DTR;
    msg.len = 8;
    msg.data[1] = 0x11;
    msg.data[2] = 0x22;
    msg.data[3] = 0x33;
    msg.data[4] = 0x44;
    msg.data[5] = 0x55;
    msg.data[6] = 0x66;

    while (1)
    {
        msg.data[0] = num;
        msg.data[7] = num++;

        size = rt_device_write(can_dev, 0, &msg, sizeof(msg));
        if (size != sizeof(msg))
        {
            rt_kprintf("can dev write data failed!\n");
            break;
        }
    }

    rt_device_close(can_dev);
}

static void can_rx_thread(void *parameter)
{
    int i;
    rt_size_t size;
    struct rt_can_msg rxmsg = {0};

    while (1)
    {
        rt_sem_take(&rx_sem, RT_WAITING_FOREVER);

        rxmsg.hdr = -1;
        size = rt_device_read(can_dev, 0, &rxmsg, sizeof(rxmsg));
        if (!size)
        {
            rt_kprintf("CAN read error\n");
            break;
        }

        rt_kprintf("ID: 0x%08x ", rxmsg.id);

        if (rxmsg.len)
            rt_kprintf("DATA: ");
        for (i = 0; i < rxmsg.len; i++)
        {
            rt_kprintf("%02x ", rxmsg.data[i]);
        }

        rt_kprintf("\n");
    }

    rt_device_close(can_dev);
}

int test_can(int argc, char *argv[])
{
    rt_err_t ret = 0;
    rt_thread_t thread1, thread2;

    can_dev = rt_device_find(CAN_DEV_NAME);
    if (!can_dev)
    {
        rt_kprintf("find %s failed!\n", CAN_DEV_NAME);
        return -RT_ERROR;
    }

    ret = rt_device_open(can_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_device_control(can_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN1MBaud);
    RT_ASSERT(ret == RT_EOK);

    struct rt_can_filter_item items[1] =
    {
        //Receive standard data frame, 0x100~0x1FF
        RT_CAN_FILTER_ITEM_INIT(0x100, 0, 0, 0, 0x700, RT_NULL, RT_NULL),
    };

    struct rt_can_filter_config cfg = {1, 1, items};

    ret = rt_device_control(can_dev, RT_CAN_CMD_SET_FILTER, &cfg);

    rt_device_set_rx_indicate(can_dev, can_rx_call);

    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_PRIO);

    thread1 = rt_thread_create("can_rx", can_rx_thread, RT_NULL, 8192, 25, 10);
    if (thread1 != RT_NULL)
    {
        rt_thread_startup(thread1);
    }
    else
    {
        rt_kprintf("create can_rx thread1 failed!\n");
        ret = -RT_ERROR;
    }

    thread2 = rt_thread_create("can_tx", can_tx_thread, RT_NULL, 8192, 25, 10);
    if (thread2 != RT_NULL)
    {
        rt_thread_startup(thread2);
    }
    else
    {
        rt_kprintf("create can_tx thread2 failed!\n");
        ret = -RT_ERROR;
    }

    return ret;
}

MSH_CMD_EXPORT_ALIAS(test_can, test_can, can tx rx sample);
