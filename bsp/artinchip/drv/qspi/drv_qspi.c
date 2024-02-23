/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <aic_hal.h>
#include <hal_qspi.h>
#include <hal_dma.h>
#include <aic_dma_id.h>
#include <aic_clk_id.h>

#define ASYNC_DATA_SIZE 64

#define DMA_SLAVE_MAXBURST_DEFAULT 0

struct aic_qspi {
    struct rt_spi_bus dev;
    char *name;
    uint32_t idx;
    uint32_t clk_id;
    uint32_t clk_in_hz;
    uint32_t dma_port_id;
    uint32_t irq_num;
    qspi_master_handle handle;
    struct rt_qspi_configuration configuration;
    rt_sem_t xfer_sem;
    bool inited;
};

static struct aic_qspi qspi_controller[] = {
#if defined(AIC_USING_QSPI0)
    {
        .name = "qspi0",
        .idx = 0,
        .clk_id = CLK_QSPI0,
        .clk_in_hz = AIC_DEV_QSPI0_MAX_SRC_FREQ_HZ,
        .dma_port_id = DMA_ID_SPI0,
        .irq_num = QSPI0_IRQn,
    },
#endif
#if defined(AIC_USING_QSPI1)
    {
        .name = "qspi1",
        .idx = 1,
        .clk_id = CLK_QSPI1,
        .clk_in_hz = AIC_DEV_QSPI1_MAX_SRC_FREQ_HZ,
        .dma_port_id = DMA_ID_SPI1,
        .irq_num = QSPI1_IRQn,
    },
#endif
#if defined(AIC_USING_QSPI2)
    {
        .name = "qspi2",
        .idx = 2,
        .clk_id = CLK_QSPI2,
        .clk_in_hz = AIC_DEV_QSPI2_MAX_SRC_FREQ_HZ,
        .dma_port_id = DMA_ID_SPI2,
        .irq_num = QSPI2_IRQn,
    },
#endif
#if defined(AIC_USING_QSPI3)
    {
        .name = "qspi3",
        .idx = 3,
        .clk_id = CLK_QSPI3,
        .clk_in_hz = AIC_DEV_QSPI3_MAX_SRC_FREQ_HZ,
        .dma_port_id = DMA_ID_SPI3,
        .irq_num = QSPI3_IRQn,
    },
#endif
};

#ifdef AIC_QSPI_DRV_DEBUG
void dump_cmd(uint8_t *data, uint32_t len)
{
    uint32_t i;

    printf("CMD: (%lu) ", (unsigned long)len);
    for (i = 0; i < len; i++)
        printf("%02X ", data[i]);
    printf("\n");
}
#endif

static uint32_t address_copy(uint32_t addr, uint32_t size, uint8_t *dst)
{
    uint32_t i;

    i = 0;
    while (size) {
        dst[i++] = (addr >> (8 * (size - 1))) & 0xFF;
        size--;
    }
    return i;
}

static rt_err_t qspi_send_command(struct aic_qspi *qspi,
                                  struct rt_qspi_message *qspi_message)
{
    uint32_t single_cnt, rest_cnt, cmd_rest_bw, dmycnt, dmybw;
    uint8_t cmdbuf1[16], cmdbuf2[16];
    struct qspi_transfer t;
    rt_err_t ret = RT_EOK;

    single_cnt = 0;
    rest_cnt = 0;
    cmd_rest_bw = HAL_QSPI_BUS_WIDTH_SINGLE;
    dmybw = HAL_QSPI_BUS_WIDTH_SINGLE;
    if (qspi_message->instruction.qspi_lines == 1) {
        cmdbuf1[single_cnt++] = qspi_message->instruction.content;
    } else if (qspi_message->instruction.qspi_lines > 1) {
        cmdbuf2[rest_cnt++] = qspi_message->instruction.content;
        cmd_rest_bw = qspi_message->instruction.qspi_lines;
        dmybw = qspi_message->instruction.qspi_lines;
    }

    if ((qspi_message->address.size != 0) &&
        (qspi_message->address.qspi_lines == 1)) {
        single_cnt +=
            address_copy(qspi_message->address.content,
                         qspi_message->address.size, &cmdbuf1[single_cnt]);
        dmybw = qspi_message->address.qspi_lines;
    } else if ((qspi_message->address.size != 0) &&
               (qspi_message->address.qspi_lines > 1)) {
        rest_cnt +=
            address_copy(qspi_message->address.content,
                         qspi_message->address.size, &cmdbuf2[rest_cnt]);
        cmd_rest_bw = qspi_message->address.qspi_lines;
        dmybw = qspi_message->address.qspi_lines;
    }

