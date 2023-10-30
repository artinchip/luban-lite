/*
 * Copyright (c) 2006-2020, Sunwancn(bwsheng2000@163.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-02-14     Sunwancn     the first version
 */

#include <rtdevice.h>
#include <rtthread.h>
#include <beep.h>

#ifdef LPKG_USING_BEEP

#if !defined(LPKG_BEEP_THREAD_STACK_USING_HEAP)
ALIGN(RT_ALIGN_SIZE)
static char beep_thread_stack[LPKG_BEEP_THREAD_STACK_SIZE];
static struct rt_thread beep_thread_struct = {0};
#endif /* !defined(LPKG_BEEP_THREAD_STACK_USING_HEAP) */

static struct beep_struct beep_data = {0};
static rt_thread_t beep_thread = RT_NULL;

#ifdef LPKG_BEEP_PASSIVE_BUZZER
static void beep_set(void);
#endif
static void beep_on(void);
static void beep_off(void);
static void beep_thread_entry(void *parameter);

#if defined(LPKG_BEEP_PASSIVE_BUZZER) && defined(LPKG_BEEP_SUPPORT_PM_RUN_FREQ_CHANGE)
int pm_run_freq_change(const struct rt_device *device, rt_uint8_t mode)
{
    if (&beep_data.pwm_dev->parent == device)
    {
        beep_set();
    }
    return RT_EOK;
}

static struct rt_device_pm_ops beep_pm_ops =
{
    RT_NULL,
    RT_NULL,
    pm_run_freq_change
};
#endif /* defined(LPKG_BEEP_PASSIVE_BUZZER) && defined(LPKG_BEEP_SUPPORT_PM_RUN_FREQ_CHANGE) */

/***************************************************************************************************
 * @fn      beep_init
 *
 * @brief   初始化
 *
 * @param   pin         - 蜂鸣器驱动引脚，可查看drv_gpio.c，或使用GET_PIN()宏
 *          reset_level - 蜂鸣器关断时引脚电平，PIN_LOW或PIN_HIGH
 *
 ***************************************************************************************************/
void beep_init(rt_base_t pin, rt_base_t reset_level)
{
#ifdef LPKG_BEEP_PASSIVE_BUZZER
    (void)pin;
    (void)reset_level;

    beep_data.pwm_dev = (struct rt_device_pwm *)rt_device_find(LPKG_BEEP_PWM_DEV_NAME);
    RT_ASSERT(beep_data.pwm_dev);

    beep_data.freq = LPKG_BEEP_DEFAULT_FREQ;

#ifdef LPKG_BEEP_SUPPORT_PM_RUN_FREQ_CHANGE
    rt_pm_device_register(&beep_data.pwm_dev->parent, &beep_pm_ops);
#endif /* LPKG_BEEP_SUPPORT_PM_RUN_FREQ_CHANGE */

    beep_set();
#else /* LPKG_BEEP_ACTIVE_BUZZER */
    rt_pin_write(pin, reset_level);
    rt_pin_mode(pin, PIN_MODE_OUTPUT);

    beep_data.pin = pin;
    beep_data.pin_reset_level = reset_level;
#endif /* LPKG_BEEP_PASSIVE_BUZZER */

    beep_data.inited = RT_TRUE;
}

int passive_beep_init(void)
{
    beep_init(0, 0);
    return 0;
}

INIT_COMPONENT_EXPORT(passive_beep_init);

/***************************************************************************************************
 * @fn      beep_deinit
 *
 * @brief   去初始化
 *
 ***************************************************************************************************/
void beep_deinit(void)
{
#ifdef LPKG_BEEP_PASSIVE_BUZZER
#ifdef LPKG_BEEP_SUPPORT_PM_RUN_FREQ_CHANGE
    rt_pm_device_unregister(&beep_data.pwm_dev->parent);
#endif /* LPKG_BEEP_SUPPORT_PM_RUN_FREQ_CHANGE */

    beep_data.pwm_dev = RT_NULL;
#endif /* LPKG_BEEP_PASSIVE_BUZZER */

    beep_data.inited = RT_FALSE;
}

