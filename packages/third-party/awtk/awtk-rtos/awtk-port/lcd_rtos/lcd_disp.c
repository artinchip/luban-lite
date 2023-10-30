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
 * 2023-9-07 Zequan Liang <zequan.liang@artinchip.com> docking 2D hardware acceleration
 * 2023-10-10 Zequan Liang <zequan.liang@artinchip.com> add a display refresh mechanism with swap
 * 2023-10-11 Zequan Liang <zequan.liang@artinchip.com> add a display refresh mechanism with flash
 * 2023-10-16 Zequan Liang <zequan.liang@artinchip.com> adapt to rtos platform
 *
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
#endif

#ifdef WITH_LCD_FLUSH_AND_SWAP
static tk_thread_t* s_t_fbswap = NULL;
static tk_semaphore_t* s_sem_spare = NULL;
static tk_semaphore_t* s_sem_ready = NULL;
static tk_mutex_t* s_lck_fblist = NULL;
static bool_t s_app_quited = FALSE;
#endif

#ifdef WITH_AIC_G2D
#ifdef WITH_LCD_FLUSH_AND_SWAP
static cma_buffer offline_buf;
#endif
static cma_buffer online_buffer[2];
#endif

static fb_info_t s_fb;

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

#ifdef WITH_LCD_FLUSH_AND_SWAP
  s_app_quited = TRUE;
  tk_semaphore_post(s_sem_spare);
  tk_semaphore_post(s_sem_ready);
  sleep_ms(200);

  if (s_t_fbswap) {
    tk_thread_join(s_t_fbswap);
    tk_thread_destroy(s_t_fbswap);
  }

  if (s_sem_spare) {
    tk_semaphore_destroy(s_sem_spare);
  }

  if (s_sem_ready) {
    tk_semaphore_destroy(s_sem_ready);
  }

  if (s_lck_fblist) {
    tk_mutex_destroy(s_lck_fblist);
  }
#endif

#ifdef WITH_AIC_G2D
  tk_aic_g2d_close();
#ifdef WITH_LCD_FLUSH_AND_SWAP
  aic_cma_buf_del_ge(offline_buf.buf);
#endif
  aic_cma_buf_del_ge(online_buffer[0].buf);
  aic_cma_buf_del_ge(online_buffer[1].buf);
#endif

  lcd_rtos_fb_close(fb);

  log_info("on_app_exit\n");
}

static ret_t lcd_rtos_init_drawing_fb(lcd_mem_t* mem, bitmap_t* fb) {
  return_value_if_fail(mem != NULL && fb != NULL, RET_BAD_PARAMS);

  memset(fb, 0x00, sizeof(bitmap_t));

  fb->w = lcd_get_physical_width((lcd_t*)mem);
  fb->h = lcd_get_physical_height((lcd_t*)mem);
  fb->format = mem->format;
  fb->buffer = mem->offline_gb;
  graphic_buffer_attach(mem->offline_gb, mem->offline_fb, fb->w, fb->h);
  bitmap_set_line_length(fb, mem->line_length);

  return RET_OK;
}

static ret_t lcd_rtos_init_online_fb(lcd_mem_t* mem, bitmap_t* fb, uint8_t* buff, uint32_t w, uint32_t h, uint32_t line_length) {
  return_value_if_fail(mem != NULL && fb != NULL && buff != NULL, RET_BAD_PARAMS);

  memset(fb, 0x00, sizeof(bitmap_t));

  fb->w = w;
  fb->h = h;
  fb->format = mem->format;
  fb->buffer = mem->online_gb;
  graphic_buffer_attach(mem->online_gb, buff, w, h);
  bitmap_set_line_length(fb, line_length);

  return RET_OK;
}