    dmycnt = (dmybw * qspi_message->dummy_cycles) / 8;
    if ((dmycnt != 0) && (dmybw == 1)) {
        memset(&cmdbuf1[single_cnt], 0, dmycnt);
        single_cnt += dmycnt;
    } else if (dmycnt != 0) {
        memset(&cmdbuf2[rest_cnt], 0, dmycnt);
        rest_cnt += dmycnt;
    }

#ifdef AIC_QSPI_DRV_DEBUG
    if (single_cnt)
        dump_cmd(cmdbuf1, single_cnt);
    if (rest_cnt)
        dump_cmd(cmdbuf2, rest_cnt);
#endif
    /* Command + Address + Dummy stage */
    if (single_cnt > 0) {
        hal_qspi_master_set_bus_width(&qspi->handle, HAL_QSPI_BUS_WIDTH_SINGLE);
        t.rx_data = NULL;
        t.tx_data = cmdbuf1;
        t.data_len = single_cnt;
        /* Data is short, use sync API to send */
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
        if (ret) {
            pr_err("Send command failed.\n");
            return -1;
        }
    }
    if (rest_cnt > 0) {
        hal_qspi_master_set_bus_width(&qspi->handle, cmd_rest_bw);
        t.rx_data = NULL;
        t.tx_data = cmdbuf2;
        t.data_len = rest_cnt;
        /* Data is short, use sync API to send */
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
        if (ret) {
            pr_err("Send command failed.\n");
            return -1;
        }
    }

    return 0;
}

static rt_uint32_t drv_qspi_send(struct aic_qspi *qspi,
                                 struct rt_spi_message *message,
                                 const uint8_t *tx, uint32_t size)
{
    struct rt_qspi_message *qspi_message;
    struct qspi_transfer t;
    rt_err_t ret = RT_EOK;

    RT_ASSERT(qspi != RT_NULL);
    RT_ASSERT(message != RT_NULL);

    qspi_master_handle *h;
    struct qspi_bm_transfer bm_t = { 0 };
    h = &qspi->handle;
    if (h->bit_mode) {
        bm_t.tx_data = (uint8_t *)tx;
        bm_t.tx_bits_len = size;
        ret = hal_qspi_master_transfer_bit_mode(h, &bm_t);
        return ret;
    }

    qspi_message = (struct rt_qspi_message *)message;

    /* Command + Address + Dummy stage */
    ret = qspi_send_command(qspi, qspi_message);
    if (ret)
        goto err;

    /* Data stage */
    if (size > 0) {
#ifdef AIC_QSPI_DRV_DEBUG
        printf("tx data bus width: %u, len %lu\n",
               qspi_message->qspi_data_lines, (unsigned long)size);
#endif
        hal_qspi_master_set_bus_width(&qspi->handle,
                                      qspi_message->qspi_data_lines);
        t.rx_data = NULL;
        t.tx_data = (uint8_t *)tx;
        t.data_len = size;
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
        if (ret) {
            pr_err("Send data failed.\n");
            goto err;
        }
    }

    return size;
err:
    return 0;
}
static rt_uint32_t drv_qspi_receive(struct aic_qspi *qspi,
                                    struct rt_spi_message *message, uint8_t *rx,
                                    uint32_t size)
{
    struct rt_qspi_message *qspi_message;
    struct qspi_transfer t;
    rt_err_t ret = RT_EOK;

    RT_ASSERT(qspi != RT_NULL);
    RT_ASSERT(message != RT_NULL);
    RT_ASSERT(rx != RT_NULL);
    RT_ASSERT(size != 0);

    qspi_master_handle *h;
    struct qspi_bm_transfer bm_t = { 0 };
    h = &qspi->handle;
    if (h->bit_mode) {
        bm_t.rx_data = rx;
        bm_t.rx_bits_len = size;
        ret = hal_qspi_master_transfer_bit_mode(h, &bm_t);
        return ret;
    }

    qspi_message = (struct rt_qspi_message *)message;

    /* Command + Address + Dummy stage */
    ret = qspi_send_command(qspi, qspi_message);
    if (ret)
        goto err;

    /* Data stage */
    if (size > 0) {
#ifdef AIC_QSPI_DRV_DEBUG
        printf("rx data bus width: %u, len %lu\n",
               qspi_message->qspi_data_lines, (unsigned long)size);
#endif
        hal_qspi_master_set_bus_width(&qspi->handle,
                                      qspi_message->qspi_data_lines);
        t.tx_data = NULL;
        t.rx_data = (uint8_t *)rx;
        t.data_len = size;
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
        if (ret) {
            pr_err("Recv data failed.\n");
            goto err;
        }
    }

    return size;
err:
    return 0;
}

