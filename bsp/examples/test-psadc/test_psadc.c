/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <rtthread.h>

#include "rtdevice.h"
#include "aic_core.h"
#include "aic_log.h"

/* Global macro and variables */
#define AIC_PSADC_NAME               "psadc"
#define AIC_PSADC_CH_NUM             12
#define AIC_PSADC_ADC_MAX_VAL        0xFFF
#define AIC_PSADC_DEFAULT_VOLTAGE    3

static rt_adc_device_t psadc_dev;
static const char sopts[] = "c:t:h";
static const struct option lopts[] = {
    {"channel", required_argument, NULL, 'c'},
    {"voltage", required_argument, NULL, 't'},
    {"help",          no_argument, NULL, 'h'},
    {0, 0, 0, 0}
    };

/* Functions */

static void cmd_psadc_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -c, --channel\t\tSelect one channel in [0, 11],default is 0\n");
    printf("\t -t, --voltage\t\tInput standard voltage, default is 3\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example: %s -c 4 -t 3\n", program);
}

static int psadc_get_adc(int chan)
{
    int ret, val;

    psadc_dev = (rt_adc_device_t)rt_device_find(AIC_PSADC_NAME);
    if (!psadc_dev) {
        rt_kprintf("Failed to open %s device\n", AIC_PSADC_NAME);
        return -RT_ERROR;
    }

    ret = rt_adc_enable(psadc_dev, chan);
    if (!ret) {
        val = rt_adc_read(psadc_dev, chan);
        rt_kprintf("PSADC ch%d: %d\n", chan, val);
        rt_adc_disable(psadc_dev, chan);
        return val;
    }
    return -RT_ERROR;
}

static void adc2voltage(float st_voltage, int adc_value, int chan)
{
    int voltage;

    voltage = (adc_value*st_voltage*100) / AIC_PSADC_ADC_MAX_VAL;
    rt_kprintf("PSADC ch%d-voltage:%d.%2d\n", chan, voltage / 100,
               voltage % 100);
    return;
}

static void cmd_test_psadc(int argc, char **argv)
{
    int c;
    u32 ch = 0;
    int adc_value = -1;
    float st_voltage = AIC_PSADC_DEFAULT_VOLTAGE;

    if (argc < 2) {
        cmd_psadc_usage(argv[0]);
        return;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'c':
            ch = atoi(optarg);
            if ((ch < 0) || (ch >= AIC_PSADC_CH_NUM)) {
                pr_err("Invalid channel No.%s\n", optarg);
            }
            adc_value = psadc_get_adc(ch);
            break;
        case 't':
            st_voltage = atof(optarg);

            break;
        case 'h':
        default:
            cmd_psadc_usage(argv[0]);
            return;
        }
    }

    if (adc_value < 0){
        rt_kprintf("Please select a channel first\n");
        return;
    }
    if (st_voltage < 0){
        rt_kprintf("Please input standard voltage\n");
        return;
    }
    adc2voltage(st_voltage, adc_value, ch);

    return;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_psadc, test_psadc, psadc device sample);
