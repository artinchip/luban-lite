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

/* The default voltages are set to D21x->3.0V, D31x->2.5V */
#define AIC_GPAI_DEFAULT_VOLTAGE        3
#define AIC_GPAI_ADC_MAX_VAL            0xFFF
#define AIC_GPAI_VOLTAGE_ACCURACY       10000

static float g_def_voltage = AIC_GPAI_DEFAULT_VOLTAGE;

static void cmd_gpai_usage(void)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: test_gpai [options]\n");
    printf("test_gpai read <channel_id>      : Select one channel in [0, %d], default is 0\n",
           AIC_GPAI_CH_NUM - 1);
    printf("test_gpai set <default_voltage>  : Modify default voltage\n");
    printf("test_gpai help                   : Get this help\n");
    printf("\n");
    printf("Example: test_gpai read 4\n");
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

static void adc2voltage(float def_voltage, int adc_value, int chan)
{
    int voltage;
    int scale = AIC_GPAI_VOLTAGE_ACCURACY;

    voltage = hal_adcim_auto_calibration(adc_value, def_voltage, scale);
    if (voltage)
        printf("GPAI ch%d-voltage:%d.%04d v\n", chan, voltage / scale,
                   voltage % scale);
    return;
}

static void cmd_gpai_set(int argc, char **argv)
{
    g_def_voltage = strtod(argv[1], NULL);
    if (g_def_voltage < 0) {
        printf("Please input valid default voltage\n");
        return;
    }
    printf("Successfully set the default voltage");
}


static int cmd_gpai_read(int argc, char **argv)
{
    u32 value, ch;
    struct aic_gpai_ch *chan;
    ch = strtod(argv[1], NULL);

    if ((ch < 0) || (ch >= AIC_GPAI_CH_NUM)) {
        printf("Invalid channel No.%d\n", ch);
        return -1;
    }

    test_gpai_init(ch);

    chan = hal_gpai_ch_is_valid(ch);
    chan->complete = aicos_sem_create(0);
    aicos_request_irq(GPAI_IRQn, aich_gpai_isr, 0, NULL, NULL);
    aich_gpai_read(chan, &value, AIC_GPAI_TIMEOUT);
    printf("ch %d:%d\n", ch, value);

    adc2voltage(g_def_voltage, value, ch);

    return 0;
}

static int cmd_test_gpai(int argc, char *argv[])
{
    if (argc < 3) {
        cmd_gpai_usage();
        return 0;
    }

    if (!strcmp(argv[1], "read")) {
        cmd_gpai_read(argc - 1, &argv[1]);
        return 0;
    }

    if (!strcmp(argv[1], "set")) {
        cmd_gpai_set(argc - 1, &argv[1]);
        return 0;
    }

    cmd_gpai_usage();

    return 0;
}

CONSOLE_CMD(test_gpai, cmd_test_gpai,  "GPAI test example");
