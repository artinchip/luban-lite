/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "fb_disp_info.h"
#include "tkc/mem.h"
#include "base/lcd.h"
#include "tkc/thread.h"
#include "awtk_global.h"
#include "tkc/time_now.h"
#include "tkc/mutex.h"
#include "tkc/semaphore.h"
#include "blend/image_g2d.h"
#include "base/system_info.h"
#include "lcd/lcd_mem_bgr565.h"
#include "lcd/lcd_mem_rgb565.h"
#include "lcd/lcd_mem_bgra8888.h"
#include "lcd/lcd_mem_rgba8888.h"
#include "lcd/lcd_mem_bgr888.h"
#include "lcd/lcd_mem_rgb888.h"
#include "base/lcd_orientation_helper.h"

#ifdef WITH_AIC_G2D
#include "aic_g2d.h"
#include "aic_rtos_mem.h"

static cma_buffer lcd_buffer[2];
#endif

static fb_info_t s_fb;

int lcd_format_get(void)
{
  fb_info_t* fb = &s_fb;
  return fb_format(fb);
}

int lcd_size_get(void) {
  fb_info_t* fb = &s_fb;
  return fb_size(fb) * fb_number(fb);
}

static bool_t lcd_rtos_fb_open(fb_info_t* fb) {
  if (tk_fb_disp_init(fb) == RET_OK) {
    return TRUE;
  }
  return FALSE;
}

static void lcd_rtos_fb_close(fb_info_t* fb) {
  tk_fb_disp_exit(fb);
}

static void on_app_exit(void) {
  fb_info_t* fb = &s_fb;

#ifdef WITH_AIC_G2D
  tk_aic_g2d_close();

  aic_cma_buf_del_ge(lcd_buffer[0].buf_head);
  aic_cma_buf_del_ge(lcd_buffer[1].buf_head);
  aic_cma_buf_close();
#endif

  lcd_rtos_fb_close(fb);

  log_info("on_app_exit\n");
}

static ret_t (*lcd_mem_rtos_flush_default)(lcd_t* lcd);
static ret_t lcd_mem_rtos_flush(lcd_t* lcd) {
  fb_info_t* fb = &s_fb;
  int dummy = 0;
  mpp_fb_ioctl(fb->fb, AICFB_WAIT_FOR_VSYNC, &dummy);
  if (lcd_mem_rtos_flush_default) {
    lcd_mem_rtos_flush_default(lcd);
  }

  return RET_OK;
}

static ret_t lcd_swap_sync(lcd_t* lcd) {
  static int swap_lcd = 1;
  fb_info_t* fb = &s_fb;

  mpp_fb_ioctl(fb->fb, AICFB_PAN_DISPLAY, &swap_lcd);
  mpp_fb_ioctl(fb->fb, AICFB_WAIT_FOR_VSYNC, 0);

  swap_lcd = swap_lcd == 1 ? 0 : 1;

  return RET_OK;
}

static void lcd_swap_fb(lcd_t* lcd) {
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  uint8_t* tmp_fb = mem->offline_fb;

  lcd_mem_set_offline_fb(mem, mem->online_fb);
  lcd_mem_set_online_fb(mem, tmp_fb);
}

static ret_t lcd_mem_rtos_swap(lcd_t* lcd) {
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  uint8_t* tmp_fb = mem->offline_fb;
  fb_info_t* fb = &s_fb;
  static int swap_lcd = 1;

  /* before hardware operations, it is necessary to clear the dcache to prevent CPU write backs. */
  aicos_dcache_clean_invalid_range((unsigned long *)mem->offline_fb, (unsigned long)fb_size(fb));
  aicos_dcache_clean_invalid_range((unsigned long *)mem->online_fb, (unsigned long)fb_size(fb));

  if (lcd_swap_sync(lcd) != RET_OK)
    log_error("lcd_swap_sync failed\n");

  lcd_swap_fb(lcd);

  return RET_OK;
}

static ret_t lcd_mem_init_drawing_fb(lcd_t* lcd, bitmap_t* fb) {
  lcd_mem_t* mem = (lcd_mem_t*)lcd;

  if (fb != NULL) {
    memset(fb, 0x00, sizeof(bitmap_t));

    fb->format = mem->format;
    fb->buffer = mem->offline_gb;
    fb->w = lcd_get_physical_width(lcd);
    fb->h = lcd_get_physical_height(lcd);
    graphic_buffer_attach(mem->offline_gb, mem->offline_fb, fb->w, fb->h);
    bitmap_set_line_length(fb, mem->line_length);
  }

  return RET_OK;
}

