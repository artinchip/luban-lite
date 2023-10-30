/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date         Author          Notes
 * 2018-08-15   misonyo         first implementation.
 * 2023-05-25   geo.dong        ArtInChip
 */

#include <rtthread.h>
#include <aic_core.h>
#define SAMPLE_UART_NAME "uart4"

struct rt_semaphore rx_sem;
rt_device_t serial;
char str_send[] = "1234567890ArtInChip1234567890\n";
int g_exit = 0;

rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    if (size > 0)
        rt_sem_release(&rx_sem);

    return RT_EOK;
}
void serial_thread_entry(void *parameter)
{
    char ch;
    int ret = 0;
    char str_receive[32] = {0};
    int index = 0;
    while (1)
    {
        ret = rt_device_read(serial, -1, &ch, 1);
        if (ret == 1) {
            str_receive[index] = ch;
            index ++;
            if(index == 31 || ch == '\n')
            {
                printf("send   : %s \n", str_send);
                printf("receive: %s \n", str_receive);
                break;
            }
        } else {
            printf("RT_WAITING_SEM\n");
            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
            aicos_msleep(100);
        }
    }

    rt_sem_detach(&rx_sem);
    g_exit = 1;
}

int test_uart(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX];
    g_exit = 0;

    if (argc == 2)
    {
        rt_strncpy(uart_name, argv[1], RT_NAME_MAX);
    }
    else
    {
        rt_strncpy(uart_name, SAMPLE_UART_NAME, RT_NAME_MAX);
    }

    serial = rt_device_find(uart_name);
    if (!serial)
    {
        rt_kprintf("find %s failed!\n", uart_name);
        return RT_ERROR;
    }

    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);

    ret = rt_device_open(serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);

    if (ret != RT_EOK)
    {
        rt_kprintf("open %s failed : %d !\n", uart_name, ret);
        return RT_ERROR;
    }

    rt_device_set_rx_indicate(serial, uart_input);

    // NOTE: thread stack-size at least for 1024*2 Bytes !!!
    rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024*2, 25, 10);

    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        ret = RT_ERROR;
    }

    rt_device_write(serial, 0, str_send, (sizeof(str_send) - 1));
    while ( !g_exit )
    {
        aicos_msleep(100);
    }
    rt_device_close(serial);
    return ret;
}

MSH_CMD_EXPORT(test_uart, ArtInChip Uart Test);
