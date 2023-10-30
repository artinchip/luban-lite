/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#include <stdlib.h>
#include <posix/string.h>
#include <getopt.h>
#include <rtthread.h>
#include <rtdevice.h>

#include "aic_core.h"
#include "drv_gptimer.h"
#include "boot_param.h"

#ifndef USEC_PER_SEC
#define USEC_PER_SEC            1000000
#endif

#define GPTIMER_MAX_ELAPSE      (60 * USEC_PER_SEC) // 60 sec

#define TIMER_NUM     AIC_GPTIMER_CH_NUM

static rt_device_t g_gptimer_dev[TIMER_NUM] = {RT_NULL};
static u32 g_debug = 0;
static u32 g_loop_max = 0;
static u32 g_loop_cnt = 0;
static ulong g_start_us = 0;

static const char sopts[] = "m:c:s:u:g:a:f:dh";
static const struct option lopts[] = {
    {"mode",        required_argument, NULL, 'm'},
    {"channel",     required_argument, NULL, 'c'},
    {"sec",         required_argument, NULL, 's'},
    {"microsecond", required_argument, NULL, 'u'},
    {"gptmode",     required_argument, NULL, 'g'},
    {"trgmode",     required_argument, NULL, 'a'},
    {"frequency",   required_argument, NULL, 'f'},
    {"debug",             no_argument, NULL, 'v'},
    {"usage",             no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

static void usage(char *program)
{
    printf("Usage: %s [options]: \n", program);
    printf("\t -m, --mode\t\tmode of timer, oneshot/period\n");
    printf("\t -c, --channel\t\tthe number of gptimer [0, 2]\n");
    printf("\t -s, --second\t\tthe second of timer (must > 0)\n");
    printf("\t -u, --microsecond\tthe microsecond of timer (must > 0)\n");
    printf("\t -g, --gptmode\t\the mode of gptimer, count/match\n");
    printf("\t -a, --triggermode\tthe trigger mode of gptimer, auto/rsi/fall/bil\n");
    printf("\t -f, --frequency\tthe frequncy of the gptimer (must > 0)\n");
    printf("\t -d, --debug\t\tshow the timeout log\n");
    printf("\t -h, --usage \n");
    printf("\n");
    printf("Example: %s -m period -c 0 -s 1 -u 3 -g count -a auto -f 1000 -d \n", program);
}

struct test_gptimer_para {
    rt_hwtimerval_t tm;
    struct gptimer_para gpt_para;
};

struct gptimer_match_out g_outval[GPT_OUT_NUMS] = {
    {1, OUT_INIT_LOW, CMP_OUT_HIGH, CMP_OUT_LOW},
    {1, OUT_INIT_HIGH, CMP_OUT_LOW, CMP_OUT_HIGH},
    {0, OUT_INIT_LOW, CMP_OUT_HIGH, CMP_OUT_LOW},
    {0, OUT_INIT_LOW, CMP_OUT_HIGH, CMP_OUT_LOW},
};

enum gpt_cmp_act g_cmpa_act = GPTIMER_CNT_CONTINUE;
enum gpt_cmp_act g_cmpb_act = GPTIMER_CNT_CONTINUE;

/* Timer timeout callback function */
static rt_err_t gptimer_cb(rt_device_t dev, rt_size_t size)
{
    struct gptimer_info *info = (struct gptimer_info *)dev->user_data;

#ifdef ULOG_USING_ISR_LOG
    if (g_debug)
        printf("%d/%d gptimer%d timeout callback! Elapsed %ld us\n",
               g_loop_cnt, g_loop_max,
               info->id, aic_timer_get_us() - g_start_us);
#endif

    g_start_us = aic_timer_get_us();
    g_loop_cnt++;
    if ((g_loop_max > 1) && (g_loop_cnt > g_loop_max))
        rt_device_control(g_gptimer_dev[info->id], HWTIMER_CTRL_STOP, NULL);

    return RT_EOK;
}

static void cmd_test_gptimer(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    u32 c, ch = 0;
    struct test_gptimer_para para = {0};
    rt_hwtimer_mode_t mode = HWTIMER_MODE_ONESHOT;
    enum gptimer_mode gpt_mode = GPTIMER_MODE_COUNT;
    enum gpt_trg_mode trg_mode = GPT_TRG_MODE_AUTO;
    u32 freq= 1000000;

    optind = 0;
    g_debug = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'm':
            if (strncasecmp("period", optarg, strlen(optarg)) == 0)
                mode = HWTIMER_MODE_PERIOD;
            continue;

        case 'c':
            ch = atoi(optarg);
            if (ch > TIMER_NUM) {
                pr_err("Channel number %s is invalid\n", optarg);
                return;
            }
            continue;

        case 's':
            para.tm.sec = atoi(optarg);
            continue;

        case 'u':
            para.tm.usec = atoi(optarg);
            continue;

        case 'd':
            g_debug = 1;
            continue;

        case 'g':
            if (strncasecmp("count", optarg, strlen(optarg)) == 0)
                gpt_mode = GPTIMER_MODE_COUNT;
            else if (strncasecmp("match", optarg, strlen(optarg)) == 0)
                gpt_mode = GPTIMER_MODE_MATCH;
            continue;

        case 'a':
            if (strncasecmp("auto", optarg, strlen(optarg)) == 0)
                trg_mode = GPT_TRG_MODE_AUTO;
            else if (strncasecmp("rsi", optarg, strlen(optarg)) == 0)
                trg_mode = GPT_TRG_MODE_RSI;
            else if (strncasecmp("fall", optarg, strlen(optarg)) == 0)
                trg_mode = GPT_TRG_MODE_FALL;
            else if (strncasecmp("bil", optarg, strlen(optarg)) == 0)
                trg_mode = GPT_TRG_MODE_BILATERAL;
            continue;

        case 'f':
            freq = atoi(optarg);
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

    if ((para.tm.sec == 0) && (para.tm.usec == 0)) {
        pr_err("Invalid argument\n");
        usage(argv[0]);
        return;
    }

    if (!g_gptimer_dev[ch]) {
        char name[10] = "";

        snprintf(name, 10, "gptimer%d", ch);

        /* find timer device */
        g_gptimer_dev[ch] = rt_device_find(name);
        if (g_gptimer_dev[ch] == RT_NULL) {
            pr_err("Can't find %s device!\n", name);
            return;
        }

        /* Open the device in read-write mode */
        ret = rt_device_open(g_gptimer_dev[ch], RT_DEVICE_OFLAG_RDWR);
        if (ret != RT_EOK) {
            pr_err("Failed to open %s device!\n", name);
            return;
        }
    }

    para.gpt_para.gptimer_mode = gpt_mode;
    para.gpt_para.gptimer_trgmode = trg_mode;

    if (gpt_mode == GPTIMER_MODE_MATCH) {
        para.gpt_para.matchval.cmpa_act = g_cmpa_act;
        para.gpt_para.matchval.cmpb_act = g_cmpb_act;
        memcpy(&para.gpt_para.matchval.outval[0], g_outval, sizeof(struct gptimer_match_out) * 4);
    }

    /* set timeout callback function */
    rt_device_set_rx_indicate(g_gptimer_dev[ch], gptimer_cb);

    /* set the timer mode, oneshot or period */
    ret = rt_device_control(g_gptimer_dev[ch], HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK) {
        pr_err("Failed to set mode! ret is %d\n", ret);
        return;
    }

    /* set the timer frequency to freqHz */
    ret = rt_device_control(g_gptimer_dev[ch], HWTIMER_CTRL_FREQ_SET, &freq);
    if (ret != RT_EOK) {
        pr_err("Failed to set the freq! ret is %d\n", ret);
        return;
    }

    printf("gptimer%d: Create a timer of %d.%06d sec, %s mode\n",
           ch, (u32)para.tm.sec, (u32)para.tm.usec,
           mode == HWTIMER_MODE_ONESHOT ? "Oneshot" : "Period");
    if (mode != HWTIMER_MODE_ONESHOT) {
        g_loop_max = GPTIMER_MAX_ELAPSE / (para.tm.sec * USEC_PER_SEC + para.tm.usec);
        printf("\tWill loop %d times\n", g_loop_max);
    }

    g_loop_cnt = 0;
    g_start_us = aic_timer_get_us();
    if (!rt_device_write(g_gptimer_dev[ch], 0, &para, sizeof(struct test_gptimer_para))) {
        pr_err("set timeout value failed\n");
        return;
    }

    // rt_device_close(g_gptimer_dev[ch]);
}
MSH_CMD_EXPORT_ALIAS(cmd_test_gptimer, test_gptimer, test gptimer);
