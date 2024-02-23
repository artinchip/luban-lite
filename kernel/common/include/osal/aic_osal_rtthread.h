/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_OSAL_RTTHREAD_H_
#define _ARTINCHIP_AIC_OSAL_RTTHREAD_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <rtthread.h>
#include <rthw.h>
#include <aic_errno.h>

//--------------------------------------------------------------------+
// Timeout Define
//--------------------------------------------------------------------+

#define AICOS_WAIT_FOREVER RT_WAITING_FOREVER

//--------------------------------------------------------------------+
// Thread API
//--------------------------------------------------------------------+

static inline aicos_thread_t aicos_thread_create(const char *name, uint32_t stack_size, uint32_t prio, aic_thread_entry_t entry, void *args)
{
    rt_thread_t htask;
    htask = rt_thread_create(name, entry, args, stack_size, prio, 10);
    rt_thread_startup(htask);
    return (aicos_thread_t)htask;
}

static inline void aicos_thread_delete(aicos_thread_t thread)
{
    rt_thread_delete(thread);
}

static inline void aicos_thread_suspend(aicos_thread_t thread)
{
    rt_thread_suspend(thread);
}

static inline void aicos_thread_resume(aicos_thread_t thread)
{
    rt_thread_resume(thread);
}

//--------------------------------------------------------------------+
// Counting Semaphore API
//--------------------------------------------------------------------+

static inline aicos_sem_t aicos_sem_create(uint32_t initial_count)
{
    return (aicos_sem_t)rt_sem_create("aicos_sem", initial_count, RT_IPC_FLAG_FIFO);
}

static inline void aicos_sem_delete(aicos_sem_t sem)
{
    rt_sem_delete((rt_sem_t)sem);
}

static inline int aicos_sem_take(aicos_sem_t sem, uint32_t msec)
{
    int ret = 0;
    rt_err_t result = RT_EOK;

    if (msec == AICOS_WAIT_FOREVER)
        result = rt_sem_take((rt_sem_t)sem, RT_WAITING_FOREVER);
    else
        result = rt_sem_take((rt_sem_t)sem, rt_tick_from_millisecond(msec));
    if (result == RT_ETIMEOUT) {
        ret = -ETIMEDOUT;
    } else if (result == RT_ERROR) {
        ret = -EINVAL;
    } else {
        ret = 0;
    }

    return (int)ret;
}

static inline int aicos_sem_give(aicos_sem_t sem)
{
    return (int)rt_sem_release((rt_sem_t)sem);
}

//--------------------------------------------------------------------+
// Mutex API (priority inheritance)
//--------------------------------------------------------------------+

static inline aicos_mutex_t aicos_mutex_create(void)
{
    return (aicos_mutex_t)rt_mutex_create("aicos_mutex", RT_IPC_FLAG_FIFO);
}

static inline void aicos_mutex_delete(aicos_mutex_t mutex)
{
    rt_mutex_delete((rt_mutex_t)mutex);
}

static inline int aicos_mutex_take(aicos_mutex_t mutex, uint32_t msec)
{
    if (msec == AICOS_WAIT_FOREVER)
        return (int)rt_mutex_take((rt_mutex_t)mutex, RT_WAITING_FOREVER);
    else
        return (int)rt_mutex_take((rt_mutex_t)mutex, rt_tick_from_millisecond(msec));
}

static inline int aicos_mutex_give(aicos_mutex_t mutex)
{
    return (int)rt_mutex_release((rt_mutex_t)mutex);
}

//--------------------------------------------------------------------+
// Event API
//--------------------------------------------------------------------+

static inline aicos_event_t aicos_event_create(void)
{
    return (aicos_event_t)rt_event_create("aicos_event", RT_IPC_FLAG_FIFO);
}

static inline void aicos_event_delete(aicos_event_t event)
{
    rt_event_delete((rt_event_t)event);
}

