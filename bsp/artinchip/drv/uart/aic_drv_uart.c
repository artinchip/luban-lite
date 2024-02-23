/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <aic_drv.h>
#include "aic_hal_uart.h"
#include "aic_drv_uart.h"

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

void drv_usart_irqhandler(int irq, void * data);

struct
{
    uint32_t base;
    uint32_t irq;
    void *handler;
}
const drv_usart_config[AIC_UART_DEV_NUM] =
{
#if (AIC_UART_DEV_NUM >= 4)
    {UART0_BASE, UART0_IRQn, drv_usart_irqhandler},
    {UART1_BASE, UART1_IRQn, drv_usart_irqhandler},
    {UART2_BASE, UART2_IRQn, drv_usart_irqhandler},
    {UART3_BASE, UART3_IRQn, drv_usart_irqhandler},
#endif
#if (AIC_UART_DEV_NUM >= 8)
    {UART4_BASE, UART4_IRQn, drv_usart_irqhandler},
    {UART5_BASE, UART5_IRQn, drv_usart_irqhandler},
    {UART6_BASE, UART6_IRQn, drv_usart_irqhandler},
    {UART7_BASE, UART7_IRQn, drv_usart_irqhandler},
#endif
};

static  usart_handle_t uart_handle[AIC_UART_DEV_NUM];
static struct rt_serial_device  serial[AIC_UART_DEV_NUM];

int32_t drv_usart_target_init(int32_t idx, uint32_t *base, uint32_t *irq, void **handler)
{
    if (idx >= AIC_UART_DEV_NUM)
    {
        return -1;
    }

    if (base != NULL)
    {
        *base = drv_usart_config[idx].base;
    }

    if (irq != NULL)
    {
        *irq = drv_usart_config[idx].irq;
    }

    if (handler != NULL)
    {
        *handler = drv_usart_config[idx].handler;
    }

    return idx;
}

void drv_usart_irqhandler(int irq, void * data)
{
    int index = irq - UART0_IRQn;
    uint8_t status= 0;

    if (index >= AIC_UART_DEV_NUM)
        return;

    status = hal_usart_get_irqstatus(index);

    switch (status)
    {
    case AIC_IIR_RECV_DATA:
        rt_hw_serial_isr(&serial[index],RT_SERIAL_EVENT_RX_IND);
        break;

    default:
        break;
    }
}

/*
 * UART interface
 */
static rt_err_t drv_uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    int ret;
    usart_handle_t uart;

    uint32_t bauds;
    usart_mode_e mode;
    usart_parity_e parity;
    usart_stop_bits_e stopbits;
    usart_data_bits_e databits;

    RT_ASSERT(serial != RT_NULL);
    uart = (usart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);

    /* set baudrate parity...*/
    bauds = cfg->baud_rate;
    mode = USART_MODE_ASYNCHRONOUS;

    if (cfg->parity == PARITY_EVEN)
        parity = USART_PARITY_EVEN;
    else if (cfg->parity == PARITY_ODD)
        parity = USART_PARITY_ODD;
    else
        parity = USART_PARITY_NONE;

    if (cfg->stop_bits == 1)
        stopbits = USART_STOP_BITS_1;
    else if (cfg->stop_bits == 2)
        stopbits = USART_STOP_BITS_2;
    else if (cfg->stop_bits == 3)
        stopbits = USART_STOP_BITS_1_5;
    else
        stopbits = USART_STOP_BITS_0_5;

    if (cfg->data_bits == 5)
        databits = USART_DATA_BITS_5;
    else if (cfg->data_bits == 6)
        databits = USART_DATA_BITS_6;
    else if (cfg->data_bits == 7)
        databits = USART_DATA_BITS_7;
    else if (cfg->data_bits == 8)
        databits = USART_DATA_BITS_8;
    else if (cfg->data_bits == 9)
        databits = USART_DATA_BITS_9;
    else
        databits = USART_DATA_BITS_8;

    ret = hal_usart_config(uart, bauds, mode, parity, stopbits, databits, cfg->function);

    if (ret < 0)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t drv_uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    usart_handle_t uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (usart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        /* Disable the UART Interrupt */
        if ((uintptr_t)arg == RT_DEVICE_FLAG_INT_RX)
            hal_usart_set_interrupt(uart, USART_INTR_READ, 0);
        break;

    case RT_DEVICE_CTRL_SET_INT:
        /* Enable the UART Interrupt */
        if ((uintptr_t)arg == RT_DEVICE_FLAG_INT_RX)
            hal_usart_set_interrupt(uart, USART_INTR_READ, 1);
        break;
    }

    return (RT_EOK);
}

static int drv_uart_putc(struct rt_serial_device *serial, char c)
{
    usart_handle_t uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (usart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);
    hal_usart_putchar(uart,c);

    return (1);
}

static int drv_uart_getc(struct rt_serial_device *serial)
{
    int ch;
    usart_handle_t uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (usart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);


    ch = hal_uart_getchar(uart);

    return ch;
}

