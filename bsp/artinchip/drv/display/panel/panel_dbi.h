/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PANEL_DBI_H_
#define _PANEL_DBI_H_

#include <aic_hal_dbi.h>

#include "panel_com.h"

int panel_dbi_default_enable(struct aic_panel *panel);

int panel_dbi_commands_execute(struct aic_panel *panel,
                    struct panel_dbi_commands *commands);

#endif /* _PANEL_DBI_H_ */
