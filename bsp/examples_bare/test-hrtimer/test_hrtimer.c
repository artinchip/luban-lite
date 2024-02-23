/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruqi.zheng@artinchip.com>
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <getopt.h>
#include <hal_cap.h>
#include "aic_hal_gpio.h"
#include <aic_core.h>
#include "aic_common.h"

#define HRTIMER_MAX_CH 6
#define GPIO_TEST_PIN "PA.4"
u8 g, p;

static void cmd_hrtimer_usage(void)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: test_hrtimer [options]\n");
    printf("the hrtimer will timed toggle the gpio PA.4 as a demonstration\n");
    printf("test_hrtimer <channel> <time> : Channel range [0, 5], the time unit is 1us\n");
    printf("test_hrtimer help             : Get this help\n");
    printf("\n");
    printf("Example: test_hrtimer 0 20\n");
}

irqreturn_t hrtiemr_irq_handler(int irq, void *args)
{
    u32 i;

    for (i = 0; i < HRTIMER_MAX_CH; i++) {
        if (hal_cap_is_pending(i))
            hal_gpio_toggle_output(g, p);
    }

    return IRQ_HANDLED;
}

void test_hrtimer_ch_init()
{
#ifdef AIC_USING_HRTIMER0
    hal_cap_ch_init(0);
#endif
#ifdef AIC_USING_HRTIMER1
    hal_cap_ch_init(1);
#endif
#ifdef AIC_USING_HRTIMER2
    hal_cap_ch_init(2);
#endif
#ifdef AIC_USING_HRTIMER3
    hal_cap_ch_init(3);
#endif
#ifdef AIC_USING_HRTIMER4
    hal_cap_ch_init(4);
#endif
#ifdef AIC_USING_HRTIMER5
    hal_cap_ch_init(5);
#endif
}

void test_hrtimer_init(u32 ch)
{
    hal_cap_init();
    hal_cap_set_freq(ch, CAP_MAX_FREQ);
    aicos_request_irq(PWMCS_CAP_IRQn, hrtiemr_irq_handler, 0, NULL, NULL);
}

void test_hrtimer_start(u32 ch, u32 cnt)
{
    hal_cap_enable(ch);
    hal_cap_set_cnt(ch, cnt);
    hal_cap_int_enable(ch, 1);
    hal_cap_cnt_start(ch);
}

void test_hrtimer_stop(u32 ch)
{
    hal_cap_cnt_stop(ch);
    hal_cap_int_enable(ch, 0);
    hal_cap_disable(ch);
}

static int cmd_test_hrtimer(int argc, char *argv[])
{
    u32 ch, time_us;
    u8 pin;

    if (argc != 3)
        goto cmd_usage;

    ch = atoi(argv[1]);

    if ((ch < 0) || (ch >= HRTIMER_MAX_CH))
        goto cmd_usage;

    time_us = atoi(argv[2]);

    if (time_us < 0)
        goto cmd_usage;

    /* gpio configuration*/
    pin = hal_gpio_name2pin("PA.4");
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_set_func(g, p, 1);
    hal_gpio_direction_output(g, p);

    /* hrtimer configuration */
    test_hrtimer_ch_init();
    test_hrtimer_init(ch);
    /* stop the hrtimer first */
    test_hrtimer_stop(ch);
    /* start the hrtimer */
    test_hrtimer_start(ch, time_us);

    return 0;

cmd_usage:
    cmd_hrtimer_usage();

    return -1;
}
CONSOLE_CMD(test_hrtimer, cmd_test_hrtimer, "Hrtimer test example");
