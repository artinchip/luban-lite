/*aic_rtt includes */
// #include <aic_rtt.h>
// #include <task.h>
// #include <timers.h>
// #include <semphr.h>
//#include <autoconf.h>
#include <string.h>
#include <osdep_service.h>
#include <stdio.h>
#include <stdarg.h>
//#include <tcm_heap.h>
#include <rtdef.h>
#include <wlan_intf.h>
// #include <customer_rtos_service.h>


/********************* os depended utilities ********************/

#ifndef USE_MUTEX_FOR_SPINLOCK
#define USE_MUTEX_FOR_SPINLOCK 1
#endif

//PRIORITIE_OFFSET  defined to adjust the priority of threads in wlan_lib
unsigned int g_prioritie_offset = 4;

//----- ------------------------------------------------------------------
// Misc Function
//----- ------------------------------------------------------------------

int rtw_printf(const char *format, ...)
{
    va_list args;
    unsigned int length;
    static char rt_log_buf[RT_CONSOLEBUF_SIZE];

    va_start(args, format);
    length = rt_vsnprintf(rt_log_buf, sizeof(rt_log_buf) - 1, format, args);
    if (length > RT_CONSOLEBUF_SIZE - 1)
        length = RT_CONSOLEBUF_SIZE - 1;

    int n = rt_kprintf("%s", rt_log_buf);

    va_end(args);
    return n;
}

#if CONFIG_MP_LOG_FUNC_INDEPENDENT
int rtw_mplog(const char *format, ...)
{
    va_list args;
    unsigned int length;
    static char rt_log_buf[RT_CONSOLEBUF_SIZE];

    va_start(args, format);
    length = rt_vsnprintf(rt_log_buf, sizeof(rt_log_buf) - 1, format, args);
    if (length > RT_CONSOLEBUF_SIZE - 1)
        length = RT_CONSOLEBUF_SIZE - 1;

    int n = rt_kprintf("%s", rt_log_buf);

    va_end(args);
    return n;
}
#endif

void save_and_cli()
{
    rt_enter_critical();
}

void restore_flags()
{
    rt_exit_critical();
}

void cli()
{
    aicos_local_irq_disable();
}

/* Not needed on 64bit architectures */
static unsigned int __div64_32(u64 *n, unsigned int base)
{
    u64 rem = *n;
    u64 b = base;
    u64 res, d = 1;
    unsigned int high = rem >> 32;

    /* Reduce the thing a bit first */
    res = 0;
    if (high >= base) {
        high /= base;
        res = (u64)high << 32;
        rem -= (u64)(high * base) << 32;
    }

    while ((u64)b > 0 && b < rem) {
        b = b + b;
        d = d + d;
    }

    do {
        if (rem >= b) {
            rem -= b;
            res += d;
        }
        b >>= 1;
        d >>= 1;
    } while (d);

    *n = res;
    return rem;
}

/********************* os depended service ********************/

u8 *rtt_malloc(u32 sz)
{
    return aicos_malloc_align(MEM_DEFAULT, (size_t)sz, CACHE_LINE_SIZE);
}

u8 *rtt_zmalloc(u32 sz)
{
    u8 *pbuf = rtt_malloc((size_t)sz);

    if (pbuf != NULL)
        memset(pbuf, 0, (size_t)sz);

    return pbuf;
}

void rtt_mfree(u8 *pbuf, u32 sz)
{
    aicos_free_align(MEM_DEFAULT, (void *)pbuf);
}

static void rtt_memcpy(void *dst, const void *src, u32 sz)
{
    memcpy(dst, src, sz);
}

static int rtt_memcmp(void *dst, void *src, u32 sz)
{
    //under Linux/GNU/GLibc, the return value of memcmp for two same mem. chunk is 0
    if (!(memcmp(dst, src, sz)))
        return 1;

    return 0;
}

static void rtt_memset(void *pbuf, int c, u32 sz)
{
    memset(pbuf, c, sz);
}

static void rtt_init_sema(aicos_sem_t *sema, int init_val)
{
    *sema = aicos_sem_create((uint32_t)init_val);
}

