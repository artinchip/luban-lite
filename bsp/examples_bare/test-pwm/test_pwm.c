/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <getopt.h>

#include "aic_common.h"
#include "aic_log.h"

#include "hal_pwm.h"

static void usage(char *program)
{
    printf("Usage: %s [ch] [period] [duty]\n", program);
    printf("Note: The unit of period/duty is ns.\n");
    printf("Example: %s 0 1000 500\n", program);
}

static int test_pwm_init(u32 ch)
{
    static u32 inited = 0;

    struct aic_pwm_action action0 = {
        /*       CBD,          CBU,          CAD, */
        PWM_ACT_NONE, PWM_ACT_NONE, PWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        PWM_ACT_LOW,  PWM_ACT_HIGH, PWM_ACT_NONE};
    struct aic_pwm_action action1 = {
        /*       CBD,          CBU,          CAD, */
        PWM_ACT_NONE, PWM_ACT_NONE, PWM_ACT_NONE,
        /*      CAU,           PRD,         ZRO  */
        PWM_ACT_LOW, PWM_ACT_HIGH,  PWM_ACT_NONE};

    if (!inited && hal_pwm_init())
        return -1;

    hal_pwm_ch_init(ch, PWM_MODE_UP_COUNT, 0, &action0, &action1);
    inited = 1;
    return 0;
}

static int cmd_test_pwm(int argc, char **argv)
{
    u32 ch = 0, period = 0, duty = 0;

    if (argc != 4) {
        pr_err("Invalid argument\n");
        usage(argv[0]);
        return -1;
    }

    ch = atoi(argv[1]);
    if (ch >= AIC_PWM_CH_NUM) {
        pr_err("Invalid channel No. %s\n", argv[1]);
        return -1;
    }
    period = atoi(argv[2]);
    duty = atoi(argv[3]);
    if (duty > period) {
        pr_info("Duty %s is out of range.\n", argv[3]);
        duty = period;
    }

    printf("Set PWM%d: %d/%d ns\n", ch, duty, period);
    if (test_pwm_init(ch))
        return -1;

    if (hal_pwm_enable(ch))
        return -1;

    if (hal_pwm_set(ch, duty, period))
        return -1;

    return 0;
}
CONSOLE_CMD(test_pwm, cmd_test_pwm, "PWM test example");
