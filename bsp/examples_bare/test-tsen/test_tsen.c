/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include "hal_tsen.h"
#include "hal_adcim.h"
#include "mpp_fb.h"

struct aic_tsen_ch aic_tsen_chs[] = {
    {
        .id = 0,
        .available = 1,
        .name = "tsen-cpu",
        .mode = AIC_TSEN_MODE_SINGLE,
        .hta_enable = 0,
        .lta_enable = 0,
        .otp_enable = 0,
#ifndef CONFIG_FPGA_BOARD_ARTINCHIP
        .slope  = -1134,
        .offset = 2439001,
#endif
    }
};

#define AIC_TSEN_CH_CHOSEN      0

static int test_tsen_init()
{
    static int inited = 0;
    struct aic_tsen_ch *chan;

    if (!inited) {
        hal_adcim_probe();
        inited = 1;
    }
    if (hal_tsen_clk_init())
        return -1;

    chan = &aic_tsen_chs[AIC_TSEN_CH_CHOSEN];
    hal_tsen_enable(1);
    hal_tsen_ch_enable(0, 1);

    hal_tsen_pclk_get(chan);
    hal_tsen_ch_init(chan, chan->pclk_rate);
    return 0;
}

static void test_tsen_read(int ch)
{
    int num;
    s32 value;
    struct aic_tsen_ch *chan;

    chan = &aic_tsen_chs[AIC_TSEN_CH_CHOSEN];
    hal_tsen_curve_fitting(chan);
    chan->complete = aicos_sem_create(0);
    aicos_request_irq(TSEN_IRQn, hal_tsen_irq_handle, 0, NULL, NULL);
    for (num = 0; num < 10; num++) {
        if (hal_tsen_get_temp(chan, &value) >= 0)
            printf("num:%3d, temp:%3d.%d C (%d)\n", num, value / 10,value % 10,
                   chan->latest_data);
    }
    aicos_sem_delete(chan->complete);
}

static int cmd_test_tsen(int argc, char *argv[])
{
    if (test_tsen_init())
        return -1;
    test_tsen_read(0);

    return 0;
}

CONSOLE_CMD(test_tsen, cmd_test_tsen,  "TSensor test example");