static void rtt_free_sema(aicos_sem_t *sema)
{
    if (*sema != NULL)
        aicos_sem_delete(*sema);

    *sema = NULL;
}

static void rtt_up_sema(aicos_sem_t *sema)
{
    aicos_sem_give(*sema);
}

static void rtt_up_sema_from_isr(aicos_sem_t *sema)
{
    aicos_sem_give(*sema);
}

static u32 rtt_down_sema(aicos_sem_t *sema, u32 timeout)
{
    if(timeout == RTW_MAX_DELAY) {
        timeout = AICOS_WAIT_FOREVER;
    }

    if(aicos_sem_take(*sema, timeout) != RT_EOK) {
        return 0;
    }

    return 1;
}

static void rtt_mutex_init(aicos_mutex_t *pmutex)
{
    *pmutex = aicos_mutex_create();
}

static void rtt_mutex_free(aicos_mutex_t *pmutex)
{
    if (*pmutex != NULL)
        aicos_mutex_delete(*pmutex);

    *pmutex = NULL;
}

static void rtt_mutex_get(aicos_mutex_t *plock)
{
    while(aicos_mutex_take(*plock, 60 * 1000) != RT_EOK)
        pr_info("%s(%p) failed, retry\n", __FUNCTION__, plock);
}

static int rtt_mutex_get_timeout(aicos_mutex_t *plock, u32 timeout_ms)
{
    if (aicos_mutex_take(*plock, timeout_ms) != RT_EOK) {
        pr_info("%s(%p) failed, retry\n", __FUNCTION__, plock);
        return -1;
    }
    return 0;
}

static void rtt_mutex_put(aicos_mutex_t *plock)
{
    aicos_mutex_give(*plock);
}

static void rtt_enter_critical(aicos_mutex_t *plock, _irqL *pirqL)
{
    rt_enter_critical();
}

static void rtt_exit_critical(aicos_mutex_t *plock, _irqL *pirqL)
{
    rt_exit_critical();
}

static void rtt_enter_critical_from_isr(aicos_mutex_t *plock, _irqL *pirqL)
{
    rt_enter_critical();
}

static void rtt_exit_critical_from_isr(aicos_mutex_t *plock, _irqL *pirqL)
{
    rt_exit_critical();
}

static int rtt_enter_critical_mutex(aicos_mutex_t *pmutex, _irqL *pirqL)
{
    rtt_mutex_get(pmutex);
    return 0;
}

static void rtt_exit_critical_mutex(aicos_mutex_t *pmutex, _irqL *pirqL)
{
    rtt_mutex_put(pmutex);
}

static void rtt_cpu_lock(void)
{

}

static void rtt_cpu_unlock(void)
{

}

static void rtt_spinlock_init(aicos_mutex_t *plock)
{
#if USE_MUTEX_FOR_SPINLOCK
    rtt_mutex_init(plock);
#endif
}

static void rtt_spinlock_free(aicos_mutex_t *plock)
{
#if USE_MUTEX_FOR_SPINLOCK
    rtt_mutex_free(plock);
#endif
}

static void rtt_spinlock(aicos_mutex_t *plock)
{
#if USE_MUTEX_FOR_SPINLOCK
    rtt_mutex_get(plock);
#endif
}

static void rtt_spinunlock(aicos_mutex_t *plock)
{
#if USE_MUTEX_FOR_SPINLOCK
    rtt_mutex_put(plock);
#endif
}

static void rtt_spinlock_irqsave(aicos_mutex_t *plock, _irqL *irqL)
{
    rtt_enter_critical(plock, irqL);
#if USE_MUTEX_FOR_SPINLOCK
    rtt_spinlock(plock);
#endif
}

static void rtt_spinunlock_irqsave(aicos_mutex_t *plock, _irqL *irqL)
{
#if USE_MUTEX_FOR_SPINLOCK
    rtt_spinunlock(plock);
#endif
    rtt_exit_critical(plock, irqL);
}

static int rtt_init_xqueue(aicos_queue_t *queue, const char *name,
                               u32 message_size, u32 number_of_messages)
{
    if ((*queue = aicos_queue_create(message_size, number_of_messages))
         == NULL)
        return -1;

    return 0;
}

