/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Notes
 * 2023-01-06     the first version
 */

#ifndef __FT7411_H__
#define __FT7411_H__

#include "touch.h"
#include "aic_hal_gpio.h"

#define FT7411_SALVE_ADDR       0x28        /* i2c device slave addr*/
#define FT_DEVIDE_MODE          0x00        /* FT7411 mode register */
#define FT_REG_NUM_FINGER       0x02        /* touch status reg */
#define FT_TP1_REG              0X03        /* first touch data addr */
#define FT_TP2_REG              0X09        /* second touch data addr */
#define FT_TP3_REG              0X0F        /* third touch data addr */
#define FT_TP4_REG              0X15        /* fourth touch data addr */
#define FT_TP5_REG              0X1B        /* fifth touch data addr */
#define FT_ID_G_THGROUP         0x80        /* valid touch detect threshold */
#define FT_ID_G_PERIODACTIVE    0x88        /* active state cycle setting reg */
#define FT_ID_G_LIB_VERSION     0xA1        /* version */
#define FT_ID_G_MODE            0xA4        /* interrupt mode setting */
#define Chip_Vendor_ID          0xA3        /* Chip vendor ID */
#define ID_G_FT6236ID           0xA8        /* CTPM Vendor ID */

#define FT7411_I2C_CHAN  AIC_TOUCH_PANEL_FT7411_I2C_CHA
#define FT7411_RST_PIN   AIC_TOUCH_PANEL_FT7411_RST_PIN
#define FT7411_INT_PIN   AIC_TOUCH_PANEL_FT7411_INT_PIN
#define FT7411_X_RANGE   AIC_TOUCH_PANEL_FT7411_X_RANGE
#define FT7411_Y_RANGE   AIC_TOUCH_PANEL_FT7411_Y_RANGE

#endif /* _FT7411_H_ */