static inline int aicos_event_recv(aicos_event_t event, uint32_t set, uint32_t *recved, uint32_t msec)
{
    int ret = 0;
    rt_err_t result = RT_EOK;

    if (msec == AICOS_WAIT_FOREVER)
        result = rt_event_recv((rt_event_t)event, set, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, (rt_uint32_t *)recved);
    else
        result = rt_event_recv((rt_event_t)event, set, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, rt_tick_from_millisecond(msec), (rt_uint32_t *)recved);
    if (result == RT_ETIMEOUT) {
        ret = -ETIMEDOUT;
    } else if (result == RT_ERROR) {
        ret = -EINVAL;
    } else {
        ret = 0;
    }

    return ret;
}

static inline int aicos_event_send(aicos_event_t event, uint32_t set)
{
    int ret = 0;
    rt_err_t result = RT_EOK;

    result = rt_event_send((rt_event_t)event, set);
    if (result != RT_EOK) {
        ret = -EINVAL;
    }

    return ret;
}

//--------------------------------------------------------------------+
// Queue API
//--------------------------------------------------------------------+

static inline aicos_queue_t aicos_queue_create(uint32_t item_size, uint32_t queue_size)
{
    return (aicos_queue_t)rt_mq_create("aicos_queue", item_size, queue_size, RT_IPC_FLAG_FIFO);
}

static inline void aicos_queue_delete(aicos_queue_t queue)
{
    rt_mq_delete((rt_mq_t)queue);
}

static inline int aicos_queue_send(aicos_queue_t queue, void const *buff)
{
    rt_mq_t qhdl = (rt_mq_t)queue;

    return rt_mq_send(qhdl, (void *)buff, qhdl->msg_size) == RT_EOK;
}

static inline int aicos_queue_receive(aicos_queue_t queue, void *buff, uint32_t msec)
{
    rt_mq_t qhdl = (rt_mq_t)queue;

    if (msec == AICOS_WAIT_FOREVER)
        return rt_mq_recv(qhdl, buff, qhdl->msg_size, RT_WAITING_FOREVER) == RT_EOK;
    else
        return rt_mq_recv(qhdl, buff, qhdl->msg_size, rt_tick_from_millisecond(msec)) == RT_EOK;
}

static inline int aicos_queue_empty(aicos_queue_t queue)
{
    rt_mq_t qhdl = (rt_mq_t)queue;

    return (qhdl->entry) == 0;
}

//--------------------------------------------------------------------+
// Critical API
//--------------------------------------------------------------------+

static inline size_t aicos_enter_critical_section(void)
{
    return rt_hw_interrupt_disable();
}

static inline void aicos_leave_critical_section(size_t flag)
{
    rt_hw_interrupt_enable(flag);
}

//--------------------------------------------------------------------+
// Sleep API
//--------------------------------------------------------------------+

static inline void aicos_msleep(uint32_t delay)
{
    rt_thread_mdelay(delay);
}

//--------------------------------------------------------------------+
// Mem API
//--------------------------------------------------------------------+
void *aic_memheap_malloc(int type, size_t size);
void aic_memheap_free(int type, void *rmem);

static inline void *aicos_malloc(unsigned int mem_type, size_t size)
{
    if (mem_type == MEM_DEFAULT)
        return rt_malloc(size);
    else
        return aic_memheap_malloc(mem_type, size);
}

static inline void aicos_free(unsigned int mem_type, void *mem)
{
    if (mem_type == MEM_DEFAULT)
        rt_free(mem);
    else
        aic_memheap_free(mem_type, mem);
}

static inline void *aicos_malloc_align(uint32_t mem_type, size_t size, size_t align)
{
    if (mem_type == MEM_DEFAULT)
        return rt_malloc_align(size, align);
    else
        return _aicos_malloc_align_(size, align, mem_type, (void *)aic_memheap_malloc);
}

static inline void aicos_free_align(uint32_t mem_type, void *mem)
{
    if (mem_type == MEM_DEFAULT)
        rt_free_align(mem);
    else
        _aicos_free_align_(mem, mem_type, (void *)aic_memheap_free);
}

#ifdef __cplusplus
 }
#endif
#endif
