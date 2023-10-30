/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */
#include "spinand.h"

struct spinand_manufacturer_ops {
    const struct aic_spinand_info *(*detect)(struct aic_spinand *flash);
    int (*init)(struct aic_spinand *flash);
    void (*cleanup)(void);
};

struct spinand_manufacturer {
    u8 id;
    char *name;
    const struct spinand_manufacturer_ops *ops;
};
