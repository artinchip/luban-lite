/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _SDMC_DISK_H_
#define _SDMC_DISK_H_

#include <stdint.h>
#include <ff.h>
#include "diskio.h"
#include "mmc.h"

/*!
 * @addtogroup SDMC Disk
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*************************************************************************************************
 * API
 ************************************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name SDMC Disk Function
 * @{
 */

/*!
 * @brief Initializes SDMC disk.
 *
 * @param pdrv Physical drive number.
 * @param device_name the name of device which includes a file system.
 * @retval STA_NOINIT Failed.
 * @retval RES_OK Success.
 */
DSTATUS sdmc_disk_initialize(uint8_t pdrv, const char *device_name);

/*!
 * Gets SDMC disk status
 *
 * @param pdrv Physical drive number.
 * @param device_name the name of device which includes a file system.
 * @retval STA_NOINIT Failed.
 * @retval RES_OK Success.
 */
DSTATUS sdmc_disk_status(uint8_t pdrv, const char *device_name);

/*!
 * @brief Reads SDMC disk.
 *
 * @param pdrv Physical drive number.
 * @param device_name the name of device which includes a file system.
 * @param buf The data buffer pointer to store read content.
 * @param sector The start sector number to be read.
 * @param cnt The sector count to be read.
 * @retval RES_PARERR Failed.
 * @retval RES_OK Success.
 */
DRESULT sdmc_disk_read(uint8_t pdrv, const char *device_name, uint8_t *buf, uint32_t sector, uint8_t cnt);

/*!
 * @brief Writes SDMC disk.
 *
 * @param pdrv Physical drive number.
 * @param device_name the name of device which includes a file system.
 * @param buf The data buffer pointer to store write content.
 * @param sector The start sector number to be written.
 * @param cnt The sector count to be written.
 * @retval RES_PARERR Failed.
 * @retval RES_OK Success.
 */
DRESULT sdmc_disk_write(uint8_t pdrv, const char *device_name, const uint8_t *buf, uint32_t sector, uint8_t cnt);

/*!
 * @brief SDMC disk IO operation.
 *
 * @param pdrv Physical drive number.
 * @param device_name the name of device which includes a file system.
 * @param command The command to be set.
 * @param buf The buffer to store command result.
 * @retval RES_PARERR Failed.
 * @retval RES_OK Success.
 */
DRESULT sdmc_disk_ioctl(uint8_t pdrv, const char *device_name, uint8_t command, void *buf);

/* @} */
#if defined(__cplusplus)
}
#endif

#endif /* _SDMC_DISK_H_ */
