/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <driver.h>
#include <uart.h>

#ifndef AIC_CLK_UART0_FREQ
#define AIC_CLK_UART0_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART1_FREQ
#define AIC_CLK_UART1_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART2_FREQ
#define AIC_CLK_UART2_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART3_FREQ
#define AIC_CLK_UART3_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART4_FREQ
#define AIC_CLK_UART4_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART5_FREQ
#define AIC_CLK_UART5_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART6_FREQ
#define AIC_CLK_UART6_FREQ 48000000 /* default 48M*/
#endif
#ifndef AIC_CLK_UART7_FREQ
#define AIC_CLK_UART7_FREQ 48000000 /* default 48M*/
#endif

#define DEVICE_FLAG_INT_RX 0x100 /**< INT mode on Rx */
#define DEVICE_FLAG_DMA_RX 0x200 /**< DMA mode on Rx */
#define DEVICE_FLAG_INT_TX 0x400 /**< INT mode on Tx */
#define DEVICE_FLAG_DMA_TX 0x800 /**< DMA mode on Tx */
#define SERIAL_MODE (DEVICE_FLAG_INT_RX)

#ifndef SERIAL_RB_BUFSZ
#define SERIAL_RB_BUFSZ 64
#endif

/*
 * Serial FIFO mode
 */
struct serial_rx_fifo
{
    u16 put_index;
    u16 get_index;
    u16 bufsz;
    u8 is_full;
    u8 buffer[0];
};

struct drv_uart_dev_para
{
    u32 index                   :4;
    u32 data_bits               :4;
    u32 stop_bits               :2;
    u32 parity                  :2;
    u32 rx_mode                 :2;
    u32 rx_bufsz                :12;
    u32 baud_rate;
    uint32_t clk_freq;
    u32 function;
    char * name;
};

const struct drv_uart_dev_para uart_dev_paras[] =
{
#ifdef AIC_USING_UART0
    {0, AIC_DEV_UART0_DATABITS, AIC_DEV_UART0_STOPBITS, AIC_DEV_UART0_PARITY, AIC_DEV_UART0_RX_MODE,
     AIC_DEV_UART0_RX_BUFSZ, AIC_DEV_UART0_BAUDRATE, AIC_CLK_UART0_FREQ, AIC_DEV_UART0_MODE, "uart0"},
#endif
#ifdef AIC_USING_UART1
    {1, AIC_DEV_UART1_DATABITS, AIC_DEV_UART1_STOPBITS, AIC_DEV_UART1_PARITY, AIC_DEV_UART1_RX_MODE,
     AIC_DEV_UART1_RX_BUFSZ, AIC_DEV_UART1_BAUDRATE, AIC_CLK_UART1_FREQ,  AIC_DEV_UART1_MODE, "uart1"},
#endif
#ifdef AIC_USING_UART2
    {2, AIC_DEV_UART2_DATABITS, AIC_DEV_UART2_STOPBITS, AIC_DEV_UART2_PARITY, AIC_DEV_UART2_RX_MODE,
    AIC_DEV_UART2_RX_BUFSZ, AIC_DEV_UART2_BAUDRATE, AIC_CLK_UART2_FREQ,  AIC_DEV_UART2_MODE, "uart2"},
#endif
#ifdef AIC_USING_UART3
    {3, AIC_DEV_UART3_DATABITS, AIC_DEV_UART3_STOPBITS, AIC_DEV_UART3_PARITY, AIC_DEV_UART3_RX_MODE,
    AIC_DEV_UART3_RX_BUFSZ, AIC_DEV_UART3_BAUDRATE, AIC_CLK_UART3_FREQ,  AIC_DEV_UART3_MODE, "uart3"},
#endif
#ifdef AIC_USING_UART4
    {4, AIC_DEV_UART4_DATABITS, AIC_DEV_UART4_STOPBITS, AIC_DEV_UART4_PARITY, AIC_DEV_UART4_RX_MODE,
    AIC_DEV_UART4_RX_BUFSZ, AIC_DEV_UART4_BAUDRATE, AIC_CLK_UART4_FREQ,  AIC_DEV_UART4_MODE, "uart4"},
#endif
#ifdef AIC_USING_UART5
    {5, AIC_DEV_UART5_DATABITS, AIC_DEV_UART5_STOPBITS, AIC_DEV_UART5_PARITY, AIC_DEV_UART5_RX_MODE,
    AIC_DEV_UART5_RX_BUFSZ, AIC_DEV_UART5_BAUDRATE, AIC_CLK_UART5_FREQ,  AIC_DEV_UART5_MODE, "uart5"},
#endif
#ifdef AIC_USING_UART6
    {6, AIC_DEV_UART6_DATABITS, AIC_DEV_UART6_STOPBITS, AIC_DEV_UART6_PARITY, AIC_DEV_UART6_RX_MODE,
    AIC_DEV_UART6_RX_BUFSZ, AIC_DEV_UART6_BAUDRATE, AIC_CLK_UART6_FREQ,  AIC_DEV_UART6_MODE, "uart6"},
#endif
#ifdef AIC_USING_UART7
    {7, AIC_DEV_UART7_DATABITS, AIC_DEV_UART7_STOPBITS, AIC_DEV_UART7_PARITY, AIC_DEV_UART7_RX_MODE,
    AIC_DEV_UART7_RX_BUFSZ, AIC_DEV_UART7_BAUDRATE, AIC_CLK_UART7_FREQ,  AIC_DEV_UART7_MODE, "uart7"},
#endif
};