static bitmap_t* lcd_mem_init_online_fb(lcd_t* lcd, bitmap_t* fb, lcd_orientation_t o) {
  uint32_t w = 0;
  uint32_t h = 0;
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  uint32_t bpp = bitmap_get_bpp_of_format(BITMAP_FMT_BGR565);

  if (o == LCD_ORIENTATION_0 || o == LCD_ORIENTATION_180) {
    w = lcd_get_width(lcd);
    h = lcd_get_height(lcd);
  } else {
    log_debug("ldc swap ex mode did't support rotation\n");
  }

  memset(fb, 0x00, sizeof(bitmap_t));
  fb->w = w;
  fb->h = h;
  fb->buffer = mem->online_gb;
  fb->format = mem->format;
  graphic_buffer_attach(mem->online_gb, mem->online_fb, w, h);
  bitmap_set_line_length(fb, tk_max(fb->w * bpp, mem->online_line_length));

  return fb;
}

static ret_t lcd_mem_rtos_swap_ex(lcd_t* lcd) {
  bitmap_t online_fb;
  bitmap_t offline_fb;
  const dirty_rects_t* dirty_rects;
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  system_info_t* info = system_info();
  uint8_t* fb = lcd_mem_get_offline_fb(mem);
  lcd_orientation_t o = info->lcd_orientation;

  lcd_mem_init_drawing_fb(lcd, &offline_fb);
  lcd_mem_init_online_fb(lcd, &online_fb, o);

  if (mem->wait_vbi != NULL) {
    mem->wait_vbi(mem->wait_vbi_ctx);
  }

  if (lcd_swap_sync(lcd) != RET_OK)
    log_error("lcd_swap_sync failed\n");

  dirty_rects = lcd_fb_dirty_rects_get_dirty_rects_by_fb(&(mem->fb_dirty_rects_list), fb);
  if (dirty_rects != NULL && dirty_rects->nr > 0) {
    if (dirty_rects->disable_multiple) {
      const rect_t* dr = (const rect_t*)&(dirty_rects->max);
      if (o == LCD_ORIENTATION_0) {
        image_copy(&online_fb, &offline_fb, dr, dr->x, dr->y);
      } else {
        image_rotate(&online_fb, &offline_fb, dr, o);
      }
    } else {
      uint32_t i = 0;
      for (i = 0; i < dirty_rects->nr; i++) {
        const rect_t* dr = (const rect_t*)dirty_rects->rects + i;
        if (o == LCD_ORIENTATION_0) {
          image_copy(&online_fb, &offline_fb, dr, dr->x, dr->y);
        } else {
          image_rotate(&online_fb, &offline_fb, dr, o);
        }
      }
    }
  }

  lcd_swap_fb(lcd);

  return RET_OK;
}

