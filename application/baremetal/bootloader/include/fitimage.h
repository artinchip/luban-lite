/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */
#ifndef __FITIMAGE_H__
#define __FITIMAGE_H__

#include "libfdt.h"

struct spl_fit_info {
    const void *fit;    /* Pointer to a valid FIT blob */
    size_t ext_data_offset; /* Offset to FIT external data (end of FIT) */
    int images_node;    /* FDT offset to "/images" node */
    int conf_node;      /* FDT offset to selected configuration node */
};

/**
 * Information required to load data from a device
 *
 * @dev: Pointer to the device, e.g. struct mmc *
 * @priv: Private data for the device
 * @bl_len: Block length for reading in bytes
 */
struct spl_load_info {
    void *dev;
    void *priv;
    int bl_len;
};


#define FIT_IMAGES_PATH     "/images"
#define FIT_CONFS_PATH      "/configurations"

/* image node */
#define FIT_DATA_PROP       "data"
#define FIT_DATA_POSITION_PROP  "data-position"
#define FIT_DATA_OFFSET_PROP    "data-offset"
#define FIT_DATA_SIZE_PROP  "data-size"
#define FIT_TIMESTAMP_PROP  "timestamp"
#define FIT_DESC_PROP       "description"
#define FIT_ARCH_PROP       "arch"
#define FIT_TYPE_PROP       "type"
#define FIT_OS_PROP     "os"
#define FIT_COMP_PROP       "compression"
#define FIT_ENTRY_PROP      "entry"
#define FIT_LOAD_PROP       "load"

/* configuration node */
#define FIT_KERNEL_PROP     "kernel"
#define FIT_RAMDISK_PROP    "ramdisk"
#define FIT_FDT_PROP        "fdt"
#define FIT_LOADABLE_PROP   "loadables"
#define FIT_DEFAULT_PROP    "default"
#define FIT_SETUP_PROP      "setup"
#define FIT_FPGA_PROP       "fpga"
#define FIT_FIRMWARE_PROP   "firmware"
#define FIT_STANDALONE_PROP "standalone"

static inline const char *fit_get_name(const void *fit_hdr,
        int noffset, int *len)
{
    return fdt_get_name(fit_hdr, noffset, len);
}

#define uimage_to_cpu(x)    __REV(x)
#define FIT_ALIGN(size, align)      (((size) + (align) - 1) & ~((align) - 1))

int spl_load_simple_fit(struct spl_load_info *info, ulong *entry_point);

#endif