const struct rt_uart_ops drv_uart_ops =
{
    drv_uart_configure,
    drv_uart_control,
    drv_uart_putc,
    drv_uart_getc,
};

struct drv_uart_dev_para
{
    uint32_t index                   :4;
    uint32_t data_bits               :4;
    uint32_t stop_bits               :2;
    uint32_t parity                  :2;
    uint32_t baud_rate;
    uint32_t clk_freq;
    uint32_t function;
    char * name;
};

const struct drv_uart_dev_para uart_dev_paras[] =
{
#ifdef AIC_USING_UART0
    {0, AIC_DEV_UART0_DATABITS, AIC_DEV_UART0_STOPBITS, AIC_DEV_UART0_PARITY, AIC_DEV_UART0_BAUDRATE, AIC_CLK_UART0_FREQ, AIC_DEV_UART0_MODE, "uart0"},
#endif
#ifdef AIC_USING_UART1
    {1, AIC_DEV_UART1_DATABITS, AIC_DEV_UART1_STOPBITS, AIC_DEV_UART1_PARITY, AIC_DEV_UART1_BAUDRATE, AIC_CLK_UART1_FREQ, AIC_DEV_UART1_MODE, "uart1"},
#endif
#ifdef AIC_USING_UART2
    {2, AIC_DEV_UART2_DATABITS, AIC_DEV_UART2_STOPBITS, AIC_DEV_UART2_PARITY, AIC_DEV_UART2_BAUDRATE, AIC_CLK_UART2_FREQ, AIC_DEV_UART2_MODE, "uart2"},
#endif
#ifdef AIC_USING_UART3
    {3, AIC_DEV_UART3_DATABITS, AIC_DEV_UART3_STOPBITS, AIC_DEV_UART3_PARITY, AIC_DEV_UART3_BAUDRATE, AIC_CLK_UART3_FREQ, AIC_DEV_UART3_MODE, "uart3"},
#endif
#ifdef AIC_USING_UART4
    {4, AIC_DEV_UART4_DATABITS, AIC_DEV_UART4_STOPBITS, AIC_DEV_UART4_PARITY, AIC_DEV_UART4_BAUDRATE, AIC_CLK_UART4_FREQ, AIC_DEV_UART4_MODE, "uart4"},
#endif
#ifdef AIC_USING_UART5
    {5, AIC_DEV_UART5_DATABITS, AIC_DEV_UART5_STOPBITS, AIC_DEV_UART5_PARITY, AIC_DEV_UART5_BAUDRATE, AIC_CLK_UART5_FREQ, AIC_DEV_UART5_MODE, "uart5"},
#endif
#ifdef AIC_USING_UART6
    {6, AIC_DEV_UART6_DATABITS, AIC_DEV_UART6_STOPBITS, AIC_DEV_UART6_PARITY, AIC_DEV_UART6_BAUDRATE, AIC_CLK_UART6_FREQ, AIC_DEV_UART6_MODE, "uart6"},
#endif
#ifdef AIC_USING_UART7
    {7, AIC_DEV_UART7_DATABITS, AIC_DEV_UART7_STOPBITS, AIC_DEV_UART7_PARITY, AIC_DEV_UART7_BAUDRATE, AIC_CLK_UART7_FREQ, AIC_DEV_UART7_MODE, "uart7"},
#endif
};

int drv_usart_init(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    int u = 0;
    int i = 0;

    for (i=0; i<sizeof(uart_dev_paras)/sizeof(struct drv_uart_dev_para); i++) {
        u = uart_dev_paras[i].index;
        serial[u].ops                 = & drv_uart_ops;
        serial[u].config              = config;
        serial[u].config.bufsz        = 2048;
        serial[u].config.baud_rate    = uart_dev_paras[i].baud_rate;
        serial[u].config.data_bits    = uart_dev_paras[i].data_bits;
        serial[u].config.stop_bits    = uart_dev_paras[i].stop_bits;
        serial[u].config.parity       = uart_dev_paras[i].parity;
        serial[u].config.function     = uart_dev_paras[i].function;

        hal_clk_set_freq(CLK_UART0 + u, uart_dev_paras[i].clk_freq);
        hal_clk_enable(CLK_UART0 + u);
        hal_reset_assert(RESET_UART0 + u);
        aic_udelay(10000);
        hal_reset_deassert(RESET_UART0 + u);
#ifdef FINSH_POLL_MODE
        uart_handle[u] = hal_usart_initialize(u, NULL, NULL);
#else
        uart_handle[u] = hal_usart_initialize(u, NULL, drv_usart_irqhandler);
#endif

        rt_hw_serial_register(&serial[u],
                              uart_dev_paras[i].name,
#ifdef FINSH_POLL_MODE
                              RT_DEVICE_FLAG_RDWR,
#else
                              RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
#endif
                              uart_handle[u]);
    }

    return 0;
}
INIT_BOARD_EXPORT(drv_usart_init);
