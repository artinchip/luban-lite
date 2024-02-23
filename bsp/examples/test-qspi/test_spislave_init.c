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

struct spi_pinmux {
    unsigned char func;
    unsigned char bias;
    unsigned char drive;
    char *name;
};

/* Please change your pinmux setting according your board */
static struct spi_pinmux spi_pinmux_config[] = {
    /* qspi1  */
    { 3, PIN_PULL_UP, 3, "PD.4" }, // CS, default set to high
    { 3, PIN_PULL_UP, 3, "PD.5" },
    { 3, PIN_PULL_UP, 3, "PD.6" }, // SI
    { 3, PIN_PULL_UP, 7, "PD.7" }, // CLK
    { 3, PIN_PULL_UP, 3, "PD.8" },
    { 3, PIN_PULL_UP, 3, "PD.9" },

    /* qspi2 */
    { 3, PIN_PULL_UP, 3, "PB.6" },
    { 3, PIN_PULL_UP, 3, "PB.7" },
    { 3, PIN_PULL_UP, 3, "PB.8" },
    { 3, PIN_PULL_UP, 3, "PB.9" },
    { 3, PIN_PULL_UP, 3, "PB.10" },
    { 3, PIN_PULL_UP, 3, "PB.11" },

    /* qspi3  */
    { 3, PIN_PULL_UP, 3, "PD.0" },
    { 3, PIN_PULL_UP,  3, "PD.1" }, //CS, default set to high
    { 3, PIN_PULL_UP, 3, "PD.2" },
    { 3, PIN_PULL_UP, 3, "PD.3" },
};

static u32 qspi_clk_ids[4] = {CLK_QSPI0, CLK_QSPI1, CLK_QSPI2, CLK_QSPI3};
static u32 qspi_irq_num[] = {QSPI0_IRQn, QSPI1_IRQn, QSPI2_IRQn, QSPI3_IRQn};
static u32 qspi_input_clk[4] = {
#ifdef AIC_USING_QSPI0
    [0] = AIC_DEV_QSPI0_MAX_SRC_FREQ_HZ,
#endif
#ifdef AIC_USING_QSPI1
    [1] = AIC_DEV_QSPI1_MAX_SRC_FREQ_HZ,
#endif
#ifdef AIC_USING_QSPI2
    [2] = AIC_DEV_QSPI2_MAX_SRC_FREQ_HZ,
#endif
#ifdef AIC_USING_QSPI3
    [3] = AIC_DEV_QSPI3_MAX_SRC_FREQ_HZ,
#endif
};


void test_qspi_slave_set_pinmux(void)
{
    uint32_t i = 0;
    long pin = 0;
    unsigned int g;
    unsigned int p;

    for (i=0; i<ARRAY_SIZE(spi_pinmux_config); i++) {
        pin = hal_gpio_name2pin(spi_pinmux_config[i].name);
        if (pin < 0)
            continue;
        g = GPIO_GROUP(pin);
        p = GPIO_GROUP_PIN(pin);
        hal_gpio_set_func(g, p, spi_pinmux_config[i].func);
        hal_gpio_set_bias_pull(g, p, spi_pinmux_config[i].bias);
        hal_gpio_set_drive_strength(g, p, spi_pinmux_config[i].drive);
    }
}

static irqreturn_t qspi_slave_irq_handler(int irq_num, void *arg)
{
    qspi_slave_handle *h = arg;

    rt_interrupt_enter();
    hal_qspi_slave_irq_handler(h);
    rt_interrupt_leave();

    return IRQ_HANDLED;
}

int test_qspi_slave_controller_init(u32 id, u32 bus_width,
                                    qspi_slave_async_cb cb, void *priv,
                                    qspi_slave_handle *h)
{
    struct qspi_slave_config cfg;
    int ret;
    u32 clk_id, irq_num, clk_in;

    clk_id = qspi_clk_ids[id];
    irq_num = qspi_irq_num[id];
    clk_in = qspi_input_clk[id];

    memset(&cfg, 0, sizeof(cfg));

    cfg.idx = id;
    cfg.clk_in_hz = clk_in;
    cfg.clk_id = clk_id;

    /* Default is Mode0 */
    cfg.cpol = HAL_QSPI_CPOL_ACTIVE_HIGH;
    cfg.cpha = HAL_QSPI_CPHA_FIRST_EDGE;
    cfg.cs_polarity = HAL_QSPI_CS_POL_VALID_LOW;

    ret = hal_qspi_slave_init(h, &cfg);
    if (ret) {
        pr_err("hal_qspi_slave_init failed. ret %d\n", ret);
        return -1;
    }

    hal_qspi_slave_set_bus_width(h, bus_width);

    ret = hal_qspi_slave_register_cb(h, cb, priv);
    if (ret) {
        pr_err("qspi register async callback failed.\n");
        return ret;
    }
    aicos_request_irq(irq_num, qspi_slave_irq_handler, 0, NULL, (void *)h);
    aicos_irq_enable(irq_num);

    return 0;
}

void test_qspi_slave_controller_deinit(qspi_slave_handle *h)
{
    hal_qspi_slave_deinit(h);
}


