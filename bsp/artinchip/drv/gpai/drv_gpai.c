/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <drivers/adc.h>

#define LOG_TAG            "GPAI"
#include "aic_core.h"

#include "hal_gpai.h"

#define AIC_GPAI_NAME      "gpai"

struct aic_gpai_dev {
    struct rt_adc_device *dev;
    struct aic_gpai_ch *chan;
};

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
#ifdef AIC_GPAI_DRV_V11
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
#endif
};

static rt_err_t drv_gpai_enabled(struct rt_adc_device *dev,
                                 rt_uint32_t ch, rt_bool_t enabled)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(ch);

    hal_gpai_clk_get(chan);
    if (!chan)
        return -RT_EINVAL;

    if (enabled) {
        aich_gpai_ch_init(chan, chan->pclk_rate);
        chan->irq_count = 0;
        if (chan->mode == AIC_GPAI_MODE_SINGLE)
            chan->complete = aicos_sem_create(0);
    } else {
        aich_gpai_ch_enable(chan->id, 0);
        if (chan->mode == AIC_GPAI_MODE_SINGLE) {
            aicos_sem_delete(chan->complete);
            chan->complete = NULL;
        }
    }

    return RT_EOK;
}

static rt_err_t drv_gpai_convert(struct rt_adc_device *dev,
                                 rt_uint32_t ch, rt_uint32_t *value)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(ch);

    if (!chan)
        return -RT_EINVAL;

    return aich_gpai_read(chan, (u32 *)value, AIC_GPAI_TIMEOUT);
}

static rt_uint8_t drv_gpai_resolution(struct rt_adc_device *dev)
{
    return 12;
}

static rt_err_t drv_gpai_get_irq_count(struct rt_adc_device *dev, rt_uint32_t ch)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(ch);

    if (!chan)
        return -RT_EINVAL;
    return chan->irq_count;
}

static const struct rt_adc_ops aic_adc_ops =
{
    .enabled = drv_gpai_enabled,
    .convert = drv_gpai_convert,
    .get_resolution = drv_gpai_resolution,
    .get_irq_count = drv_gpai_get_irq_count,
};

static int drv_gpai_init(void)
{
    struct rt_adc_device *dev = NULL;
    s32 ret = 0;

    if (hal_gpai_clk_init())
        return -RT_ERROR;

    aicos_request_irq(GPAI_IRQn, aich_gpai_isr, 0, NULL, NULL);
    aich_gpai_enable(1);
    hal_gpai_set_ch_num(ARRAY_SIZE(aic_gpai_chs));

    dev = aicos_malloc(0, sizeof(struct rt_adc_device));
    if (!dev) {
        LOG_E("Failed to malloc(%d)", sizeof(struct rt_adc_device));
        return -RT_ERROR;
    }
    memset(dev, 0, sizeof(struct rt_adc_device));

    ret = rt_hw_adc_register(dev, AIC_GPAI_NAME, &aic_adc_ops, NULL);
    if (ret) {
        LOG_E("Failed to register ADC. ret %d", ret);
        return ret;
    }
    return 0;
}
INIT_BOARD_EXPORT(drv_gpai_init);

#if defined(RT_USING_FINSH)
#include <finsh.h>

static void cmd_gpai_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -c, --channel\t\tSelect one channel in [0, 7], default is 0\n");
    printf("\t -s, --status\t\tShow more hardware information\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example: %s -c 3 -s\n", program);
}

static void cmd_gpai(int argc, char **argv)
{
    u32 ch = 0;
    s32 c, val = 0;
    rt_err_t ret = RT_EOK;
    struct rt_adc_device *dev = NULL;
    struct aic_gpai_ch *chan = NULL;
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
            if ((ch < 0) || (ch >= AIC_GPAI_CH_NUM)) {
                pr_err("Invalid channel No.%s\n", optarg);
                return;
            }
            continue;
        case 's':
            show_status = true;
            continue;
        case 'h':
        default:
            cmd_gpai_usage(argv[0]);
            return;
        }
    }

    chan = hal_gpai_ch_is_valid(ch);
    if (!chan)
        return;

    if (show_status) {
        aich_gpai_status_show(chan);
        return;
    }

    dev = (struct rt_adc_device *)rt_device_find(AIC_GPAI_NAME);
    if (!dev) {
        LOG_E("Failed to open %s device\n", AIC_GPAI_NAME);
        return;
    }
    ret = rt_adc_enable(dev, ch);
    if (!ret) {
        val = rt_adc_read(dev, ch);
        printf("GPAI ch%d: %d\n", ch, val);
    }

    rt_adc_disable(dev, ch);
}
MSH_CMD_EXPORT_ALIAS(cmd_gpai, gpai, Read the status and data of GPAI);

#endif
