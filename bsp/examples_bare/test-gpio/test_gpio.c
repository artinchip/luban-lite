/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao.Li <siyao.li@artinchip.com>
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <getopt.h>
#include "aic_hal_gpio.h"
#include <aic_core.h>
#include "aic_common.h"

#define TEST_GPIO_INPUT_PIN     ("PD.15")
#define GPIO_PINS_NUM_PER_GROUP 32

u32 rising_time_point = 0;
u32 falling_time_point = 0;
volatile u32 rising_flg = 0;
volatile u32 falling_flg = 0;
u32 g, p;

static void cmd_gpio_usage(void)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: test_gpio [options]\n");
    printf("test_gpio input <PIN>      : Configure PIN as input-pin, and print pressed count. Default as PD.15\n");
    printf("test_gpio output <PIN>     : Configure PIN as output-pin\n");
    printf("test_gpio help             : Get this help\n");
    printf("\n");
    printf("Example: test_gpio input PD.4\n");
}

irqreturn_t gpio_input_irq_handler(int irq, void *args)
{
    u32 stat, mask, i, value;

    hal_gpio_group_get_irq_stat(g, &stat);
    hal_gpio_group_get_irq_en(g, &mask);
    stat &= mask;

    for (i = 0; i < GPIO_PINS_NUM_PER_GROUP; i++) {
        if (!(stat & (1U << i)))
            continue;

        if (i == p) {
            hal_gpio_get_value(g, p, &value);
            if (value) {
                /* obtain the rising level time*/
                rising_time_point = aic_get_time_us();
                rising_flg = 1;
            } else {
                /* obtain the falling level time*/
                falling_time_point = aic_get_time_us();
                falling_flg = 1;
            }
            if (rising_flg && falling_flg) {
                rising_flg = 0;
                falling_flg = 0;
                printf("time interval: %dus\n", abs(falling_time_point - rising_time_point));
            }
        }
    }
    hal_gpio_group_set_irq_stat(g, 0xFFFFFFFF);

    return IRQ_HANDLED;
}

static u32 test_gpio_pin_check(char *arg_pin)
{
    u32 pin;
    if (arg_pin == NULL || strlen(arg_pin) == 0) {
        printf("pin set default PD.15\n");
        pin = hal_gpio_name2pin(TEST_GPIO_INPUT_PIN);
    } else {
        printf("pin set: [%s]\n", arg_pin);
        pin = hal_gpio_name2pin(arg_pin);
    }

    return pin;
}


static int cmd_gpio_input_pin_cfg(int argc, char **argv)
{
    u32 pin;
    pin = test_gpio_pin_check(argv[1]);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, 1);
    hal_gpio_direction_input(g, p);
    hal_gpio_set_irq_mode(g, p, PIN_IRQ_MODE_EDGE_BOTH);

    aicos_request_irq(GPIO_IRQn + g, gpio_input_irq_handler, 0, "pin_group", NULL);
    aicos_irq_enable(GPIO_IRQn + g);
    hal_gpio_enable_irq(g, p);
    return 0;
}

static int cmd_gpio_output_pin_cfg(int argc, char **argv)
{
    u32 pin;
    pin = test_gpio_pin_check(argv[1]);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, 1);
    hal_gpio_direction_output(g, p);

    for(int i = 0; i < 5; i++)
    {
        hal_gpio_clr_output(g, p);
        aicos_mdelay(50);
        hal_gpio_set_output(g, p);
        aicos_mdelay(50);
    }
    return 0;
}

static int cmd_test_gpio(int argc, char *argv[])
{
    if (argc < 3) {
        cmd_gpio_usage();
        return 0;
    }

    if (!strcmp(argv[1], "input")) {
        cmd_gpio_input_pin_cfg(argc - 1, &argv[1]);
        return 0;
    }

    if (!strcmp(argv[1], "output")) {
        cmd_gpio_output_pin_cfg(argc - 1, &argv[1]);
        return 0;
    }

    cmd_gpio_usage();

    return 0;
}
CONSOLE_CMD(test_gpio, cmd_test_gpio, "GPIO test example");
