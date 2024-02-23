/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <drivers/watchdog.h>

#define LOG_TAG         "WDT"
#include "aic_core.h"
#include "aic_hal.h"
#include "hal_wdt.h"
#include "aic_drv_wdt.h"

#define AIC_WDT_DEFAULT_CLR_THD     3

struct aic_wdt_dev {
    rt_watchdog_t wdt;
    struct aic_wdt chan;
    s32 dbg_continue;
    u32 cur_chan;
};
static struct aic_wdt_dev g_wdt_dev = {0};

static int aic_wdt_ping(u32 ch)
{
    struct aic_wdt *chan = &g_wdt_dev.chan;

    hal_wdt_op_clr(chan->clr_thd);
    hal_wdt_clr_int();
    LOG_D("Clear the watchdog");
    return 0;
}

rt_err_t drv_wdt_set_clr_thd(rt_watchdog_t *wdt, uint32_t sec)
{
    struct aic_wdt_dev *wdt_dev = (struct aic_wdt_dev*)wdt;

    if (sec > AIC_WDT_DEFAULT_CLR_THD)
        wdt_dev->chan.clr_thd = AIC_WDT_DEFAULT_CLR_THD;
    else
        wdt_dev->chan.clr_thd = sec;

    hal_wdt_clr_thd_set(wdt_dev->cur_chan, &wdt_dev->chan);
    hal_wdt_op_clr(RT_NULL);

    return RT_EOK;
}

rt_err_t drv_wdt_set_irq_thd(rt_watchdog_t *wdt, uint32_t sec)
{
    struct aic_wdt_dev *wdt_dev = (struct aic_wdt_dev*)wdt;

    if (sec >= wdt_dev->chan.rst_thd)
        wdt_dev->chan.irq_thd = wdt_dev->chan.rst_thd - 1;
    else
        wdt_dev->chan.irq_thd = sec;

    hal_wdt_irq_thd_set(wdt_dev->cur_chan, &wdt_dev->chan);
    hal_wdt_op_clr(RT_NULL);

    if (wdt_dev->chan.irq_thd != wdt_dev->chan.rst_thd)
        hal_wdt_irq_enable(1);

    return RT_EOK;
}

rt_err_t drv_wdt_set_timeout(rt_watchdog_t *wdt, uint32_t sec)
{
    struct aic_wdt_dev *wdt_dev = (struct aic_wdt_dev*)wdt;

    wdt_dev->chan.timeout = sec;
    wdt_dev->chan.rst_thd = sec;

    if (hal_wdt_irq_sta() && wdt_dev->chan.irq_thd >=  wdt_dev->chan.rst_thd)
        drv_wdt_set_irq_thd(wdt, wdt_dev->chan.rst_thd);

    hal_wdt_rst_thd_set(wdt_dev->cur_chan, &wdt_dev->chan);
    hal_wdt_op_clr(RT_NULL);

    return RT_EOK;
}

rt_err_t drv_wdt_start(int ch)
{
    LOG_D("Start chan0");
    if (hal_wdt_is_running())
        return aic_wdt_ping(ch);

    hal_wdt_enable(1, g_wdt_dev.dbg_continue);
    return RT_EOK;
}

void drv_wdt_stop(void)
{
    LOG_D("Stop chan0");
    hal_wdt_enable(0, g_wdt_dev.dbg_continue);
    hal_wdt_irq_enable(0);
}

void drv_wdt_switch(rt_watchdog_t *wdt, int ch)
{
    struct aic_wdt_dev *wdt_dev = (struct aic_wdt_dev*)wdt;
    if (ch < 0 || ch >= AIC_WDT_CHAN_NUM) {
        LOG_D("NO such channel:%d", ch);
    } else {
        wdt_dev->cur_chan = ch;
        hal_wdt_switch_chan(wdt_dev->cur_chan);
        LOG_D("Switch channel:%d", wdt_dev->cur_chan);
    }
}