static usart_handle_t uart_dev[AIC_UART_DEV_NUM];
static struct serial_rx_fifo *uart_rx_fifo[AIC_UART_DEV_NUM];

static void uart_irqhandler(int irq, void * data)
{
    int id = irq - UART0_IRQn;
    struct serial_rx_fifo* rx_fifo;
    int ch = -1;
    static unsigned char already_output = 0;

    if (id >= AIC_UART_DEV_NUM)
        return;

    /* UART Received Interrupt */
    rx_fifo = uart_rx_fifo[id];
    if (rx_fifo == NULL)
        return;

    while (1)
    {
        ch = hal_uart_getchar(uart_dev[id]);
        if (ch == -1) break;

        rx_fifo->buffer[rx_fifo->put_index] = ch;
        rx_fifo->put_index += 1;
        if (rx_fifo->put_index >= rx_fifo->bufsz)
            rx_fifo->put_index = 0;

        /* if the next position is read index, discard this 'read char' */
        if (rx_fifo->put_index == rx_fifo->get_index)
        {
            rx_fifo->get_index += 1;
            rx_fifo->is_full = 1;
            if (rx_fifo->get_index >= rx_fifo->bufsz) {
                rx_fifo->get_index = 0;

                if (already_output == 0)
                {
                    //printf("Warning: There is no enough buffer for saving data,"
                    //      " please increase the AIC_DEV_UARTx_RX_BUFSZ option.");
                    already_output = 1;
                }
            }
        }
    }
}

static u32 get_parity(u32 cfg)
{
    switch (cfg) {
        case 2:
            return USART_PARITY_EVEN;
        case 1:
            return USART_PARITY_ODD;
        default:
            return USART_PARITY_NONE;
    }
}

static u32 get_stop_bits(u32 cfg)
{
    switch (cfg) {
        case 0:
            return USART_STOP_BITS_0_5;
        case 1:
            return USART_STOP_BITS_1;
        case 2:
            return USART_STOP_BITS_2;
        case 3:
            return USART_STOP_BITS_1_5;
        default:
            return USART_STOP_BITS_1;
    }
}

static u32 get_data_bits(u32 cfg)
{
    switch (cfg) {
        case 5:
            return USART_DATA_BITS_5;
        case 6:
            return USART_DATA_BITS_6;
        case 7:
            return USART_DATA_BITS_7;
        case 8:
            return USART_DATA_BITS_8;
        case 9:
            return USART_DATA_BITS_9;
    }
    return USART_DATA_BITS_8;
}