#ifdef LPKG_BEEP_PASSIVE_BUZZER
void beep_set(void)
{
    RT_ASSERT(beep_data.pwm_dev);

    // 频率(Hz)转换为相应的周期(ns)
    rt_uint32_t period = 1000000000ULL / beep_data.freq;

    // PWM设置恒为50%占空比方波
    rt_pwm_set(beep_data.pwm_dev, LPKG_BEEP_PWM_DEV_CHANNEL, period, period >> 1);
}
#endif /* LPKG_BEEP_PASSIVE_BUZZER */

/***************************************************************************************************
 * @fn      beep
 *
 * @brief   蜂鸣器闪鸣，由于使用了信号量，请谨慎在中断中使用
 *
 * @param   nums        - 闪鸣次数
 *          period      - 蜂鸣器闪鸣的周期，以毫秒为单位，[10-100000]
 *          percent     - 蜂鸣器闪鸣的脉冲宽度百分比，[1-100]
 *          freq        - 蜂鸣器鸣叫频率，0：维持上次的频率，[0, 500-10000]
 *
 * @return  None
 ***************************************************************************************************/
void beep(rt_uint32_t nums, rt_uint32_t period, rt_uint32_t percent, rt_uint32_t freq)
{
#ifdef LPKG_BEEP_PASSIVE_BUZZER
    RT_ASSERT(beep_data.pwm_dev);
    RT_ASSERT(freq == 0 || (freq >= LPKG_BEEP_FREQ_MIN && freq <= LPKG_BEEP_FREQ_MAX));
#endif
    RT_ASSERT(period >= LPKG_BEEP_PERIOD_MIN && period <= LPKG_BEEP_PERIOD_MAX);
    RT_ASSERT(percent > 0 && percent <= 100);

    if (nums)
    {
        beep_data.nums = nums;
        beep_data.pulse = period * percent / 100;
        beep_data.npulse = period - beep_data.pulse;

#ifdef LPKG_BEEP_PASSIVE_BUZZER
        if (freq && beep_data.freq != freq)
        {
            beep_data.freq = freq;
            beep_set();
        }
#endif /* LPKG_BEEP_PASSIVE_BUZZER */

        if (beep_thread == RT_NULL || beep_thread->stat == RT_THREAD_CLOSE)
        {
#ifdef LPKG_BEEP_THREAD_STACK_USING_HEAP
            beep_thread = rt_thread_create("beep", beep_thread_entry, RT_NULL,
                                           LPKG_BEEP_THREAD_STACK_SIZE, LPKG_BEEP_THREAD_PRIORITY, LPKG_BEEP_THREAD_TIMESLICE);
            if (beep_thread != RT_NULL)
                rt_thread_startup(beep_thread);
#else
            beep_thread = &beep_thread_struct;
            rt_thread_init(beep_thread, "beep", beep_thread_entry, RT_NULL,
                           &beep_thread_stack, sizeof(beep_thread_stack), LPKG_BEEP_THREAD_PRIORITY, LPKG_BEEP_THREAD_TIMESLICE);
            rt_thread_startup(beep_thread);
#endif /* LPKG_BEEP_THREAD_STACK_USING_HEAP */
        }
    }
}

/***************************************************************************************************
 * @fn      beep_stop
 *
 * @brief   关闭蜂鸣器，停止发声
 *
 ***************************************************************************************************/
void beep_stop(void)
{
    beep_off();

    if (beep_thread != RT_NULL)
#ifdef LPKG_BEEP_THREAD_STACK_USING_HEAP
        rt_thread_delete(beep_thread);
#else
        rt_thread_detach(beep_thread);
#endif /* LPKG_BEEP_THREAD_STACK_USING_HEAP */

    beep_thread = RT_NULL;
}

void beep_on(void)
{
#ifdef LPKG_BEEP_PASSIVE_BUZZER
    RT_ASSERT(beep_data.pwm_dev);

    rt_pwm_enable(beep_data.pwm_dev, LPKG_BEEP_PWM_DEV_CHANNEL);
#else
    rt_pin_write(beep_data.pin, (beep_data.pin_reset_level == PIN_HIGH) ? PIN_LOW : PIN_HIGH);
#endif
}

