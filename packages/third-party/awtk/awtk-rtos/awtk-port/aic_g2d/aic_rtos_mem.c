/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

/**
 * History:
 * ================================================================
 * 2023-9-14 Zequan Liang <zequan.liang@artinchip.com> created
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <aic_core.h>
#include "aic_hal.h"
#include "aic_log.h"
#include "aic_rtos_mem.h"
#include "tkc/utils.h"

#define MAX_BUF_SIZE    200

static cma_buffer_hash_map *g_hash_map = NULL;

static int hash_delivery(void *buf, int size)
{
  unsigned long long addr = (unsigned long long)buf;

  return addr % size;
}

static cma_buffer_hash_map *cma_buf_hash_create(int size)
{
  cma_buffer_hash_map *map;

  map = (cma_buffer_hash_map *)rt_malloc(sizeof(cma_buffer_hash_map));
  if (map != NULL) {
    map->size = size;
    map->cur_size = 0;
    map->buckets = (cma_buffer_hash **)rt_calloc(size, sizeof(cma_buffer_hash *));

    if (map->buckets == NULL) {
      rt_free(map);
      return NULL;
    }
  }

  return map;
}

static int cma_buf_hash_add(cma_buffer_hash_map *map, cma_buffer *data) {
  int hash_index = hash_delivery(data->buf, map->size);
  cma_buffer_hash *cur_node = NULL;
  cma_buffer_hash *new_node = (cma_buffer_hash *)rt_calloc(1, sizeof(cma_buffer_hash));

  if (map->cur_size > map->size - 1) {
    if (new_node != NULL)
      rt_free(new_node);
    return -1;
  }

  if (new_node != NULL) {
    memcpy(&new_node->data, data, sizeof(cma_buffer));
    new_node->next = NULL;

    if (map->buckets[hash_index] == NULL) {
      map->buckets[hash_index] = new_node;
    } else {
      cur_node = map->buckets[hash_index];
      while (cur_node->next != NULL) {
        cur_node = cur_node->next;
      }
      cur_node->next = new_node;
    }
  } else {
    return -1;
  }

  map->cur_size++;
  return 0;
}

static int cma_buf_hash_find(cma_buffer_hash_map *map, void *buf, cma_buffer *data) {
  int hash_index = hash_delivery(buf, map->size);
  cma_buffer_hash *cur_node = map->buckets[hash_index];

  while(cur_node != NULL) {
    if (cur_node->data.buf == buf) {
      memcpy(data, &cur_node->data, sizeof(cma_buffer));
      return 0;
    }
    cur_node = cur_node->next;
  }

  return -1;
}

static int cma_buf_hash_remove(cma_buffer_hash_map *map, void *buf) {
  int hash_index = hash_delivery(buf, map->size);
  cma_buffer_hash *cur_node = map->buckets[hash_index];
  cma_buffer_hash *prev_node = NULL;

  if (map->cur_size <= 0)
    return -1;

  while(cur_node != NULL) {
    if (cur_node->data.buf == buf) {
      if (prev_node == NULL) {
        map->buckets[hash_index] = cur_node->next;
      } else {
        prev_node->next = cur_node->next;
      }

      map->cur_size--;
      rt_free(cur_node);
      return 0;
    }

    prev_node = cur_node;
    cur_node = cur_node->next;
  }

  return -1;
}


static void cma_buf_hash_destroy(cma_buffer_hash_map *map)
{
  int i = 0;
  cma_buffer_hash *cur_node = NULL;
  cma_buffer_hash *tmp_node = NULL;

  if (map != NULL) {
    for (i = 0; i < map->size; i++) {
      cur_node = map->buckets[i];
      while(cur_node != NULL) {
        tmp_node = cur_node;
        cur_node = cur_node->next;
        rt_free(tmp_node);
      }
    }

    rt_free(map->buckets);
    rt_free(map);
  }
}


int aic_cma_buf_malloc(cma_buffer *back, int size)
{
  void *cma_buf = NULL;

  cma_buf = aicos_malloc(MEM_CMA, size);
  if (!cma_buf) {
    log_error("aic_cma_buf_malloc fail, size = %d\n", size);
    return -1;
  }
  memset(cma_buf, 0, size);

  back->type = PHY_TYPE;
  back->fd = -1;
  back->buf = cma_buf;
  back->size = size;

  return 0;
}

void aic_cma_buf_free(cma_buffer *data)
{
  if (data->type == PHY_TYPE) {
    if (!data->buf) {
      aicos_free(MEM_CMA, data->buf);
    }
  } else {
    return;
  }
}

int aic_cma_buf_add(cma_buffer *data)
{
  return cma_buf_hash_add(g_hash_map, data);
}

int aic_cma_buf_find(void *buf, cma_buffer *back)
{
  return cma_buf_hash_find(g_hash_map, buf, back);
}

int aic_cma_buf_del(void *buf)
{
  int ret = -1;
  cma_buffer node = { 0 };
  void *buf_node = NULL;

  ret = aic_cma_buf_find(buf, &node);
  if (ret < 0) {
    log_error("node is not exit\n");
    return -1;
  }

  buf_node = node.buf;

  aic_cma_buf_free(&node);
  return cma_buf_hash_remove(g_hash_map, buf_node);
}

static void aic_cma_buf_destroy()
{
  int i = 0;
  cma_buffer_hash *cur_node = NULL;

  if (g_hash_map != NULL) {
    for (i = 0; i < g_hash_map->size; i++) {
      cur_node = g_hash_map->buckets[i];
      while(cur_node != NULL) {
        aic_cma_buf_free(&cur_node->data);
        cur_node = cur_node->next;
      }
    }
    cma_buf_hash_destroy(g_hash_map);
  } else {
    log_error("aic_cma_buf_destroy err\n");
  }
}

int aic_cma_buf_open(void)
{
  if (g_hash_map == NULL) {
    g_hash_map = cma_buf_hash_create(MAX_BUF_SIZE);
    if (g_hash_map == NULL) {
      log_error("Failed to create cma_buf_hash\n");
    }
  }

  return 0;
}

void aic_cma_buf_close(void)
{
  aic_cma_buf_destroy();
}

void aic_cma_buf_debug(int flag)
{
  int i = 0;
  cma_buffer_hash *cur_node = NULL;

  if (g_hash_map == NULL) {
    log_error("hash need init\n");
    return;
  }

  if (flag & AIC_CMA_BUF_DEBUG_SIZE) {
    log_debug("map->cur_size = %d, size = %d\n", g_hash_map->cur_size, g_hash_map->size);
  }

  if (flag & AIC_CMA_BUF_DEBUG_CONTEXT) {
    for (i = 0; i < g_hash_map->size; i++) {
      cur_node = g_hash_map->buckets[i];
      while(cur_node != NULL) {
        if (cur_node->data.type == FD_TYPE) {
          log_debug("fd = %d, buf = 0x%08x, size = %d\n",
                    cur_node->data.fd,
                    (unsigned int)cur_node->data.buf,
                    cur_node->data.size);
        } else {
          log_debug("phy = 0x%08x, buf = 0x%08x, size = %d\n",
                    (unsigned int)cur_node->data.phy_addr,
                    (unsigned int)cur_node->data.buf,
                    cur_node->data.size);
        }
        cur_node = cur_node->next;
      }
    }
  }
}

