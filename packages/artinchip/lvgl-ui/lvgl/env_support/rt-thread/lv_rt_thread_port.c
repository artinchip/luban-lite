/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: MIT
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-10-18     Meco Man     the first version
 * 2022-05-10     Meco Man     improve rt-thread initialization process
 */

#ifdef __RTTHREAD__

#include <lvgl.h>
#include <rtthread.h>
#include <rtdevice.h>

#define DBG_TAG    "LVGL"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

#ifndef LPKG_LVGL_THREAD_STACK_SIZE
#define LPKG_LVGL_THREAD_STACK_SIZE 4096 * 2
#endif /* LPKG_LVGL_THREAD_STACK_SIZE */

#ifndef LPKG_LVGL_THREAD_PRIO
#define LPKG_LVGL_THREAD_PRIO (RT_THREAD_PRIORITY_MAX*2/3)
#endif /* LPKG_LVGL_THREAD_PRIO */

#define SLEEP_PERIOD 1
extern void lv_port_disp_init(void);
extern void lv_port_indev_init(void);
extern void lv_user_gui_init(void);

static struct rt_thread lvgl_thread;
static ALIGN(8) rt_uint8_t lvgl_thread_stack[LPKG_LVGL_THREAD_STACK_SIZE];

#if LV_USE_LOG
static void lv_rt_log(const char *buf)
{
    LOG_I(buf);
}
#endif /* LV_USE_LOG */

#ifdef RT_USING_PM
struct rt_semaphore pm_sem;
void app_notify(rt_uint8_t event, rt_uint8_t mode, void *data)
{
    if (event == RT_PM_ENTER_SLEEP)
    {
        rt_sem_take(&pm_sem, RT_WAITING_FOREVER);
    }
    else if (event == RT_PM_EXIT_SLEEP)
    {
        rt_sem_release(&pm_sem);
    }
}
#endif

static void lvgl_thread_entry(void *parameter)
{
#if LV_USE_LOG
    lv_log_register_print_cb(lv_rt_log);
#endif /* LV_USE_LOG */
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_user_gui_init();
#ifdef RT_USING_PM
    rt_sem_init(&pm_sem, "pm_sem", 1, RT_IPC_FLAG_PRIO);
    rt_pm_notify_set(app_notify, NULL);
#endif
    /* handle the tasks of LVGL */
    while(1)
    {
#ifdef RT_USING_PM
        rt_sem_take(&pm_sem, RT_WAITING_FOREVER);
        rt_pm_module_request(PM_MAIN_ID, PM_SLEEP_MODE_NONE);
#endif
        lv_task_handler();
#ifdef RT_USING_PM
        rt_pm_module_release(PM_MAIN_ID, PM_SLEEP_MODE_NONE);
        rt_sem_release(&pm_sem);
#endif
        //rt_thread_mdelay(LV_DISP_DEF_REFR_PERIOD);
#ifndef AIC_LVGL_METER_DEMO
        rt_thread_mdelay(SLEEP_PERIOD);
#endif

#if defined(AIC_LVGL_METER_DEMO) && defined(RT_USING_PM)
        rt_uint32_t status = rt_pm_module_get_status();
        if (!(status & (1 << PM_POWER_ID)))
            rt_thread_mdelay(SLEEP_PERIOD);
#endif
    }
}

int lvgl_thread_init(void)
{
    rt_err_t err;

    err = rt_thread_init(&lvgl_thread, "LVGL", lvgl_thread_entry, RT_NULL,
           &lvgl_thread_stack[0], sizeof(lvgl_thread_stack), LPKG_LVGL_THREAD_PRIO, 0);
    if(err != RT_EOK)
    {
        LOG_E("Failed to create LVGL thread");
        return -1;
    }
    rt_thread_startup(&lvgl_thread);

    return 0;
}
//INIT_ENV_EXPORT(lvgl_thread_init);

#endif /*__RTTHREAD__*/