static int rtt_push_to_xqueue(aicos_queue_t *queue, void *message, u32 timeout_ms)
{
    if (aicos_queue_send(*queue, message))
        return 0;
    else
        return -1;
}

static int rtt_pop_from_xqueue(aicos_queue_t *queue, void *message,
                                   u32 timeout_ms)
{
    if(timeout_ms == RTW_WAIT_FOREVER) {
        timeout_ms = AICOS_WAIT_FOREVER;
    }

    if (aicos_queue_receive(*queue, message, timeout_ms))
        return 0;
    else
        return -1;
}

static int rtt_deinit_xqueue(aicos_queue_t *queue)
{
    aicos_queue_delete(*queue);
    return 0;
}

static u32 rtt_get_current_time(void)
{
    return rt_tick_get(); //The count of ticks since  was called.
}

static u32 rtt_systime_to_ms(u32 systime)
{
    return systime * 1000 / RT_TICK_PER_SECOND;
}

static u32 rtt_systime_to_sec(u32 systime)
{
    return systime / RT_TICK_PER_SECOND;
}

static u32 rtt_ms_to_systime(u32 ms)
{
    return rt_tick_from_millisecond(ms);
}

static u32 rtt_sec_to_systime(u32 sec)
{
    return rt_tick_from_millisecond(sec * 1000);
}

static void rtt_msleep_os(int ms)
{
    aicos_msleep(ms);
}

static void rtt_usleep_os(int us)
{
    aicos_udelay(us);
}

static void rtt_mdelay_os(int ms)
{
    aicos_udelay(ms * 1000);
}

static void rtt_udelay_os(int us)
{
    aicos_udelay(us);
}

static void rtt_yield_os(void)
{
    rt_thread_yield();
}

static void rtt_ATOMIC_SET(ATOMIC_T *v, int i)
{
    atomic_set(v, i);
}

static int rtt_ATOMIC_READ(ATOMIC_T *v)
{
    return atomic_read(v);
}

static void rtt_ATOMIC_ADD(ATOMIC_T *v, int i)
{
    save_and_cli();
    v->counter += i;
    restore_flags();
}

static void rtt_ATOMIC_SUB(ATOMIC_T *v, int i)
{
    save_and_cli();
    v->counter -= i;
    restore_flags();
}

static void rtt_ATOMIC_INC(ATOMIC_T *v)
{
    rtt_ATOMIC_ADD(v, 1);
}

static void rtt_ATOMIC_DEC(ATOMIC_T *v)
{
    rtt_ATOMIC_SUB(v, 1);
}

static int rtt_ATOMIC_ADD_RETURN(ATOMIC_T *v, int i)
{
    int temp;

    save_and_cli();
    temp = v->counter;
    temp += i;
    v->counter = temp;
    restore_flags();

    return temp;
}

static int rtt_ATOMIC_SUB_RETURN(ATOMIC_T *v, int i)
{
    int temp;

    save_and_cli();
    temp = v->counter;
    temp -= i;
    v->counter = temp;
    restore_flags();

    return temp;
}

static int rtt_ATOMIC_INC_RETURN(ATOMIC_T *v)
{
    return rtt_ATOMIC_ADD_RETURN(v, 1);
}

static int rtt_ATOMIC_DEC_RETURN(ATOMIC_T *v)
{
    return rtt_ATOMIC_SUB_RETURN(v, 1);
}

static u64 rtt_modular64(u64 n, u64 base)
{
    unsigned int __base = (base);
    unsigned int __rem;

    if (((n) >> 32) == 0) {
        __rem = (unsigned int)(n) % __base;
        (n) = (unsigned int)(n) / __base;
    } else
        __rem = __div64_32(&(n), __base);

    return __rem;
}

/* Refer to ecos bsd tcpip codes */
static int rtt_arc4random(void)
{
    u32 res = rt_tick_get();
    static unsigned long seed = 0xDEADB00B;

    seed = ((seed & 0x007F00FF) << 7) ^
           ((seed & 0x0F80FF00) >> 8) ^ // be sure to stir those low bits
           (res << 13) ^ (res >> 9);    // using the clock too!
    return (int)seed;
}