static ret_t lcd_rtos_flush(lcd_t* base, int fbid) {
  /* log_debug("==========lcd_rtos_flush fbid=%d\n", fbid);//###DEBUG### */

  uint8_t* buff = NULL;
  fb_info_t* fb = &s_fb;
  int fb_nr = fb_number(fb);
  uint32_t size = fb_size(fb);
  lcd_mem_t* lcd = (lcd_mem_t*)base;
  const dirty_rects_t* dirty_rects = NULL;
  lcd_orientation_t o = system_info()->lcd_orientation;

  return_value_if_fail(lcd != NULL && fb != NULL && fbid < fb_nr, RET_BAD_PARAMS);

  buff = fb->fbmem0 + size * fbid;

  bitmap_t online_fb;
  bitmap_t offline_fb;
  lcd_rtos_init_drawing_fb(lcd, &offline_fb);
  lcd_rtos_init_online_fb(lcd, &online_fb, buff, fb_width(fb), fb_height(fb), fb_line_length(fb));

  lcd_fb_dirty_rects_add_fb_info(&(lcd->fb_dirty_rects_list), buff);
  lcd_fb_dirty_rects_update_all_fb_dirty_rects(&(lcd->fb_dirty_rects_list), base->dirty_rects);

  dirty_rects = lcd_fb_dirty_rects_get_dirty_rects_by_fb(&(lcd->fb_dirty_rects_list), buff);
  if (dirty_rects != NULL && dirty_rects->nr > 0) {
    for (int i = 0; i < dirty_rects->nr; i++) {
      const rect_t* dr = (const rect_t*)dirty_rects->rects + i;
        if (o == LCD_ORIENTATION_0) {
          image_copy(&online_fb, &offline_fb, dr, dr->x, dr->y);
        } else {
          image_rotate(&online_fb, &offline_fb, dr, o);
        }
    }
  }

  lcd_fb_dirty_rects_reset_dirty_rects_by_fb(&(lcd->fb_dirty_rects_list), buff);
  return RET_OK;
}