static rt_err_t drv_wdt_control(rt_watchdog_t *wdt, int cmd, void *args)
{
    RT_ASSERT(wdt != NULL);
    struct aic_wdt_dev *wdt_dev = (struct aic_wdt_dev*)wdt;

    switch (cmd) {
    /* RST_THD */
    case RT_DEVICE_CTRL_WDT_GET_TIMEOUT:
        *(uint16_t *)args = wdt_dev->chan.timeout;
        break;

    case RT_DEVICE_CTRL_WDT_SET_TIMEOUT:
        return drv_wdt_set_timeout(&wdt_dev->wdt, *(uint16_t *)args);

    case RT_DEVICE_CTRL_WDT_GET_TIMELEFT:
        *(uint16_t *)args = (u16)hal_wdt_remain(&wdt_dev->chan);
        break;

    case RT_DEVICE_CTRL_WDT_KEEPALIVE:
        aic_wdt_ping(wdt_dev->cur_chan);
        break;

    case RT_DEVICE_CTRL_WDT_START:
        return drv_wdt_start(wdt_dev->cur_chan);

    case RT_DEVICE_CTRL_WDT_STOP:
        drv_wdt_stop();
        break;

    /* IRQ_THD */
    case RT_DEVICE_CTRL_WDT_SET_IRQ_TIMEOUT:
        return drv_wdt_set_irq_thd(&wdt_dev->wdt, *(uint16_t *)args);

    case RT_DEVICE_CTRL_WDT_IRQ_ENABLE:
        aicos_request_irq(WDT_IRQn, args, 0, AIC_WDT_NAME, &wdt_dev);
        hal_wdt_irq_enable(1);
        break;

    case RT_DEVICE_CTRL_WDT_IRQ_DISABLE:
        hal_wdt_irq_enable(0);
        break;

    /* CLR_THD*/
    case RT_DEVICE_CTRL_WDT_SET_CLR_THD:
        drv_wdt_set_clr_thd(&wdt_dev->wdt, *(uint16_t *)args);
        break;

#ifdef AIC_WDT_DRV_V11
    /* RST_EN_SEL*/
    case RT_DEVICE_CTRL_WDT_SET_RST_CPU:
        hal_wdt_rst_type_set(RST_CPU);
        break;

    case RT_DEVICE_CTRL_WDT_SET_RST_SYS:
        hal_wdt_rst_type_set(RST_SYS);
        break;

    case RT_DEVICE_CTRL_WDT_GET_RST_EN:
        return hal_wdt_rst_type_get();
#endif

    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }

    return RT_EOK;
}

rt_err_t drv_wdt_init(rt_watchdog_t *wdt)
{
    rt_err_t ret = RT_EOK;
    struct aic_wdt_dev *wdt_dev = (struct aic_wdt_dev*)wdt;

    RT_ASSERT(wdt != NULL);
    if (hal_clk_enable(CLK_WDT) < 0) {
        LOG_E("Watchdog clk enable failed!");
        return -RT_EINVAL;
    }

    hal_reset_assert(RESET_WDT);
    if (ret < -1) {
        LOG_E("Watchdog reset assert failed.");
        return -RT_EINVAL;
    }

    hal_reset_deassert(RESET_WDT);
    if (ret < -1) {
        LOG_E("Watchdog reset deassert failed.");
        return -RT_EINVAL;
    }

    hal_wdt_thd_get(wdt_dev->cur_chan, &wdt_dev->chan);

    wdt_dev->dbg_continue = 0;
    wdt_dev->cur_chan = 0;

    return ret;
}

void drv_wdt_uninit(rt_watchdog_t *wdt)
{
    aicos_irq_disable(WDT_IRQn);
    hal_clk_disable_assertrst(CLK_WDT);
    hal_clk_disable(CLK_WDT);
}

static struct rt_watchdog_ops aic_wdt_ops = {
    .init = drv_wdt_init,
    .control = drv_wdt_control,
};

int rt_hw_wdt_init(void)
{
    int ret = RT_EOK;

    g_wdt_dev.wdt.ops = &aic_wdt_ops;
    ret = rt_hw_watchdog_register(&g_wdt_dev.wdt, "wdt",
                                  RT_DEVICE_FLAG_RDWR, 0);
    if (ret != RT_EOK)
        LOG_E("Failed to register WDT, return %d", ret);

    LOG_I("ArtInChip WDT loaded");
    return ret;
}
INIT_DEVICE_EXPORT(rt_hw_wdt_init);

#if defined(RT_USING_FINSH)
#include <finsh.h>

static void cmd_wdt_status(int argc, char **argv)
{
    hal_wdt_status_show(0);
}
MSH_CMD_EXPORT_ALIAS(cmd_wdt_status, wdt_status, Show the status of Watchdog);

#endif