static int rtt_get_random_bytes(void *buf, /*size_t*/u32 len)
{
#if 1 //becuase of 4-byte align, we use the follow code style.
    unsigned int ranbuf;
    unsigned int *lp;
    int i, count;
    count = len / sizeof(unsigned int);
    lp = (unsigned int *)buf;

    for (i = 0; i < count; i++) {
        lp[i] = rtt_arc4random();
        len -= sizeof(unsigned int);
    }

    if (len > 0) {
        ranbuf = rtt_arc4random();
        rtt_memcpy(&lp[i], &ranbuf, len);
    }
    return 0;
#else
    unsigned long ranbuf, *lp;
    lp = (unsigned long *)buf;
    while (len > 0) {
        ranbuf = rtt_arc4random();
        *lp++ = ranbuf; //this op need the pointer is 4Byte-align!
        len -= sizeof(ranbuf);
    }
    return 0;
#endif
}

u32 rtt_GetFreeHeapSize(void)
{
    rt_size_t total, used, max;

    rt_memory_info(&total, &used, &max);

    return total - used;
}

void *tcm_heap_malloc(int size);
static int rtt_create_task(struct task_struct *ptask, const char *name,
                               u32 stack_size, u32 priority, thread_func_t func,
                               void *thctx)
{
    int ret = 1;

    stack_size = (sizeof(void *) == 8) ? 8192:3072;
    ptask->task_name = name;

    ptask->task = (_thread_hdl_)aicos_thread_create(
            name, stack_size, priority, func, thctx);

    if (ptask->task == NULL) {
        pr_err("Create Task \"%s\" Failed!\n", ptask->task_name);
        ret = -1;
    }

    return ret;
}

static void rtt_delete_task(struct task_struct *ptask)
{
    if (!ptask->task) {
        pr_err("aic_rtt_delete_task(): ptask is NULL!\n");
        return;
    }

    aicos_thread_delete((aicos_thread_t)ptask->task);
    ptask->task = 0;
}

static void rtt_wakeup_task(struct task_struct *ptask)
{
    rtt_up_sema(&ptask->wakeup_sema);
}

static void rtt_thread_enter(char *name)
{
    pr_info("RTKTHREAD %s\n", name);
}

static void rtt_thread_exit(void)
{
    pr_info("RTKTHREAD exit %s\n", __FUNCTION__);
}

_timerHandle rtt_timerCreate(const signed char *pcTimerName,
                                 osdepTickType xTimerPeriodInTicks,
                                 u32 uxAutoReload,
                                 void *pvTimerID,
                                 TIMER_FUN pxCallbackFunction)
{
    rt_uint8_t flag = 0;
    rt_timer_t rt_timer = NULL;

    if (uxAutoReload)
        flag |= RT_TIMER_FLAG_PERIODIC;
    else
        flag |= RT_TIMER_FLAG_ONE_SHOT;

    flag |= RT_TIMER_FLAG_SOFT_TIMER;

    rt_timer = rt_timer_create((const char *)pcTimerName,
                                         pxCallbackFunction, pvTimerID,
                                         xTimerPeriodInTicks, flag);
    rt_timer->parameter = rt_timer;

     return (void *)rt_timer;
}

u32 rtt_timerDelete(_timerHandle xTimer, osdepTickType xBlockTime)
{
    if (rt_timer_delete((rt_timer_t)xTimer))
        return 0;
    else
        return 1;
}

u32 rtt_timerIsTimerActive(_timerHandle xTimer)
{
    rt_uint32_t state;

    rt_timer_control((rt_timer_t)xTimer, RT_TIMER_CTRL_GET_STATE, &state);

    if (state == RT_TIMER_FLAG_ACTIVATED)
        return 1;
    else
        return 0;
}

u32 rtt_timerStop(_timerHandle xTimer, osdepTickType xBlockTime)
{
    if (rt_timer_stop((rt_timer_t)xTimer))
        return 0;
    else
        return 1;
}