static void on_signal_int(int sig) {
  tk_quit();
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

enum {
  FB_TAG_UND = 0,
  FB_TAG_SPARE,
  FB_TAG_READY,
  FB_TAG_BUSY
};
typedef struct fb_taged {
  int fbid;
  int tags;
} fb_taged_t;

#define FB_LIST_NUM 3
static fb_taged_t s_fblist[FB_LIST_NUM];
static void init_fblist(int num) {
  memset(s_fblist, 0, sizeof(s_fblist));
  for (int i = 0; i < num && i < FB_LIST_NUM; i++) {
    s_fblist[i].fbid = i;
    s_fblist[i].tags = FB_TAG_SPARE;
  }
}
static fb_taged_t* get_spare_fb() {
  for (int i = 0; i < FB_LIST_NUM; i++) {
    if (s_fblist[i].tags == FB_TAG_SPARE) {
      return &s_fblist[i];
    }
  }
  return NULL;
}
static fb_taged_t* get_busy_fb() {
  for (int i = 0; i < FB_LIST_NUM; i++) {
    if (s_fblist[i].tags == FB_TAG_BUSY) {
      return &s_fblist[i];
    }
  }
  return NULL;
}
inline static fb_taged_t* get_ready_fb() {
  fb_taged_t* last_busy_fb = get_busy_fb();

  if (last_busy_fb) {
    /* get the first ready slot next to the busy one */
    for (int i = 1; i < FB_LIST_NUM; i++) {
      int next_ready_fbid = (last_busy_fb->fbid + i) % FB_LIST_NUM;

      if (s_fblist[next_ready_fbid].tags == FB_TAG_READY) {
        return &s_fblist[next_ready_fbid];
      }
    }
  } else {
    /* no last busy, get the first ready one */
    for (int i = 0; i < FB_LIST_NUM; i++) {
      if (s_fblist[i].tags == FB_TAG_READY) {
        return &s_fblist[i];
      }
    }
  }
  return NULL;
}

#ifdef WITH_LCD_SWAP
static ret_t lcd_mem_rtos_swap(lcd_t* lcd) {
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  uint8_t* tmp_fb = mem->offline_fb;
  fb_info_t* fb = &s_fb;
  static int swap_lcd = 1;

  /* before hardware operations, it is necessary to clear the dcache to prevent CPU write backs. */
  aicos_dcache_clean_invalid_range((unsigned long *)mem->offline_fb, (unsigned long)fb_size(fb));
  aicos_dcache_clean_invalid_range((unsigned long *)mem->online_fb, (unsigned long)fb_size(fb));

  mpp_fb_ioctl(fb->fb, AICFB_PAN_DISPLAY, &swap_lcd);
  mpp_fb_ioctl(fb->fb, AICFB_WAIT_FOR_VSYNC, 0);

  swap_lcd = swap_lcd == 1 ? 0 : 1;
  lcd_mem_set_offline_fb(mem, mem->online_fb);
  lcd_mem_set_online_fb(mem, tmp_fb);
  return RET_OK;
}
#endif

#ifdef WITH_LCD_FLUSH_AND_SWAP
static ret_t lcd_mem_rtos_write_buff(lcd_t* lcd) {
  ret_t ret = RET_OK;
  if (s_app_quited) {
    return ret;
  }

  if (lcd->draw_mode != LCD_DRAW_OFFLINE) {
    tk_semaphore_wait(s_sem_spare, -1);
    if (s_app_quited) {
      return ret;
    }

    tk_mutex_lock(s_lck_fblist);
    fb_taged_t* spare_fb = get_spare_fb();
    assert(spare_fb);
    tk_mutex_unlock(s_lck_fblist);
    ret = lcd_rtos_flush(lcd, spare_fb->fbid);

    tk_mutex_lock(s_lck_fblist);
    spare_fb->tags = FB_TAG_READY;
    tk_semaphore_post(s_sem_ready);
    tk_mutex_unlock(s_lck_fblist);

    int sched_yield(void);
    sched_yield();

  }

  return ret;
}

static void* fbswap_thread(void* ctx) {
  fb_info_t* fb = &s_fb;
  static int swap_lcd = 0;

  while (!s_app_quited) {
    tk_semaphore_wait(s_sem_ready, -1);
    if (s_app_quited) {
      break;
    }

    tk_mutex_lock(s_lck_fblist);
    fb_taged_t* ready_fb = get_ready_fb();
    assert(ready_fb);
    int ready_fbid = ready_fb->fbid;
    tk_mutex_unlock(s_lck_fblist);

    mpp_fb_ioctl(fb->fb, AICFB_PAN_DISPLAY, &swap_lcd);

    int dummy = 0;
    mpp_fb_ioctl(fb->fb, AICFB_WAIT_FOR_VSYNC, &dummy);
    swap_lcd = swap_lcd == 0 ? 1 : 0;

    tk_mutex_lock(s_lck_fblist);
    fb_taged_t* last_busy_fb = get_busy_fb();
    if (last_busy_fb) {
      last_busy_fb->tags = FB_TAG_SPARE;
      tk_semaphore_post(s_sem_spare);
    }
    ready_fb->tags = FB_TAG_BUSY;
    tk_mutex_unlock(s_lck_fblist);
  }

  log_info("display_thread end\n");
  return NULL;
}
#endif

static lcd_t* lcd_mem_create_fb(fb_info_t* fb)
{
  int w = fb_width(fb);
  int h = fb_height(fb);
  int bpp = fb_bpp(fb);
  lcd_t* lcd = NULL;
#ifdef WITH_LCD_FLUSH_AND_SWAP
  uint8_t* offline_fb = (uint8_t*)(fb->fbmem_offline);
  return_value_if_fail(offline_fb != NULL, NULL);

  if (bpp == 16) {
    if (fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_single_fb(w, h, offline_fb);
    } else if (fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_single_fb(w, h, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_single_fb(w, h, offline_fb);
    } else if (fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_single_fb(w, h, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
    if (fb_is_bgr888(fb)) {
      lcd = lcd_mem_bgr888_create_single_fb(w, h, offline_fb);
    } else if (fb_is_rgb888(fb)) {
      lcd = lcd_mem_rgb888_create_single_fb(w, h, offline_fb);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else {
    assert(!"not supported framebuffer format.");
  }
/* flush mode or swap mode */
#else
  if (bpp == 16) {
    if (fb_is_bgr565(fb)) {
      lcd = lcd_mem_bgr565_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else if (fb_is_rgb565(fb)) {
      lcd = lcd_mem_rgb565_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 32) {
    if (fb_is_bgra8888(fb)) {
      lcd = lcd_mem_bgra8888_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else if (fb_is_rgba8888(fb)) {
      lcd = lcd_mem_rgba8888_create_double_fb(w, h, fb->fbmem0, fb->fbmem_offline);
    } else {
      assert(!"not supported framebuffer format.");
    }
  } else if (bpp == 24) {
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
#endif
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
#ifdef WITH_LCD_FLUSH_AND_SWAP
  ret = aic_cma_buf_malloc(&offline_buf, h * line_length);
  if (ret < 0) {
    assert("aic_cma_buf_malloc err\n");
  }

  if (ret >= 0) {
    fb->fbmem_offline = offline_buf.buf;
    log_info("offline_buf.fd = %d, offline_buf.buf = 0x%08x, size = %d\n",
        offline_buf.fd, (unsigned int)offline_buf.buf, offline_buf.size);
  }

  aic_cma_buf_add_ge(&offline_buf);
#endif /* WITH_LCD_FLUSH_AND_SWAP */
  online_buffer[0].type = PHY_TYPE;
  online_buffer[0].buf = (unsigned char *)fb->fbmem0; /* in an rtos, the physical address is essentially the pointer address. */
  online_buffer[0].phy_addr = (unsigned char *)fb->fbmem0;
  online_buffer[0].size = line_length * h;

  aic_cma_buf_add_ge(&online_buffer[0]);
  if (fb_is_2fb(fb)) {
    online_buffer[1].type = PHY_TYPE;
    online_buffer[1].buf = (unsigned char *)fb->fbmem0 + (line_length * h);
    online_buffer[1].phy_addr = (unsigned char *)fb->fbmem0 + (line_length * h);
    online_buffer[1].size = line_length * h;
    aic_cma_buf_add_ge(&online_buffer[1]);
  }

  aic_cma_buf_debug(AIC_CMA_BUF_DEBUG_SIZE | AIC_CMA_BUF_DEBUG_CONTEXT);
#endif

  lcd = lcd_mem_create_fb(fb);
#ifdef WITH_LCD_FLUSH_AND_SWAP
  log_debug("lcd mode is flush and swap\n");
  uint8_t* offline_fb = (uint8_t*)(fb->fbmem_offline);
  return_value_if_fail(offline_fb != NULL, NULL);

  if (lcd != NULL) {
    lcd->swap = lcd_mem_rtos_write_buff;
    lcd->flush = lcd_mem_rtos_write_buff;
    lcd_mem_set_line_length(lcd, line_length);

    lcd_mem_t* mem = (lcd_mem_t*)lcd;
#if WITH_LCD_FLUSH_AND_SWAP
    s_lck_fblist = tk_mutex_create();
    s_sem_spare = tk_semaphore_create(fb_number(fb), NULL);
    s_sem_ready = tk_semaphore_create(0, NULL);
    s_t_fbswap = tk_thread_create(fbswap_thread, lcd);
    tk_thread_start(s_t_fbswap);
#endif
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

#ifdef WITH_LCD_FLUSH
  if (lcd != NULL) {
    log_debug("lcd mode is flush\n");
    lcd_mem_rtos_flush_default = lcd->flush;
    lcd->flush = lcd_mem_rtos_flush;
  }
#endif

  return lcd;
}

static lcd_t* lcd_rtos_create(fb_info_t* fb) {
  init_fblist(fb_number(fb));

  if (fb_is_1fb(fb)) {
    return lcd_rtos_single_framebuffer_create(fb);
  } else {
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