static rt_uint32_t qspi_xfer(struct rt_spi_device *device,
                             struct rt_spi_message *message)
{
    const rt_uint8_t *sndb = message->send_buf;
    rt_uint8_t *rcvb = message->recv_buf;
    struct rt_qspi_configuration *cfg;
    struct aic_qspi *qspi;
    struct rt_qspi_message *qspi_msg;
    rt_uint32_t ret = 0;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);

    qspi = (struct aic_qspi *)device->bus;
    cfg = &qspi->configuration;
    qspi_msg = (struct rt_qspi_message *)message;

    if (message->cs_take && !(cfg->parent.mode & RT_SPI_NO_CS))
        hal_qspi_master_set_cs(&qspi->handle, 0, true);

    if (rcvb)
        ret = drv_qspi_receive(qspi, message, rcvb, message->length);
    else if (sndb || qspi_msg->instruction.qspi_lines)
        ret = drv_qspi_send(qspi, message, sndb, message->length);

    if (message->cs_release && !(cfg->parent.mode & RT_SPI_NO_CS))
        hal_qspi_master_set_cs(&qspi->handle, 0, false);

    return ret;
}

static void qspi_master_async_callback(qspi_master_handle *h, void *priv)
{
    struct aic_qspi *qspi = priv;
    rt_sem_release(qspi->xfer_sem);
}

static irqreturn_t qspi_irq_handler(int irq_num, void *arg)
{
    qspi_master_handle *h = arg;

    rt_interrupt_enter();
    hal_qspi_master_irq_handler(h);
    rt_interrupt_leave();

    return IRQ_HANDLED;
}

static rt_err_t qspi_configure(struct rt_spi_device *device,
                               struct rt_spi_configuration *configuration)
{
    struct qspi_master_config cfg;
    struct aic_qspi *qspi;
    rt_uint32_t bus_hz;
    rt_err_t ret = RT_EOK;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(configuration != RT_NULL);

    qspi = (struct aic_qspi *)device->bus;

#ifdef AIC_SUPPORT_SPI_IN_BIT_MODE
    if (configuration->mode & RT_SPI_3WIRE) {
        qspi->handle.bit_mode = true;
    } else {
        qspi->handle.bit_mode = false;
    }
#else
    qspi->handle.bit_mode = false;
#endif

