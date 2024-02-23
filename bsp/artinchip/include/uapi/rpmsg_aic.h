/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __RPMSG_AIC_H__
#define __RPMSG_AIC_H__

#define RPMSG_CMD_DEF(m, c)     ((m) << 8 | (c))
#define RPMSG_IS_USER(cmd)      ((cmd) >> 8 == 0xAA || (cmd) >> 8 == 0x55)

/* Common command */

#define RPMSG_CMD_ACK           RPMSG_CMD_DEF('a', 0) /* Acknowledgement */
#define RPMSG_CMD_NACK          RPMSG_CMD_DEF('a', 1) /* Negative Ack */
#define RPMSG_CMD_IS_IDLE       RPMSG_CMD_DEF('a', 2)
#define RPMSG_CMD_PRE_STANDBY   RPMSG_CMD_DEF('a', 3) /* Prepare to standby */
#define RPMSG_CMD_SYS_RDY       RPMSG_CMD_DEF('a', 0) /* System is ready */

/* CSYS is receiver */

#define RPMSG_CMD_GPIO_CFG      RPMSG_CMD_DEF('c', 1) /* config the GPIO */

/* SESS is receiver */

/* Only in BROM environment */
#define RPMSG_CMD_EXEC          RPMSG_CMD_DEF('e', 0) /* Jump to an address */
#define RPMSG_CMD_DO_STANDBY    RPMSG_CMD_DEF('e', 1)
#define RPMSG_CMD_DO_RESUME     RPMSG_CMD_DEF('e', 2)
#define RPMSG_CMD_EFUSE_RD      RPMSG_CMD_DEF('e', 3) /* Read eFuse */
#define RPMSG_CMD_EFUSE_WR      RPMSG_CMD_DEF('e', 4) /* Write eFuse */
/* Only in APP environment */
#define RPMSG_CMD_REQ_STANDBY   RPMSG_CMD_DEF('e', 5) /* Request to enter standby */
#define RPMSG_CMD_DDR_RDY       RPMSG_CMD_DEF('e', 5) /* DDR is ready */

/* SCSS is receiver */

#define RPMSG_CMD_SCAN_START    RPMSG_CMD_DEF('s', 0)
#define RPMSG_CMD_SCAN_STOP     RPMSG_CMD_DEF('s', 1)

/* SPSS is receiver */

#define RPMSG_CMD_PRINT_START   RPMSG_CMD_DEF('p', 0)
#define RPMSG_CMD_PRINT_STOP    RPMSG_CMD_DEF('p', 1)

struct aic_rpmsg {
    uint16_t cmd;
    uint8_t  seq;
    uint8_t  len;    /* length of data[], unit: dword */
    uint32_t data[]; /* length varies according to the scene */
};

#define AIC_RPMSG_REAL_SIZE(data_len)       (4 + (data_len) * 4)

#endif

