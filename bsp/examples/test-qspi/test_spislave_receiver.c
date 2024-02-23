/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xuan.Wen <xuan.wen@artinchip.com>
 */

#include <string.h>
#include <finsh.h>
#include <rtconfig.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <hal_qspi.h>
#include <rtthread.h>
#include "test_spislave.h"

#define RUN_STATE_IN 0
#define RUN_STATE_OUT 1

#define MEM_STATE_IDLE 0
#define MEM_STATE_CMD 1
#define MEM_STATE_DATA 2

struct qspirecv_state {
    u32 qspi_id;
    u32 bus_width;
    qspi_slave_handle handle;
    u8 *work_buf;
};

static struct qspirecv_state g_state;

extern void slave_dump_data(char *msg, u8 *buf, u32 len);
static void qspi_usage(void)
{
}


static int recv_new_data(struct qspirecv_state *state, u8 *buf, u32 len)
{
    struct qspi_transfer t;
    int ret;

    memset(&t, 0, sizeof(t));
    t.rx_data = buf;
    t.data_len = len;

    // memset(buf, 0, len);
    // printf("%s, reset rx fifo\n", __func__);
    hal_qspi_slave_fifo_reset(&state->handle, HAL_QSPI_RX_FIFO);
    ret = hal_qspi_slave_transfer_async(&state->handle, &t);
    if (ret < 0)
        return -1;
    return 0;
}

static void qspirecv_slave_async_callback(qspi_slave_handle *h, void *priv)
{
    struct qspirecv_state *state = priv;
    int status, cnt;
    u32 *p32, cksum;

    status = hal_qspi_slave_get_status(&state->handle);
    cnt = 0;
    if (status == HAL_QSPI_STATUS_OK) {
        /*
         * status OK:
         *   TRANSFER DONE or CS INVALID
         */
        // p = state->work_buf;
        cnt = hal_qspi_slave_transfer_count(&state->handle);
        printf("%s, status %d, cnt %d\n", __func__, status, cnt);
        p32 = (void *)state->work_buf;
        cksum = 0;
        for (int i = 0; i<PKT_SIZE/4; i++) {
            cksum += *p32;
            p32++;
        }
        printf("cksum 0x%x\n", cksum);
        recv_new_data(state, state->work_buf, PKT_SIZE);
        // slave_dump_data("Data", p, cnt);
    } else {
        /* Error process */
        printf("%s, status %d\n", __func__, status);
    }
}

static int test_qspirecv_start(int argc, char **argv)
{
    unsigned long val;
    int ret;

    if (argc < 2) {
        qspi_usage();
        return -1;
    }
    val = strtol(argv[1], NULL, 10);
    g_state.qspi_id = val;
    g_state.bus_width = 1; // Default is 1
    if (g_state.work_buf == NULL)
        g_state.work_buf =
            aicos_malloc_align(0, TEST_BUF_SIZE, CACHE_LINE_SIZE);
    if (argc >= 3) {
        val = strtol(argv[2], NULL, 10);
        g_state.bus_width = val;
    }
    ret = test_qspi_slave_controller_init(g_state.qspi_id, g_state.bus_width,
                                          qspirecv_slave_async_callback, &g_state,
                                          &g_state.handle);
    if (ret) {
        printf("QSPI Slave init failure.\n");
        return -1;
    }

    /* Start with waiting command */
    recv_new_data(&g_state, g_state.work_buf, PKT_SIZE);
    return 0;
}

static int test_qspirecv_stop(int argc, char **argv)
{
    test_qspi_slave_controller_deinit(&g_state.handle);
    if (g_state.work_buf) {
        aicos_free_align(0, g_state.work_buf);
        g_state.work_buf = NULL;
    }
    return 0;
}

static void cmd_test_qspislave_receiver(int argc, char **argv)
{
    if (argc < 2)
        goto help;

    if (!rt_strcmp(argv[1], "help")) {
        goto help;
    } else if (!rt_strcmp(argv[1], "start")) {
        test_qspirecv_start(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "stop")) {
        test_qspirecv_stop(argc - 1, &argv[1]);
        return;
    }
help:
    qspi_usage();
}


MSH_CMD_EXPORT_ALIAS(cmd_test_qspislave_receiver, spirecv, Test QSPI Slave);
