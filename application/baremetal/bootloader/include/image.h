/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wudehuang <dehuang.wu@artinchip.com>
 */

#ifndef __BL_IMAGE_H_
#define __BL_IMAGE_H_

#include <aic_common.h>

#define AIC_IMAGE_MAGIC 0x20434941 /* Magic is 'AIC ' */

#define AIC_FW_BLOCK1_SIZ        256
#define AIC_FW_SIGN_ALGO_NONE    0
#define AIC_FW_SIGN_ALGO_RSA2048 1

#define AIC_FW_ENCRYPT_ALGO_NONE    0
#define AIC_FW_ENCRYPT_ALGO_AES_CBC 1

struct image_header {
    /* Magic value, should be "AIC " */
    u32 magic;
    /* Image checksum value */
    u32 checksum;
    /* Image header version */
    u32 head_ver;
    /* Image size from start to end */
    u32 image_len;
    /* Fireware's version information: Anti-rollback version counter */
    u8 fwver_counter;
    u8 fwver_revision;
    u8 fwver_minor;
    u8 fwver_major;
    /* Firmware binary's length */
    u32 firmware_len;
    /* Firmware should be loaded this address */
    u32 load_address;
    /* Firmware's entry point */
    u32 entry_point;
    /* If firmware is signed by RSA, then specify the algorithm here */
    u32 sign_algo;
    /* If firmware is encrypted, then specify the algorithm here */
    u32 encrypt_algo;
    /* The position of signature, offset from image start */
    u32 sign_data_offset;
    /* The length of signature */
    u32 sign_data_len;
    /* The position of public key used when sign this firmware, offset from
	 * image head. */
    u32 sign_key_offset;
    /* The public key length */
    u32 sign_key_len;
    /* If encryption algorithm is AES-CBC, then IV is required. */
    u32 iv_data_offset;
    /* The IV data length */
    u32 iv_data_len;
};

int image_verify_magic(u8 *fw_base, u32 magic);
int image_verify_checksum(u8 *fw_base, u32 size);
int image_authentication(u8 *fw_base);
u32 image_calc_checksum(u8 *buf, u32 size);
u32 image_get_fw_len(u8 *fw_base);
void *image_get_entry_point(u8 *fw_base);
void *image_get_load_address(u8 *fw_base);

#endif /* __BL_IMAGE_H_ */
