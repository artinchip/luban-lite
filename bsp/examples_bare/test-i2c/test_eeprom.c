/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: hjh <jiehua.huang@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <console.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <aic_core.h>
#include <unistd.h>
#include <hal_i2c.h>
#include <aic_soc.h>

#define I2C_HELP                                                                        \
    "i2c write <I2C BUS ID> <slave addr> <reg_addr> data\n"                             \
    "i2c read <I2C BUS ID> <slave addr> <reg_addr>\n"                                   \
    "This program is used to test master read&write eeprom function of i2c\n "          \
    "Example:\n"                                                                        \
    "write one byte: i2c write 0 0x50 0x0000 0x11\n"                                    \
    "read one byte: i2c read 0 0x50 0x0000 \n"                                          \
    "tips: slave addr is 16bit such as : 0x1234\n"

#ifdef SE_I2C_BASE
#define I2C_REG_BASE SE_I2C_BASE
#else
#define I2C_REG_BASE I2C0_BASE
#endif

void i2c_usage(void)
{
    puts(I2C_HELP);
}

struct aic_i2c_msg *i2c_msg;

void read_eeprom(unsigned long reg_base, struct aic_i2c_msg *msg)
{
    uint8_t *receive_data = msg->buf;
    unsigned long reg_data;
    aic_i2c_module_disable(reg_base);
    aic_i2c_target_addr(reg_base, msg->addr);
    aic_i2c_module_enable(reg_base);

    aic_i2c_transmit_data(I2C_REG_BASE, receive_data[0]);                           //write high_address
    aic_i2c_transmit_data(I2C_REG_BASE, receive_data[1]);                           //write low_address
    aic_i2c_transmit_data(I2C_REG_BASE, I2C_INTR_ACTIVITY | I2C_INTR_STOP_DET);     //read signal

    while ((aic_i2c_get_raw_interrupt_state(I2C_REG_BASE) & I2C_INTR_STOP_DET) == 0) {;};

    reg_data = aic_i2c_get_receive_data(I2C_REG_BASE);
    printf("read_data = 0x%lx\n", reg_data);
}

void write_eeprom(unsigned long reg_base, struct aic_i2c_msg *msg)
{
    uint8_t *send_data = msg->buf;
    aic_i2c_module_disable(reg_base);
    aic_i2c_target_addr(reg_base, msg->addr);
    aic_i2c_module_enable(reg_base);

    aic_i2c_transmit_data(I2C_REG_BASE, send_data[0]);          //write high_address
    aic_i2c_transmit_data(I2C_REG_BASE, send_data[1]);          //write low_address
    aic_i2c_transmit_data(I2C_REG_BASE, send_data[2] | I2C_INTR_STOP_DET);      //write data

    while ((aic_i2c_get_raw_interrupt_state(I2C_REG_BASE) & I2C_INTR_STOP_DET) == 0) {;};
    aic_i2c_module_disable(I2C_REG_BASE);
}

static int test_i2c_example(int argc, char *argv[])
{
    int bus_id = 0;
    struct aic_i2c_msg *msgs = NULL;

    msgs = aicos_malloc(MEM_CMA, sizeof(struct aic_i2c_msg));
    if (!msgs) {
        hal_log_err("msgs malloc fail\n");
        goto __exit;
    }
    memset(msgs, 0, sizeof(struct aic_i2c_msg));

    uint16_t buf_size = 10;
    msgs->buf = aicos_malloc(MEM_CMA, buf_size);
    if (!msgs->buf) {
        hal_log_err("buf malloc fail\n");
        goto __exit;
    }

    if (argc < 5) {
        i2c_usage();
        goto __exit;
    }

    bus_id = atoi(argv[2]);
    if (bus_id < 0 || bus_id > 3) {
        hal_log_err("bus id param error,pleaseinput range 0-2\n");
        goto __exit;
    }

    uint16_t slave_addr = strtol((argv[3]), NULL, 16);
    msgs->addr = slave_addr;

    /* get reg high addr&low addr */
    uint16_t data = strtol((argv[4]), NULL, 16);
    uint8_t high_addr = data >> 8;
    uint8_t low_addr = data & 0xff;
    memcpy(&msgs->buf[0], &high_addr, 1);
    memcpy(&msgs->buf[1], &low_addr, 1);

    if (argc == 6) {
        uint16_t write_data = strtol(argv[5], NULL, 16);
        memcpy(&msgs->buf[2], &write_data, 1);
        printf("write_data = 0x%x\n", write_data);
    }

    int ret = -1;
    ret = aic_i2c_init(bus_id);
    if (ret) {
        hal_log_err("init error\n");
        goto __exit;
    }
    ret = aic_i2c_set_master_slave_mode(I2C_REG_BASE, true);
    if (ret) {
        hal_log_err("mode set error\n");
        goto __exit;
    }

    hal_i2c_set_hold(I2C_REG_BASE, 10);
    aic_i2c_master_10bit_addr(I2C_REG_BASE, false);
    aic_i2c_slave_10bit_addr(I2C_REG_BASE, false);
    aic_i2c_master_enable_transmit_irq(I2C_REG_BASE);
    aic_i2c_master_enable_receive_irq(I2C_REG_BASE);
    aic_i2c_speed_mode_select(I2C_REG_BASE, I2C_DEFALT_CLOCK, true);
    aic_i2c_module_enable(I2C_REG_BASE);

    if (!strcmp(argv[1], "write")) {
        write_eeprom(I2C_REG_BASE, msgs);
    } else if (!strcmp(argv[1], "read")) {
        read_eeprom(I2C_REG_BASE, msgs);
    }

__exit:
    if (msgs != NULL) {
        if (msgs->buf != NULL) {
            aicos_free(MEM_CMA, msgs->buf);
        }
        aicos_free(MEM_CMA, msgs);
    }

    return 0;
}

CONSOLE_CMD(i2c, test_i2c_example, "i2c-tools");

