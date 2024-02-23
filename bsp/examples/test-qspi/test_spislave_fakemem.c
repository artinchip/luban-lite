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

struct fakemem_state {
    int run_state;
    int mem_state;
    u32 qspi_id;
    u32 bus_width;
    qspi_slave_handle handle;
    u8 *data_buf;
    u8 *work_buf;
};

static struct fakemem_state g_state;

#define USAGE \
    "fakemem help : Get this information.\n" \
    "fakemem start <spi id> <bus width>: Start fake slave device(mem) on SPI bus.\n" \
    "fakemem stop : Stop fake slave device.\n" \
    "example: standard spi by using QSPI1\n" \
    " fakemem start 1 1\n" \
    " fakemem stop\n" \
    " \n" \
    "example: dual spi by using QSPI1\n" \
    " fakemem start 1 2\n" \
    " fakemem stop\n"

static void qspi_usage(void)
{
    printf("%s", USAGE);
}

void slave_dump_data(char *msg, u8 *buf, u32 len)
{
    printf("%s:\n", msg);
    for (u32 i = 0; i < len; i++) {
        if (i != 0 && (i % 16 == 0))
            printf("\n");
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

static int read_cmd_start(struct fakemem_state *state, u8 *buf, u32 len)
{
    struct qspi_transfer t;
    int ret;

    memset(&t, 0, sizeof(t));
    buf[0] = 0;
    t.rx_data = buf;
    t.data_len = len;

    printf("%s, reset rx fifo\n", __func__);
    hal_qspi_slave_fifo_reset(&state->handle, HAL_QSPI_RX_FIFO);
    ret = hal_qspi_slave_transfer_async(&state->handle, &t);
    if (ret < 0)
        return -1;
    return 0;
}

static int send_data_start(struct fakemem_state *state, u32 start_addr, u8 *buf)
{
    struct qspi_transfer t;
    u32 offset, ready_to_read_flag;
    u8 *p, *status_data;

    status_data = state->work_buf;
    memset(&t, 0, sizeof(t));
    offset = start_addr;

    printf("Send load status + data\n");
    /* Read/Write data length required 4 bytes aligned */
    ready_to_read_flag = LOAD_STATUS_VAL;
    memcpy(&status_data[0], &ready_to_read_flag, 4);

    /* Data need to following status flag, and required to send togather,
     * otherwise master won't know the start of data
     */
    p = buf + (offset % TEST_BUF_SIZE);
    // printf("offset %d\n", offset);
    memcpy(&status_data[4], p, PKT_SIZE);
    t.tx_data = status_data;
    t.data_len = STATUS_SIZE + PKT_SIZE;

    /* Clear TX FIFO before write new data */
    hal_qspi_slave_fifo_reset(&state->handle, HAL_QSPI_TX_FIFO);
    /* Set data to slave qspi, it will wait master's clock to send data out */
    hal_qspi_slave_transfer_async(&state->handle, &t);
    return 0;
}

static int send_write_ok(struct fakemem_state *state)
{
    struct qspi_transfer t;
    u32 write_ok_flag;
    u8 *status;

    status = state->work_buf;
    memset(&t, 0, sizeof(t));

    printf("Send write status\n");
    /* Read/Write data length required 4 bytes aligned */
    write_ok_flag = WRITE_STATUS_VAL;
    memcpy(&status[0], &write_ok_flag, 4);
    t.tx_data = status;
    t.data_len = STATUS_SIZE;
    hal_qspi_slave_fifo_reset(&state->handle, HAL_QSPI_TX_FIFO);
    hal_qspi_slave_transfer_async(&state->handle, &t);
    return 0;
}

void qspi_slave_fakemem_async_callback(qspi_slave_handle *h, void *priv)
{
    struct fakemem_state *state = priv;
    int status, cnt;
    u32 addr = 0;
    u8 cmd = 0;
    u8 *p;

    status = hal_qspi_slave_get_status(&state->handle);
    cnt = 0;
    if (status == HAL_QSPI_STATUS_OK) {
        printf("%s, status %d\n", __func__, status);
        /*
         * status OK:
         *   TRANSFER DONE or CS INVALID
         */
        p = state->work_buf;
        if (state->run_state == RUN_STATE_IN) {
            if (state->mem_state == MEM_STATE_IDLE) {
                cnt = hal_qspi_slave_transfer_count(&state->handle);
                if (cnt < CMD_SIZE)
                    return;
                printf("Got new command\n");
                slave_dump_data("Command", p, cnt);
                state->mem_state = MEM_STATE_CMD;
            }
            if (state->mem_state == MEM_STATE_CMD) {
                cmd = p[0];
                // addr = (p[1] << 16) | (p[2] << 8) | p[3];
                addr = 0;
                memcpy(&addr, &p[1], 3);
                state->mem_state = MEM_STATE_DATA;
            }
            if (state->mem_state == MEM_STATE_DATA) {
                if (cmd == MEM_CMD_WRITE) {
                    if (cnt > CMD_SIZE) {
                        p += CMD_SIZE;
                        memcpy(state->data_buf + addr , p, cnt - CMD_SIZE);
                    }

                    /* Stop to receive data now, switch to send out: write
                     * status flag
                     */
                    hal_qspi_slave_transfer_abort(&state->handle);
                    send_write_ok(state);
                    state->mem_state = MEM_STATE_IDLE;
                    state->run_state = RUN_STATE_OUT;
                }
                if (cmd == MEM_CMD_LOAD) {
                    /* Prepare data and ready to send out, next wait the host
                     * to read all data
                     */
                    hal_qspi_slave_transfer_abort(&state->handle);
                    send_data_start(state, addr, state->data_buf);
                    state->run_state = RUN_STATE_OUT;
                }
            }
        } else { /* (state->run_state == RUN_STATE_OUT) */
            /* Get the send-out data count */
            cnt = hal_qspi_slave_transfer_count(&state->handle);
            if ((cnt == STATUS_SIZE) || (cnt == (STATUS_SIZE + PKT_SIZE))) {
                state->mem_state = MEM_STATE_IDLE;
                state->run_state = RUN_STATE_IN;
                /* Send data is finished, prepare to wait new command */
                read_cmd_start(state, state->work_buf, TEST_BUF_SIZE);
            }
        }
    } else {
        /* Error process */
        printf("%s, status %d\n", __func__, status);
    }
}

static int test_fakemem_start(int argc, char **argv)
{
    unsigned long val;
    int ret;

    if (argc < 2) {
        qspi_usage();
        return -1;
    }
    val = strtol(argv[1], NULL, 10);
    g_state.qspi_id = val;
    g_state.mem_state = MEM_STATE_IDLE;
    g_state.bus_width = 1; // Default is 1
    if (g_state.data_buf == NULL)
        g_state.data_buf =
            aicos_malloc_align(0, TEST_BUF_SIZE, CACHE_LINE_SIZE);
    memset(g_state.data_buf, 0, TEST_BUF_SIZE);
    if (g_state.work_buf == NULL)
        g_state.work_buf =
            aicos_malloc_align(0, TEST_BUF_SIZE, CACHE_LINE_SIZE);
    if (argc >= 3) {
        val = strtol(argv[2], NULL, 10);
        g_state.bus_width = val;
    }
    ret = test_qspi_slave_controller_init(g_state.qspi_id, g_state.bus_width,
                                          qspi_slave_fakemem_async_callback, &g_state,
                                          &g_state.handle);
    if (ret) {
        printf("QSPI Slave init failure.\n");
        return -1;
    }

    /* Start with waiting command */
    read_cmd_start(&g_state, g_state.work_buf, TEST_BUF_SIZE);
    g_state.run_state = RUN_STATE_IN;
    return 0;
}

static int test_fakemem_stop(int argc, char **argv)
{
    test_qspi_slave_controller_deinit(&g_state.handle);
    g_state.run_state = RUN_STATE_IN;
    if (g_state.data_buf) {
        aicos_free_align(0, g_state.data_buf);
        g_state.data_buf = NULL;
    }
    if (g_state.work_buf) {
        aicos_free_align(0, g_state.work_buf);
        g_state.work_buf = NULL;
    }
    return 0;
}

extern void test_qspi_slave_set_pinmux(void);
static void cmd_test_qspislave_pinmux(int argc, char **argv)
{
    test_qspi_slave_set_pinmux();
}

static void cmd_test_qspislave(int argc, char **argv)
{
    if (argc < 2)
        goto help;

    if (!rt_strcmp(argv[1], "help")) {
        goto help;
    } else if (!rt_strcmp(argv[1], "start")) {
        test_fakemem_start(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "stop")) {
        test_fakemem_stop(argc - 1, &argv[1]);
        return;
    }
help:
    qspi_usage();
}


MSH_CMD_EXPORT_ALIAS(cmd_test_qspislave, fakemem, Test QSPI Slave);
MSH_CMD_EXPORT_ALIAS(cmd_test_qspislave_pinmux, qpinmux, Test QSPI Slave);
