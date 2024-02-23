/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-01-10        the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "touch.h"
#include <rtdbg.h>
#include "gsl1680.h"

#define DBG_TAG "gsl1680"
#define DBG_LVL DBG_LOG

static void gsl1680_write_reg(struct rt_i2c_bus_device *bus, rt_uint8_t reg, rt_uint8_t value)
{
    struct rt_i2c_msg msg = {0};
    rt_uint8_t buf[2];

    buf[0] = reg;
    buf[1] = value;

    msg.addr = GSL1680_SLAVE_ADDR;
    msg.flags = RT_I2C_WR;
    msg.buf = buf;
    msg.len = 2;
   if (rt_i2c_transfer(bus, &msg, 1) != 1)
   {
       LOG_D("gsl1680 write register error");
   }
}

static void gsl1680_write_regs(struct rt_i2c_bus_device *bus, rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs;

    msgs.addr = GSL1680_SLAVE_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf = data;
    msgs.len = len;
    if (rt_i2c_transfer(bus, &msgs, 1) != 1)
    {
        LOG_D("gsl1680 read register error");
    }

}

static void gsl1680_read_reg(struct rt_i2c_bus_device *bus, rt_uint8_t reg, rt_uint8_t *buf, rt_uint8_t len)
{
    struct rt_i2c_msg msg = {0};
    rt_uint8_t temp_reg;

    temp_reg = reg;

    msg.addr = GSL1680_SLAVE_ADDR;
    msg.flags = RT_I2C_WR;
    msg.buf = &temp_reg;
    msg.len = 1;
    if (rt_i2c_transfer(bus, &msg, 1) != 1)
    {
        LOG_D("gsl1680 write register error");
    }

    msg.addr = GSL1680_SLAVE_ADDR;
    msg.flags = RT_I2C_RD;
    msg.buf = buf;
    msg.len = len;
    if (rt_i2c_transfer(bus, &msg, 1) != 1)
    {
        LOG_D("gsl1680 read register error");
    }
}

static void gsl1680_clr_reg(struct rt_i2c_bus_device *bus)
{
    gsl1680_write_reg(bus, GSL1680_RESET_REG, 0x88);
    rt_thread_mdelay(20);
    gsl1680_write_reg(bus, GSL1680_TOUCH_NUMBER_REG, 0x3);
    rt_thread_mdelay(5);
    gsl1680_write_reg(bus, GSL1680_CLK_REG, 0x4);
    rt_thread_mdelay(5);
    gsl1680_write_reg(bus, GSL1680_RESET_REG, 0x0);
    rt_thread_mdelay(20);
}

static void gsl1680_reset_chip(struct rt_i2c_bus_device *bus)
{
    rt_uint8_t buf[5];

    gsl1680_write_reg(bus, GSL1680_RESET_REG, 0x88);
    rt_thread_mdelay(10);
    gsl1680_write_reg(bus, GSL1680_CLK_REG, 0x4);
    rt_thread_mdelay(10);

    buf[0] = GSL1680_POWER_OFF_DETECT_REG;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = 0;
    gsl1680_write_regs(bus, buf, 5);
    rt_thread_mdelay(10);
}

static void gsl1680_startup_chip(struct rt_i2c_bus_device *bus)
{
    gsl1680_write_reg(bus, GSL1680_RESET_REG, 0x0);
    rt_thread_mdelay(10);
}

static int gsl1680_check_mem_data(struct rt_i2c_bus_device *bus)
{
    int ret = GSL1680_CHIP_STATUS_OK;
    rt_uint8_t buf[4];

    rt_thread_mdelay(30);
    gsl1680_read_reg(bus, GSL1680_CHIP_INSPECTION_REG, buf, 4);
    if (buf[0] != GSL1680_MEM_DATA || buf[1] != GSL1680_MEM_DATA \
            || buf[2] != GSL1680_MEM_DATA || buf[3] != GSL1680_MEM_DATA) {
                LOG_D("gsl1680 chip status is abnormal");
                return GSL1680_CHIP_STATUS_ERR;
            }

    return ret;
}

static void gsl1680_load_fw(struct rt_i2c_bus_device *bus)
{
    rt_uint8_t buf[5];
    rt_uint8_t reg;
    rt_uint32_t source_line = 0;
    rt_uint32_t source_len = sizeof(GSLX680_FW) / sizeof(struct fw_data);

    LOG_D("[TP]_GSLX680_load_fw start=========\n");

    for (source_line = 0; source_line < source_len; source_line++) {
        reg = GSLX680_FW[source_line].offset;
        buf[0] = reg;
        buf[1] = (char)(GSLX680_FW[source_line].val & 0x000000ff);
        buf[2] = (char)((GSLX680_FW[source_line].val & 0x0000ff00) >> 8);
        buf[3] = (char)((GSLX680_FW[source_line].val & 0x00ff0000) >> 16);
        buf[4] = (char)((GSLX680_FW[source_line].val & 0xff000000) >> 24);

        gsl1680_write_regs(bus, buf, 5);
    }

    LOG_D("[TP]_GSLX680_load_fw end=========\n");
}

static void gsl1680_init(struct rt_i2c_bus_device *bus, rt_base_t pin)
{
    int ret = 0;

    rt_pin_mode(pin, PIN_MODE_OUTPUT);
    rt_pin_write(pin, 0);
    rt_thread_mdelay(20);
    rt_pin_write(pin, 1);
    rt_thread_mdelay(20);
    gsl1680_clr_reg(bus);
    gsl1680_reset_chip(bus);
    gsl1680_startup_chip(bus);

    ret = gsl1680_check_mem_data(bus);
    if (ret) {
        rt_pin_write(pin, 0);
        rt_thread_mdelay(20);
        rt_pin_write(pin, 1);
        rt_thread_mdelay(20);
        gsl1680_clr_reg(bus);
        gsl1680_reset_chip(bus);
        gsl1680_load_fw(bus);
        gsl1680_startup_chip(bus);
    }
}

