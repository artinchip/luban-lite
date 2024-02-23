/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_HAL_I2C_
#define _AIC_HAL_I2C_

#include <stdint.h>
#include <stdbool.h>
#include "aic_errno.h"
#include "aic_common.h"
#include "aic_hal_clk.h"
#include "aic_log.h"
#include "aic_io.h"

#ifdef __cplusplus
extern "C" {
#endif

struct aic_i2c_msg
{
    uint16_t addr;
    uint16_t flags;
    uint16_t len;
    uint8_t  *buf;
};

typedef enum {
    I2C_OK = 0,
    I2C_ERR = -1,
    I2C_BUSY = -2,
    I2C_TIMEOUT = -3,
    I2C_UNSPUPPORTED = -4,
} i2c_error_t;

#define I2C_DEFALT_CLOCK      24000000

#define I2C_CTL               0x00
#define I2C_TAR               0x04
#define I2C_SAR               0x08
#define I2C_ACK_GEN_CALL      0x0C
#define I2C_DATA_CMD          0x10
#define I2C_SS_SCL_HCNT       0x20
#define I2C_SS_SCL_LCNT       0x24
#define I2C_FS_SCL_HCNT       0x28
#define I2C_FS_SCL_LCNT       0x2C
#define I2C_SDA_HOLD          0x30
#define I2C_SDA_SETUP         0x34
#define I2C_INTR_MASK         0x38
#define I2C_INTR_CLR          0x3C
#define I2C_INTR_RAW_STAT     0x40
#define I2C_ENABLE            0x48
#define I2C_ENABLE_STATUS     0x4C
#define I2C_STATUS            0x50
#define I2C_TX_ABRT_SOURCE    0x54
#define I2C_RX_TL             0x90
#define I2C_TX_TL             0x94
#define I2C_TXFLR             0x98
#define I2C_RXFLR             0x9C
#define I2C_SCL_STUCK_TIMEOUT 0xA0
#define I2C_SDA_STUCK_TIMEOUT 0xA4
#define I2C_FS_SPIKELEN       0xB0
#define I2C_VERSION           0xFC

#define I2C_CTL_MASTER_SLAVE_SELECT_MASK (3)
#define I2C_CTL_10BIT_SELECT_MASTER      (1 << 2)
#define I2C_CTL_10BIT_SELECT_SLAVE       (1 << 3)
#define I2C_CTL_SPEED_MODE_SELECT_MASK   (3 << 4)
#define I2C_CTL_SPEED_MODE_SS            (0x1 << 4)
#define I2C_CTL_SPEED_MODE_FS            (0x2 << 4)
#define I2C_CTL_RESTART_ENABLE           (1 << 6)
#define I2C_CTL_STOP_DET_IFADDR          (1 << 7)
#define I2C_CTL_TX_EMPTY_CTL             (1 << 8)
#define I2C_CTL_RX_FIFO_FULL_HLD         (1 << 9)
#define I2C_CTL_BUS_CLEAR_FEATURE        (1 << 10)

#define I2C_TAR_ADDR_MASK    (0x3FF)
#define I2C_TAR_START_BYTE   (1 << 10)
#define I2C_TAR_GEN_CALL_CTL (1 << 11)

#define I2C_DATA_CMD_DAT_MASK (0xFF)
#define I2C_DATA_CMD_READ     (1 << 8)
#define I2C_DATA_CMD_STOP     (1 << 9)
#define I2C_DATA_CMD_RESTART  (1 << 10)

#define I2C_SDA_TX_HOLD_MASK (0xFFFF)
#define I2C_SDA_RX_HOLD_MASK (0xFF << 16)

#define I2C_INTR_RX_UNDER         (1 << 0)
#define I2C_INTR_RX_FULL          (1 << 2)
#define I2C_INTR_TX_EMPTY         (1 << 4)
#define I2C_INTR_RD_REQ           (1 << 5)
#define I2C_INTR_TX_ABRT          (1 << 6)
#define I2C_INTR_RX_DONE          (1 << 7)
#define I2C_INTR_ACTIVITY         (1 << 8)
#define I2C_INTR_STOP_DET         (1 << 9)
#define I2C_INTR_START_DET        (1 << 10)
#define I2C_INTR_GEN_CALL         (1 << 11)
#define I2C_INTR_MASTER_ON_HOLD   (1 << 13)
#define I2C_INTR_SCL_STUCK_AT_LOW (1 << 14)

#define I2C_ENABLE_BIT                (1 << 0)
#define I2C_ENABLE_ABORT              (1 << 1)
#define I2C_TX_CMD_BLOCK              (1 << 2)
#define I2C_SDA_STUCK_RECOVERY_ENABLE (1 << 3)

#define I2C_STATUS_ACTIVITY (1 << 0)

#define ABRT_7BIT_ADDR_NOACK   0
#define ABRT_10BIT_ADDR1_NOACK 1
#define ABRT_10BIT_ADDR2_NOACK 2
#define ABRT_TXDATA_NOACK      3
#define ABRT_GCALL_NOACK       4
#define ABRT_GCALL_READ        5
#define ABRT_SBYTE_ACKDET      7
#define ABRT_SBYTE_NORSTRT     9
#define ABRT_10BIT_RD_NORSTRT  10
#define ABRT_MASTER_DIS        11
#define ABRT_LOST              12
#define ABRT_SLVFLUSH_TXFIFO   13
#define ABRT_SLV_ARBLOST       14
#define ABRT_SLVRD_INTX        15
#define ABRT_USER_ABRT         16
#define ABRT_SDA_STUCK_AT_LOW  17

#define I2C_ABRT_7BIT_ADDR_NOACK   (1 << 0)
#define I2C_ABRT_10BIT_ADDR1_NOACK (1 << 1)
#define I2C_ABRT_10BIT_ADDR2_NOACK (1 << 2)
#define I2C_ABRT_TXDATA_NOACK      (1 << 3)
#define I2C_ABRT_GCALL_NOACK       (1 << 4)
#define I2C_ABRT_GCALL_READ        (1 << 5)
#define I2C_ABRT_SBYTE_ACKDET      (1 << 7)
#define I2C_ABRT_SBYTE_NORSTRT     (1 << 9)
#define I2C_ABRT_10BIT_RD_NORSTRT  (1 << 10)
#define I2C_ABRT_MASTER_DIS        (1 << 11)
#define I2C_ABRT_LOST              (1 << 12)
#define I2C_ABRT_SLVFLUSH_TXFIFO   (1 << 13)
#define I2C_ABRT_SLV_ARBLOST       (1 << 14)
#define I2C_ABRT_SLVRD_INTX        (1 << 15)
#define I2C_ABRT_USER_ABRT         (1 << 16)
#define I2C_ABRT_SDA_STUCK_AT_LOW  (1 << 17)

#define I2C_ENABLE_MASTER_DISABLE_SLAVE (0x3)

#define I2C_FIFO_DEPTH       8
#define I2C_TXFIFO_THRESHOLD (I2C_FIFO_DEPTH / 2 - 1)
#define I2C_RXFIFO_THRESHOLD (I2C_FIFO_DEPTH / 2)

#define I2C_INTR_MASTER_TX_MASK \
    (I2C_INTR_TX_EMPTY | I2C_INTR_TX_ABRT | I2C_INTR_STOP_DET)

#define I2C_INTR_MASTER_RX_MASK \
    (I2C_INTR_RX_UNDER | I2C_INTR_RX_FULL | I2C_INTR_STOP_DET)

#define I2C_INTR_SLAVE_TX_MASK \
    (I2C_INTR_RD_REQ | I2C_INTR_RX_DONE | I2C_INTR_STOP_DET)

#define I2C_INTR_SLAVE_RX_MASK \
    (I2C_INTR_RX_FULL | I2C_INTR_RX_UNDER | I2C_INTR_STOP_DET)

#define FS_MIN_SCL_HIGH 600
#define FS_MIN_SCL_LOW  1300
#define SS_MIN_SCL_HIGH 4000
#define SS_MIN_SCL_LOW  4700

#define I2C_TIMEOUT_DEF_VAL 1000

static inline void aic_i2c_module_enable(unsigned long reg_base)
{
    uint32_t reg_val;

    reg_val = readl(reg_base + I2C_ENABLE);
    reg_val |= I2C_ENABLE_BIT;
    writel(reg_val, reg_base + I2C_ENABLE);
}

static inline void aic_i2c_module_disable(unsigned long reg_base)
{
    uint32_t reg_val;

    writel(0x100, reg_base + I2C_INTR_CLR);
    reg_val = readl(reg_base + I2C_ENABLE);
    reg_val &= ~I2C_ENABLE_BIT;
    writel(reg_val, reg_base + I2C_ENABLE);
}

static inline unsigned long aic_i2c_module_status(unsigned long reg_base)
{
    return readl(reg_base + I2C_ENABLE_STATUS) & 1;
}

static inline void aic_i2c_transmit_data(unsigned long reg_base, uint16_t data)
{
    writel(data, reg_base + I2C_DATA_CMD);
}

static inline void aic_i2c_transmit_data_with_cmd(unsigned long reg_base,
                                                  unsigned long data)
{
    writel(data, reg_base + I2C_DATA_CMD);
}

static inline void aic_i2c_transmit_data_with_stop_bit(unsigned long reg_base,
                                                       uint8_t data)
{
    uint32_t reg_val;

    reg_val = I2C_DATA_CMD_STOP | data;
    writel(reg_val, reg_base + I2C_DATA_CMD);
}

static inline unsigned long
aic_i2c_get_transmit_fifo_num(unsigned long reg_base)
{
    return readl(reg_base + I2C_TXFLR);
}

static inline void aic_i2c_transfer_stop_bit(unsigned long reg_base)
{
    writel(I2C_DATA_CMD_STOP, reg_base + I2C_DATA_CMD);
}

static inline void aic_i2c_read_data_cmd(unsigned long reg_base)
{
    writel(I2C_DATA_CMD_READ, reg_base + I2C_DATA_CMD);
}

static inline void aic_i2c_read_data_cmd_with_stop_bit(unsigned long reg_base)
{
    writel(I2C_DATA_CMD_READ | I2C_DATA_CMD_STOP, reg_base + I2C_DATA_CMD);
}

static inline unsigned long aic_i2c_get_receive_fifo_num(unsigned long reg_base)
{
    return readl(reg_base + I2C_RXFLR);
}

static inline uint8_t aic_i2c_get_receive_data(unsigned long reg_base)
{
    return readb(reg_base + I2C_DATA_CMD);
}

static inline void
aic_i2c_read_data_cmd_with_restart_stop_bit(unsigned long reg_base)
{
    writel(I2C_DATA_CMD_READ | I2C_DATA_CMD_STOP | I2C_DATA_CMD_RESTART,
           reg_base + I2C_DATA_CMD);
}

static inline void
aic_i2c_read_data_cmd_with_restart_bit(unsigned long reg_base)
{
    writel(I2C_DATA_CMD_READ | I2C_DATA_CMD_RESTART, reg_base + I2C_DATA_CMD);
}

static inline unsigned long aic_i2c_set_read_cmd(unsigned long reg_val)
{
    return (reg_val | I2C_DATA_CMD_READ);
}

static inline unsigned long aic_i2c_set_stop_bit(unsigned long reg_val)
{
    return (reg_val | I2C_DATA_CMD_STOP);
}

static inline unsigned long aic_i2c_set_restart_bit(unsigned long reg_val)
{
    return (reg_val | I2C_DATA_CMD_RESTART);
}

static inline void aic_i2c_set_restart_bit_with_data(unsigned long reg_base,
                                                     uint8_t data)
{
    writel(data | I2C_DATA_CMD_RESTART, reg_base + I2C_DATA_CMD);
}

static inline unsigned long
aic_i2c_get_raw_interrupt_state(unsigned long reg_base)
{
    return readl(reg_base + I2C_INTR_RAW_STAT);
}

static inline unsigned long aic_i2c_get_interrupt_state(unsigned long reg_base)
{
    return readl(reg_base + I2C_INTR_CLR);
}

static inline void aic_i2c_disable_all_irq(unsigned long reg_base)
{
    writel(0, reg_base + I2C_INTR_MASK);
}

static inline void aic_i2c_clear_irq_flags(unsigned long reg_base,
                                           unsigned long flags)
{
    writel(flags, reg_base + I2C_INTR_CLR);
}

static inline void aic_i2c_clear_all_irq_flags(unsigned long reg_base)
{
    writel(0xffff, reg_base + I2C_INTR_CLR);
}

static inline void aic_i2c_clear_rx_full_flag(unsigned long reg_base)
{
    writel(I2C_INTR_RX_FULL, reg_base + I2C_INTR_CLR);
}

static inline void aic_i2c_clear_tx_empty_flag(unsigned long reg_base)
{
    writel(I2C_INTR_TX_EMPTY, reg_base + I2C_INTR_CLR);
}

static inline void aic_i2c_master_enable_transmit_irq(unsigned long reg_base)
{
    writel(I2C_INTR_MASTER_TX_MASK, reg_base + I2C_INTR_MASK);
}

static inline void aic_i2c_master_enable_receive_irq(unsigned long reg_base)
{
    writel(I2C_INTR_MASTER_RX_MASK, reg_base + I2C_INTR_MASK);
}

static inline void aic_i2c_slave_enable_transmit_irq(unsigned long reg_base)
{
    writel(I2C_INTR_SLAVE_TX_MASK, reg_base + I2C_INTR_MASK);
}

static inline void aic_i2c_slave_enable_receive_irq(unsigned long reg_base)
{
    writel(I2C_INTR_SLAVE_RX_MASK, reg_base + I2C_INTR_MASK);
}

static inline void aic_i2c_set_transmit_fifo_threshold(unsigned long reg_base)
{
    writel(I2C_TXFIFO_THRESHOLD, reg_base + I2C_TX_TL);
}

static inline void aic_i2c_set_receive_fifo_threshold(unsigned long reg_base,
                                                      uint8_t level)
{
    writel(level - 1, reg_base + I2C_RX_TL);
}

/**
  \brief       I2C initialization: clock enable and release reset signal
  \param[in]   i2c_idx     i2c index number
  \return      0, if success, error code if failed
*/
int aic_i2c_init(int32_t i2c_idx);


void hal_i2c_set_hold(unsigned long reg_base, u32 val);

/**
  \brief       Configure i2c master mode or slave mode
  \param[in]   reg_base     iic controller register base
  \param[in]   mode         if true, master mode; if false, slave mode
  \return      0, if success, error code if failed
*/
int aic_i2c_set_master_slave_mode(unsigned long reg_base, uint8_t mode);

/**
  \brief       Configure i2c master address mode
  \param[in]   reg_base     iic controller register base
  \param[in]   enable       if true, 10bit address mode;
  			    if false, 7bit address mode
  \return      0, if success, error code if failed
*/
int aic_i2c_master_10bit_addr(unsigned long reg_base, uint8_t enable);

/**
  \brief       Configure i2c slave address mode
  \param[in]   reg_base     iic controller register base
  \param[in]   enable       if true, 10bit address mode;
  			    if false, 7bit address mode
  \return      0, if success, error code if failed
*/
int aic_i2c_slave_10bit_addr(unsigned long reg_base, uint8_t enable);

/**
  \brief       Configure i2c speed mode
  \param[in]   reg_base     iic controller register base
  \param[in]   mode         if true, fast mode; if false, standard mode
  \return      0, if success, error code if failed
*/
int aic_i2c_speed_mode_select(unsigned long reg_base, uint32_t clk_freq,
                              uint8_t mode);

/**
  \brief       Configure target device address
  \param[in]   reg_base     iic controller register base
  \param[in]   addr         target address
  \return      0, if success, error code if failed
*/
void aic_i2c_target_addr(unsigned long reg_base, uint32_t addr);

/**
  \brief       Configure i2c own address in slave mode
  \param[in]   reg_base     iic controller register base
  \param[in]   addr         i2c own address
  \return      0, if success, error code if failed
*/
int aic_i2c_slave_own_addr(unsigned long reg_base, uint32_t addr);

int32_t aic_i2c_master_send_msg(unsigned long reg_base, struct aic_i2c_msg *msg);
int32_t aic_i2c_master_receive_msg(unsigned long reg_base,
                                   struct aic_i2c_msg *msg);

#ifdef __cplusplus
}
#endif

#endif /* _AIC_HAL_I2C_ */
