/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <rtthread.h>
#include "rtdevice.h"
#include <aic_core.h>

#define CAN_DEV_RX_NAME       "can0"
#define CAN_DEV_TX_NAME       "can1"

static struct rt_semaphore rx_sem;
static rt_device_t can_dev;
static rt_device_t can_dev_rx;

static rt_err_t can_rx_call(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&rx_sem);

    return RT_EOK;
}

static void can_rx_thread(void *parameter)
{
    int i;
    rt_err_t ret;
    struct rt_can_msg rxmsg = {0};

    rxmsg.hdr = -1;

    ret = rt_sem_take(&rx_sem, 200);
    if (ret == -RT_ETIMEOUT)
    {
        rt_kprintf("CAN receive timeout\n");
        goto __exit;
    }

    rt_device_read(can_dev_rx, 0, &rxmsg, sizeof(rxmsg));

    rt_kprintf("ID:0x%08x ", rxmsg.id);
    for (i = 0; i < 8; i++)
        rt_kprintf("0x%02x ", rxmsg.data[i]);

    rt_kprintf("\n");

__exit:
    rt_sem_release(&rx_sem);
    rt_sem_detach(&rx_sem);
    rt_device_close(can_dev);
    rt_device_close(can_dev_rx);
}

int can_sample(int argc, char *argv[])
{
    struct rt_can_msg msg = {0};
    rt_err_t res;
    rt_size_t  size;
    rt_thread_t thread;

    can_dev_rx = rt_device_find(CAN_DEV_RX_NAME);
    if (!can_dev_rx)
    {
        rt_kprintf("find %s failed!\n", CAN_DEV_RX_NAME);
        return -RT_ERROR;
    }

    res = rt_device_open(can_dev_rx, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    RT_ASSERT(res == RT_EOK);
    res = rt_device_control(can_dev_rx, RT_CAN_CMD_SET_BAUD, (void *)CAN1MBaud);
    RT_ASSERT(res == RT_EOK);

    struct rt_can_filter_item items[1] =
    {
        RT_CAN_FILTER_ITEM_INIT(0x100, 0, 0, 0, 0x700, RT_NULL, RT_NULL),
    };

    struct rt_can_filter_config cfg = {1, 1, items};

    res = rt_device_control(can_dev_rx, RT_CAN_CMD_SET_FILTER, &cfg);

    rt_device_set_rx_indicate(can_dev_rx, can_rx_call);

    can_dev = rt_device_find(CAN_DEV_TX_NAME);
    if (!can_dev)
    {
        rt_kprintf("find %s failed!\n", CAN_DEV_TX_NAME);
        return -RT_ERROR;
    }

    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_PRIO);

    res = rt_device_open(can_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    RT_ASSERT(res == RT_EOK);

    res = rt_device_control(can_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN1MBaud);
    RT_ASSERT(res == RT_EOK);

    thread = rt_thread_create("can_rx", can_rx_thread, RT_NULL, 8192, 25, 10);
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        rt_kprintf("create can_rx thread failed!\n");
    }

    msg.id = 0x1FF;
    msg.ide = RT_CAN_STDID;
    msg.rtr = RT_CAN_DTR;
    msg.len = 8;
    msg.data[0] = 0x00;
    msg.data[1] = 0x11;
    msg.data[2] = 0x22;
    msg.data[3] = 0x33;
    msg.data[4] = 0x44;
    msg.data[5] = 0x55;
    msg.data[6] = 0x66;
    msg.data[7] = 0x77;

    size = rt_device_write(can_dev, 0, &msg, sizeof(msg));
    if (size == 0)
    {
        rt_kprintf("can dev write data failed!\n");
    }

    return res;
}

MSH_CMD_EXPORT(can_sample, can device sample);
