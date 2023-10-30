/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: geo <guojun.dong@artinchip.com>
 */

#include <rtconfig.h>
#include <stdbool.h>
#include <string.h>
#include <hal_i2c.h>
#include "aic_errno.h"

#define gen_reg(val) (volatile void *)(val)
#define USEC_PER_SEC (1000000)

int aic_i2c_init(uint32_t i2c_idx)
{
    int ret = 0;
    ret = hal_clk_enable_deassertrst(CLK_I2C0 + i2c_idx);
    if (ret < 0)
        pr_err("I2C clock and reset init error\n");
    return ret;
}

void hal_i2c_set_hold(ptr_t reg_base, u32 val)
{
    writel(val, reg_base + I2C_SDA_HOLD);
}

int aic_i2c_set_master_slave_mode(unsigned long reg_base, uint8_t mode)
{
    uint32_t reg_val;

    CHECK_PARAM(reg_base, -EINVAL);

    reg_val = readl(gen_reg(reg_base + I2C_CTL));
    reg_val &= ~I2C_CTL_MASTER_SLAVE_SELECT_MASK;
    if (mode)
        reg_val |= I2C_ENABLE_MASTER_DISABLE_SLAVE;
    else
        /* slave mode, and will detect stop signal only if addressed */
        reg_val |= I2C_CTL_STOP_DET_IFADDR;

    writel(reg_val, gen_reg(reg_base + I2C_CTL));

    return 0;
}

int aic_i2c_master_10bit_addr(unsigned long reg_base, uint8_t enable)
{
    uint32_t reg_val;

    CHECK_PARAM(reg_base, -EINVAL);

    reg_val = readl(gen_reg(reg_base + I2C_CTL));
    reg_val &= ~I2C_CTL_10BIT_SELECT_MASTER;
    if (enable)
        reg_val |= I2C_CTL_10BIT_SELECT_MASTER;

    writel(reg_val, gen_reg(reg_base + I2C_CTL));

    return 0;
}

int aic_i2c_slave_10bit_addr(unsigned long reg_base, uint8_t enable)
{
    uint32_t reg_val;

    CHECK_PARAM(reg_base, -EINVAL);

    reg_val = readl(gen_reg(reg_base + I2C_CTL));
    reg_val &= ~I2C_CTL_10BIT_SELECT_SLAVE;
    if (enable)
        reg_val |= I2C_CTL_10BIT_SELECT_SLAVE;

    writel(reg_val, gen_reg(reg_base + I2C_CTL));

    return 0;
}

static int i2c_scl_cnt(uint32_t clk_freq, uint8_t isStandardSpeed,
                       uint16_t *hcnt, uint16_t *lcnt)
{
    uint16_t hcnt_tmp, lcnt_tmp;

    CHECK_PARAM(hcnt, -EINVAL);
    CHECK_PARAM(lcnt, -EINVAL);

    if (isStandardSpeed) {
        /* Minimum value of tHIGH in standard mode is 4000ns
                 * Plus 2 is just to increase the time of tHIGH, appropriately.
                 * SS_MIN_SCL_HIGH * (clk_freq / 1000) is just to prevent 32bits
                 * overflow. SS_MIN_SCL_HIGH * clk_freq will 32bits overflow.
                 */
        hcnt_tmp = SS_MIN_SCL_HIGH * (clk_freq / 1000) / 1000000 + 2;
        lcnt_tmp = SS_MIN_SCL_LOW * (clk_freq / 1000) / 1000000 + 2;
    } else {
        /* If isStandardSpeed is false, set i2c to fast speed
                 * Minimum value of tHIGH in fast mode is 600ns
                 * Plus 3 is just to increase the time of tHIGH, appropriately.
                 * FS_MIN_SCL_HIGH * (clk_freq / 1000)
                 */
        hcnt_tmp = FS_MIN_SCL_HIGH * (clk_freq / 1000) / 1000000 + 2;
        lcnt_tmp = FS_MIN_SCL_LOW * (clk_freq / 1000) / 1000000 + 2;
    }

    *hcnt = hcnt_tmp;
    *lcnt = lcnt_tmp;

    return 0;
}

