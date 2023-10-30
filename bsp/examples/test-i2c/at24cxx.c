/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-13     XiaojieFan   the first version
 * 2019-12-04     RenMing      ADD PAGE WRITE and input address can be selected
 * 2022-10-11     GuangweiRen  Delay 2ms after writing one byte
 * 2023-09-28	  JiehuaHuang  fix input address invalid bug、add data intput func and fix some problems
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <string.h>
#include <stdlib.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME "at24xx"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#include "at24cxx.h"

#ifdef PKG_USING_AT24CXX
//#define AT24CXX_ADDR (0xA2 >> 1)                      //A0 A1 A2 connect GND

#if (EE_TYPE == AT24C01)
    #define AT24CXX_PAGE_BYTE               8
    #define AT24CXX_MAX_MEM_ADDRESS         128
#elif (EE_TYPE == AT24C02)
    #define AT24CXX_PAGE_BYTE               8
    #define AT24CXX_MAX_MEM_ADDRESS         256
#elif (EE_TYPE == AT24C04)
    #define AT24CXX_PAGE_BYTE               16
    #define AT24CXX_MAX_MEM_ADDRESS         512
#elif (EE_TYPE == AT24C08)
    #define AT24CXX_PAGE_BYTE               16
    #define AT24CXX_MAX_MEM_ADDRESS         1024
#elif (EE_TYPE == AT24C16)
    #define AT24CXX_PAGE_BYTE               16
    #define AT24CXX_MAX_MEM_ADDRESS         2048
#elif (EE_TYPE == AT24C32)
    #define AT24CXX_PAGE_BYTE               32
    #define AT24CXX_MAX_MEM_ADDRESS         4096
#elif (EE_TYPE == AT24C64)
    #define AT24CXX_PAGE_BYTE               32
    #define AT24CXX_MAX_MEM_ADDRESS         8192
#elif (EE_TYPE == AT24C128)
    #define AT24CXX_PAGE_BYTE               64
    #define AT24CXX_MAX_MEM_ADDRESS         16384
#elif (EE_TYPE == AT24C256)
    #define AT24CXX_PAGE_BYTE               64
    #define AT24CXX_MAX_MEM_ADDRESS         32768
#elif (EE_TYPE == AT24C512)
    #define AT24CXX_PAGE_BYTE               128
    #define AT24CXX_MAX_MEM_ADDRESS         65536
#endif

