/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <string.h>
#include <aic_osal.h>
#include <getopt.h>

#include "aic_drv_mtop.h"

struct mtop_dev aic_mtop =
{
    .name = "mtop",
};

rt_err_t mtop_ops_init(rt_device_t dev)
{
    struct mtop_dev *p_aic_mtop = (struct mtop_dev *)dev;

    hal_mtop_init(&p_aic_mtop->mtop_handle);
    return RT_EOK;
}

void aic_mtop_callback(struct aic_mtop_dev *phandle, void *arg)
{
    struct mtop_dev *p_aic_mtop;
    rt_device_t dev;

    p_aic_mtop = rt_container_of(phandle, struct mtop_dev, mtop_handle);
    dev = (rt_device_t)p_aic_mtop;

    if (dev->rx_indicate)
        dev->rx_indicate(dev, 0);
}

rt_err_t mtop_ops_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct mtop_dev *p_aic_mtop = (struct mtop_dev *)dev;
    struct aic_mtop_dev *phandle = &p_aic_mtop->mtop_handle;

    hal_mtop_attach_callback(phandle, aic_mtop_callback, NULL);

    aicos_request_irq(phandle->irq_num, hal_mtop_irq_handler, 0, NULL, (void *)phandle);
    hal_mtop_irq_enable(phandle);
    return RT_EOK;
}

rt_err_t mtop_ops_close(rt_device_t dev)
{
    struct mtop_dev *p_aic_mtop = (struct mtop_dev *)dev;
    struct aic_mtop_dev *phandle = &p_aic_mtop->mtop_handle;

    hal_mtop_detach_callback(phandle);
    hal_mtop_deinit(phandle);

    return RT_EOK;
}

rt_err_t mtop_ops_control(rt_device_t dev, int cmd, void *args)
{
    struct mtop_dev *p_aic_mtop = (struct mtop_dev *)dev;
    struct aic_mtop_dev *phandle = &p_aic_mtop->mtop_handle;
    uint32_t freq, period_cnt;

    switch (cmd) {
    case MTOP_SET_PERIOD_MODE:
        freq = hal_clk_get_freq(CLK_APB0);
        period_cnt = freq / *(unsigned int*)args - 1;
        hal_mtop_set_period_cnt(phandle, period_cnt);
        break;
    case MTOP_ENABLE:
        hal_mtop_enable(phandle);
        break;
    default:
        break;
    }

    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops aic_mtop_ops =
{
    mtop_ops_init,
    mtop_ops_open,
    mtop_ops_close,
    NULL,
    NULL,
    mtop_ops_control
};
#endif

int drv_mtop_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    aic_mtop.dev.ops = &aic_mtop_ops;
#else
    aic_mtop.dev.init = mtop_ops_init;
    aic_mtop.dev.open = mtop_ops_open;
    aic_mtop.dev.close = mtop_ops_close;
    aic_mtop.dev.control = mtop_ops_control;
    aic_mtop.dev.type = RT_Device_Class_Miscellaneous;
#endif
    rt_device_register(&aic_mtop.dev, "mtop", 0);
    return 0;
}

INIT_DEVICE_EXPORT(drv_mtop_init);

#if defined(RT_USING_FINSH) && defined(AIC_MTOP_DRV_TEST)
#include <finsh.h>

int32_t iter = -1;
u32 delay = 1;
volatile uint32_t update_done_flag = 0;

static void usage(char * program)
{
    printf("\n");
    printf("Usage: %s [-n iter] [-d delay] [-h]\n", program);
    printf("   -n NUM   Number of updates before this program exiting.\n");
    printf("   -d NUM   Seconds to wait between update.\n");
    printf("   -h Display this help.\n");
    printf("\n");
}

rt_err_t mtop_update_data_done(rt_device_t dev, rt_size_t size)
{
    RT_UNUSED(size);

    update_done_flag = 1;
    return RT_EOK;
}

static void mtop_update(void)
{
    int i, j, index, pos;

    /* Clear screen */
    printf("\033[2J\033[H\t\t\n");

    for (i = 0; i < MTOP_GROUP_MAX; i++) {
        printf("\t\t\t\t%s Group\n", grp_name[i]);
        printf("\t\t=========================================\n");
        printf("\t\t\t\tread\t\t write\n");
        for (j = 0; j < MTOP_PORT_MAX; j++) {
            pos = group_id[i] * 8 + j;

            if ((1 << pos) & PORT_BITMAP) {
                index = i * MTOP_PORT_MAX + j;
                printf("\t\t%4s:", prt_name[j]);
                printf("%12d.%03dMB/s %8d.%03dMB/s\n",
                    aic_mtop.mtop_handle.port_bw[index].rcnt / (delay * 1000000),
                    aic_mtop.mtop_handle.port_bw[index].rcnt % 1000000 / (delay * 1000),
                    aic_mtop.mtop_handle.port_bw[index].wcnt / (delay * 1000000),
                    aic_mtop.mtop_handle.port_bw[index].wcnt % 1000000 / (delay * 1000));
            }
        }
        printf("\n");
    }
    printf("\n\n");
}

static void test_mtop_thread(void *arg)
{
    mtop_update();
    while (iter == -1 || iter > 0) {
        if (update_done_flag) {
            if (iter > 0)
                iter--;
            update_done_flag = 0;
            mtop_update();
        }
    }
}

void test_mtop(int argc, char **argv)
{
    int opt;
    rt_err_t ret;
    rt_device_t pmdev;

    optind = 0;
    while ((opt = getopt(argc, argv, "n:d:rwh")) != -1) {
        switch (opt) {
        case 'n':
            if (!optarg) {
                usage(argv[0]);
                exit(0);
            }
            iter = strtoul(optarg, NULL, 10);
            break;
        case 'd':
            if (!optarg) {
                usage(argv[0]);
                exit(0);
            }
            delay = strtoul(optarg, NULL, 10);
            break;
        case 'h':
        default:
            usage(argv[0]);
            return;
        }
    }

    pmdev = rt_device_find("mtop");

    ret = rt_device_init(pmdev);
    if (ret)
        LOG_E("device init error\n");

    ret = rt_device_open(pmdev, 0);
    if (ret)
        LOG_E("device open error\n");

    ret = rt_device_set_rx_indicate(pmdev, mtop_update_data_done);
    if (ret)
        LOG_E("device set callback error\n");

    ret = rt_device_control(pmdev, MTOP_SET_PERIOD_MODE, &delay);
    if (ret)
        LOG_E("set period error\n");

    ret = rt_device_control(pmdev, MTOP_ENABLE, 0);
    if (ret)
        LOG_E("mtop enable error\n");

    aicos_thread_t thid = NULL;

    thid = aicos_thread_create("test_mtop", 8096, 25, test_mtop_thread, NULL);
    if (thid == NULL) {
        LOG_E("Failed to create thread\n");
        return;
    }
}
MSH_CMD_EXPORT_ALIAS(test_mtop, mtop, test mtop function);
#endif
