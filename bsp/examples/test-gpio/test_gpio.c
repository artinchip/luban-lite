/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo.Dong <guojun.dong@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <rtthread.h>
#include "rtdevice.h"
#include "aic_core.h"

// input Key, using "WAKEUP" in board D133CBV-QFN88
#define INPUT_KEY_PIN               "PD.15"

static const char sopts[] = "i::o::h";
static const struct option lopts[] = {
    {"input",       optional_argument,  NULL, 'i'},
    {"output",      optional_argument,  NULL, 'o'},
    {"help",        no_argument,        NULL, 'h'},
    {0, 0, 0, 0}
};


static void test_gpio_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -i, --input\t\tConfigure PIN as input-pin, and print pressed count. Default as PD.15\n");
    printf("\t -o, --output\t\tConfigure PIN as output-pin .\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example： \n");
    printf("         %s -i \n", program);
    printf("    or\n");
    printf("         %s -iPD.4 \n", program);
}

static void test_gpio_input_irq_handler(void *args)
{
    static u32 cnt = 0;
    printf("Key pressed, cnt:%d\n", cnt++);
}


/*
 * parameter:
 *          arg_pin: input para of pin-name
 * return :
 *          0:      pin check failed !!!!
 *          other:  pin number
 *
 */
static u32 test_gpio_pin_check(char *arg_pin)
{
    u32 pin;

    if (arg_pin == RT_NULL || strlen(arg_pin) == 0) {
        printf("pin set default PD.15\n");
        pin = rt_pin_get(INPUT_KEY_PIN);
    } else {
        printf("pin set: [%s]\n", arg_pin);
        pin = rt_pin_get(arg_pin);
    }

    return pin;
}

static void test_gpio_input_pin_cfg(char *arg_pin)
{
    // 1.get pin number
    u32 pin = test_gpio_pin_check(arg_pin);

    // 2.set pin mode to Input-PullUp
    rt_pin_mode(pin, PIN_MODE_INPUT_PULLUP);

    // 3.attach irq handler
    rt_pin_attach_irq(pin, PIN_IRQ_MODE_FALLING, test_gpio_input_irq_handler, NULL);

    // 4.enable pin irq
    rt_pin_irq_enable(pin, PIN_IRQ_ENABLE);
}

static void test_gpio_output_pin(char *arg_pin)
{
    // 1.get pin number
    u32 pin = test_gpio_pin_check(arg_pin);

    // 2.set pin mode to Output
    rt_pin_mode(pin, PIN_MODE_OUTPUT);

    // 3.set pin High and Low
    for(int i = 0; i < 5; i++)
    {
        rt_pin_write(pin, PIN_HIGH);
        rt_thread_mdelay(1000);
        rt_pin_write(pin, PIN_LOW);
        rt_thread_mdelay(1000);
    }
}

int test_gpio(int argc, char **argv)
{
    int c = 0;

    if (argc < 2) {
        test_gpio_usage(argv[0]);
        return -1;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'i':
            test_gpio_input_pin_cfg(optarg);
            break;
        case 'o':
            test_gpio_output_pin(optarg);
            break;
        case 'h':
        default:
            test_gpio_usage(argv[0]);
            return 0;
        }
    }

    return 0;
}


MSH_CMD_EXPORT(test_gpio, gpio device sample);
