/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_OSAL_FREERTOS_H_
#define _ARTINCHIP_AIC_OSAL_FREERTOS_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <FreeRTOS.h>
#include <semphr.h>
#include <timers.h>
#include <event_groups.h>
#include <aic_errno.h>
#include <aic_tlsf.h>

//--------------------------------------------------------------------+
// Interrupt Define
//--------------------------------------------------------------------+

extern unsigned int g_aicos_irq_nested_cnt;
static inline int aicos_in_irq(void)
{
    return g_aicos_irq_nested_cnt;
}

//--------------------------------------------------------------------+
// Timeout Define
//--------------------------------------------------------------------+

#define AICOS_WAIT_FOREVER (-1)

//--------------------------------------------------------------------+
// Thread API
//--------------------------------------------------------------------+

static inline aicos_thread_t aicos_thread_create(const char *name, uint32_t stack_size, uint32_t prio, aic_thread_entry_t entry, void *args)
{
    TaskHandle_t htask = NULL;
    stack_size /= sizeof(StackType_t);
    xTaskCreate(entry, name, stack_size, args, prio, &htask);
    return (aicos_thread_t)htask;
}

static inline void aicos_thread_delete(aicos_thread_t thread)
{
    vTaskDelete(thread);
}

static inline void aicos_thread_suspend(aicos_thread_t thread)
{
    vTaskSuspend(thread);
}

static inline void aicos_thread_resume(aicos_thread_t thread)
{
    vTaskResume(thread);
}

//--------------------------------------------------------------------+
// Counting Semaphore API
//--------------------------------------------------------------------+

static inline aicos_sem_t aicos_sem_create(uint32_t initial_count)
{
    return (aicos_sem_t)xSemaphoreCreateCounting(1, initial_count);
}

static inline void aicos_sem_delete(aicos_sem_t sem)
{
    vSemaphoreDelete((SemaphoreHandle_t)sem);
}

static inline int aicos_sem_take(aicos_sem_t sem, uint32_t msec)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int ret;

    if (aicos_in_irq()) {
        ret = xSemaphoreTakeFromISR((SemaphoreHandle_t)sem, &xHigherPriorityTaskWoken);
        if (ret == pdPASS) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

    } else {
        if (msec == AICOS_WAIT_FOREVER)
            ret = xSemaphoreTake((SemaphoreHandle_t)sem, portMAX_DELAY);
        else
            ret = xSemaphoreTake((SemaphoreHandle_t)sem, pdMS_TO_TICKS(msec));
    }

    return (ret == pdPASS) ? 0 : -ETIMEDOUT;
}

static inline int aicos_sem_give(aicos_sem_t sem)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int ret;

    if (aicos_in_irq()) {
        ret = xSemaphoreGiveFromISR((SemaphoreHandle_t)sem, &xHigherPriorityTaskWoken);
        if (ret == pdPASS) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

    } else {
        ret = xSemaphoreGive((SemaphoreHandle_t)sem);
    }

    return (ret == pdPASS) ? 0 : -EINVAL;
}

//--------------------------------------------------------------------+
// Mutex API (priority inheritance)
//--------------------------------------------------------------------+

static inline aicos_mutex_t aicos_mutex_create(void)
{
    return (aicos_mutex_t)xSemaphoreCreateMutex();
}

static inline void aicos_mutex_delete(aicos_mutex_t mutex)
{
    vSemaphoreDelete((SemaphoreHandle_t)mutex);
}

static inline int aicos_mutex_take(aicos_mutex_t mutex, uint32_t msec)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int ret;

    if (aicos_in_irq()) {
        ret = xSemaphoreTakeFromISR((SemaphoreHandle_t)mutex, &xHigherPriorityTaskWoken);
        if (ret == pdPASS) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

    } else {
        if (msec == AICOS_WAIT_FOREVER)
            ret = xSemaphoreTake((SemaphoreHandle_t)mutex, portMAX_DELAY);
        else
            ret = xSemaphoreTake((SemaphoreHandle_t)mutex, pdMS_TO_TICKS(msec));
    }

    return (ret == pdPASS) ? 0 : -ETIMEDOUT;
}


static inline int aicos_mutex_give(aicos_mutex_t mutex)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int ret;

    if (aicos_in_irq()) {
        ret = xSemaphoreGiveFromISR((SemaphoreHandle_t)mutex, &xHigherPriorityTaskWoken);
        if (ret == pdPASS) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

    } else {
        ret = xSemaphoreGive((SemaphoreHandle_t)mutex);
    }

    return (ret == pdPASS) ? 0 : -EINVAL;
}

//--------------------------------------------------------------------+
// Event API
//--------------------------------------------------------------------+

