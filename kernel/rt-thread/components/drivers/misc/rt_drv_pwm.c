/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-07     aozima       the first version
 * 2022-05-14     Stanley Lwin add pwm function
 * 2022-07-25     liYony       fix complementary outputs and add usage information in finsh
 */

#include <rtdevice.h>

static rt_err_t _pwm_control(rt_device_t dev, int cmd, void *args)
{
    rt_err_t result = RT_EOK;
    struct rt_device_pwm *pwm = (struct rt_device_pwm *)dev;

    if (pwm->ops->control)
    {
        result = pwm->ops->control(pwm, cmd, args);
    }

    return result;
}


/*
pos: channel
void *buffer: rt_uint32_t pulse[size]
size : number of pulse, only set to sizeof(rt_uint32_t).
*/
static rt_size_t _pwm_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    rt_err_t result = RT_EOK;
    struct rt_device_pwm *pwm = (struct rt_device_pwm *)dev;
    rt_uint32_t *pulse = (rt_uint32_t *)buffer;
    struct rt_pwm_configuration configuration = {0};

    configuration.channel = (pos > 0) ? (pos) : (-pos);

    if (pwm->ops->control)
    {
        result = pwm->ops->control(pwm, PWM_CMD_GET,  &configuration);
        if (result != RT_EOK)
        {
            return 0;
        }

        *pulse = configuration.pulse;
    }

    return size;
}

/*
pos: channel
void *buffer: rt_uint32_t pulse[size]
size : number of pulse, only set to sizeof(rt_uint32_t).
*/
static rt_size_t _pwm_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    rt_err_t result = RT_EOK;
    struct rt_device_pwm *pwm = (struct rt_device_pwm *)dev;
    rt_uint32_t *pulse = (rt_uint32_t *)buffer;
    struct rt_pwm_configuration configuration = {0};

    configuration.channel = (pos > 0) ? (pos) : (-pos);

    if (pwm->ops->control)
    {
        result = pwm->ops->control(pwm, PWM_CMD_GET, &configuration);
        if (result != RT_EOK)
        {
            return 0;
        }

        configuration.pulse = *pulse;

        result = pwm->ops->control(pwm, PWM_CMD_SET, &configuration);
        if (result != RT_EOK)
        {
            return 0;
        }
    }

    return size;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops pwm_device_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    _pwm_read,
    _pwm_write,
    _pwm_control
};
#endif /* RT_USING_DEVICE_OPS */

rt_err_t rt_device_pwm_register(struct rt_device_pwm *device, const char *name, const struct rt_pwm_ops *ops, const void *user_data)
{
    rt_err_t result = RT_EOK;

    rt_memset(device, 0, sizeof(struct rt_device_pwm));

#ifdef RT_USING_DEVICE_OPS
    device->parent.ops = &pwm_device_ops;
#else
    device->parent.init = RT_NULL;
    device->parent.open = RT_NULL;
    device->parent.close = RT_NULL;
    device->parent.read  = _pwm_read;
    device->parent.write = _pwm_write;
    device->parent.control = _pwm_control;
#endif /* RT_USING_DEVICE_OPS */

    device->parent.type         = RT_Device_Class_PWM;
    device->ops                 = ops;
    device->parent.user_data    = (void *)user_data;

    result = rt_device_register(&device->parent, name, RT_DEVICE_FLAG_RDWR);

    return result;
}

rt_err_t rt_pwm_enable(struct rt_device_pwm *device, int channel)
{
    rt_err_t result = RT_EOK;
    struct rt_pwm_configuration configuration = {0};

    if (!device)
    {
        return -RT_EIO;
    }

    configuration.channel = (channel > 0) ? (channel) : (-channel);         /* Make it is positive num forever */
    configuration.complementary = (channel > 0) ? (RT_FALSE) : (RT_TRUE);   /* If nagetive, it's complementary */
    result = rt_device_control(&device->parent, PWM_CMD_ENABLE, &configuration);

    return result;
}

rt_err_t rt_pwm_disable(struct rt_device_pwm *device, int channel)
{
    rt_err_t result = RT_EOK;
    struct rt_pwm_configuration configuration = {0};

    if (!device)
    {
        return -RT_EIO;
    }

    configuration.channel = (channel > 0) ? (channel) : (-channel);         /* Make it is positive num forever */
    configuration.complementary = (channel > 0) ? (RT_FALSE) : (RT_TRUE);   /* If nagetive, it's complementary */
    result = rt_device_control(&device->parent, PWM_CMD_DISABLE, &configuration);

    return result;
}

