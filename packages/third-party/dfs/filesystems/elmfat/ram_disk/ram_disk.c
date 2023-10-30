/*
 * The Clear BSD License
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 * All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// #include "fsl_common.h"
#include "ff.h"
#include "ram_disk.h"
#include "ffconf.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* clang-format off */
#define SECTOR_SIZE 512 /* usualy 512 B */
#define DISK_SIZE 128*1024 /* minmal disk size calculated as 256 * _MIN_SS */
/* clang-format on */

/*******************************************************************************
 * Globals
 ******************************************************************************/
static uint8_t disk_space[DISK_SIZE];

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Get RAM disk status.
 */
DSTATUS ram_disk_status(BYTE pdrv)
{
    return 0;
}

/*!
 * @brief Inidialize a RAM disk.
 */
DSTATUS ram_disk_initialize(BYTE pdrv)
{
    return 0;
}

/*!
 * @brief Read Sector(s) from RAM disk.
 */
DRESULT ram_disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    memcpy(buff, disk_space + sector * SECTOR_SIZE, SECTOR_SIZE * count);
    return RES_OK;
}

/*!
 * @brief Write Sector(s) to RAM disk.
 */
DRESULT ram_disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    memcpy(disk_space + sector * SECTOR_SIZE, buff, SECTOR_SIZE * count);
    return RES_OK;
}

/*!
 * @brief Miscellaneous RAM disk Functions.
 */
DRESULT ram_disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    switch (cmd) {
        case GET_SECTOR_COUNT:
            *(uint32_t *)buff = DISK_SIZE / SECTOR_SIZE;
            return RES_OK;
            break;

        case GET_SECTOR_SIZE:
            *(uint32_t *)buff = SECTOR_SIZE;
            return RES_OK;
            break;

        case CTRL_SYNC:
            return RES_OK;
            break;

        default:
            break;
    }

    return RES_PARERR;
}
