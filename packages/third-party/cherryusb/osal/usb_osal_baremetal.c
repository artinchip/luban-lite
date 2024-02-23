/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usb_osal.h"
#include "usb_errno.h"
#include <aic_osal.h>

usb_osal_thread_t usb_osal_thread_create(const char *name, uint32_t stack_size, uint32_t prio, usb_thread_entry_t entry, void *args)
{
    return aicos_thread_create(name, stack_size, prio, entry, args);
}

void usb_osal_thread_delete(usb_osal_thread_t thread)
{
    aicos_thread_delete(thread);
}

usb_osal_sem_t usb_osal_sem_create(uint32_t initial_count)
{
    return (usb_osal_sem_t)aicos_sem_create(initial_count);
}

void usb_osal_sem_delete(usb_osal_sem_t sem)
{
    aicos_sem_delete((aicos_sem_t)sem);
}

int usb_osal_sem_take(usb_osal_sem_t sem, uint32_t timeout)
{
    return aicos_sem_take((aicos_sem_t)sem, timeout);
}

int usb_osal_sem_give(usb_osal_sem_t sem)
{
    return (int)aicos_sem_give((aicos_sem_t)sem);
}

usb_osal_mutex_t usb_osal_mutex_create(void)
{
    return (usb_osal_mutex_t)aicos_mutex_create();
}

void usb_osal_mutex_delete(usb_osal_mutex_t mutex)
{
    aicos_mutex_delete((aicos_mutex_t)mutex);
}

int usb_osal_mutex_take(usb_osal_mutex_t mutex)
{
    return (int)aicos_mutex_take((aicos_mutex_t)mutex, AICOS_WAIT_FOREVER);
}

int usb_osal_mutex_give(usb_osal_mutex_t mutex)
{
    return (int)aicos_mutex_give((aicos_mutex_t)mutex);
}

usb_osal_mq_t usb_osal_mq_create(uint32_t max_msgs)
{
    return (usb_osal_mq_t)aicos_queue_create(sizeof(uintptr_t), max_msgs);
}

int usb_osal_mq_send(usb_osal_mq_t mq, uintptr_t addr)
{
    return aicos_queue_send((aicos_queue_t)mq, &addr);
}

int usb_osal_mq_recv(usb_osal_mq_t mq, uintptr_t *addr, uint32_t timeout)
{
    return aicos_queue_receive((aicos_queue_t)mq, addr, timeout);
}

size_t usb_osal_enter_critical_section(void)
{
    return aicos_enter_critical_section();
}

void usb_osal_leave_critical_section(size_t flag)
{
    aicos_leave_critical_section(flag);
}

void usb_osal_msleep(uint32_t delay)
{
    aicos_msleep(delay);
}