void beep_off(void)
{
#ifdef LPKG_BEEP_PASSIVE_BUZZER
    RT_ASSERT(beep_data.pwm_dev);

    rt_pwm_disable(beep_data.pwm_dev, LPKG_BEEP_PWM_DEV_CHANNEL);
#else
    rt_pin_write(beep_data.pin, beep_data.pin_reset_level);
#endif
}

void beep_thread_entry(void *parameter)
{
    while ((beep_data.nums --) && beep_data.pulse)
    {
#if defined(RT_USING_PM) && defined(LPKG_BEEP_PASSIVE_BUZZER) && defined(LPKG_BEEP_BLOCK_POWER_STOP)
        rt_pm_request(LPKG_BEEP_REQUEST_PM_MODE);
#endif
        beep_on();
        rt_thread_mdelay(beep_data.pulse);

#if defined(RT_USING_PM) && defined(LPKG_BEEP_PASSIVE_BUZZER) && defined(LPKG_BEEP_BLOCK_POWER_STOP)
        rt_pm_release(LPKG_BEEP_REQUEST_PM_MODE);
#endif
        beep_off();
        if (beep_data.npulse)
        {
            rt_thread_mdelay(beep_data.npulse);
        }
    }
    beep_thread = RT_NULL;
}

#if defined(RT_USING_FINSH) && defined(LPKG_BEEP_USING_MSH_CMD)
#include <stdlib.h>

static void __beep(rt_uint8_t argc, char **argv)
{
    int nums, period, prcent, freq;

    if (!beep_data.inited)
    {
        rt_kprintf("Not initialized! Must be initialize first.\n");
    }
    else if (argc <= 2)
    {
        rt_kprintf("Please input: beep <nums> <period> [prcent] [freq]\n");
    }
    else if (argc == 3)
    {
        nums = atoi(argv[1]);
        period = atoi(argv[2]);
        if (nums > 0 && period >= LPKG_BEEP_PERIOD_MIN && period <= LPKG_BEEP_PERIOD_MAX)
            beep(nums, period, 50, 0);
        else
            rt_kprintf("Out of range! Must be at: nums[1-any] period[%d-%d]\n", LPKG_BEEP_PERIOD_MIN, LPKG_BEEP_PERIOD_MAX);
    }
    else if (argc == 4)
    {
        nums = atoi(argv[1]);
        period = atoi(argv[2]);
        prcent = atoi(argv[3]);
        if (nums > 0 && period >= LPKG_BEEP_PERIOD_MIN && period <= LPKG_BEEP_PERIOD_MAX
                && prcent > 0 && prcent <= 100)
            beep(nums, period, prcent, 0);
        else
            rt_kprintf("Out of range! Must be at: nums[1-any] period[%d-%d] prcent[1-100]\n", LPKG_BEEP_PERIOD_MIN, LPKG_BEEP_PERIOD_MAX);
    }
    else
    {
        nums = atoi(argv[1]);
        period = atoi(argv[2]);
        prcent = atoi(argv[3]);
        freq = atoi(argv[4]);
        if (nums > 0 && period >= LPKG_BEEP_PERIOD_MIN && period <= LPKG_BEEP_PERIOD_MAX
                && prcent > 0 && prcent <= 100 && (freq == 0 || (freq >= LPKG_BEEP_FREQ_MIN && freq <= LPKG_BEEP_FREQ_MAX)))
            beep(nums, period, prcent, freq);
        else
            rt_kprintf("Out of range! Must be at: nums[1-any] period[%d-%d] prcent[1-100] freq[0,%d-%d]\n", \
                       LPKG_BEEP_PERIOD_MIN, LPKG_BEEP_PERIOD_MAX, LPKG_BEEP_FREQ_MIN, LPKG_BEEP_FREQ_MAX);
    }
}
MSH_CMD_EXPORT_ALIAS(__beep, beep, Buzzer beep any);
#endif /* defined(RT_USING_FINSH) && defined(LPKG_BEEP_USING_MSH_CMD) */

#endif /* LPKG_USING_BEEP */