#ifdef AIC_XPWM_DRV
rt_err_t rt_pwm_set(struct rt_device_pwm *device, int channel, rt_uint32_t period, rt_uint32_t pulse, rt_uint32_t pulse_cnt)
#else
rt_err_t rt_pwm_set(struct rt_device_pwm *device, int channel, rt_uint32_t period, rt_uint32_t pulse)
#endif
{
    rt_err_t result = RT_EOK;
    struct rt_pwm_configuration configuration = {0};

    if (!device)
    {
        return -RT_EIO;
    }

    configuration.channel = (channel > 0) ? (channel) : (-channel);
    configuration.period = period;
    configuration.pulse = pulse;
#ifdef AIC_XPWM_DRV
    configuration.pulse_cnt = pulse_cnt;
#endif
    result = rt_device_control(&device->parent, PWM_CMD_SET, &configuration);

    return result;
}

#ifdef AIC_XPWM_DRV
rt_err_t rt_pwm_set_fifo_num(struct rt_device_pwm *device, int channel, rt_uint32_t fifo_num)
{
    rt_err_t result = RT_EOK;
    struct rt_pwm_configuration configuration = {0};

    if (!device)
    {
        return -RT_EIO;
    }

    configuration.channel = (channel > 0) ? (channel) : (-channel);
    configuration.fifo_num = fifo_num;
    result = rt_device_control(&device->parent, PWM_CMD_SET_FIFO_NUM, &configuration);

    return result;
}

rt_err_t rt_pwm_set_fifo(struct rt_device_pwm *device, int channel, rt_uint32_t fifo_index, rt_uint32_t period, rt_uint32_t pulse, rt_uint32_t pulse_cnt)
{
    rt_err_t result = RT_EOK;
    struct rt_pwm_configuration configuration = {0};

    if (!device)
    {
        return -RT_EIO;
    }

    configuration.channel = (channel > 0) ? (channel) : (-channel);
    configuration.fifo_index = fifo_index;
    configuration.pul_prd = period;
    configuration.pul_cmp = pulse;
    configuration.pul_num = pulse_cnt;
    result = rt_device_control(&device->parent, PWM_CMD_SET_FIFO, &configuration);

    return result;
}

rt_err_t rt_pwm_get_fifo(struct rt_device_pwm *device, int channel)
{
    rt_err_t result = RT_EOK;
    struct rt_pwm_configuration configuration = {0};

    if (!device)
    {
        return -RT_EIO;
    }

    configuration.channel = (channel > 0) ? (channel) : (-channel);
    result = rt_device_control(&device->parent, PWM_CMD_GET_FIFO, &configuration);

    return result;
}
#endif

rt_err_t rt_pwm_set_period(struct rt_device_pwm *device, int channel, rt_uint32_t period)
{
    rt_err_t result = RT_EOK;
    struct rt_pwm_configuration configuration = {0};

    if (!device)
    {
        return -RT_EIO;
    }

    configuration.channel = (channel > 0) ? (channel) : (-channel);
    configuration.period = period;
    result = rt_device_control(&device->parent, PWM_CMD_SET_PERIOD, &configuration);

    return result;
}

rt_err_t rt_pwm_set_pulse(struct rt_device_pwm *device, int channel, rt_uint32_t pulse)
{
    rt_err_t result = RT_EOK;
    struct rt_pwm_configuration configuration = {0};

    if (!device)
    {
        return -RT_EIO;
    }

    configuration.channel = (channel > 0) ? (channel) : (-channel);
    configuration.pulse = pulse;
    result = rt_device_control(&device->parent, PWM_CMD_SET_PULSE, &configuration);

    return result;
}

rt_err_t rt_pwm_get(struct rt_device_pwm *device, struct rt_pwm_configuration *cfg)
{
    rt_err_t result = RT_EOK;

    if (!device)
    {
        return -RT_EIO;
    }

    result = rt_device_control(&device->parent, PWM_CMD_GET, cfg);

    return result;
}

#ifdef RT_USING_FINSH
#include <stdlib.h>
#include <string.h>
#include <finsh.h>