static inline aicos_event_t aicos_event_create(void)
{
    return (aicos_event_t)xEventGroupCreate();
}

static inline void aicos_event_delete(aicos_event_t event)
{
    vEventGroupDelete((EventGroupHandle_t)event);
}

static inline int aicos_event_recv(aicos_event_t event, uint32_t set, uint32_t *recved, uint32_t msec)
{
    if (msec == AICOS_WAIT_FOREVER)
        *recved = xEventGroupWaitBits((EventGroupHandle_t)event, set, pdTRUE, pdFALSE, portMAX_DELAY);
    else
        *recved = xEventGroupWaitBits((EventGroupHandle_t)event, set, pdTRUE, pdFALSE, pdMS_TO_TICKS(msec));
    return 0;
}

static inline int aicos_event_send(aicos_event_t event, uint32_t set)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int ret;

    if (aicos_in_irq()) {
        ret = xEventGroupSetBitsFromISR((EventGroupHandle_t)event, set, &xHigherPriorityTaskWoken);
        if (ret == pdPASS) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    } else {
        ret = xEventGroupSetBits((EventGroupHandle_t)event, set);
    }

    return (ret == pdPASS) ? 0 : -EINVAL;
}

//--------------------------------------------------------------------+
// Queue API
//--------------------------------------------------------------------+

static inline aicos_queue_t aicos_queue_create(uint32_t item_size, uint32_t queue_size)
{
    return (aicos_queue_t)xQueueCreate(queue_size, item_size);
}

static inline void aicos_queue_delete(aicos_queue_t queue)
{
    vQueueDelete((QueueHandle_t)queue);
}

static inline int aicos_queue_send(aicos_queue_t queue, void const *buff)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int ret;

    if (aicos_in_irq()) {
        ret = xQueueSendToBackFromISR((QueueHandle_t)queue, buff, &xHigherPriorityTaskWoken);
        if (ret == pdPASS) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    } else {
        ret = xQueueSendToBack((QueueHandle_t)queue, buff, portMAX_DELAY);
    }

    return (ret == pdPASS) ? 0 : -EINVAL;
}

static inline int aicos_queue_receive(aicos_queue_t queue, void *buff, uint32_t msec)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int ret;

    if (aicos_in_irq()) {
        ret = xQueueReceiveFromISR((QueueHandle_t)queue, buff, &xHigherPriorityTaskWoken);
        if (ret == pdPASS) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    } else {
        if (msec == AICOS_WAIT_FOREVER)
            ret = xQueueReceive((QueueHandle_t)queue, buff, portMAX_DELAY);
        else
            ret = xQueueReceive((QueueHandle_t)queue, buff, pdMS_TO_TICKS(msec));
    }

    return (ret == pdPASS) ? 0 : -EINVAL;
}

static inline int aicos_queue_empty(aicos_queue_t queue)
{
    return uxQueueMessagesWaiting((QueueHandle_t)queue) == 0;
}

//--------------------------------------------------------------------+
// Critical API
//--------------------------------------------------------------------+

static inline size_t aicos_enter_critical_section(void)
{
    taskENTER_CRITICAL();
    return 1;
}

static inline void aicos_leave_critical_section(size_t flag)
{
    taskEXIT_CRITICAL();
}

//--------------------------------------------------------------------+
// Sleep API
//--------------------------------------------------------------------+

static inline void aicos_msleep(uint32_t delay)
{
    vTaskDelay(pdMS_TO_TICKS(delay));
}

//--------------------------------------------------------------------+
// Mem API
//--------------------------------------------------------------------+

static inline void *aicos_malloc(unsigned int mem_type, size_t size)
{
    if (mem_type == MEM_DEFAULT)
        return pvPortMalloc(size);
    else
        return aic_tlsf_malloc(mem_type, size);
}

static inline void aicos_free(unsigned int mem_type, void *mem)
{
    if (mem_type == MEM_DEFAULT)
        vPortFree(mem);
    else
        aic_tlsf_free(mem_type, mem);
}

static inline void *aicos_malloc_align(uint32_t mem_type, size_t size, size_t align)
{
    if (mem_type == MEM_DEFAULT)
        return _aicos_malloc_align_(size, align, 0xFFFF, (void *)pvPortMalloc);
    else
        return aic_tlsf_malloc_align(mem_type, size, align);
}

static inline void aicos_free_align(uint32_t mem_type, void *mem)
{
    if (mem_type == MEM_DEFAULT)
        _aicos_free_align_(mem, 0xFFFF, (void *)vPortFree);
    else
        aic_tlsf_free_align(mem_type, mem);
}

#ifdef __cplusplus
 }
#endif
#endif
