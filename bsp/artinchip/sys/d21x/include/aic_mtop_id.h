/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_MTOP_SPEC_DEF_V10H_
#define _ARTINCHIP_HAL_MTOP_SPEC_DEF_V10H_

/* Each group is 8 bits */
#define PORT_BITMAP		     0x10F

extern uint8_t group_id[];
extern uint8_t grp;
extern uint8_t prt;

extern const char * grp_name[];
extern const char * prt_name[];

#define MTOP_GROUP_MAX  2
#define MTOP_PORT_MAX   4

#endif
