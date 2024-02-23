/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_OSAL_RHINO_H_
#define _ARTINCHIP_AIC_OSAL_RHINO_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <aos/aos.h>
#include <k_api.h>

//--------------------------------------------------------------------+
// Timeout Define
//--------------------------------------------------------------------+

#define AICOS_WAIT_FOREVER AOS_WAIT_FOREVER

//--------------------------------------------------------------------+
// Thread API
//--------------------------------------------------------------------+

static inline aicos_thread_t aicos_thread_create(const char *name, uint32_t stack_size, uint32_t prio, aic_thread_entry_t entry, void *args)
{
    aos_task_t task = NULL;
    int ret = 0;

    ret = aos_task_create(&task, name, entry,
                          args, NULL, stack_size, prio, 0);
    if (ret != 0) {
        return NULL;
    }

    ret = aos_task_resume(&task);
    if (ret != 0) {
        return NULL;
    }

    return (aicos_thread_t)task;
}

static inline void aicos_thread_delete(aicos_thread_t thread)
{
    aos_task_delete(&thread);
}

static inline void aicos_thread_suspend(aicos_thread_t thread)
{
    aos_task_suspend(&thread);
}

static inline void aicos_thread_resume(aicos_thread_t thread)
{
    aos_task_resume(&thread);
}

//--------------------------------------------------------------------+
// Counting Semaphore API
//--------------------------------------------------------------------+

static inline aicos_sem_t aicos_sem_create(uint32_t initial_count)
{
    aos_sem_t sem = NULL;
    int ret = 0;

    ret = aos_sem_create(&sem, initial_count, 0);
    if (ret != 0) {
        return NULL;
    }

    return (aicos_sem_t)sem;
}

static inline void aicos_sem_delete(aicos_sem_t sem)
{
    aos_sem_free(&sem);
}

static inline int aicos_sem_take(aicos_sem_t sem, uint32_t msec)
{
    return aos_sem_wait(&sem, msec);
}

static inline int aicos_sem_give(aicos_sem_t sem)
{
    aos_sem_signal(&sem);
    return 0;
}

//--------------------------------------------------------------------+
// Mutex API (priority inheritance)
//--------------------------------------------------------------------+

static inline aicos_mutex_t aicos_mutex_create(void)
{
    aos_mutex_t mutex = NULL;
    int ret = 0;

    ret = aos_mutex_new(&mutex);
    if (ret != 0) {
        return NULL;
    }

    return (aicos_mutex_t)mutex;
}

static inline void aicos_mutex_delete(aicos_mutex_t mutex)
{
    aos_mutex_free(&mutex);
}

static inline int aicos_mutex_take(aicos_mutex_t mutex, uint32_t msec)
{
    return aos_mutex_lock(&mutex, msec);
}

static inline int aicos_mutex_give(aicos_mutex_t mutex)
{
    return aos_mutex_unlock(&mutex);
}

//--------------------------------------------------------------------+
// Event API
//--------------------------------------------------------------------+

static inline aicos_event_t aicos_event_create(void)
{
    aos_event_t event = NULL;
    int ret = 0;

    ret = aos_event_new(&event, 0);
    if (ret != 0) {
        return NULL;
    }

    return (aicos_event_t)event;
}

static inline void aicos_event_delete(aicos_event_t event)
{
    aos_event_free(&event);
}

static inline int aicos_event_recv(aicos_event_t event, uint32_t set, uint32_t *recved, uint32_t msec)
{

    return aos_event_get(&event, set, AOS_EVENT_OR_CLEAR, recved, msec);
}

static inline int aicos_event_send(aicos_event_t event, uint32_t set)
{
    return aos_event_set(&event, set, AOS_EVENT_OR);
}

//--------------------------------------------------------------------+
// Queue API
//--------------------------------------------------------------------+

typedef struct {
    int item_size;
    int depth;
    void *buf;
    void *q;
} osal_queue_def_t;

static inline aicos_queue_t aicos_queue_create(uint32_t item_size, uint32_t queue_size)
{
    osal_queue_def_t *p_queue_def;
    int ret = 0;

    p_queue_def = aicos_malloc(0, sizeof(osal_queue_def_t) + (item_size * queue_size));
    if (p_queue_def == NULL)
        return NULL;

    p_queue_def->buf = (char *)p_queue_def + sizeof(osal_queue_def_t);
    p_queue_def->item_size = item_size;
    p_queue_def->depth = queue_size;

    ret = aos_queue_new((aos_queue_t *)&(p_queue_def->q), p_queue_def->buf,
                        p_queue_def->item_size, p_queue_def->depth);
    if (ret != 0) {
        aicos_free(0, p_queue_def);
        return NULL;
    }

    return (aicos_queue_t)p_queue_def;
}

static inline void aicos_queue_delete(aicos_queue_t queue)
{
    osal_queue_def_t *p_queue_def = (osal_queue_def_t *)queue;

    aos_queue_free((aos_queue_t *)&(p_queue_def->q));
    aicos_free(0, p_queue_def);
}

static inline int aicos_queue_send(aicos_queue_t queue, void const *buff)
{
    osal_queue_def_t *p_queue_def = (osal_queue_def_t *)queue;

    return aos_queue_send((aos_queue_t *)&(p_queue_def->q), (void *)buff, p_queue_def->item_size);
}

static inline int aicos_queue_receive(aicos_queue_t queue, void *buff, uint32_t msec)
{
    osal_queue_def_t *p_queue_def = (osal_queue_def_t *)queue;
    size_t size;

    return aos_queue_recv((aos_queue_t *)&(p_queue_def->q), msec, buff, &size);
}

static inline int aicos_queue_empty(aicos_queue_t queue)
{
    osal_queue_def_t *p_queue_def = (osal_queue_def_t *)queue;

    return aos_queue_get_count((aos_queue_t *)&(p_queue_def->q)) == 0;
}

//--------------------------------------------------------------------+
// Critical API
//--------------------------------------------------------------------+

static inline size_t aicos_enter_critical_section(void)
{
    CPSR_ALLOC();
    RHINO_CRITICAL_ENTER();
    return cpsr;
}

static inline void aicos_leave_critical_section(size_t flag)
{
    CPSR_ALLOC();
    cpsr = flag;
    RHINO_CRITICAL_EXIT();
}

//--------------------------------------------------------------------+
// Sleep API
//--------------------------------------------------------------------+

static inline void aicos_msleep(uint32_t delay)
{
    aos_msleep(delay);
}

#ifdef __cplusplus
 }
#endif

#endif