int aic_i2c_speed_mode_select(unsigned long reg_base, uint32_t clk_freq,
                              uint8_t mode)
{
    uint32_t reg_val;
    uint16_t hcnt, lcnt;
    int ret;

    CHECK_PARAM(reg_base, -EINVAL);

    reg_val = readl(gen_reg(reg_base + I2C_CTL));
    reg_val &= ~I2C_CTL_SPEED_MODE_SELECT_MASK;
    if (mode) {
        reg_val |= I2C_CTL_SPEED_MODE_FS;
        /* Calculate fast speed HCNT and LCNT */
        ret = i2c_scl_cnt(clk_freq, false, &hcnt, &lcnt);
        if (ret)
            return ret;

        writel(hcnt, gen_reg(reg_base + I2C_FS_SCL_HCNT));
        writel(lcnt, gen_reg(reg_base + I2C_FS_SCL_LCNT));
    } else {
        reg_val |= I2C_CTL_SPEED_MODE_SS;
        /* Calculate standard speed HCNT and LCNT */
        ret = i2c_scl_cnt(clk_freq, true, &hcnt, &lcnt);
        if (ret)
            return ret;

        writel(hcnt, gen_reg(reg_base + I2C_SS_SCL_HCNT));
        writel(lcnt, gen_reg(reg_base + I2C_SS_SCL_LCNT));
    }

    writel(reg_val, gen_reg(reg_base + I2C_CTL));

    return 0;
}

/*
 * Set the target address when i2c worked as master mode
 */
void aic_i2c_target_addr(unsigned long reg_base, uint32_t addr)
{
    uint32_t reg_val;

    reg_val = readl(gen_reg(reg_base + I2C_TAR));
    reg_val &= ~I2C_TAR_ADDR_MASK;
    reg_val |= addr;

    writel(reg_val, gen_reg(reg_base + I2C_TAR));
}

int aic_i2c_slave_own_addr(unsigned long reg_base, uint32_t addr)
{
    CHECK_PARAM(reg_base, -EINVAL);
    CHECK_PARAM(!(addr > I2C_TAR_ADDR_MASK), -EINVAL);

    writel(addr, gen_reg(reg_base + I2C_SAR));

    return 0;
}

/**
  \brief       Start sending data as IIC Master.
               This function is non-blocking,\ref csi_iic_event_e is signaled when transfer completes or error happens.
  \param[in]   iic            handle to operate.
  \param[in]   devaddr        iic addrress of slave device. |_BIT[7:1]devaddr_|_BIT[0]R/W_|
                              eg: BIT[7:0] = 0xA0, devaddr = 0x50.
  \param[in]   data           data to send to IIC Slave
  \param[in]   num            size of data items to send
  \return      \ref csi_error_t
*/
int aic_i2c_master_send_msg_async(unsigned long reg_base, uint32_t devaddr,
                                  const void *data, uint32_t size)
{
    //    CSI_PARAM_CHK(iic, CSI_ERROR);
    //    CSI_PARAM_CHK(data, CSI_ERROR);
    //    CSI_PARAM_CHK(size, CSI_ERROR);
    int ret = EOK;

    //    csi_irq_attach((uint32_t)iic->dev.irq_num, &aich_twi_master_tx_handler, &iic->dev);
    //    csi_irq_enable((uint32_t)iic->dev.irq_num);
    //    iic_master_send_intr(iic, devaddr, data, size);

    return ret;
}

/**
  \brief       wait_iic_transmit
  \param[in]   reg_base:    i2c
  \return      \ref csi_error_t
*/
static int32_t aic_i2c_wait_iic_transmit(unsigned long reg_base,
                                         uint32_t timeout)
{
    int32_t ret = I2C_OK;

    do {
        uint32_t timecount = timeout + aic_get_time_ms();

        while ((aic_i2c_get_transmit_fifo_num(reg_base) != 0U) &&
               (ret == EOK)) {
            if (aic_get_time_ms() >= timecount) {
                ret = I2C_TIMEOUT;
            }
        }

    } while (0);

    return ret;
}

/**
  \brief       wait_iic_receive
  \param[in]   iic handle of iic instance
  \param[in]   wait receive data num
  \return      \ref csi_error_t
*/
static int32_t aic_i2c_wait_receive(unsigned long reg_base,
                                    uint32_t wait_data_num, uint32_t timeout)
{
    int32_t ret = I2C_OK;

    do {
        uint32_t timecount = timeout + aic_get_time_ms();

        while ((aic_i2c_get_receive_fifo_num(reg_base) < wait_data_num) &&
               (ret == I2C_OK)) {
            if (aic_get_time_ms() >= timecount) {
                ret = I2C_TIMEOUT;
            }
        }
    } while (0);

    return ret;
}

