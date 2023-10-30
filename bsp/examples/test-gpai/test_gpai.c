/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao.Li <lisy@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <rtthread.h>

#include "hal_adcim.h"
#include "rtdevice.h"
#include "aic_core.h"
#include "aic_log.h"

/* Global macro and variables */
#define AIC_GPAI_NAME               "gpai"
#define AIC_GPAI_ADC_MAX_VAL        0xFFF
#define AIC_GPAI_DEFAULT_VOLTAGE    3
#define AIC_GPAI_VOLTAGE_ACCURACY   100

static rt_adc_device_t gpai_dev;
static const char sopts[] = "c:t:h";
static const struct option lopts[] = {
    {"channel", required_argument, NULL, 'c'},
    {"voltage", required_argument, NULL, 't'},
    {"help",          no_argument, NULL, 'h'},
    {0, 0, 0, 0}
    };

/* Functions */

static void cmd_gpai_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -c, --channel\t\tSelect one channel in [0, %d], default is 0\n",
           AIC_GPAI_CH_NUM);
    printf("\t -t, --voltage\t\tInput standard voltage, default is 3\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example: %s -c 4 -t 3\n", program);
}

static int gpai_get_adc(int chan)
{
    int ret, val;
  
    gpai_dev = (rt_adc_device_t)rt_device_find(AIC_GPAI_NAME);
    if (!gpai_dev) {
        rt_kprintf("Failed to open %s device\n", AIC_GPAI_NAME);
        return -RT_ERROR;
    }

    ret = rt_adc_enable(gpai_dev, chan);
    if (!ret) {
        val = rt_adc_read(gpai_dev, chan);
        rt_kprintf("GPAI ch%d: %d\n", chan, val);
        rt_adc_disable(gpai_dev, chan);
        return val;
    }
    return -RT_ERROR;
}

static void adc2voltage(float st_voltage, int adc_value, int chan)
{
    int voltage;
    int scale = AIC_GPAI_VOLTAGE_ACCURACY;

    voltage = hal_adcim_auto_calibration(adc_value, st_voltage, scale,
                                        AIC_GPAI_ADC_MAX_VAL);
    rt_kprintf("GPAI ch%d-voltage:%d.%02d v\n", chan, voltage / scale,
           voltage % scale);
    return;
}

static void cmd_test_gpai(int argc, char **argv)
{
    int c;
    u32 ch = 0;
    int adc_value = -1;
    float st_voltage = AIC_GPAI_DEFAULT_VOLTAGE;

    if (argc < 2) {
        cmd_gpai_usage(argv[0]);
        return;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'c':
            ch = atoi(optarg);
            if ((ch < 0) || (ch >= AIC_GPAI_CH_NUM)) {
                pr_err("Invalid channel No.%s\n", optarg);
            }
            adc_value = gpai_get_adc(ch);
            break;
        case 't':
            st_voltage = atof(optarg);

            break;
        case 'h':
        default:
            cmd_gpai_usage(argv[0]);
            return;
        }
    }

    if (adc_value < 0) {
        rt_kprintf("Please select a channel first\n");
        return;
    }
    if (st_voltage < 0) {
        rt_kprintf("Please input standard voltage\n");
        return;
    }
    adc2voltage(st_voltage, adc_value, ch);

    return;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_gpai, test_gpai, gpai device sample);
