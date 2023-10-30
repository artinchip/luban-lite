/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <aic_core.h>
#include <drivers/watchdog.h>
#include <aic_drv_wdt.h>
#include <hal_wdt.h>
#include <getopt.h>


irqreturn_t aic_wdt_irq(int irq, void *arg)
{
    rt_kprintf("Watchdog chan0 IRQ happened\n");

    return IRQ_HANDLED;
}

static void idle_hook(void)
{
    rt_device_t wdt_dev = RT_NULL;
    wdt_dev = rt_device_find("wdt");
    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
}

static void usage(char * program)
{
    printf("\n");
    printf("Usage: %s [-s timeout] [-p pretimeout] [-c clear threshold] [-g] [-k] [-u]\n",\
        program);
    printf("\t -s, --set-timeout\tSet a timeout, in second\n");
    printf("\t -p, --set-pretimeout\tSet a pretimeout, in second\n");
    printf("\t -c, --set-clear threshold\tSet clear threshold,in second(0~3)\n");
    printf("\t -g, --get-timeout\tGet the current timeout, in second\n");
    printf("\t -k, --keepalive\tKeepalive the watchdog\n");
    #ifdef AIC_WDT_DRV_V11
    printf("\t -r, --change reset object \t change reset cpu or system\n");
    #endif
    printf("\t -u, --usage \n");
    printf("\n");
}

void test_wdt(int argc, char **argv)
{
    int opt;
    int status;
    int timeout = 0;
    rt_device_t wdt_dev = RT_NULL;

    wdt_dev =  rt_device_find("wdt");
    rt_device_init(wdt_dev);

    optind = 0;
    while ((opt = getopt(argc, argv, "s:p:c:gkru")) != -1) {
        switch (opt) {
            case 'c':
                timeout = strtoul(optarg, NULL, 10);
                rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_CLR_THD, &timeout);
                rt_kprintf("set clear threshold:%d\n", timeout);
                break;
            case 's':
                timeout = strtoul(optarg, NULL, 10);
                rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
                rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
                rt_kprintf("set timeout:%d\n", timeout);
                break;
            case 'g':
                rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_GET_TIMEOUT, &timeout);
                rt_kprintf("timeout:%d\n", timeout);
                break;
            case 'p':
                timeout = strtoul(optarg, NULL, 10);
                rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_IRQ_TIMEOUT, &timeout);
                rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_IRQ_ENABLE, &aic_wdt_irq);
                rt_kprintf("set pretimeout:%d\n", timeout);
                break;
            case 'k':
                rt_thread_idle_sethook(idle_hook);
                rt_kprintf("feed the dog! \n");
                break;
            case 'r':
            #ifdef AIC_WDT_DRV_V11
                status = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_GET_RST_EN, RT_NULL);
                if (status)
                    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_RST_SYS, RT_NULL);
                else
                    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_RST_CPU, RT_NULL);
                break;
            #endif
            case 'u':
            default:
                usage(argv[0]);
        }
    }
}
MSH_CMD_EXPORT_ALIAS(test_wdt, test_wdt, Reboot the system);