static lcd_t* lcd_rtos_single_framebuffer_create(fb_info_t* fb) {
  lcd_t* lcd = NULL;
  int w = fb_width(fb);
  int h = fb_height(fb);
  int line_length = fb_line_length(fb);

  int bpp = fb_bpp(fb);
  uint8_t* online_fb = (uint8_t*)(fb->fbmem0);
  uint8_t* offline_fb = NULL;
  return_value_if_fail(offline_fb != NULL, NULL);

  offline_fb = (uint8_t*)malloc(line_length * h);

  if (bpp == 16) {
    if (fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_double_fb(w, h, online_fb, offline_fb);
    } else if (fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_double_fb(w, h, online_fb, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_double_fb(w, h, online_fb, offline_fb);
    } else if (fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_double_fb(w, h, online_fb, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    if (fb_is_bgr888(fb)) {
      lcd = lcd_mem_bgr888_create_double_fb(w, h, online_fb, offline_fb);
    } else if (fb_is_rgb888(fb)) {
      lcd = lcd_mem_rgb888_create_double_fb(w, h, online_fb, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else {
    assert(!"not supported framebuffer format.");
  }

  if (lcd != NULL) {
    lcd_mem_rtos_flush_default = lcd->flush;
    lcd->flush = lcd_mem_rtos_flush;
    lcd_mem_set_line_length(lcd, line_length);
  }

  return lcd;
}

static lcd_t* lcd_mem_create_fb(fb_info_t* fb)
{
  int w = fb_width(fb);
  int h = fb_height(fb);
  int bpp = fb_bpp(fb);
  lcd_t* lcd = NULL;

  if (bpp == 16) {
    log_info("lcd mem format is bgr565\n");
    if (fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else if (fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    log_info("lcd mem format is bgra8888\n");
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else if (fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    log_info("lcd mem format is bgr888\n");
    if (fb_is_bgr888(fb)) {
      lcd = lcd_mem_bgr888_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else if (fb_is_rgb888(fb)) {
      lcd = lcd_mem_rgb888_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else {
    assert(!"not supported framebuffer format.");
  }

  return lcd;
}

static lcd_t* lcd_rtos_double_framebuffer_create(fb_info_t* fb) {
  lcd_t* lcd = NULL;
  int h = fb_height(fb);
  int line_length = fb_line_length(fb);
#ifdef WITH_AIC_G2D
  int ret = 0;

  tk_aic_g2d_open();

  ret = aic_cma_buf_open();
  if (ret < 0) {
    assert("aic_cma_buf_open err\n");
  }

  lcd_buffer[0].type = PHY_TYPE;
  lcd_buffer[0].buf_head = (unsigned char *)fb->fbmem0; /* in an rtos, the physical address is essentially the pointer address. */
  lcd_buffer[0].phy_addr = (unsigned int)fb->fbmem0;
  lcd_buffer[0].buf = (void *)fb->fbmem0;
  lcd_buffer[0].size = line_length * h;

  aic_cma_buf_add_ge(&lcd_buffer[0]);
  if (fb_is_2fb(fb)) {
    lcd_buffer[1].type = PHY_TYPE;
    lcd_buffer[1].buf_head = (unsigned char *)fb->fbmem0 + (line_length * h);
    lcd_buffer[1].phy_addr = (unsigned int)(fb->fbmem0 + (line_length * h));
    lcd_buffer[1].buf = (void *)((unsigned int)(fb->fbmem0 + (line_length * h)));
    lcd_buffer[1].size = line_length * h;
    aic_cma_buf_add_ge(&lcd_buffer[1]);
  }

  /* aic_cma_buf_debug(AIC_CMA_BUF_DEBUG_SIZE | AIC_CMA_BUF_DEBUG_CONTEXT); */
#endif

  lcd = lcd_mem_create_fb(fb);

#ifdef WITH_LCD_FLUSH
  if (lcd != NULL) {
    log_debug("lcd mode is flush\n");
    lcd_mem_rtos_flush_default = lcd->flush;
    lcd->flush = lcd_mem_rtos_flush;
  }
#endif

#ifdef WITH_LCD_SWAP
  log_debug("lcd mode is swap\n");
  /* Only using the double framebuffer assigned by the platform */
  if (lcd != NULL) {
    lcd->swap = lcd_mem_rtos_swap;
    lcd->support_dirty_rect = FALSE;
  } else {
    assert(!"lcd create err.\n");
  }
#endif

#ifdef WITH_LCD_SWAP_EX
  log_debug("lcd mode is swap ex\n");
  /* Only using the double framebuffer assigned by the platform */
  if (lcd != NULL) {
    lcd->swap = lcd_mem_rtos_swap_ex;
    lcd->support_dirty_rect = TRUE;
  } else {
    assert(!"lcd create err.\n");
  }
#endif

  return lcd;
}

static lcd_t* lcd_rtos_create(fb_info_t* fb) {
  if (fb_is_1fb(fb)) {
    log_debug("frame buffer is singe buffer\n");
    return lcd_rtos_single_framebuffer_create(fb);
  } else {
    log_debug("frame buffer is double buffer\n");
    return lcd_rtos_double_framebuffer_create(fb);
  }
}

lcd_t* lcd_rtos_fb_create() {
  lcd_t* lcd = NULL;
  fb_info_t* fb = &s_fb;

  if (lcd_rtos_fb_open(fb)) {
    lcd = lcd_rtos_create(fb);
  } else {
    tk_fb_disp_exit(fb);
  }

  return lcd;
}

lcd_t* platform_create_lcd(wh_t w, wh_t h) {
  return lcd_rtos_fb_create();
}
