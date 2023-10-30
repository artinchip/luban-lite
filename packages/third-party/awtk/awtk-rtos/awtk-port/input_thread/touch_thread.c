/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author           Notes
 * 2021-01-13     RiceChen         the first version
 * 2023-10-20     Zequan Liang     adapt to awtk
 */

#include <rtconfig.h>
#include <rtdevice.h>
#include <rtthread.h>
#include "tkc/mem.h"
#include "base/keys.h"
#include "tkc/thread.h"
#include "touch_thread.h"
#include "tkc/utils.h"
#include "touch_thread.h"
#include "aic_osal.h"

#ifdef AIC_TOUCH_PANEL_GT911
extern rt_size_t gt911_read_point(struct rt_touch_device *touch, void *buf,
                                    rt_size_t read_num);
#endif

static rt_thread_t  gt911_thread = RT_NULL;
static rt_sem_t     gt911_sem = RT_NULL;
static rt_device_t  dev = RT_NULL;
static struct rt_touch_data *read_data;
static struct rt_touch_info info;

static event_type_t rt_touch_state_to_tk_touch_state(rt_uint8_t rt_state)
{
  switch (rt_state) {
  case RT_TOUCH_EVENT_DOWN:
    return EVT_POINTER_DOWN;
  case RT_TOUCH_EVENT_UP:
    return EVT_POINTER_UP;
  case RT_TOUCH_EVENT_MOVE:
    return EVT_POINTER_MOVE;
  default:
    return EVT_NONE;
  }

  return EVT_NONE;
}

void aic_tk_touch_input_event_cb(rt_int16_t x, rt_int16_t y,
                                 rt_uint8_t state, main_loop_t* ctx)
{
  event_queue_req_t req;
  main_loop_t* l = (main_loop_t *)ctx;
  static int pressed = FALSE;

  memset(&req, 0x00, sizeof(req));
  req.pointer_event.x = x;
  req.pointer_event.y = y;
  req.event.type = rt_touch_state_to_tk_touch_state(state);

  event_queue_req_t* e = &req;
  if (l != NULL) {
    switch (e->event.type) {
      case EVT_CONTEXT_MENU: {
        e->event.size = sizeof(e->pointer_event);
        break;
      }
      case EVT_POINTER_DOWN: {
        pressed = TRUE;
        e->pointer_event.pressed = pressed;
        e->event.size = sizeof(e->pointer_event);
        break;
      }
      case EVT_POINTER_MOVE: {
        e->pointer_event.pressed = pressed;
        e->event.size = sizeof(e->pointer_event);
        break;
      }
      case EVT_POINTER_UP: {
        e->pointer_event.pressed = pressed;
        pressed = FALSE;
        e->event.size = sizeof(e->pointer_event);
        break;
      }
      default:
        break;
    }

    main_loop_queue_event(l, e);
#ifdef AIC_TOUCH_PANEL_GT911_DEBUG
    input_dispatch_print(ctx, e, AIC_TOUCH_PANEL_GT911_NAME);
#endif
  }
}

static void gt911_entry(void *parameter)
{
  rt_device_control(dev, RT_TOUCH_CTRL_GET_INFO, &info);

  read_data = (struct rt_touch_data *)rt_malloc(sizeof(struct rt_touch_data) * info.point_num);

  while (1) {
    rt_sem_take(gt911_sem, RT_WAITING_FOREVER);
    int num = 0;
#ifdef AIC_TOUCH_PANEL_GT911
    num = rt_device_read(dev, 0, read_data, info.point_num);
#endif
    if (num == info.point_num) {
      for (rt_uint8_t i = 0; i < info.point_num; i++) {
        if (read_data[i].event == RT_TOUCH_EVENT_DOWN
            || read_data[i].event == RT_TOUCH_EVENT_UP
            || read_data[i].event == RT_TOUCH_EVENT_MOVE) {

          rt_uint16_t  u16X, u16Y;
          u16X = read_data[i].x_coordinate;
          u16Y = read_data[i].y_coordinate;
#ifdef AIC_TOUCH_PANEL_GT911_DEBUG
          log_debug("[%d] %d %d\n", read_data[i].event, u16X, u16Y);
#endif
          aic_tk_touch_input_event_cb(u16X, u16Y, read_data[i].event, (main_loop_t *)parameter);
        }
      }
    }

    rt_device_control(dev, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);
  }
}

static rt_err_t rx_callback(rt_device_t dev, rt_size_t size)
{
#ifdef AIC_PM_POWER_TOUCH_WAKEUP
  rt_uint8_t sleep_mode;
#endif
  rt_sem_release(gt911_sem);
  rt_device_control(dev, RT_TOUCH_CTRL_DISABLE_INT, RT_NULL);
#ifdef AIC_PM_POWER_TOUCH_WAKEUP
  sleep_mode = rt_pm_get_sleep_mode();
  if (sleep_mode != PM_SLEEP_MODE_NONE && !wakeup_triggered) {
    rt_pm_module_request(PM_POWER_ID, PM_SLEEP_MODE_NONE);
    wakeup_triggered = 1;
  }
  /* touch timer restart */
  rt_timer_start(touch_timer);
#endif
  return 0;
}

int tk_touch_run(const char *name, main_loop_t* ctx)
{
  void *id;

  dev = rt_device_find(name);
  if (dev == RT_NULL) {
    log_error("can't find device:%s\n", name);
    return -1;
  }

  if (rt_device_open(dev, RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
    log_error("open device failed!");
    return -1;
  }

  id = rt_malloc(sizeof(rt_uint8_t) * 8);
  rt_device_control(dev, RT_TOUCH_CTRL_GET_ID, id);

  /* if possible you can set your x y coordinate */
  //rt_device_control(dev, RT_TOUCH_CTRL_SET_X_RANGE, &x);
  //rt_device_control(dev, RT_TOUCH_CTRL_SET_Y_RANGE, &y);
  rt_device_control(dev, RT_TOUCH_CTRL_GET_INFO, id);
#ifdef AIC_TOUCH_PANEL_GT911_DEBUG
  rt_uint8_t * read_id = (rt_uint8_t *)id;

  log_debug("id = GT%d%d%d \n", read_id[0] - '0', read_id[1] - '0', read_id[2] - '0');
  log_debug("range_x = %d \n", (*(struct rt_touch_info*)id).range_x);
  log_debug("range_y = %d \n", (*(struct rt_touch_info*)id).range_y);
  log_debug("point_num = %d \n", (*(struct rt_touch_info*)id).point_num);
#endif
  rt_free(id);
  rt_device_set_rx_indicate(dev, rx_callback);
  gt911_sem = rt_sem_create("dsem", 0, RT_IPC_FLAG_FIFO);

  if (gt911_sem == RT_NULL) {
    log_error("create dynamic semaphore failed.\n");
    return -1;
  }

  gt911_thread = rt_thread_create("awtk-touch",
                                  gt911_entry,
                                  ctx,
                                  TK_TOUCH_THREAD_STACK_SIZE,
                                  TK_TOUCH_THREAD_PRIORITY,
                                  TK_TOUCH_THREAD_TICK);

  if (gt911_thread != RT_NULL)
    rt_thread_startup(gt911_thread);

  return 0;
}