/**
  \brief       aic_i2c_master_send_msg
  \param[in]   reg_base
  \param[in]
  \return      bytes of sent msg
*/
int32_t aic_i2c_master_send_msg(unsigned long reg_base, struct aic_i2c_msg *msg)
{
    CHECK_PARAM(msg, -EINVAL);

    int32_t ret = I2C_OK;
    uint16_t size = msg->len;
    uint32_t send_count = 0;
    uint32_t stop_time = 0;
    uint32_t timeout = 1000;
    uint32_t reg_val;

    aic_i2c_module_disable(reg_base);
    aic_i2c_target_addr(reg_base, msg->addr);
    aic_i2c_module_enable(reg_base);

    if (!size)
    {
        aic_i2c_transmit_data_with_stop_bit(reg_base, 0);
        while (1)
        {
            reg_val = readl(reg_base + I2C_INTR_RAW_STAT);
            if (reg_val & I2C_INTR_STOP_DET)
            {
                if (reg_val & I2C_INTR_TX_ABRT)
                {
                    return -1;
                }
                else
                {
                    return 0;
                }
            }
        }
    }

    while (1) {
        if (size < I2C_FIFO_DEPTH) {
            for (uint16_t len = 0; len < msg->len - 1; len++) {
                aic_i2c_transmit_data(reg_base, msg->buf[len]);
            }
            aic_i2c_transmit_data_with_stop_bit(reg_base,
                                                msg->buf[msg->len - 1]);

            ret = aic_i2c_wait_iic_transmit(reg_base, timeout);
            if (ret != I2C_OK) {
                send_count = ret;
                break;
            }

            send_count += size;
        } else {
            while (send_count < size) {
                uint8_t send_num =
                    I2C_FIFO_DEPTH - aic_i2c_get_transmit_fifo_num(reg_base);

                for (uint8_t idx = 0; idx < send_num - 1; idx++) {
                    aic_i2c_transmit_data(reg_base, msg->buf[idx]);
                }
                aic_i2c_transmit_data_with_stop_bit(reg_base,
                                                    msg->buf[msg->len - 1]);

                send_count += send_num;

                if (aic_get_time_ms() >= timeout) {
                    ret = I2C_TIMEOUT;
                    break;
                }
            }

            if (ret != I2C_OK) {
                break;
            }
        }

        if ((send_count == size) && (ret == I2C_OK)) {
            while (!(aic_i2c_get_raw_interrupt_state(reg_base) &
                     I2C_INTR_STOP_DET)) {
                stop_time++;

                if (stop_time > I2C_TIMEOUT_DEF_VAL) {
                    return I2C_TIMEOUT;
                }
            }
            break;
        }
    }

    return send_count;
}

/**
  \brief       aic_i2c_master_send_msg
  \param[in]   reg_base
  \param[in]
  \return      state
*/
int32_t aic_i2c_master_receive_msg(unsigned long reg_base,
                                   struct aic_i2c_msg *msg)
{
    CHECK_PARAM(msg, -EINVAL);

    int32_t ret = I2C_OK;
    uint16_t size = msg->len;
    uint32_t read_count = 0;
    uint8_t *receive_data = msg->buf;
    uint32_t timeout = 100;

    CHECK_PARAM(receive_data, -EINVAL);

    aic_i2c_module_disable(reg_base);
    aic_i2c_target_addr(reg_base, msg->addr);
    aic_i2c_module_enable(reg_base);

    if (size < I2C_FIFO_DEPTH) {
        for (uint16_t len = 0; len < size - 1; len++) {
            aic_i2c_read_data_cmd(reg_base);
        }
        aic_i2c_read_data_cmd_with_stop_bit(reg_base);

        ret = aic_i2c_wait_receive(reg_base, size, timeout);
        if (ret == I2C_OK) {
            for (read_count = 0; read_count < (int32_t)size; read_count++) {
                *(receive_data++) = aic_i2c_get_receive_data(reg_base);
                //                pr_notice("receive count %d:[%02x]\n", read_count, *receive_data);
            }
        } else {
            read_count = (int32_t)ret;
            //            pr_err("ret : %d\n", ret);
        }

    } else {
        read_count = 0;
        uint32_t cmd_num = 0;

        for (cmd_num = size; cmd_num > (size - I2C_FIFO_DEPTH); cmd_num--)
            aic_i2c_read_data_cmd(reg_base);

        while (read_count < size) {
            ret = aic_i2c_wait_receive(reg_base, 1U, timeout);

            if (ret != I2C_OK) {
                read_count = (int32_t)ret;
                break;
            }

            *(receive_data++) = aic_i2c_get_receive_data(reg_base);
            read_count++;

            if (cmd_num > 0U) {
                if (cmd_num == 1)
                    aic_i2c_read_data_cmd_with_stop_bit(reg_base);
                else
                    aic_i2c_read_data_cmd(reg_base);
                cmd_num--;
            }
        }

        uint32_t timecount = timeout + aic_get_time_ms();

        while (
            !(aic_i2c_get_raw_interrupt_state(reg_base) & I2C_INTR_STOP_DET)) {
            if (aic_get_time_ms() >= timecount) {
                ret = I2C_TIMEOUT;
                break;
            }
        }
    }
    return read_count;
}
