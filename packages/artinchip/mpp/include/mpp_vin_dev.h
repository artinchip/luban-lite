/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_MPP_VIN_DEV_H_
#define _ARTINCHIP_MPP_VIN_DEV_H_

/* The command of Display Bus Interface */

#define DBI_CMD_SOFT_RESET          0x01
#define DBI_CMD_SLEEP_OUT           0x11
#define DBI_CMD_DISP_OFF            0x28
#define DBI_CMD_DISP_ON             0x29
#define DBI_CMD_COL_ADDR_SET        0x2A /* width */
#define DBI_CMD_PAGE_ADDR_SET       0x2B /* height */
#define DBI_CMD_PIXEL_FMT           0x3A /* Bit3: ArtInChip customed */
#define DBI_CMD_BRIGHTNESS          0x51
#define DBI_CMD_AIC_GE_CTL          0xAC /* ArtInChip customed */
#define DBI_CMD_FR_CTL              0xB1
#define DBI_CMD_BLANK_PORCH_CTL     0xB5

#define DBI_DAT_MAX_LEN             16

#define DBI_CMD_DPI_MASK            GENMASK(6, 4)
#define DBI_CMD_DPI_SHIFT           4
#define DBI_CMD_DBI_FMT_FLAG        BIT(3)
#define DBI_CMD_DBI_MASK            GENMASK(2, 0)
#define DBI_CMD_DBI_SHIFT           0

#define DBI_CMD_GE_H_FLIP           BIT(6)
#define DBI_CMD_GE_V_FLIP           BIT(5)
#define DBI_CMD_GE_SCALE            BIT(4)
#define DBI_CMD_GE_ROT_MASK         GENMASK(1, 0)

#define DBI_CMD_FR_MASK             GENMASK(7, 4)
#define DBI_CMD_FR_SHIFT            4

#define DBI_CMD_VP_MASK             GENMASK(4, 0)

enum dpi_rgb_if {
    DPI_RGB_IF_16BIT = 5,
    DPI_RGB_IF_18BIT = 6,
    DPI_RGB_IF_24BIT = 7
};

enum dbi_mcu_if {
    DBI_MCU_IF_3BIT = 1,
    DBI_MCU_IF_16BIT = 5,
    DBI_MCU_IF_18BIT = 6,
    DBI_MCU_IF_24BIT = 7
};

enum dbi_rotation {
    DBI_CMD_ROTATION_0 = 0,
    DBI_CMD_ROTATION_90,
    DBI_CMD_ROTATION_180,
    DBI_CMD_ROTATION_270
};

#define dbi_byte2short(h, l)        ((h & 0xff) << 8 | (l & 0xff))

struct aic_dbi_cmd {
    u8 code;
    u8 data_len;
    char name[16];
    int (*proc)(u8 code, u8 *data);
};

int mpp_vin_dev_init(u32 cnt);
void mpp_vin_dev_deinit(void);

#endif
