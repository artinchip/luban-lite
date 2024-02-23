/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _ARTINCHIP_HAL_MTOP_SPEC_DEF_V12H_
#define _ARTINCHIP_HAL_MTOP_SPEC_DEF_V12H_

/* Each group is 8 bits */
#define PORT_BITMAP		     0x3f0000

extern uint8_t group_id[];
extern uint8_t grp;
extern uint8_t prt;

extern const char * grp_name[];
extern const char * prt_name[];

#define MTOP_GROUP_MAX  1
#define MTOP_PORT_MAX   6

#endif
