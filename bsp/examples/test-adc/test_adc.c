/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao.Li <siyao.li@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <rtthread.h>
#include <time.h>
#include "rtdevice.h"
#include "aic_core.h"
#include "aic_log.h"
#include "hal_adcim.h"

#include "aic_core.h"

/* Global macro and variables */
#define AIC_ADCIM_NAME              "adcim"
#define AIC_GPAI_NAME               "gpai"

#ifdef AIC_ADCIM_DM_DRV
#define ADC_CHAN_GPAI11             11
#define ADC_CHAN_TSEN3              15
#endif

#define ADC_CHAN_NUM                16
#define ADC_DM_SRAM_SIZE            512
#define ADC_TEST_DATA_COUNT         64

static rt_adc_device_t gpai_dev;
static const char sopts[] = "c:h";
static const struct option lopts[] = {
    {"channel", required_argument, NULL, 'c'},
    {"usage",   no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

static int g_sram_data[ADC_DM_SRAM_SIZE] = {0};
static int g_expect_data[ADC_DM_SRAM_SIZE] = {0};
static int g_cur_data[ADC_DM_SRAM_SIZE] = {0};
static int g_verbose = 0;

static void cmd_adcim_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -c, --channel\t\tSelect one channel in [0, 15]\n");
    printf("\t -h, --usage \n");
    printf("\n");
    printf("Example: %s -c 0\n", program);
}

int abs(int val)
{
    if (val < 0)
        return -val;
    else
        return val;
}

static void gen_adc_data(u32 *size)
{
    int i;
    int *pdata = g_sram_data;

    if (g_verbose)
        pr_info("Generate %d data ...\n", ADC_DM_SRAM_SIZE);
    for (i = 0; i < ADC_DM_SRAM_SIZE; i++)
        *pdata++ = 1800 + abs(i % 256 - 128) - 64;

    *size = ADC_DM_SRAM_SIZE;
    if (g_verbose)
        pr_info("The ADC data, size %d:\n", *size);
}

int average(int *data, u32 size, int trim)
{
    int sum = 0, i, min = *data, max = 0;

    if (!data || size < 3)
        return 0;

    for (i = 0; i < size; i++) {
        if (data[i] < min)
            min = data[i];
        if (data[i] > max)
            max = data[i];
        sum += data[i];
    }

    if (trim)
        return (sum - min - max) / (size - 2);
    else
        return sum/size;
}

static void gpai_check_adc(void)
{
    int i;

    for(i = 0;i < ADC_TEST_DATA_COUNT;i++) {
        if (g_cur_data[i] != g_expect_data[i])
            printf("[%d] Failed%d/%d\n", i, g_cur_data[i], g_expect_data[i]);
        else
            printf("[%d] OK! %d/%d\n",i, g_cur_data[i], g_expect_data[i]);

        aicos_msleep(100);
    }
}
static int gpai_get_adc(int chan)
{
    int ret;
    int i;
    int current_irq_count;
    int count = 0;
    int cur_data_count = 0;
    int cur;


    gpai_dev = (rt_adc_device_t)rt_device_find(AIC_GPAI_NAME);
    if (!gpai_dev) {
        rt_kprintf("Failed to open %s device\n", AIC_GPAI_NAME);
        return -RT_ERROR;
    }
    ret = rt_adc_enable(gpai_dev, chan);
    if (ret) {
        rt_kprintf("Failed to enable %s device\n", AIC_GPAI_NAME);
        return -RT_ERROR;
    }

    while (count < ADC_TEST_DATA_COUNT) {
        current_irq_count = rt_adc_control(gpai_dev, RT_ADC_CMD_IRQ_COUNT,
                                           (void *)chan);
        cur = rt_adc_read(gpai_dev, chan);
        if (current_irq_count == cur_data_count) {
            g_cur_data[count] = cur;
            count++;
            cur_data_count++;
        }
    }

    for (i = 0; i < ADC_DM_SRAM_SIZE / 8; i++) {
        g_expect_data[i] = average(&g_sram_data[i * 8], 8, 0);
        aicos_msleep(100);
    }
    gpai_check_adc();

    rt_adc_disable(gpai_dev, chan);

    return 0;
}


static void adc_dm_test(u32 chan)
{
    u32 size = 0;

    if (chan <= ADC_CHAN_GPAI11) {
        gen_adc_data(&size);
        hal_dm_chan_store(chan);
        hal_adcdm_sram_write(g_sram_data, 0, ADC_DM_SRAM_SIZE);
        gpai_get_adc(chan);
    }
}

static void cmd_test_adcim(int argc, char **argv)
{
    int c;
    u32 ch = 0;

    if (argc < 2) {
        cmd_adcim_usage(argv[0]);
        return;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'c':
            ch = atoi(optarg);
            if ((ch < 0) || (ch >= AIC_GPAI_CH_NUM))
                pr_err("Invalid channel No.%s\n", optarg);
            adc_dm_test(ch);
            break;
        case 'h':
        default:
            cmd_adcim_usage(argv[0]);
            return;
        }
    }

    return;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_adc, test_adc, adcim dm device sample);
