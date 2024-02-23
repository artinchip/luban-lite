/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <string.h>
#include <aic_osal.h>
#include <getopt.h>
#include <drivers/pm.h>

#include "aic_hal_can.h"

struct aic_can
{
    struct rt_can_device can;
    struct can_handle canHandle;
    char *name;
    uint32_t can_idx;
};

struct aic_can aic_can_arr[] =
{
#ifdef AIC_USING_CAN0
    {
        .name = "can0",
        .can_idx = 0,
    },
#endif
#ifdef AIC_USING_CAN1
    {
        .name = "can1",
        .can_idx = 1,
    },
#endif
};

static void aic_can_sw_filter_to_hw_filter(struct rt_can_filter_config *pfilter,
                                           can_filter_config_t *pfilter_cfg)
{
    int i;

    if (!pfilter)
    {
        pfilter_cfg->rxmask.sfs.data0_filter = 0xFF;
        pfilter_cfg->rxmask.sfs.data1_filter = 0xFF;
        pfilter_cfg->rxmask.sfs.id_filter = 0xFFFF;
        pfilter_cfg->rxmask.sfs.rtr_filter = 0xF;
    }
    else
    {
        for (i = 0; i < pfilter->count; i++)
        {
            pfilter_cfg->filter_mode = SINGLE_FILTER_MODE;
            if (pfilter->items[i].ide == 1)
                pfilter_cfg->is_eff = 1;

            if (pfilter_cfg->is_eff)
            {
                //Extended frame
                pfilter_cfg->rxmask.sfe.id_filter = ~pfilter->items[i].mask;

                pfilter_cfg->rxcode.sfe.id_filter = pfilter->items[i].id &
                                                pfilter->items[i].mask;
                pfilter_cfg->rxcode.sfe.rtr_filter = pfilter->items[i].rtr;
            }
            else
            {
                // Standard frame
                pfilter_cfg->rxmask.sfs.data0_filter = 0xFF;
                pfilter_cfg->rxmask.sfs.data1_filter = 0xFF;
                pfilter_cfg->rxmask.sfs.id_filter = ~pfilter->items[i].mask;

                pfilter_cfg->rxcode.sfs.id_filter = pfilter->items[i].id &
                                                pfilter->items[i].mask;
                pfilter_cfg->rxcode.sfs.rtr_filter = pfilter->items[i].rtr;
            }
        }
    }
}

static rt_err_t aic_can_control(struct rt_can_device *can, int cmd, void *arg)
{
    RT_ASSERT(can != RT_NULL);
    struct aic_can *p_aic_can = (struct aic_can *)can;
    can_handle *phandle = &p_aic_can->canHandle;
    struct rt_can_status *status;
    unsigned long baudrate;
    struct rt_can_filter_config *pfilter;
    can_filter_config_t filter_cfg;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_SET_INT:
        aicos_request_irq(p_aic_can->canHandle.irq_num, hal_can_isr_handler,
                          0, NULL, (void *)&p_aic_can->canHandle);
        hal_can_enable_interrupt(&p_aic_can->canHandle);
        break;
    case RT_DEVICE_CTRL_CLR_INT:
        hal_can_disable_interrupt(&p_aic_can->canHandle);
        break;
    case RT_CAN_CMD_SET_BAUD:
        baudrate = (unsigned long)arg;
        hal_can_ioctl(phandle, CAN_IOCTL_SET_BAUDRATE, (void *)baudrate);
        break;
    case RT_CAN_CMD_GET_STATUS:
        status = (struct rt_can_status *)arg;
        status->rcvpkg = can->status.rcvpkg;
        status->dropedrcvpkg = can->status.dropedrcvpkg;
        status->sndpkg = can->status.sndpkg;
        status->dropedsndpkg = can->status.dropedsndpkg;
        status->rcverrcnt = phandle->status.recverrcnt;
        status->snderrcnt = phandle->status.snderrcnt;
        status->bitpaderrcnt = phandle->status.stufferrcnt;
        status->formaterrcnt = phandle->status.formaterrcnt;
        status->biterrcnt = phandle->status.biterrcnt;
        status->errcode = phandle->status.current_state;
        break;
    case RT_CAN_CMD_SET_FILTER:
        pfilter = (struct rt_can_filter_config *)arg;
        if (pfilter->count > 1)
        {
            rt_kprintf("CAN of this version only support one filter\n");
            return -RT_EINVAL;
        }

        rt_memset(&filter_cfg, 0, sizeof(can_filter_config_t));
        aic_can_sw_filter_to_hw_filter(pfilter, &filter_cfg);

        hal_can_ioctl(phandle, CAN_IOCTL_SET_FILTER, (void *)&filter_cfg);
        break;
    default:
        rt_kprintf("cmd not support\n");
        break;
    }

    return RT_EOK;
}