    if ((rt_memcmp(&qspi->configuration, configuration,
                   sizeof(struct rt_spi_configuration)) != 0) ||
        (qspi->handle.bit_mode == true)) {
        rt_memcpy(&qspi->configuration, configuration,
                  sizeof(struct rt_spi_configuration));
        rt_memset(&cfg, 0, sizeof(cfg));
        cfg.idx = qspi->idx;
        cfg.clk_in_hz = qspi->clk_in_hz;
        if (cfg.clk_in_hz > HAL_QSPI_MAX_FREQ_HZ)
            cfg.clk_in_hz = HAL_QSPI_MAX_FREQ_HZ;
        cfg.clk_id = qspi->clk_id;

        if (configuration->mode & RT_SPI_MSB)
            cfg.lsb_en = false;
        else
            cfg.lsb_en = true;
        if (configuration->mode & RT_SPI_CPHA)
            cfg.cpha = HAL_QSPI_CPHA_SECOND_EDGE;
        else
            cfg.cpha = HAL_QSPI_CPHA_FIRST_EDGE;

        if (configuration->mode & RT_SPI_CPOL)
            cfg.cpol = HAL_QSPI_CPOL_ACTIVE_LOW;
        else
            cfg.cpol = HAL_QSPI_CPOL_ACTIVE_HIGH;
        if (configuration->mode & RT_SPI_CS_HIGH)
            cfg.cs_polarity = HAL_QSPI_CS_POL_VALID_HIGH;
        else
            cfg.cs_polarity = HAL_QSPI_CS_POL_VALID_LOW;

        ret = hal_qspi_master_init(&qspi->handle, &cfg);
        if (ret) {
            pr_err("qspi init failed.\n");
            return ret;
        }

        if (!qspi->inited) {
#ifdef AIC_DMA_DRV
            struct qspi_master_dma_config dmacfg;
            rt_memset(&dmacfg, 0, sizeof(dmacfg));
            dmacfg.port_id = qspi->dma_port_id;

            dmacfg.tx_bus_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;
            dmacfg.rx_bus_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;
            dmacfg.tx_max_burst = DMA_SLAVE_MAXBURST_DEFAULT;
            dmacfg.rx_max_burst = DMA_SLAVE_MAXBURST_DEFAULT;

            ret = hal_qspi_master_dma_config(&qspi->handle, &dmacfg);
            if (ret) {
                pr_err("qspi dma config failed.\n");
                return ret;
            }
#endif
            ret = hal_qspi_master_register_cb(&qspi->handle,
                                              qspi_master_async_callback, qspi);
            if (ret) {
                pr_err("qspi register async callback failed.\n");
                return ret;
            }
            aicos_request_irq(qspi->irq_num, qspi_irq_handler, 0, NULL,
                              (void *)&qspi->handle);
            aicos_irq_enable(qspi->irq_num);
        }
        bus_hz = configuration->max_hz;
        if (bus_hz > HAL_QSPI_MAX_FREQ_HZ)
            bus_hz = HAL_QSPI_MAX_FREQ_HZ;
        hal_qspi_master_set_bus_freq(&qspi->handle, bus_hz);
        if (ret) {
            pr_err("qspi set bus frequency failed.\n");
            return ret;
        }
    }
    qspi->inited = true;
    return ret;
}

rt_err_t aic_qspi_bus_attach_device(const char *bus_name,
                                    const char *device_name, rt_uint32_t pin,
                                    rt_uint8_t data_line_width,
                                    void (*enter_qspi_mode)(),
                                    void (*exit_qspi_mode)())
{
    struct rt_qspi_device *qspi_device = RT_NULL;
    rt_err_t result = RT_EOK;

    RT_ASSERT(bus_name != RT_NULL);
    RT_ASSERT(device_name != RT_NULL);
    RT_ASSERT(data_line_width == 1 || data_line_width == 2 ||
              data_line_width == 4);

    qspi_device =
        (struct rt_qspi_device *)rt_malloc(sizeof(struct rt_qspi_device));
    if (qspi_device == RT_NULL) {
        pr_err("Failed to malloc memory for qspi device.\n");
        result = RT_ENOMEM;
        goto __exit;
    }

    qspi_device->enter_qspi_mode = enter_qspi_mode;
    qspi_device->exit_qspi_mode = exit_qspi_mode;
    qspi_device->config.qspi_dl_width = data_line_width;

    result = rt_spi_bus_attach_device(&qspi_device->parent, device_name,
                                      bus_name, RT_NULL);

__exit:
    if (result != RT_EOK) {
        if (qspi_device)
            rt_free(qspi_device);
    }

    return result;
}

static const struct rt_spi_ops aic_qspi_ops = {
    .configure = qspi_configure,
    .xfer = qspi_xfer,
};

static int rt_hw_qspi_bus_init(void)
{
    char sem_name[RT_NAME_MAX];
    rt_err_t ret = RT_EOK;
    int i;

    for (i = 0; i < ARRAY_SIZE(qspi_controller); i++) {
        ret = rt_qspi_bus_register(&qspi_controller[i].dev,
                                   qspi_controller[i].name, &aic_qspi_ops);
        if (ret != RT_EOK)
            break;
        rt_sprintf(sem_name, "%s_s", qspi_controller[i].name);
        qspi_controller[i].xfer_sem =
            rt_sem_create(sem_name, 0, RT_IPC_FLAG_PRIO);
        if (!qspi_controller[i].xfer_sem) {
            ret = RT_ENOMEM;
            break;
        }
    }

    return ret;
}

INIT_BOARD_EXPORT(rt_hw_qspi_bus_init);
