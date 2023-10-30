/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <drivers/adc.h>

#define LOG_TAG            "PSADC"
#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_psadc.h"

#define AIC_PSADC_NAME      "psadc"

struct aic_psadc_dev {
    struct rt_adc_device *dev;
    struct aic_psadc_ch *chan;
};

static u32 g_psadc_pclk_rate = 0;

#ifdef AIC_PSADC_DRV_V11
#define AIC_PSADC_CLK_RATE    40000000   /* 40MHz */
#endif

struct aic_psadc_ch aic_psadc_chs[] = {
#ifdef AIC_USING_PSADC0
    {
        .id = 0,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC1
    {
        .id = 1,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC2
    {
        .id = 2,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC3
    {
        .id = 3,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC4
    {
        .id = 4,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC5
    {
        .id = 5,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC6
    {
        .id = 6,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC7
    {
        .id = 7,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC8
    {
        .id = 8,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC9
    {
        .id = 9,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC10
    {
        .id = 10,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC11
    {
        .id = 11,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC12
    {
        .id = 12,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC13
    {
        .id = 13,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC14
    {
        .id = 14,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC15
    {
        .id = 15,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
};

static rt_err_t drv_psadc_enabled(struct rt_adc_device *dev, rt_uint32_t ch,
                                  rt_bool_t enabled)
{
    struct aic_psadc_ch *chan = hal_psadc_ch_is_valid(ch);

    if (!chan)
        return -RT_EINVAL;
    if (enabled) {
        aich_psadc_ch_init(chan, g_psadc_pclk_rate);
        if (chan->mode == AIC_PSADC_MODE_SINGLE)
            chan->complete = aicos_sem_create(0);
    } else {
        aich_psadc_qc_irq_enable(0);
        if (chan->mode == AIC_PSADC_MODE_SINGLE) {
            aicos_sem_delete(chan->complete);
            chan->complete = NULL;
        }
    }

    return RT_EOK;
}

static rt_err_t drv_psadc_convert(struct rt_adc_device *dev, rt_uint32_t ch,
                                  rt_uint32_t *value)
{
    struct aic_psadc_ch *chan = hal_psadc_ch_is_valid(ch);

    if (!chan)
        return -RT_EINVAL;

    return aich_psadc_read(chan, (u32 *)value, AIC_PSADC_TIMEOUT);
}

static rt_uint8_t drv_gpai_resolution(struct rt_adc_device *dev)
{
    return 12;
}

static const struct rt_adc_ops aic_adc_ops =
{
    .enabled = drv_psadc_enabled,
    .convert = drv_psadc_convert,
    .get_resolution = drv_gpai_resolution,
};

static int drv_psadc_init(void)
{
    struct rt_adc_device *dev = NULL;
    s32 ret = 0;

    ret = hal_clk_set_freq(CLK_PSADC, AIC_PSADC_CLK_RATE);
    if (ret < 0) {
            LOG_E("PSADC clk freq set failed!");
            return -RT_ERROR;
    }

    ret = hal_clk_enable(CLK_PSADC);
    if (ret < 0) {
        LOG_E("PSADC clk enable failed!");
        return -RT_ERROR;
    }

    ret = hal_clk_enable_deassertrst(CLK_PSADC);
    if (ret < 0) {
        LOG_E("PSADC reset deassert failed!");
        return -RT_ERROR;
    }

    aic_psadc_single_queue_mode(1);

    ret = aicos_request_irq(PSADC_IRQn, aich_psadc_isr, 0, NULL, NULL);
      if (ret < 0) {
        LOG_E("PSADC irq enable failed!");
        return -RT_ERROR;
    }

    aich_psadc_enable(1);
    hal_psadc_set_ch_num(ARRAY_SIZE(aic_psadc_chs));
    g_psadc_pclk_rate = hal_clk_get_freq(hal_clk_get_parent(CLK_PSADC));

    dev = aicos_malloc(0, sizeof(struct rt_adc_device));
    if (!dev) {
        LOG_E("Failed to malloc(%d)", sizeof(struct rt_adc_device));
        return -RT_ERROR;
    }
    memset(dev, 0, sizeof(struct rt_adc_device));

    ret = rt_hw_adc_register(dev, AIC_PSADC_NAME, &aic_adc_ops, NULL);
    if (ret) {
        LOG_E("Failed to register ADC. ret %d", ret);
        return ret;
    }
    LOG_I("ArtInChip PSADC loaded");
    return 0;
}
INIT_DEVICE_EXPORT(drv_psadc_init);

#if defined(RT_USING_FINSH)
#include <finsh.h>

static void cmd_psadc_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -c, --channel\t\tSelect one channel in [0, 11],default is 0\n");
    printf("\t -s, --status\t\tShow more hardware information\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example: %s -c 3 -s\n", program);
}

static void cmd_psadc(int argc, char **argv)
{
    u32 ch = 0;
    s32 c, val = 0;
    rt_err_t ret = RT_EOK;
    struct rt_adc_device *dev = NULL;
    struct aic_psadc_ch *chan = NULL;
    bool show_status = false;
    const char sopts[] = "c:sh";
    const struct option lopts[] = {
        {"channel", required_argument, NULL, 'c'},
        {"status",        no_argument, NULL, 's'},
        {"help",          no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'c':
            ch = atoi(optarg);
            if ((ch < 0) || (ch >= AIC_PSADC_CH_NUM)) {
                pr_err("Invalid channel No.%s\n", optarg);
                return;
            }
            continue;
        case 's':
            show_status = true;
            continue;
        case 'h':
        default:
            cmd_psadc_usage(argv[0]);
            return;
        }
    }

    chan = hal_psadc_ch_is_valid(ch);
    if (!chan)
        return;

    if (show_status) {
        aich_psadc_status_show(chan);
        return;
    }

    dev = (struct rt_adc_device *)rt_device_find(AIC_PSADC_NAME);
    if (!dev) {
        LOG_E("Failed to open %s device\n", AIC_PSADC_NAME);
        return;
    }
    ret = rt_adc_enable(dev, ch);
    if (!ret) {
        val = rt_adc_read(dev, ch);
        printf("PSADC ch%d: %d\n", ch, val);
    }

    rt_adc_disable(dev, ch);
}
MSH_CMD_EXPORT_ALIAS(cmd_psadc, psadc, Read the status and data of PSADC);

#endif
