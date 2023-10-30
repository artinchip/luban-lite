/**
 * File:   lcd_rtos_fb.h
 * Author: AWTK Develop Team
 * Brief:  rtos framebuffer lcd
 *
 * Copyright (c) 2018 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2018-09-07 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#ifndef TK_LCD_RTOS_FB_H
#define TK_LCD_RTOS_FB_H

#include "base/lcd.h"

BEGIN_C_DECLS

lcd_t* platform_create_lcd(wh_t w, wh_t h);

END_C_DECLS

#endif /*TK_LCD_RTOS_FB_H*/
