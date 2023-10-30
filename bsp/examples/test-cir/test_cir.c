/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#include <stdio.h>
#include <string.h>
#include <rtthread.h>
#include "rtdevice.h"
#include <aic_core.h>
#include <getopt.h>

#include "drv_cir.h"

rt_device_t cir_dev;
struct rt_semaphore rx_sem;

static rt_err_t cir_rx_call(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&rx_sem);

    return RT_EOK;
}

static void cir_rx_thread(void *parameter)
{
    uint32_t scancode = 0;

    rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
    rt_device_read(cir_dev, 0, &scancode, sizeof(scancode));
    rt_kprintf("cir received scancode: %08x\n", scancode);
    rt_sem_detach(&rx_sem);
    rt_device_close(cir_dev);
}

static void usage(char * program)
{
    rt_kprintf("\nUsage: %s [-p proto] [-h] DATA\n", program);
    rt_kprintf("   DATA         Hex data\n");
    rt_kprintf("   -p proto     CIR protocol, such as NEC/RC5.\n");
    rt_kprintf("   -h           Display this help.\n\n");
    rt_kprintf("Example:\n");
    rt_kprintf("   %s -p NEC 0x8523\n", program);
    rt_kprintf("   %s -p RC5 0x81b\n", program);
}

int test_cir(int argc, char *argv[])
{
    int opt;
    cir_config_t config;
    unsigned int tx_code = 0;
    rt_thread_t thread;

    if (argc == 1)
    {
        usage(argv[0]);
        return 0;
    }

    memset(&config, 0, sizeof(config));
    optind = 0;
    while ((opt = getopt(argc, argv, "p:h")) != -1)
    {
        switch (opt)
        {
        case 'p':
            if (!optarg)
            {
                usage(argv[0]);
                return 0;
            }

            if (!strcmp(optarg, "NEC"))
                config.protocol = CIR_PROTOCOL_NEC;
            else if (!strcmp(optarg, "RC5"))
                config.protocol = CIR_PROTOCOL_RC5;
            else
            {
                rt_kprintf("protocol not support!\n");
                return -RT_ERROR;
            }
            break;
        case 'h':
        default:
                usage(argv[0]);
                return 0;
        }
    }

    tx_code = strtoul(argv[optind], NULL, 16);

    config.tx_duty = 33;
    config.rx_level = 1;

    rt_sem_init(&rx_sem, "cir_sem", 0, RT_IPC_FLAG_PRIO);

    cir_dev = rt_device_find("cir");
    if (!cir_dev)
    {
        rt_kprintf("cir device not found!\n");
        return -RT_ERROR;
    }

    rt_device_open(cir_dev, RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_RDWR);

    rt_device_control(cir_dev, IOC_CIR_CONFIGURE, (void *)&config);

    rt_device_set_rx_indicate(cir_dev, cir_rx_call);

    thread = rt_thread_create("cir_rx", cir_rx_thread, RT_NULL, 8192, 25, 10);
    if (thread)
    {
        rt_thread_startup(thread);
    }
    else
    {
        rt_kprintf("create cir_rx thread failed!\n");
        return -RT_ERROR;
    }

    rt_device_write(cir_dev, 0, &tx_code, sizeof(tx_code));

    return 0;
}

MSH_CMD_EXPORT(test_cir, test cir send and receive data);
