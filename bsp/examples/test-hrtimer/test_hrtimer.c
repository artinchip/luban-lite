/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <stdlib.h>
#include <posix/string.h>
#include <getopt.h>
#include <rtthread.h>
#include <rtdevice.h>

#include "aic_core.h"
#include "drv_hrtimer.h"
#include "boot_param.h"

#ifndef USEC_PER_SEC
#define USEC_PER_SEC            1000000
#endif

#define HRTIMER_MAX_ELAPSE      (60 * USEC_PER_SEC) // 60 sec

static rt_device_t g_hrtimer_dev[AIC_HRTIMER_CH_NUM] = {RT_NULL};
static u32 g_debug = 0;
static u32 g_loop_max = 0;
static u32 g_loop_cnt = 0;
static ulong g_start_us = 0;

static const char sopts[] = "m:c:s:u:dh";
static const struct option lopts[] = {
    {"mode",        required_argument, NULL, 'm'},
    {"channel",     required_argument, NULL, 'c'},
    {"sec",         required_argument, NULL, 's'},
    {"microsecond", required_argument, NULL, 'u'},
    {"debug",             no_argument, NULL, 'v'},
    {"usage",             no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

static void usage(char *program)
{
    printf("Usage: %s [options]: \n", program);
    printf("\t -m, --mode\t\tmode of timer, oneshot/period\n");
    printf("\t -c, --channel\t\tthe number of hrtimer [0, 2] \n");
    printf("\t -s, --second\t\tthe second of timer (must > 0) \n");
    printf("\t -u, --microsecond\tthe microsecond of timer (must > 0) \n");
    printf("\t -d, --debug\t\tshow the timeout log\n");
    printf("\t -h, --usage \n");
    printf("\n");
    printf("Example: %s -m oneshot -c 0 -s 2 -u 3 \n", program);
}

/* Timer timeout callback function */
static rt_err_t hrtimer_cb(rt_device_t dev, rt_size_t size)
{
    struct hrtimer_info *info = (struct hrtimer_info *)dev->user_data;

#ifdef ULOG_USING_ISR_LOG
    if (g_debug)
        printf("%d/%d hrtimer%d timeout callback! Elapsed %ld us\n",
               g_loop_cnt, g_loop_max,
               info->id, aic_timer_get_us() - g_start_us);
#endif

    g_start_us = aic_timer_get_us();
    g_loop_cnt++;
    if ((g_loop_max > 1) && (g_loop_cnt > g_loop_max))
        rt_device_control(g_hrtimer_dev[info->id], HWTIMER_CTRL_STOP, NULL);

    return RT_EOK;
}

static void cmd_test_hrtimer(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    u32 c, ch = 0;
    rt_hwtimerval_t tm = {0};
    rt_hwtimer_mode_t mode = HWTIMER_MODE_ONESHOT;

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'm':
            if (strncasecmp("period", optarg, strlen(optarg)) == 0)
                mode = HWTIMER_MODE_PERIOD;
            continue;

        case 'c':
            ch = atoi(optarg);
            if (ch > AIC_HRTIMER_CH_NUM) {
                pr_err("Channel number %s is invalid\n", optarg);
                return;
            }
            continue;

        case 's':
            tm.sec = atoi(optarg);
            continue;

        case 'u':
            tm.usec = atoi(optarg);
            continue;

        case 'd':
            g_debug = 1;
            continue;

        case 'h':
            usage(argv[0]);
            return;

        default:
            pr_err("Invalid argument\n");
            usage(argv[0]);
            return;
        }
    }

    if ((tm.sec == 0) && (tm.usec == 0)) {
        pr_err("Invalid argument\n");
        usage(argv[0]);
        return;
    }

    if (!g_hrtimer_dev[ch]) {
        char name[10] = "";

        snprintf(name, 10, "hrtimer%d", ch);
        /* find timer device */
        g_hrtimer_dev[ch] = rt_device_find(name);
        if (g_hrtimer_dev[ch] == RT_NULL) {
            pr_err("Can't find %s device!\n", name);
            return;
        }

        /* Open the device in read-write mode */
        ret = rt_device_open(g_hrtimer_dev[ch], RT_DEVICE_OFLAG_RDWR);
        if (ret != RT_EOK) {
            pr_err("Failed to open %s device!\n", name);
            return;
        }
    }

    /* set timeout callback function */
    rt_device_set_rx_indicate(g_hrtimer_dev[ch], hrtimer_cb);

    ret = rt_device_control(g_hrtimer_dev[ch], HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK) {
        pr_err("Failed to set mode! ret is %d\n", ret);
        return;
    }

    printf("hrtimer%d: Create a timer of %d.%06d sec, %s mode\n",
           ch, (u32)tm.sec, (u32)tm.usec,
           mode == HWTIMER_MODE_ONESHOT ? "Oneshot" : "Period");
    if (mode != HWTIMER_MODE_ONESHOT) {
        g_loop_max = HRTIMER_MAX_ELAPSE / (tm.sec * USEC_PER_SEC + tm.usec);
        printf("\tWill loop %d times\n", g_loop_max);
    }
    g_loop_cnt = 0;
    g_start_us = aic_timer_get_us();
    if (!rt_device_write(g_hrtimer_dev[ch], 0, &tm, sizeof(tm))) {
        pr_err("set timeout value failed\n");
        return;
    }

    // rt_device_close(g_hrtimer_dev[ch]);
}
MSH_CMD_EXPORT_ALIAS(cmd_test_hrtimer, test_hrtimer, test hrtimer);