static rt_size_t gsl1680_readpoint(struct rt_touch_device *touch, void *data, rt_size_t touch_num)
{
    uint16_t x_poit, y_poit, point_num;
    static rt_uint8_t s_tp_down = 0;
    static uint16_t x_save, y_save;
    static rt_uint8_t s_count = 0;
    uint8_t buf[8] = {0};
    struct rt_i2c_bus_device * i2c_bus = RT_NULL;

    struct rt_touch_data *temp_data;

    temp_data = (struct rt_touch_data *)data;

    temp_data->event = RT_TOUCH_EVENT_NONE;
    temp_data->timestamp = rt_touch_get_ts();

    i2c_bus = rt_i2c_bus_device_find(touch->config.dev_name);
    if (i2c_bus == RT_NULL)
    {
        LOG_D("can not find i2c bus");
        return -RT_EIO;
    }

    gsl1680_read_reg(i2c_bus, GSL1680_TOUCH_NUMBER_REG, buf, 8);
    /* Here,the xy axis coordinate displacement has been performed
     * and the reversal of the x-axis after displacement
     * in order to locate the coordinate origin int the upper right corner
     * (x, y) --> ((range - y), x)
     */
    // x_poit = ((buf[7] & 0X0F) << 8) + buf[6];
    // y_poit = (buf[5] << 8) + buf[4];
    y_poit = ((buf[7] & 0X0F) << 8) + buf[6];
    x_poit = (buf[5] << 8) + buf[4];
    point_num = buf[0]; //buf[0] is the touch number

    // temp_data->x_coordinate = x_poit;
    temp_data->x_coordinate = 480 - x_poit;
    temp_data->y_coordinate = y_poit;
    temp_data->timestamp = rt_touch_get_ts();

    if (point_num == 0) {
        if (s_tp_down == 1) {
            if (++s_count > 2) {
                s_count = 0;
                s_tp_down = 0;
                temp_data->event = RT_TOUCH_EVENT_UP;
                temp_data->timestamp = rt_touch_get_ts();
                temp_data->x_coordinate = x_save;
                temp_data->y_coordinate = y_save;
            }
        }
        return RT_EOK;
    }
    s_count = 0;

    if (s_tp_down == 0)
    {
        s_tp_down = 1;
        temp_data->event = RT_TOUCH_EVENT_DOWN;
    }
    else
    {
        temp_data->event = RT_TOUCH_EVENT_MOVE;
    }
    x_save = temp_data->x_coordinate;
    y_save = temp_data->y_coordinate;
    // rt_kprintf("x:%ld,y:%ld\n", x_save, y_save);
    return RT_EOK;

}

static rt_err_t gsl1680_control(struct rt_touch_device *touch, int cmd, void *arg)
{
    struct rt_touch_info *info;

    switch(cmd)
    {
    case RT_TOUCH_CTRL_GET_ID:
        break;
    case RT_TOUCH_CTRL_GET_INFO:
        info = (struct rt_touch_info *)arg;
        info->point_num = touch->info.point_num;
        info->range_x = touch->info.range_x;
        info->range_y = touch->info.range_y;
        info->type = touch->info.type;
        info->vendor = touch->info.vendor;

        break;
    case RT_TOUCH_CTRL_SET_MODE:
        break;
    case RT_TOUCH_CTRL_SET_X_RANGE:
        break;
    case RT_TOUCH_CTRL_SET_Y_RANGE:
        break;
    case RT_TOUCH_CTRL_SET_X_TO_Y:
        break;
    case RT_TOUCH_CTRL_DISABLE_INT:
        break;
    case RT_TOUCH_CTRL_ENABLE_INT:
        break;
    default:
        break;
    }

    return RT_EOK;
}

const struct rt_touch_ops ops =
{
    gsl1680_readpoint,
    gsl1680_control,
};

struct rt_touch_info ft_info =
{
    RT_TOUCH_TYPE_CAPACITANCE,
    RT_TOUCH_VENDOR_UNKNOWN,
    1,
    (rt_int32_t)AIC_TOUCH_PANEL_GSL1680_X_RANGE,
    (rt_int32_t)AIC_TOUCH_PANEL_GSL1680_Y_RANGE,
};

int rt_hw_gsl1680_init(const char *name, struct rt_touch_config *cfg,  rt_base_t pin)
{
    rt_touch_t touch_device = RT_NULL;
    struct rt_i2c_bus_device * i2c_bus = RT_NULL;

    touch_device = (rt_touch_t)rt_calloc(1, sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    i2c_bus = rt_i2c_bus_device_find(cfg->dev_name);
    if (i2c_bus == RT_NULL)
    {
        return -RT_EIO;
    }

    gsl1680_init(i2c_bus, pin);

    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));

    touch_device->info = ft_info;
    touch_device->ops = &ops;

    if (rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_RDONLY, RT_NULL) != RT_EOK)
    {
        return -RT_EIO;
    }

    return RT_EOK;
}

static int rt_hw_gsl1680_port(void)
{
    struct rt_touch_config cfg;
    rt_base_t rst_pin;

    cfg.dev_name = AIC_TOUCH_PANEL_GSL1680_I2C_CHA;
    rst_pin = hal_gpio_name2pin(AIC_TOUCH_PANEL_GSL1680_RST_PIN);

    rt_hw_gsl1680_init("gsl1680", &cfg, rst_pin);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_gsl1680_port);
