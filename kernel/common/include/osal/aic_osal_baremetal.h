/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_OSAL_BAREMETAL_H_
#define _ARTINCHIP_AIC_OSAL_BAREMETAL_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <aic_errno.h>
#include <aic_tlsf.h>
#include <aic_time.h>

//--------------------------------------------------------------------+
// Mem API
//--------------------------------------------------------------------+

static inline void *aicos_malloc(unsigned int mem_type, size_t size)
{
    return aic_tlsf_malloc(mem_type, size);
}

static inline void aicos_free(unsigned int mem_type, void *mem)
{
    aic_tlsf_free(mem_type, mem);
}

static inline void *aicos_malloc_align(uint32_t mem_type, size_t size, size_t align)
{
    return aic_tlsf_malloc_align(mem_type, size, align);
}

static inline void aicos_free_align(uint32_t mem_type, void *mem)
{
    aic_tlsf_free_align(mem_type, mem);
}

//--------------------------------------------------------------------+
// Timeout Define
//--------------------------------------------------------------------+

#define AICOS_WAIT_FOREVER 0xffffffffu

//--------------------------------------------------------------------+
// Thread API
//--------------------------------------------------------------------+

static inline aicos_thread_t aicos_thread_create(const char *name, uint32_t stack_size, uint32_t prio, aic_thread_entry_t entry, void *args) {return NULL;}
static inline void aicos_thread_delete(aicos_thread_t thread) {}
static inline void aicos_thread_suspend(aicos_thread_t thread) {}
static inline void aicos_thread_resume(aicos_thread_t thread) {}

//--------------------------------------------------------------------+
// Counting Semaphore API
//--------------------------------------------------------------------+

typedef struct
{
  volatile unsigned short count;
}osal_semaphore_def_t;

typedef osal_semaphore_def_t* osal_semaphore_t;
typedef osal_semaphore_def_t aicos_mutex_s;

static inline aicos_sem_t aicos_sem_create(uint32_t initial_count)
{
    osal_semaphore_t sem;

    sem = aicos_malloc(0, sizeof(osal_semaphore_def_t));
    if (NULL == sem)
        return NULL;

    sem->count = initial_count;

    return (aicos_sem_t)sem;
}

static inline void aicos_sem_delete(aicos_sem_t sem)
{
    aicos_free(0, sem);
}

static inline int aicos_sem_take(aicos_sem_t sem, uint32_t msec)
{
    osal_semaphore_t sem_hdl = (osal_semaphore_t)sem;
    unsigned int s_us = 0;
    unsigned int e_us = 0;

    if (sem_hdl->count > 0){
        sem_hdl->count--;
        return 0;
    }

    if (msec == 0) {
        return -EAGAIN;
    } else if (msec == AICOS_WAIT_FOREVER) {
        while (sem_hdl->count == 0) { }
    } else {
        s_us = aic_get_time_us();
        while (sem_hdl->count == 0) {
            e_us = aic_get_time_us();
            if (((e_us-s_us) / 1000) >= msec)
                return -ETIME;
        }
    }

    if (sem_hdl->count > 0){
        sem_hdl->count--;
        return 0;
    }

    return -EAGAIN;
}

static inline int aicos_sem_give(aicos_sem_t sem)
{
    osal_semaphore_t sem_hdl = (osal_semaphore_t)sem;

    sem_hdl->count++;

    return 0;
}

//--------------------------------------------------------------------+
// Mutex API (priority inheritance)
//--------------------------------------------------------------------+

static inline void aicos_mutex_init(aicos_mutex_t mutex)
{
    osal_semaphore_t m = (osal_semaphore_t)mutex;
    m->count = 1;
}

static inline aicos_mutex_t aicos_mutex_create(void)
{
    return (aicos_mutex_t)aicos_sem_create(1);
}

static inline void aicos_mutex_delete(aicos_mutex_t mutex)
{
    aicos_sem_delete(mutex);
}

static inline int aicos_mutex_take(aicos_mutex_t mutex, uint32_t msec)
{
    return aicos_sem_take(mutex, msec);
}

static inline int aicos_mutex_give(aicos_mutex_t mutex)
{
    return aicos_sem_give(mutex);
}

//--------------------------------------------------------------------+
// Event API
//--------------------------------------------------------------------+

typedef struct
{
  volatile unsigned int set;
}osal_event_def_t;

typedef osal_event_def_t* osal_event_t;

static inline aicos_event_t aicos_event_create(void)
{
    osal_event_t event;

    event = aicos_malloc(0, sizeof(osal_event_def_t));
    if (NULL == event)
        return NULL;

    event->set = 0;

    return (aicos_event_t)event;
}

static inline void aicos_event_delete(aicos_event_t event)
{
    aicos_free(0, event);
}

static inline int aicos_event_recv(aicos_event_t event, uint32_t set, uint32_t *recved, uint32_t msec)
{
    osal_event_t event_hdl = (osal_event_t)event;
    unsigned int s_us = 0;
    unsigned int e_us = 0;

    if (set == 0)
        return -EINVAL;

    if (event_hdl->set & set) {
        if (recved)
            *recved = (event_hdl->set & set);
        event_hdl->set &= ~set;
        return 0;
    }

    if (msec == 0) {
        return -EAGAIN;
    } else if (msec == AICOS_WAIT_FOREVER) {
        while (!(event_hdl->set & set)) { }
    } else {
        s_us = aic_get_time_us();
        while (!(event_hdl->set & set)) {
            e_us = aic_get_time_us();
            if (((e_us-s_us) / 1000) >= msec)
                return -ETIME;
        }
    }

    if (event_hdl->set & set) {
        if (recved)
            *recved = (event_hdl->set & set);
        event_hdl->set &= ~set;
        return 0;
    }

    return -EINVAL;
}

static inline int aicos_event_send(aicos_event_t event, uint32_t set)
{
    osal_event_t event_hdl = (osal_event_t)event;

    /* set event */
    event_hdl->set |= set;

    return 0;
}

//--------------------------------------------------------------------+
// Queue API
//--------------------------------------------------------------------+
static inline aicos_queue_t aicos_queue_create(uint32_t item_size, uint32_t queue_size) {return NULL;}
static inline void aicos_queue_delete(aicos_queue_t queue) {}
static inline int aicos_queue_send(aicos_queue_t queue, void const *buff) {return -1;}
static inline int aicos_queue_receive(aicos_queue_t queue, void *buff, uint32_t msec) {return -1;}
static inline int aicos_queue_empty(aicos_queue_t queue) {return 0;}

//--------------------------------------------------------------------+
// Critical API
//--------------------------------------------------------------------+
static inline size_t aicos_enter_critical_section(void) {return -1;}
static inline void aicos_leave_critical_section(size_t flag) {}

//--------------------------------------------------------------------+
// Sleep API
//--------------------------------------------------------------------+
static inline void aicos_msleep(uint32_t delay) {}



#ifdef __cplusplus
 }
#endif

#endif /* _ARTINCHIP_AIC_OSAL_BAREMETAL_H_ */