int uart_init(int id)
{
    const struct drv_uart_dev_para *p;
    usart_handle_t uart;
    int ret, i;
    u32 data_bits;
    u32 stop_bits;
    u32 parity;

    ret = -1;
    p = NULL;
    for (i = 0; i < ARRAY_SIZE(uart_dev_paras); i++) {
        p = &uart_dev_paras[i];
        if (id == p->index) {
            break;
        }
    }
    if (!p)
        return -ENODEV;

    hal_clk_set_freq(CLK_UART0 + id, p->clk_freq);
    hal_clk_enable(CLK_UART0 + id);
    hal_reset_assert(RESET_UART0 + id);
    aic_udelay(500);
    hal_reset_deassert(RESET_UART0 + id);

    if (p->rx_mode != AIC_UART_RX_MODE_INT) {
        uart = hal_usart_initialize(id, NULL, NULL);
        hal_usart_set_interrupt(uart, USART_INTR_READ, 0);
    } else {
        uart_rx_fifo[id] = (struct serial_rx_fifo *)malloc(sizeof(struct serial_rx_fifo) + p->rx_bufsz);
        if (uart_rx_fifo[id] == NULL)
            return -ENOMEM;
        uart_rx_fifo[id]->get_index = 0;
        uart_rx_fifo[id]->put_index = 0;
        uart_rx_fifo[id]->is_full = 0;
        uart_rx_fifo[id]->bufsz = p->rx_bufsz;
        uart = hal_usart_initialize(id, NULL, uart_irqhandler);
        hal_usart_set_interrupt(uart, USART_INTR_READ, 1);
    }
    if (!uart) {
        ret = -ENODEV;
        goto err;
    }

    data_bits = get_data_bits(p->data_bits);
    stop_bits = get_stop_bits(p->stop_bits);
    parity = get_parity(p->parity);
    ret = hal_usart_config(uart, p->baud_rate, USART_MODE_ASYNCHRONOUS,
                           parity, stop_bits, data_bits, p->function);
    if (ret != 0)
        goto err;

    uart_dev[id] = uart;
    return ret;

err:
    if (uart_rx_fifo[id] != NULL) {
        free(uart_rx_fifo[id]);
        uart_rx_fifo[id] = NULL;
    }
    return ret;
}

int uart_config_update(int id, int baudrate)
{
    const struct drv_uart_dev_para *p;
    u32 data_bits, stop_bits, parity;
    int i;
    int ret = 0;

    p = NULL;
    for (i = 0; i < ARRAY_SIZE(uart_dev_paras); i++) {
        p = &uart_dev_paras[i];
        if (id == p->index) {
            break;
        }
    }
    if (!p)
        return -ENODEV;

    data_bits = get_data_bits(p->data_bits);
    stop_bits = get_stop_bits(p->stop_bits);
    parity = get_parity(p->parity);
    ret = hal_usart_config(uart_dev[id], baudrate, USART_MODE_ASYNCHRONOUS,
                           parity, stop_bits, data_bits, p->function);
    if (ret) {
        pr_err("set baudrate failed.\n");
    }

    return ret;
}

int uart_deinit(int id)
{
    int ret;

    ret = hal_usart_uninitialize(uart_dev[id]);

    if (uart_rx_fifo[id] != NULL) {
        free(uart_rx_fifo[id]);
        uart_rx_fifo[id] = NULL;
    }
    return ret;
}

int uart_getchar(int id)
{
    struct serial_rx_fifo* rx_fifo = uart_rx_fifo[id];
    int ch;

    if (rx_fifo == NULL) {
        return hal_uart_getchar(uart_dev[id]);
    } else {
        /* there's no data: */
        if ((rx_fifo->get_index == rx_fifo->put_index) && (rx_fifo->is_full == 0))
            return -1;

        /* otherwise there's the data: */
        ch = rx_fifo->buffer[rx_fifo->get_index];
        rx_fifo->get_index += 1;
        if (rx_fifo->get_index >= rx_fifo->bufsz)
            rx_fifo->get_index = 0;

        if (rx_fifo->is_full == 1)
            rx_fifo->is_full = 0;

        return ch;
    }
}

int uart_putchar(int id, int c)
{
    return hal_usart_putchar(uart_dev[id], (uint8_t)c);
}