static rt_err_t read_regs(at24cxx_device_t dev, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs;

    //msgs.addr = AT24CXX_ADDR | dev->addr_input;
	msgs.addr = dev->addr_input;
    msgs.flags = RT_I2C_RD;
    msgs.buf = buf;
    msgs.len = len;

    if (rt_i2c_transfer(dev->i2c, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}
uint8_t at24cxx_read_one_byte(at24cxx_device_t dev, uint16_t r_addr)
{
    rt_uint8_t buf[2];
    rt_uint8_t temp;
#if (EE_TYPE > AT24C16)
    buf[0] = (uint8_t)(r_addr>>8);
    buf[1] = (uint8_t)r_addr;
    if (rt_i2c_master_send(dev->i2c, dev->addr_input, 0, buf, 2) == 0)
#else
    buf[0] = r_addr;
    if (rt_i2c_master_send(dev->i2c, dev->addr_input, 0, buf, 1) == 0)
#endif
    {
        return RT_ERROR;
    }
    read_regs(dev, 1, &temp);
    return temp;
}

rt_err_t at24cxx_write_one_byte(at24cxx_device_t dev, uint16_t w_addr, uint8_t data)
{
    rt_uint8_t buf[3];
#if (EE_TYPE > AT24C16)
    buf[0] = (uint8_t)(w_addr>>8);
    buf[1] = (uint8_t)w_addr;
    buf[2] = data;
    if (rt_i2c_master_send(dev->i2c, dev->addr_input, 0, buf, 3) == 3)
#else
    buf[0] = w_addr; //cmd
    buf[1] = data;
    //buf[2] = data[1];


    if (rt_i2c_master_send(dev->i2c, dev->addr_input, 0, buf, 2) == 2)
#endif
        return RT_EOK;
    else
        return -RT_ERROR;

}

rt_err_t at24cxx_read_page(at24cxx_device_t dev, uint32_t r_addr, uint8_t *buffer, uint16_t num_r)
{
    struct rt_i2c_msg msgs[2];
    uint8_t addr_buf[2];

    msgs[0].addr = dev->addr_input;
    msgs[0].flags = RT_I2C_WR;

#if (EE_TYPE > AT24C16)
    addr_buf[0] = r_addr >> 8;
    addr_buf[1] = r_addr;
    msgs[0].buf = addr_buf;
    msgs[0].len = 2;
#else
    addr_buf[0] = r_addr;
    msgs[0].buf = addr_buf;
    msgs[0].len = 1;
#endif

    msgs[1].addr = dev->addr_input;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = buffer;
    msgs[1].len = num_r;

    if(rt_i2c_transfer(dev->i2c, msgs, 2) == 0)
    {
        return RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t at24cxx_write_page(at24cxx_device_t dev, uint32_t waddr, uint8_t *buffer, uint16_t num_w)
{
    struct rt_i2c_msg msgs[2];
    uint8_t addr_buf[2];

    msgs[0].addr = dev->addr_input;
    msgs[0].flags = RT_I2C_WR;

#if (EE_TYPE > AT24C16)
    addr_buf[0] = waddr >> 8;
    addr_buf[1] = waddr;
    msgs[0].buf = addr_buf;
    msgs[0].len = 2;
#else
    addr_buf[0] = waddr;
    msgs[0].buf = addr_buf;
    msgs[0].len = 1;
#endif

    msgs[1].addr = dev->addr_input;
    msgs[1].flags = RT_I2C_WR | RT_I2C_NO_START;
    msgs[1].buf = buffer;
    msgs[1].len = num_w;

    if(rt_i2c_transfer(dev->i2c, msgs, 2) <= 0)
    {
        return RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t at24cxx_check(at24cxx_device_t dev)
{
    uint8_t temp;
    RT_ASSERT(dev);

    temp = at24cxx_read_one_byte(dev, AT24CXX_MAX_MEM_ADDRESS - 1);
    if (temp == 0x55) return RT_EOK;
    else
    {
        at24cxx_write_one_byte(dev, AT24CXX_MAX_MEM_ADDRESS - 1, 0x55);
        rt_thread_mdelay(EE_TWR);                 // wait 5ms befor next operation
        temp = at24cxx_read_one_byte(dev, AT24CXX_MAX_MEM_ADDRESS - 1);
        if (temp == 0x55) return RT_EOK;
    }
    return RT_ERROR;
}

/**
 * This function read the specific numbers of data to the specific position
 *
 * @param bus the name of at24cxx device
 * @param r_addr the start position to read
 * @param buffer  the read data store position
 * @param num_r
 * @return RT_EOK  write ok.
 */
rt_err_t at24cxx_read(at24cxx_device_t dev, uint32_t r_addr, uint8_t *buffer, uint16_t num_r)
{
    rt_err_t result;
    RT_ASSERT(dev);

    if(r_addr + num_r > AT24CXX_MAX_MEM_ADDRESS)
    {
        return RT_ERROR;
    }

    result = rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
        while (num_r)
        {
            *buffer++ = at24cxx_read_one_byte(dev, r_addr++);
            num_r--;
        }
    }
    else
    {
        LOG_E("The at24cxx could not respond  at this time. Please try again");
    }
    rt_mutex_release(dev->lock);

    return RT_EOK;
}

/**
 * This function read the specific numbers of data to the specific position
 *
 * @param bus the name of at24cxx device
 * @param r_addr the start position to read
 * @param buffer  the read data store position
 * @param num_r
 * @return RT_EOK  write ok.
 */
rt_err_t at24cxx_page_read(at24cxx_device_t dev, uint32_t r_addr, uint8_t *buffer, uint16_t num_r)
{
    rt_err_t result = RT_EOK;
    uint16_t page_r_size = AT24CXX_PAGE_BYTE - r_addr % AT24CXX_PAGE_BYTE;

    RT_ASSERT(dev);

    if(r_addr + num_r > AT24CXX_MAX_MEM_ADDRESS)
    {
        return RT_ERROR;
    }

    result = rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    if(result == RT_EOK)
    {
        while (num_r)
        {
            if(num_r > page_r_size)
            {
                if(at24cxx_read_page(dev, r_addr, buffer, page_r_size))
                {
                    result = RT_ERROR;
                }

                r_addr += page_r_size;
                buffer += page_r_size;
                num_r -= page_r_size;
                page_r_size = AT24CXX_PAGE_BYTE;
            }
            else
            {
                if(at24cxx_read_page(dev, r_addr, buffer, num_r))
                {
                    result = RT_ERROR;
                }
                num_r = 0;
            }
        }
    }
    else
    {
        LOG_E("The at24cxx could not respond  at this time. Please try again");
    }

    rt_mutex_release(dev->lock);
    return result;
}

/**
 * This function write the specific numbers of data to the specific position
 *
 * @param bus the name of at24cxx device
 * @param w_addr the start position to write
 * @param buffer  the data need to write
 * @param num_w
 * @return RT_EOK  write ok.at24cxx_device_t dev
 */
rt_err_t at24cxx_write(at24cxx_device_t dev, uint32_t w_addr, uint8_t *buffer, uint16_t num_w)
{
    uint16_t i = 0;
    rt_err_t result;
    RT_ASSERT(dev);

    if(w_addr + num_w > AT24CXX_MAX_MEM_ADDRESS)
    {
        return RT_ERROR;
    }

    result = rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
        while (1) //num_w--
        {
            if (at24cxx_write_one_byte(dev, w_addr, buffer[i]) == RT_EOK)
            {
                rt_thread_mdelay(2);
                w_addr++;
            }
            if (++i == num_w)
            {
                break;
            }
            rt_thread_mdelay(EE_TWR);
        }
    }
    else
    {
        LOG_E("The at24cxx could not respond  at this time. Please try again");
    }
    rt_mutex_release(dev->lock);

    return RT_EOK;
}

/**
 * This function write the specific numbers of data to the specific position
 *
 * @param bus the name of at24cxx device
 * @param w_addr the start position to write
 * @param buffer  the data need to write
 * @param num_w
 * @return RT_EOK  write ok.at24cxx_device_t dev
 */
rt_err_t at24cxx_page_write(at24cxx_device_t dev, uint32_t w_addr, uint8_t *buffer, uint16_t num_w)
{
    rt_err_t result = RT_EOK;
    uint16_t page_w_size = AT24CXX_PAGE_BYTE - w_addr % AT24CXX_PAGE_BYTE;

    RT_ASSERT(dev);

    if(w_addr + num_w > AT24CXX_MAX_MEM_ADDRESS)
    {
        return RT_ERROR;
    }

    result = rt_mutex_take(dev->lock, RT_WAITING_FOREVER);
    if(result == RT_EOK)
    {
        while (num_w)
        {
            if(num_w > page_w_size)
            {
                if(at24cxx_write_page(dev, w_addr, buffer, page_w_size))
                {
                    result = RT_ERROR;
                }
                rt_thread_mdelay(EE_TWR);    // wait 5ms befor next operation

                w_addr += page_w_size;
                buffer += page_w_size;
                num_w -= page_w_size;
                page_w_size = AT24CXX_PAGE_BYTE;
            }
            else
            {
                if(at24cxx_write_page(dev, w_addr, buffer, num_w))
                {
                    result = RT_ERROR;
                }
                rt_thread_mdelay(EE_TWR);   // wait 5ms befor next operation

                num_w = 0;
            }
        }
    }
    else
    {
        LOG_E("The at24cxx could not respond  at this time. Please try again");
    }

    rt_mutex_release(dev->lock);
    return result;
}

/**
 * This function initializes at24cxx registered device driver
 *
 * @param dev the name of at24cxx device
 *
 * @return the at24cxx device.
 */
at24cxx_device_t at24cxx_init(const char *i2c_bus_name, uint8_t addr_input)
{
    at24cxx_device_t dev;

    RT_ASSERT(i2c_bus_name);

    dev = rt_calloc(1, sizeof(struct at24cxx_device));
    if (dev == RT_NULL)
    {
        LOG_E("Can't allocate memory for at24cxx device on '%s' ", i2c_bus_name);
        return RT_NULL;
    }

    dev->i2c = rt_i2c_bus_device_find(i2c_bus_name);
    if (dev->i2c == RT_NULL)
    {
        LOG_E("Can't find at24cxx device on '%s' ", i2c_bus_name);
        rt_free(dev);
        return RT_NULL;
    }

    dev->lock = rt_mutex_create("mutex_at24cxx", RT_IPC_FLAG_FIFO);
    if (dev->lock == RT_NULL)
    {
        LOG_E("Can't create mutex for at24cxx device on '%s' ", i2c_bus_name);
        rt_free(dev);
        return RT_NULL;
    }

    dev->addr_input = addr_input;
    return dev;
}

/**
 * This function releases memory and deletes mutex lock
 *
 * @param dev the pointer of device driver structure
 */
void at24cxx_deinit(at24cxx_device_t dev)
{
    RT_ASSERT(dev);

    rt_mutex_delete(dev->lock);

    rt_free(dev);
}

uint8_t TEST_BUFFER[] = {0x56,0x23};
#define SIZE sizeof(TEST_BUFFER)

void at24cxx(int argc, char *argv[])
{
    static at24cxx_device_t dev = RT_NULL;

    if (argc > 1)
    {
        if (!strcmp(argv[1], "probe"))
        {
            if (argc > 2)
            {
                /* initialize the sensor when first probe */
                if (!dev || strcmp(dev->i2c->parent.parent.name, argv[2]))
                {
                    /* deinit the old device */
                    if (dev)
                    {
                        at24cxx_deinit(dev);
                    }
					uint32_t slave_addr = strtol((argv[3]), NULL, 16);
					//rt_kprintf("slave_addr = %x\n", slave_addr);
                    dev = at24cxx_init(argv[2], slave_addr);
                }
            }
            else
            {
                rt_kprintf("at24cxx probe <dev_name> <addr_input> - probe sensor by given name\n");
            }
        }
        else if (!strcmp(argv[1], "read"))
        {
            if (dev)
            {
                uint8_t testbuffer[50];

                /* read the eeprom data */
                at24cxx_read(dev, 0, testbuffer, SIZE);

                rt_kprintf("read data : 0x%x\n", testbuffer[0]);

            }
            else
            {
                rt_kprintf("Please using 'at24cxx probe <dev_name>' first\n");
            }
        }
        else if (!strcmp(argv[1], "write"))
        {
			uint8_t *data = malloc(10);
			*data = strtol((argv[2]), NULL, 16);
            at24cxx_write(dev, 0, data, SIZE);
            rt_kprintf("write ok\n");
			free(data);
        }
        else if (!strcmp(argv[1], "check"))
        {
            if (at24cxx_check(dev) == 1)
            {
                rt_kprintf("check faild \n");
            }
        }
        else
        {
            rt_kprintf("Unknown command. Please enter 'at24cxx0' for help\n");
        }
    }
    else
    {
        rt_kprintf("Usage:\n");
        rt_kprintf("at24cxx probe <dev_name> <slave_addr>  - probe eeprom by given name\n");
        rt_kprintf("at24cxx check                               - check eeprom at24cxx \n");
        rt_kprintf("at24cxx read                             - read eeprom at24cxx data\n");
        rt_kprintf("at24cxx write 0x24                      - write eeprom at24cxx data\n");

    }
}
MSH_CMD_EXPORT(at24cxx, at24cxx eeprom function);
#endif