u32 rtt_timerChangePeriod(_timerHandle xTimer, osdepTickType xNewPeriod,
                              osdepTickType xBlockTime)
{
    if (xNewPeriod == 0)
        xNewPeriod += 1;

    if (rt_timer_control(xTimer, RT_TIMER_CTRL_SET_TIME, &xNewPeriod))
        return 0;

    if (rt_timer_start((rt_timer_t)xTimer))
        return 0;
    else
        return 1;
}

void *rtt_timerGetID(_timerHandle xTimer)
{
    rt_timer_t timer = (rt_timer_t)xTimer;
    return timer->parameter;
}

u32 rtt_timerStart(_timerHandle xTimer, osdepTickType xBlockTime)
{
    if (rt_timer_start((rt_timer_t)xTimer))
        return 0;
    else
        return 1;
}

u32 rtt_timerStartFromISR(_timerHandle xTimer,
                              osdepBASE_TYPE *pxHigherPriorityTaskWoken)
{
    if (rt_timer_start((rt_timer_t)xTimer))
        return 0;
    else
        return 1;
}

u32 rtt_timerStopFromISR(_timerHandle xTimer,
                             osdepBASE_TYPE *pxHigherPriorityTaskWoken)
{
    return rtt_timerStop(xTimer, (osdepTickType)(*pxHigherPriorityTaskWoken));
}
u32 rtt_timerReset(_timerHandle xTimer, osdepTickType xBlockTime)
{
    rtt_timerStop(xTimer, xBlockTime);
    return rtt_timerStart(xTimer, xBlockTime);
}

u32 rtt_timerResetFromISR(_timerHandle xTimer,
                              osdepBASE_TYPE *pxHigherPriorityTaskWoken)
{
    rtt_timerStopFromISR(xTimer, pxHigherPriorityTaskWoken);
    return rtt_timerStartFromISR(xTimer, pxHigherPriorityTaskWoken);
}

u32 rtt_timerChangePeriodFromISR(_timerHandle xTimer,
                                     osdepTickType xNewPeriod,
                                     osdepBASE_TYPE *pxHigherPriorityTaskWoken)
{
    return rtt_timerChangePeriod(xTimer, xNewPeriod,
                                     (osdepTickType)(*pxHigherPriorityTaskWoken));
}

void rtt_acquire_wakelock()
{
}

void rtt_release_wakelock()
{
}

void rtt_wakelock_timeout(uint32_t timeout)
{
}
#include <osdep_service.h>
u8 rtt_get_scheduler_state(void)
{
    rt_thread_t thd = rt_thread_self();

    switch (thd->stat) {
        case RT_THREAD_RUNNING:
            return OS_SCHEDULER_RUNNING;
        case RT_THREAD_SUSPEND:
            return OS_SCHEDULER_SUSPENDED;
        default:
            return OS_SCHEDULER_NOT_STARTED;
    }
}

