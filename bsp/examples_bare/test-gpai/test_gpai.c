/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <getopt.h>

#include "hal_adcim.h"
#include "hal_gpai.h"
#include "mpp_fb.h"

struct aic_gpai_ch aic_gpai_chs[] = {
#ifdef AIC_USING_GPAI0
    {
        .id = 0,
        .available = 1,
        .mode = AIC_GPAI_MODE_SINGLE,
        .fifo_depth = 64,
    },
#endif
#ifdef AIC_USING_GPAI1
    {
        .id = 1,
        .available = 1,
        .mode = AIC_GPAI_MODE_SINGLE,
        .fifo_depth = 64,
    },
#endif
#ifdef AIC_USING_GPAI2
    {
        .id = 2,
        .available = 1,
        .mode = AIC_GPAI_MODE_SINGLE,
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI3
    {
        .id = 3,
        .available = 1,
        .mode = AIC_GPAI_MODE_SINGLE,
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI4
    {
        .id = 4,
        .available = 1,
        .mode = AIC_GPAI_MODE_SINGLE,
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI5
    {
        .id = 5,
        .available = 1,
        .mode = AIC_GPAI_MODE_SINGLE,
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI6
    {
        .id = 6,
        .available = 1,
        .mode = AIC_GPAI_MODE_SINGLE,
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI7
    {
        .id = 7,
        .available = 1,
        .mode = AIC_GPAI_MODE_SINGLE,
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI8
    {
        .id = 8,
        .available = 1,
        .mode = AIC_GPAI_MODE_SINGLE,
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI9
    {
        .id = 9,
        .available = 1,
        .mode = AIC_GPAI_MODE_SINGLE,
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI10
    {
        .id = 10,
        .available = 1,
        .mode = AIC_GPAI_MODE_SINGLE,
        .fifo_depth = 8,
    },
#endif
#ifdef AIC_USING_GPAI11
    {
        .id = 11,
        .available = 1,
        .mode = AIC_GPAI_MODE_SINGLE,
        .fifo_depth = 8,
    },
#endif
};

static char sopts[] = "c:t:h";
static struct option lopts[] = {
    {"channel", required_argument, NULL, 'c'},
    {"voltage", required_argument, NULL, 't'},
    {"help",          no_argument, NULL, 'h'},
    {0, 0, 0, 0}
    };

#define AIC_GPAI_DEFAULT_VOLTAGE        3
#define AIC_GPAI_ADC_MAX_VAL            0xFFF
#define AIC_GPAI_VOLTAGE_ACCURACY       100

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

static int test_gpai_init(int ch)
{
    static int inited = 0;
    struct aic_gpai_ch *chan;

    if (!inited) {
        hal_adcim_probe();
        hal_gpai_clk_init();
        inited = 1;
    }

    hal_gpai_set_ch_num(AIC_GPAI_CH_NUM);
    chan = hal_gpai_ch_is_valid(ch);
    if (!chan)
        return -1;

    aich_gpai_enable(1);
    hal_gpai_clk_get(chan);
    aich_gpai_ch_init(chan, chan->pclk_rate);
    return 0;
}

static int test_gpai_read(int ch)
{
    u32 value;
    struct aic_gpai_ch *chan;

    chan = hal_gpai_ch_is_valid(ch);
    chan->complete = aicos_sem_create(0);
    aicos_request_irq(GPAI_IRQn, aich_gpai_isr, 0, NULL, NULL);
    aich_gpai_read(chan, &value, AIC_GPAI_TIMEOUT);
    printf("ch %d:%d\n", ch, value);
    return value;
}

static void test_adc2voltage(int adc_value, int chan, int st_voltage)
{
    int voltage;
    int scale = AIC_GPAI_VOLTAGE_ACCURACY;

    voltage = hal_adcim_auto_calibration(adc_value, st_voltage, scale,
                                         AIC_GPAI_ADC_MAX_VAL);
    printf("GPAI ch%d-voltage:%d.%02d\n", chan, voltage / scale,
           voltage % scale);
    return;
}

static int cmd_test_gpai(int argc, char *argv[])
{
    int c;
    int ch = 0;
    int adc_value = -1;
    int ret;
    float st_voltage = AIC_GPAI_DEFAULT_VOLTAGE;

    if (argc < 3) {
        cmd_gpai_usage(argv[0]);
        return 0;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'c':
            ch = atoi(optarg);
            ret = test_gpai_init(ch);
            if (!ret)
                adc_value = test_gpai_read(ch);
            break;
        case 't':
            st_voltage = atof(optarg);
            break;
        case 'h':
            cmd_gpai_usage(argv[0]);
        default:
            return 0;
        }
    }

    if (adc_value < 0) {
        printf("Please select a channel first\n");
        return 0;
    }
    if (st_voltage < 0) {
        printf("Please input standard voltage\n");
        return 0;
    }
    test_adc2voltage(adc_value,ch,st_voltage);

    return 0;
}

CONSOLE_CMD(test_gpai, cmd_test_gpai,  "GPAI test example");
