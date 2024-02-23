/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#ifndef _SPINAND_DISKIO_H_
#define _SPINAND_DISKIO_H_

#include <stdint.h>
#include <ff.h>
#include "diskio.h"
#include "mtd.h"

#ifdef AIC_NFTL_SUPPORT
#include "nftl_api.h"
#endif


#if defined(__cplusplus)
extern "C" {
#endif

struct spinand_blk_device {
    struct mtd_dev *mtd_device;
    struct rt_device_blk_geometry info;
#ifdef AIC_NFTL_SUPPORT
    struct nftl_api_handler_t *nftl_handler;
#endif
    u8 *pagebuf;
    enum part_attr attr;
};

/*!
 * @name SPINAND Disk Function
 * @{
 */

/*!
 * @brief Initializes SPINAND disk.
 *
 * @param device_name the name of device which includes a file system.
 * @retval the handle of disk.
 */
void *spinand_disk_initialize(const char *device_name);

/*!
 * Gets SPINAND disk status
 *
 * @param hdisk the handle of device which includes a file system.
 * @retval STA_NOINIT Failed.
 * @retval RES_OK Success.
 */
DSTATUS spinand_disk_status(void *hdisk);

/*!
 * @brief Reads SPINAND disk.
 *
 * @param hdisk the handle of device which includes a file system.
 * @param buf The data buffer pointer to store read content.
 * @param sector The start sector number to be read.
 * @param cnt The sector count to be read.
 * @retval RES_PARERR Failed.
 * @retval RES_OK Success.
 */
DRESULT spinand_disk_read(void *hdisk, uint8_t *buf, uint32_t sector, uint8_t cnt);

/*!
 * @brief Writes SPINAND disk.
 *
 * @param hdisk the handle of device which includes a file system.
 * @param buf The data buffer pointer to store write content.
 * @param sector The start sector number to be written.
 * @param cnt The sector count to be written.
 * @retval RES_PARERR Failed.
 * @retval RES_OK Success.
 */
DRESULT spinand_disk_write(void *hdisk, const uint8_t *buf, uint32_t sector, uint8_t cnt);

/*!
 * @brief SPINAND disk IO operation.
 *
 * @param hdisk the handle of device which includes a file system.
 * @param command The command to be set.
 * @param buf The buffer to store command result.
 * @retval RES_PARERR Failed.
 * @retval RES_OK Success.
 */
DRESULT spinand_disk_ioctl(void *hdisk, uint8_t command, void *buf);

/* @} */
#if defined(__cplusplus)
}
#endif

#endif /* _SPINAND_DISKIO_H_ */

