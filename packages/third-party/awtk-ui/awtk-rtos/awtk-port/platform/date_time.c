/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 *
 */

#include "aic_core.h"
#include <sys/time.h>
#include <unistd.h>
#include "time.h"
#include "tkc/date_time.h"

static ret_t date_time_get_now_impl(date_time_t* dt) {
  time_t now = time(0);
  struct tm* t = localtime(&now);

  dt->second = t->tm_sec;
  dt->minute = t->tm_min;
  dt->hour = t->tm_hour;
  dt->day = t->tm_mday;
  dt->month = t->tm_mon + 1;
  dt->year = t->tm_year + 1900;
  dt->wday = t->tm_wday;

  return RET_OK;
}

static ret_t date_time_set_now_impl(date_time_t* dt) {
  struct tm tms;
  time_t t = 0;
  memset(&tms, 0x00, sizeof(tms));

  tms.tm_year = dt->year - 1900;
  tms.tm_mon = dt->month - 1;
  tms.tm_mday = dt->day;
  tms.tm_hour = dt->hour;
  tms.tm_min = dt->minute;
  tms.tm_sec = dt->second;

  t = mktime(&tms);
  (void)t;

  return RET_OK;
}

static uint64_t date_time_to_time_impl(date_time_t* dt) {
  struct tm t;
  time_t tvalue = 0;
  return_value_if_fail(dt != NULL, RET_BAD_PARAMS);

  memset(&t, 0x00, sizeof(t));
  t.tm_sec = dt->second;
  t.tm_min = dt->minute;
  t.tm_hour = dt->hour;
  t.tm_mday = dt->day;
  t.tm_mon = dt->month - 1;
  t.tm_year = dt->year - 1900;
  t.tm_wday = dt->wday;

  tvalue = timegm(&t);

  return tvalue;
}


static ret_t date_time_from_time_impl(date_time_t* dt, uint64_t timeval) {
  time_t tm = timeval;
  struct tm* t = gmtime(&tm);
  return_value_if_fail(dt != NULL, RET_BAD_PARAMS);

  memset(dt, 0x00, sizeof(date_time_t));

  dt->second = t->tm_sec;
  dt->minute = t->tm_min;
  dt->hour = t->tm_hour;
  dt->day = t->tm_mday;
  dt->month = t->tm_mon + 1;
  dt->year = t->tm_year + 1900;
  dt->wday = t->tm_wday;

  return RET_OK;
}

static const date_time_vtable_t s_date_time_vtable = {
    date_time_get_now_impl,
    date_time_set_now_impl,
    date_time_from_time_impl,
    date_time_to_time_impl,
};

void aic_date_time_init(void) {
#ifdef AIC_USING_RTC
  date_time_global_init_ex(&s_date_time_vtable);
#endif
}