const struct osdep_service_ops osdep_service = {
    rtt_malloc,                  //rtw_vmalloc
    rtt_zmalloc,                 //rtw_zvmalloc
    rtt_mfree,                   //rtw_vmfree
    rtt_malloc,                  //rtw_malloc
    rtt_zmalloc,                 //rtw_zmalloc
    rtt_mfree,                   //rtw_mfree
    rtt_memcpy,                  //rtw_memcpy
    rtt_memcmp,                  //rtw_memcmp
    rtt_memset,                  //rtw_memset
    rtt_init_sema,               //rtw_init_sema
    rtt_free_sema,               //rtw_free_sema
    rtt_up_sema,                 //rtw_up_sema
    rtt_up_sema_from_isr,        //rtw_up_sema_from_isr
    rtt_down_sema,               //rtw_down_sema
    rtt_mutex_init,              //rtw_mutex_init
    rtt_mutex_free,              //rtw_mutex_free
    rtt_mutex_get,               //rtw_mutex_get
    rtt_mutex_get_timeout,       //rtw_mutex_get_timeout
    rtt_mutex_put,               //rtw_mutex_put
    rtt_enter_critical,          //rtw_enter_critical
    rtt_exit_critical,           //rtw_exit_critical
    rtt_enter_critical_from_isr, //rtw_enter_critical_from_isr
    rtt_exit_critical_from_isr,  //rtw_exit_critical_from_isr
    NULL,                            //rtw_enter_critical_bh
    NULL,                            //rtw_exit_critical_bh
    rtt_enter_critical_mutex,    //rtw_enter_critical_mutex
    rtt_exit_critical_mutex,     //rtw_exit_critical_mutex
    rtt_cpu_lock,
    rtt_cpu_unlock,
    rtt_spinlock_init,      //rtw_spinlock_init
    rtt_spinlock_free,      //rtw_spinlock_free
    rtt_spinlock,           //rtw_spin_lock
    rtt_spinunlock,         //rtw_spin_unlock
    rtt_spinlock_irqsave,   //rtw_spinlock_irqsave
    rtt_spinunlock_irqsave, //rtw_spinunlock_irqsave
    rtt_init_xqueue,        //rtw_init_xqueue
    rtt_push_to_xqueue,     //rtw_push_to_xqueue
    rtt_pop_from_xqueue,    //rtw_pop_from_xqueue
    rtt_deinit_xqueue,      //rtw_deinit_xqueue
    rtt_get_current_time,   //rtw_get_current_time
    rtt_systime_to_ms,      //rtw_systime_to_ms
    rtt_systime_to_sec,     //rtw_systime_to_sec
    rtt_ms_to_systime,      //rtw_ms_to_systime
    rtt_sec_to_systime,     //rtw_sec_to_systime
    rtt_msleep_os,          //rtw_msleep_os
    rtt_usleep_os,          //rtw_usleep_os
    rtt_mdelay_os,          //rtw_mdelay_os
    rtt_udelay_os,          //rtw_udelay_os
    rtt_yield_os,           //rtw_yield_os

    rtt_ATOMIC_SET,        //ATOMIC_SET
    rtt_ATOMIC_READ,       //ATOMIC_READ
    rtt_ATOMIC_ADD,        //ATOMIC_ADD
    rtt_ATOMIC_SUB,        //ATOMIC_SUB
    rtt_ATOMIC_INC,        //ATOMIC_INC
    rtt_ATOMIC_DEC,        //ATOMIC_DEC
    rtt_ATOMIC_ADD_RETURN, //ATOMIC_ADD_RETURN
    rtt_ATOMIC_SUB_RETURN, //ATOMIC_SUB_RETURN
    rtt_ATOMIC_INC_RETURN, //ATOMIC_INC_RETURN
    rtt_ATOMIC_DEC_RETURN, //ATOMIC_DEC_RETURN

    rtt_modular64,        //rtw_modular64
    rtt_get_random_bytes, //rtw_get_random_bytes
    rtt_GetFreeHeapSize,  //rtw_getFreeHeapSize

    rtt_create_task, //rtw_create_task
    rtt_delete_task, //rtw_delete_task
    rtt_wakeup_task, //rtw_wakeup_task

    rtt_thread_enter, //rtw_thread_enter
    rtt_thread_exit,  //rtw_thread_exit

    rtt_timerCreate,              //rtw_timerCreate,
    rtt_timerDelete,              //rtw_timerDelete,
    rtt_timerIsTimerActive,       //rtw_timerIsTimerActive,
    rtt_timerStop,                //rtw_timerStop,
    rtt_timerChangePeriod,        //rtw_timerChangePeriod
    rtt_timerGetID,               //rtw_timerGetID
    rtt_timerStart,               //rtw_timerStart
    rtt_timerStartFromISR,        //rtw_timerStartFromISR
    rtt_timerStopFromISR,         //rtw_timerStopFromISR
    rtt_timerResetFromISR,        //rtw_timerResetFromISR
    rtt_timerChangePeriodFromISR, //rtw_timerChangePeriodFromISR
    rtt_timerReset,               //rtw_timerReset

    rtt_acquire_wakelock,   //rtw_acquire_wakelock
    rtt_release_wakelock,   //rtw_release_wakelock
    rtt_wakelock_timeout,   //rtw_wakelock_timeout
    rtt_get_scheduler_state //rtw_get_scheduler_state
};
