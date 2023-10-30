/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#ifndef _DRV_CIR_H_
#define _DRV_CIR_H_

#include "hal_cir.h"


typedef enum cir_protocol {
        CIR_PROTOCOL_NEC = 0U,
        CIR_PROTOCOL_RC5 = 1U,
} cir_protocol_t;

typedef struct cir_config {
        cir_protocol_t      protocol;
        uint32_t            tx_duty;
        uint32_t            rx_level;  /* Indicates the idle level of RX */
} cir_config_t;

typedef struct aic_cir
{
    struct rt_device    dev;
    aic_cir_ctrl_t      aic_cir_ctrl;
    cir_config_t        config;
} aic_cir_t;

#define IOC_CIR_CONFIGURE		1

#define CIR_RX_DONE 			1
#define CIR_RX_ERROR			0

#endif