static int pwm(int argc, char **argv)
{
    rt_err_t result = -RT_ERROR;
    char *result_str;
    static struct rt_device_pwm *pwm_device = RT_NULL;
    struct rt_pwm_configuration cfg = {0};

    if(argc > 1)
    {
        if(!strcmp(argv[1], "probe"))
        {
            if(argc == 3)
            {
                pwm_device = (struct rt_device_pwm *)rt_device_find(argv[2]);
                result_str = (pwm_device == RT_NULL) ? "failure" : "success";
                rt_kprintf("probe %s %s\n", argv[2], result_str);
            }
            else
            {
                rt_kprintf("pwm probe <device name>                  - probe pwm by name\n");
            }
        }
        else
        {
            if(pwm_device == RT_NULL)
            {
                rt_kprintf("Please using 'pwm probe <device name>' first.\n");
                return -RT_ERROR;
            }
            if(!strcmp(argv[1], "enable"))
            {
                if(argc == 3)
                {
                    result = rt_pwm_enable(pwm_device, atoi(argv[2]));
                    result_str = (result == RT_EOK) ? "success" : "failure";
                    rt_kprintf("%s channel %d is enabled %s \n", pwm_device->parent.parent.name, atoi(argv[2]), result_str);
                }
                else
                {
                    rt_kprintf("pwm enable <channel>                     - enable pwm channel\n");
                    rt_kprintf("    e.g. MSH >pwm enable  1              - PWM_CH1  nomal\n");
                    rt_kprintf("    e.g. MSH >pwm enable -1              - PWM_CH1N complememtary\n");
                }
            }
            else if(!strcmp(argv[1], "disable"))
            {
                if(argc == 3)
                {
                    result = rt_pwm_disable(pwm_device, atoi(argv[2]));
                }
                else
                {
                    rt_kprintf("pwm disable <channel>                    - disable pwm channel\n");
                }
            }
            else if(!strcmp(argv[1], "get"))
            {
                cfg.channel = atoi(argv[2]);
                result = rt_pwm_get(pwm_device, &cfg);
                if(result == RT_EOK)
                {
                    rt_kprintf("Info of device [%s] channel [%d]:\n",pwm_device, atoi(argv[2]));
                    rt_kprintf("period      : %d\n", cfg.period);
                    rt_kprintf("pulse       : %d\n", cfg.pulse);
                    rt_kprintf("Duty cycle  : %d%%\n",(int)(((double)(cfg.pulse)/(cfg.period)) * 100));
                }
                else
                {
                    rt_kprintf("Get info of device: [%s] error.\n", pwm_device);
                }
            }
            else if (!strcmp(argv[1], "set"))
            {
#ifdef AIC_XPWM_DRV
                if(argc == 6)
                {
                    result = rt_pwm_set(pwm_device, atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
                    rt_kprintf("pwm info set on %s at channel %d\n",pwm_device,atoi(argv[2]));
                }
#else
                if(argc == 5)
                {
                    result = rt_pwm_set(pwm_device, atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
                    rt_kprintf("pwm info set on %s at channel %d\n",pwm_device,atoi(argv[2]));
                }
#endif
                else
                {
                    rt_kprintf("Set info of device: [%s] error\n", pwm_device);
#ifdef AIC_XPWM_DRV
                    rt_kprintf("Usage: pwm set <channel> <period> <pulse> <pulse cnt>\n");
#else
                    rt_kprintf("Usage: pwm set <channel> <period> <pulse>\n");
#endif
                }
            }
#ifdef AIC_XPWM_DRV
            else if (!strcmp(argv[1], "set_fifo_num"))
            {
                if (argc == 4)
                {
                    result = rt_pwm_set_fifo_num(pwm_device, atoi(argv[2]), atoi(argv[3]));
                    rt_kprintf("pwm set fifo num on %s at channel %d\n",pwm_device,atoi(argv[2]));
                }
                else
                {
                    rt_kprintf("Set fifo num of device: [%s] error\n", pwm_device);
                    rt_kprintf("Usage: pwm set_fifo_num <channel> <fifo_num>\n");
                }
            }
            else if (!strcmp(argv[1], "set_fifo"))
            {
                if (argc == 7)
                {
                    result = rt_pwm_set_fifo(pwm_device, atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
                    rt_kprintf("pwm set fifo on %s at channel %d\n",pwm_device,atoi(argv[2]));
                }
                else
                {
                    rt_kprintf("Set fifo of device: [%s] error\n", pwm_device);
                    rt_kprintf("Usage: pwm set_fifo <channel> <fifo_index> <period> <pulse> <pulse cnt>\n");
                }
            }
            else if (!strcmp(argv[1], "get_fifo"))
            {
                if (argc == 3)
                {
                    result = rt_pwm_get_fifo(pwm_device, atoi(argv[2]));
                }
                else
                {
                    rt_kprintf("get fifo info of device: [%s] error\n", pwm_device);
                    rt_kprintf("Usage: pwm get_fifo <channel>\n");
                }
            }
#endif
            else
            {
                rt_kprintf("pwm get <channel>                        - get pwm channel info\n");
            }
        }
    }
    else
    {
        rt_kprintf("Usage: \n");
        rt_kprintf("pwm probe   <device name>                - probe pwm by name\n");
        rt_kprintf("pwm enable  <channel>                    - enable pwm channel\n");
        rt_kprintf("pwm disable <channel>                    - disable pwm channel\n");
        rt_kprintf("pwm get     <channel>                    - get pwm channel info\n");
#ifdef AIC_XPWM_DRV
        rt_kprintf("pwm set          <channel> <period> <pulse> <pulse cnt>               - set pwm channel info\n");
        rt_kprintf("pwm set_fifo_num <channel> <fifo_num>                                 - set xpwm fifo count\n");
        rt_kprintf("pwm set_fifo     <channel> <fifo_index> <period> <pulse> <pulse cnt>  - set xpwm fifo info\n");
        rt_kprintf("pwm get_fifo      <channel>                                           - get xpwm fifo info\n");
#else
        rt_kprintf("pwm set     <channel> <period> <pulse>   - set pwm channel info\n");
#endif
        result = - RT_ERROR;
    }

    return RT_EOK;
}
MSH_CMD_EXPORT(pwm, pwm [option]);

#endif /* RT_USING_FINSH */
