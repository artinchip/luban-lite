/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <aic_core.h>
#include "aic_hal.h"
#include "aic_log.h"
#include "aic_rtos_mem.h"
#include "tkc/utils.h"
#include "aic_dec_asset.h"

#define MAX_BUF_SIZE    200
#define BYTE_ALIGN(x, byte) (((x) + ((byte) - 1))&(~((byte) - 1)))

extern int lcd_size_get(void);

static cma_buffer_hash_map *g_hash_map = NULL;

static int hash_delivery(void *buf, int size) {
  unsigned long long addr = (unsigned long long)buf;

  return addr % size;
}

static cma_buffer_hash_map *cma_buf_hash_create(int size) {
  cma_buffer_hash_map *map;

  map = (cma_buffer_hash_map *)rt_malloc(sizeof(cma_buffer_hash_map));
  if (map != NULL) {
    map->size = size;
    map->cur_size = 0;
    map->cma_size = 0;
    map->buckets = (cma_buffer_hash **)rt_calloc(size, sizeof(cma_buffer_hash *));

    if (map->buckets == NULL) {
      rt_free(map);
      return NULL;
    }
  }

  return map;
}

static int cma_buf_hash_add(cma_buffer_hash_map *map, cma_buffer *data) {
  int hash_index = hash_delivery((unsigned int*)data->buf, map->size);
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
  int hash_index = hash_delivery((unsigned int*)buf, map->size);
  cma_buffer_hash *cur_node = map->buckets[hash_index];

  while(cur_node != NULL) {
    if ((unsigned int*)cur_node->data.buf == (unsigned int*)buf) {
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
    if ((unsigned int*)cur_node->data.buf == (unsigned int*)buf) {
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

static void cma_buf_hash_destroy(cma_buffer_hash_map *map) {
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

int aic_cma_buf_malloc(cma_buffer *back, int size, int align_size) {
  void *cma_buf = NULL;
  int malloc_size = 0;
  int cache_align_size = 0;

  if (align_size <= 0) {
      malloc_size = size + CACHE_LINE_SIZE * 2;
      cache_align_size = BYTE_ALIGN(size, CACHE_LINE_SIZE);
  } else {
      malloc_size = size + align_size * 2;
      cache_align_size = BYTE_ALIGN(size, align_size);
  }

  cma_buf = aicos_malloc(MEM_CMA, malloc_size);
  if (!cma_buf) {
    log_error("aic_cma_buf_malloc fail, malloc size = %d\n", malloc_size);
    return -1;
  }

  back->type = PHY_TYPE;
  back->buf_head = cma_buf;
  back->size = cache_align_size;

  if (align_size <= 0) {
    back->phy_addr = (unsigned int)BYTE_ALIGN(((uintptr_t)cma_buf), CACHE_LINE_SIZE);
  } else {
    back->phy_addr = (unsigned int)BYTE_ALIGN(((uintptr_t)cma_buf), align_size);
  }
  back->buf = (void *)back->phy_addr;

  aicos_dcache_clean_invalid_range((unsigned long *)back->phy_addr, (long long)cache_align_size);

  g_hash_map->cma_size += cache_align_size;

  return 0;
}

void aic_cma_buf_free(cma_buffer *data) {
  if (data->type == PHY_TYPE) {
    if (data->buf != 0) {
      aicos_free(MEM_CMA, data->buf_head);
      g_hash_map->cma_size -= data->size;
    }
  }
}

int aic_cma_buf_add(cma_buffer *data) {
  return cma_buf_hash_add(g_hash_map, data);
}

int aic_cma_buf_find(void *buf, cma_buffer *back) {
  return cma_buf_hash_find(g_hash_map, buf, back);
}

int aic_cma_buf_del(void *buf) {
  int ret = -1;
  cma_buffer node = { 0 };
  void *buf_node = NULL;

  ret = aic_cma_buf_find(buf, &node);
  if (ret < 0) {
    log_error("node is not exit, buf = 0x%08x\n", (unsigned int)buf);
    return -1;
  }

  buf_node = (void *)node.buf;

  aic_cma_buf_free(&node);
  return cma_buf_hash_remove(g_hash_map, buf_node);
}

static void aic_cma_buf_destroy() {
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

int aic_cma_buf_open(void) {
  if (g_hash_map == NULL) {
    g_hash_map = cma_buf_hash_create(MAX_BUF_SIZE);
    if (g_hash_map == NULL) {
      log_error("Failed to create cma_buf_hash\n");
    }
  }

  return 0;
}

void aic_cma_buf_close(void) {
  aic_cma_buf_destroy();
}

void aic_cma_buf_debug(int flag) {
  int i = 0;
  int sum_size = 0;
  cma_buffer_hash *cur_node = NULL;
  aic_dec_asset *ctx_data;

  if (g_hash_map == NULL) {
    log_error("hash need init\n");
    return;
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
        } else if (cur_node->data.type == PHY_TYPE) {
          log_debug("phy = 0x%08x, buf = 0x%08x, size = %d\n",
                    (unsigned int)cur_node->data.phy_addr,
                    (unsigned int)cur_node->data.buf,
                    cur_node->data.size);
        } else {
          ctx_data = cur_node->data.data;
          log_debug("name = %s, buf = 0x%08x, size = %d\n",
                    ctx_data->name, (unsigned int)cur_node->data.buf,
                    (unsigned int)cur_node->data.size);
        }
        sum_size += cur_node->data.size;
        cur_node = cur_node->next;
      }
    }
  }

  if (flag & AIC_CMA_BUF_DEBUG_SIZE) {
    log_debug("used_table_size = %d, used_mem_size = %d, sum_size = %d\n",
               (g_hash_map->size - g_hash_map->cur_size),
               g_hash_map->cma_size, (sum_size - lcd_size_get()));
  }
}

