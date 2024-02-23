/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */
#include <stdint.h>
#include <sys_freq.h>
#include <rtconfig.h>
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <aic_soc.h>
#include <aic_arch.h>
#include <aic_common.h>
#include <aic_io.h>
#include <aic_errno.h>
#include <aic_list.h>
#include <aic_osal.h>
#include <drivers/i2c.h>
#include <aic_hal_gpio.h>
#include <hal_i2c.h>

struct aic_i2c_bus {
    struct rt_i2c_bus_device bus;
    uint8_t index;
    uint32_t reg_base;
    char *device_name;
};

static rt_size_t aic_i2c_master_xfer(struct rt_i2c_bus_device *bus,
                                     struct rt_i2c_msg msgs[], rt_uint32_t num)
{
    CHECK_PARAM(bus, -EINVAL);

    struct aic_i2c_bus *i2c_bus = (struct aic_i2c_bus *)bus;
    struct rt_i2c_msg *msg = NULL;
    int ret_msg_len = 0;
    int32_t bytes_cnt = 0;

    for (uint32_t index = 0; index < num; index++) {
        msg = &msgs[index];

        if ((msg->flags & RT_I2C_RD)) {
            bytes_cnt = aic_i2c_master_receive_msg(i2c_bus->reg_base, (struct aic_i2c_msg*)msg);
        } else {
            bytes_cnt = aic_i2c_master_send_msg(i2c_bus->reg_base, (struct aic_i2c_msg*)msg);
        }

        if (bytes_cnt == msg->len) {
            ret_msg_len++;
        }
    }

    return ret_msg_len;
}

static rt_err_t aic_i2c_bus_control(struct rt_i2c_bus_device *bus,
                                rt_uint32_t cmd, rt_uint32_t value)
{
    RT_ASSERT(bus != RT_NULL);

    struct aic_i2c_bus *i2c_bus = (struct aic_i2c_bus *)bus;

    switch(cmd)
    {
    case RT_I2C_DEV_CTRL_CLK:
        aic_i2c_speed_mode_select(i2c_bus->reg_base, I2C_DEFALT_CLOCK, value);
        break;
    default:
        return -RT_EIO;
    }

    return RT_EOK;
}

const struct rt_i2c_bus_device_ops i2c_ops = {
    aic_i2c_master_xfer,
    RT_NULL, // slave
    aic_i2c_bus_control,  // bus control
};

static struct aic_i2c_bus aic_i2c_list[] = {
#ifdef AIC_USING_I2C0
    {
        .index = 0,
        .reg_base = I2C0_BASE,
        .device_name = "i2c0",
        .bus.ops = &i2c_ops,
    },
#endif

#ifdef AIC_USING_I2C1
    {
        .index = 1,
        .reg_base = I2C1_BASE,
        .device_name = "i2c1",
        .bus.ops = &i2c_ops,
    },
#endif

#ifdef AIC_USING_I2C2
    {
        .index = 2,
        .reg_base = I2C2_BASE,
        .device_name = "i2c2",
        .bus.ops = &i2c_ops,
    },
#endif

#ifdef AIC_USING_I2C3
    {
        .index = 3,
        .reg_base = I2C3_BASE,
        .device_name = "i2c3",
        .bus.ops = &i2c_ops,
    },
#endif

#ifdef AIC_USING_I2C4
    {
        .index = 4,
        .reg_base = I2C4_BASE,
        .device_name = "i2c4",
        .bus.ops = &i2c_ops,
    },
#endif

#ifdef AIC_USING_SP_I2C
    {
        .index = 5,
        .reg_base = SP_I2C_BASE,
        .device_name = "sp_i2c",
        .bus.ops = &i2c_ops,
    },
#endif
};

static int aic_hw_i2c_register()
{
    int ret = -1;

    for (uint8_t i = 0; i < ARRAY_SIZE(aic_i2c_list); i++) {
        ret = aic_i2c_init(aic_i2c_list[i].index);
        if (ret) {
            return ret;
        }

        ret = aic_i2c_set_master_slave_mode(aic_i2c_list[i].reg_base, true);
        if (ret) {
            return ret;
        }

        hal_i2c_set_hold(aic_i2c_list[i].reg_base, 10);
        aic_i2c_master_10bit_addr(aic_i2c_list[i].reg_base, false);
        aic_i2c_slave_10bit_addr(aic_i2c_list[i].reg_base, false);

        aic_i2c_master_enable_transmit_irq(aic_i2c_list[i].reg_base);
        aic_i2c_master_enable_receive_irq(aic_i2c_list[i].reg_base);

        aic_i2c_speed_mode_select(aic_i2c_list[i].reg_base, I2C_DEFALT_CLOCK, true);

        ret = rt_i2c_bus_device_register(&aic_i2c_list[i].bus,
                                         aic_i2c_list[i].device_name);
        if (ret) {
            return ret;
        }
    }
    return 0;
}
INIT_BOARD_EXPORT(aic_hw_i2c_register);
