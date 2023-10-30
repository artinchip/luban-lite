/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#include "aic_flash_xip_def.h"

#ifdef AIC_QSPI_DRV_V11

static struct xip_device xip_flashs[] = {
    {
        .name = "W25Q256JV",
        .flash_id = 0xef7019,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_1, // Only need when use cmd 0xC0
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0xF0,
                        .normal = 0x00,
                },
        },
    },
    {
        .name = "ZB25VQ128", // verify ok.
        .flash_id = 0X5E4018,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4, // Only need when use cmd 0xC0
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0xf0,
                        .normal = 0x00,
                },
        },
    }, //.name = "ZB25VQ128",
    {
        .name = "ZB25VQ64", // verify ok.
        .flash_id = 0X5E4017,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4, // Only need when use cmd 0xC0
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0xf0,
                        .normal = 0x00,
                },
        },
    }, //.name = "ZB25VQ64",
    {
        .name = "W25Q64CV", //verify ok //W25Q64jv
        .flash_id = 0xef4017,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4,
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0x20,
                        .normal = 0x00,
                },
        },
    }, //.name = "W25Q64CV",
    {
        .name = "GD25Q128E", //verify ok
        .flash_id = 0xc84018,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4,
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0x20,
                        .normal = 0x00,
                },
        },
    }, //.name = "GD25Q128E",
    {
        .name = "XT25F64B-S", //verify ok
        .flash_id = 0xb4017,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4,
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0x20,
                        .normal = 0x00,
                },
        },
    }, //.name = "XT25F64B-S",
    {
        .name = "XT25F128B", //verify ok
        .flash_id = 0xb4018,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4,
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0x20,
                        .normal = 0x00,
                },
        },
    }, //.name = "XT25F128B",
    {
        .name = "IS25LP128F", //verify failed
        .flash_id = 0x9d6018,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4,
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0x20,
                        .normal = 0x00,
                },
        },
    }, //.name = "IS25LP128F",
    {
        .name = "MX25L12835F", //verify failed
        .flash_id = 0xc22018,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4,
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0x20,
                        .normal = 0x00,
                },
        },
    }, //.name = "MX25L12835F",
    {
        .name = "BY25Q128ASSIG", //verify failed
        .flash_id = 0x684018,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4,
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0x20,
                        .normal = 0x00,
                },
        },
    }, //.name = "BY25Q128ASSIG",
    {
        .name = "BY25Q64ASSIG", //verify failed
        .flash_id = 0x684017,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4,
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0x20,
                        .normal = 0x00,
                },
        },
    }, //.name = "BY25Q64ASSIG",
    {
        .name = "FM25Q64", //verify ok
        .flash_id = 0xa14017,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4,
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0x20,
                        .normal = 0x00,
                },
        },
    }, //.name = "FM25Q64",
    {
        .name = "MX25L6433FM2I", //verify failed
        .flash_id = 0xc22017,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4,
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0x20,
                        .normal = 0x00,
                },
        },
    }, //.name = "MX25L6433FM2I",
    {
        .name = "W25Q128JVSIQ", //verify ok
        .flash_id = 0xef4018,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4,
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0x20,
                        .normal = 0x00,
                },
        },
    }, //.name = "W25Q128JVSIQ",
    {
        .name = "GD25Q64ESIGR", //verify ok
        .flash_id = 0xc84017,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4,
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0x20,
                        .normal = 0x00,
                },
        },
    }, //.name = "GD25Q64ESIGR",
    {
        .name = "ZD25Q64B", //verify ok
        .flash_id = 0xba3217,
        .proto_support = CMD_PROTO_QIO|CMD_PROTO_CACHE_EN|CMD_PROTO_CACHE_DIS,
        .burst_cfg = {
                .cmd_set_burst = 0x77,
                .cmd_dummy_byte = 3,
                .cmd_bits_width = XIP_C0_BITS_WIDTH_4,
                .wrap_en = XIP_BURST_WRAPPED_WITH_FIXED_LEN,
                .wrap = {
                        .fixed_len = 0x40,
                        .disable = 0x10,
                        .auto_wl64 = 0x60,
                        .auto_wl32 = 0x40,
                        .auto_wl16 = 0x20,
                        .auto_wl8 = 0x00,
                },
        },
        .read_cfg = {
                .read_cmd = 0xEB,
                .dummy_byte = 2,
                .addr_mode = SPINOR_ADDR_MODE_3BYTE,
                .read_cmd_bypass_en = 0,
                .mode = {
                        .bypass = 0x20,
                        .normal = 0x00,
                },
        },
    }, //.name = "ZD25Q64B",
};

struct xip_device *get_xip_device_cfg(u32 flash_id, u32 support_msk, u32 support_val)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(xip_flashs); i++) {
        if (xip_flashs[i].flash_id != flash_id)
            continue;
        if ((xip_flashs[i].proto_support & support_msk) != support_val)
            continue;
        return &xip_flashs[i];
    }
    return NULL;
}

#endif // AIC_QSPI_DRV_V11