static int aic_can_send(struct rt_can_device *can, const void *buf,
                        rt_uint32_t boxno)
{
    RT_ASSERT(can != RT_NULL);
    RT_ASSERT(buf != RT_NULL);
    RT_UNUSED(boxno);
    struct aic_can *p_aic_can = (struct aic_can *)can;
    struct rt_can_msg *pmsg = (struct rt_can_msg *)buf;
    can_msg_t msg;
    int i;

    msg.id = pmsg->id;
    msg.rtr = pmsg->rtr;
    msg.ide = pmsg->ide;
    msg.dlc = pmsg->len;

    for (i = 0; i < msg.dlc; i++)
        msg.data[i] = pmsg->data[i];

    hal_can_send_frame(&p_aic_can->canHandle, &msg, TX_REQ);

    return 0;
}

static int aic_can_recv(struct rt_can_device *can, void *buf, rt_uint32_t boxno)
{
    RT_ASSERT(can != RT_NULL);
    RT_ASSERT(buf != RT_NULL);
    RT_UNUSED(boxno);
    struct aic_can *p_aic_can = (struct aic_can *)can;
    struct rt_can_msg *pmsg = (struct rt_can_msg *)buf;
    can_handle *phandle = &p_aic_can->canHandle;
    int i;

    pmsg->id = phandle->msg.id;
    pmsg->rtr = phandle->msg.rtr;
    pmsg->ide = phandle->msg.ide;
    pmsg->len = phandle->msg.dlc;
    pmsg->hdr = 0;

    for (i = 0; i < pmsg->len; i++)
        pmsg->data[i] = phandle->msg.data[i];

    return 0;
}

static rt_err_t aic_configure(struct rt_can_device *can,
                              struct can_configure *cfg)
{
    return RT_EOK;
}

static const struct rt_can_ops aic_can_ops =
{
    aic_configure,
    aic_can_control,
    aic_can_send,
    aic_can_recv,
};

void aic_can_callback(can_handle * phandle, void *arg)
{
    struct aic_can *p_aic_can;
    struct rt_can_device *pcan;
    unsigned long event = (unsigned long)arg;

    p_aic_can = rt_container_of(phandle, struct aic_can, canHandle);
    pcan = (struct rt_can_device *)p_aic_can;

    switch (event) {
    case CAN_EVENT_RX_IND:
        rt_hw_can_isr(pcan, RT_CAN_EVENT_RX_IND);
        break;
    case CAN_EVENT_TX_DONE:
        rt_hw_can_isr(pcan, RT_CAN_EVENT_TX_DONE);
        break;
    case CAN_EVENT_RXOF_IND:
        rt_hw_can_isr(pcan, RT_CAN_EVENT_RXOF_IND);
        break;
    default:
        break;
    }
}

#ifdef RT_USING_PM
static int aic_can_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct aic_can *p_aic_can = (struct aic_can *)device;

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        hal_clk_disable(CLK_CAN0 + p_aic_can->can_idx);
        break;
    default:
        break;
    }

    return 0;
}

static void aic_can_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct aic_can *p_aic_can = (struct aic_can *)device;

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
        hal_clk_enable(CLK_CAN0 + p_aic_can->can_idx);
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops aic_can_pm_ops =
{
    SET_DEVICE_PM_OPS(aic_can_suspend, aic_can_resume)
    NULL,
};
#endif

int rt_hw_aic_can_init(void)
{
    int i, ret = 0;
    struct can_configure config = CANDEFAULTCONFIG;
    config.privmode = RT_CAN_MODE_NOPRIV;
    config.ticks = 50;
#ifdef RT_CAN_USING_HDR
    config.maxhdr = 1;
#endif

    for (i = 0; i < ARRAY_SIZE(aic_can_arr); i++)
    {
        hal_can_init(&aic_can_arr[i].canHandle, aic_can_arr[i].can_idx);
        hal_can_attach_callback(&aic_can_arr[i].canHandle,
                                aic_can_callback, NULL);

        aic_can_arr[i].can.config = config;
        ret = rt_hw_can_register(&aic_can_arr[i].can, aic_can_arr[i].name,
                                 &aic_can_ops, NULL);
#ifdef RT_USING_PM
        rt_pm_device_register(&aic_can_arr[i].can.parent, &aic_can_pm_ops);
#endif
    }

    return ret;
}
INIT_DEVICE_EXPORT(rt_hw_aic_can_init);
