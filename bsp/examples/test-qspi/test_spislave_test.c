#include <string.h>
#include <finsh.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <drv_qspi.h>
#include "test_spislave.h"

struct bus_cfg {
    u8 cmd; /* CMD bus width */
    u8 addr; /* Address bus width */
    u8 dmycyc; /* Dummy clock cycle */
    u8 data; /* Data bus width */
};

extern void slave_dump_data(char *msg, u8 *buf, u32 len);

void master_tx(struct rt_qspi_device *qspi, u8 bus_width, u8 *buf, u32 datalen)
{
    struct rt_qspi_message msg;
    struct bus_cfg cfg;
    rt_size_t ret;

    cfg.cmd = 0;
    cfg.addr = 0;
    cfg.dmycyc = 0;
    cfg.data = bus_width;

    rt_memset(&msg, 0, sizeof(msg));
    msg.qspi_data_lines = cfg.data;
    msg.parent.recv_buf = NULL;
    msg.parent.send_buf = buf;
    msg.parent.length = datalen;
    msg.parent.cs_take = 1;
    msg.parent.cs_release = 1;
    rt_spi_take_bus((struct rt_spi_device *)qspi);
    ret = rt_qspi_transfer_message(qspi, &msg);
    if (ret != datalen) {
        printf("master tx failed. ret 0x%x\n", ret);
    }
    rt_spi_release_bus((struct rt_spi_device *)qspi);
}

void master_rx(struct rt_qspi_device *qspi, u8 bus_width, u8 *buf, u32 datalen)
{
    struct rt_qspi_message msg;
    struct bus_cfg cfg;
    rt_size_t ret;

    cfg.cmd = 0;
    cfg.addr = 0;
    cfg.dmycyc = 0;
    cfg.data = bus_width;

    rt_memset(&msg, 0, sizeof(msg));
    msg.qspi_data_lines = cfg.data;
    msg.parent.recv_buf = buf;
    msg.parent.send_buf = NULL;
    msg.parent.length = datalen;
    msg.parent.cs_take = 1;
    msg.parent.cs_release = 1;
    rt_spi_take_bus((struct rt_spi_device *)qspi);
    ret = rt_qspi_transfer_message(qspi, &msg);
    if (ret != datalen) {
        printf("master tx failed. ret 0x%x\n", ret);
    }
    rt_spi_release_bus((struct rt_spi_device *)qspi);
}

static void slave_get_status(struct rt_qspi_device *qspi, u8 bus_width, u8 *buf,
                             u32 datalen)
{
    master_rx(qspi, bus_width, buf, datalen);
}

void slave_write(struct rt_qspi_device *qspi, u8 bus_width, u8 *buf, u32 datalen, u32 addr)
{
    u8 *work_buf;
    u32 status;

    work_buf = aicos_malloc_align(0, datalen + 4, CACHE_LINE_SIZE);

    work_buf[0] = MEM_CMD_WRITE;
    memcpy(&work_buf[1], &addr, 3);
    memcpy(&work_buf[4], buf, datalen);
    master_tx(qspi, bus_width, work_buf, datalen + 4);

    rt_thread_mdelay(10);
    do {
        status = 0;
        slave_get_status(qspi, bus_width, (void *)&status, 4);
        printf("write status 0x%x\n", status);
        rt_thread_mdelay(1000);
    } while (status != WRITE_STATUS_VAL);

    aicos_free_align(0, work_buf);
}

static void slave_load_data(struct rt_qspi_device *qspi, u8 bus_width, u32 addr)
{
    u8 work_buf[4];

    work_buf[0] = MEM_CMD_LOAD;
    memcpy(&work_buf[1], &addr, 3);
    master_tx(qspi, bus_width, work_buf, 4);
}

static void slave_read_data(struct rt_qspi_device *qspi, u8 bus_width,
                            u8 *buf, u32 datalen)
{
    master_rx(qspi, bus_width, buf, datalen);
}

void slave_read(struct rt_qspi_device *qspi, u8 bus_width, u8 *buf, u32 datalen,
                u32 addr)
{
    u32 status;

    slave_load_data(qspi, bus_width, addr);
    rt_thread_mdelay(100);
    do {
        status = 0;
        slave_get_status(qspi, bus_width, (void *)&status, 4);
        printf("load status 0x%x\n", status);
        rt_thread_mdelay(1000);
    } while (status != LOAD_STATUS_VAL);

    slave_read_data(qspi, bus_width, buf, datalen);
}

void test_qspi_slaverw(struct rt_qspi_device *qspi, int argc, char **argv)
{
    u8 *tx_buf, *rx_buf;
    u8 bus_width;
    unsigned long val;

    bus_width = 1;

    if (argc >= 2) {
        val = strtol(argv[1], NULL, 10);
        bus_width = (u8)val;
    }
    tx_buf = aicos_malloc_align(0, TEST_BUF_SIZE, CACHE_LINE_SIZE);
    rx_buf = aicos_malloc_align(0, TEST_BUF_SIZE, CACHE_LINE_SIZE);
    for (int i = 0; i < TEST_BUF_SIZE; i++) {
        tx_buf[i] = i % 256;
    }

    printf("\nHOST: Write data to address 0\n");
    slave_write(qspi, bus_width, tx_buf, PKT_SIZE, 0);

    rt_thread_mdelay(100);

    printf("\nHOST: Write data to address 0\n");
    slave_write(qspi, bus_width, tx_buf, PKT_SIZE, 0);

    rt_thread_mdelay(100);

    printf("\nHOST: Write data to address 0\n");
    slave_write(qspi, bus_width, tx_buf, PKT_SIZE, 0);

    rt_thread_mdelay(100);
    printf("\nHOST: Read data from address 0\n");
    memset(rx_buf, 0, PKT_SIZE);
    slave_read(qspi, bus_width, rx_buf, PKT_SIZE, 0);
    slave_dump_data("Read", rx_buf, PKT_SIZE);

    rt_thread_mdelay(100);
    printf("\nHOST: Write data to address 0x%x\n", PKT_SIZE);
    slave_write(qspi, bus_width, tx_buf + PKT_SIZE, PKT_SIZE, PKT_SIZE);
    rt_thread_mdelay(100);
    printf("\nHOST: Read data from address 0x10\n");
    memset(rx_buf, 0, PKT_SIZE);
    slave_read(qspi, bus_width, rx_buf, PKT_SIZE, 0x10);
    slave_dump_data("Read", rx_buf, PKT_SIZE);

    printf("\nHOST: Read data from address 0x01\n");
    memset(rx_buf, 0, PKT_SIZE);
    slave_read(qspi, bus_width, rx_buf, PKT_SIZE, 0x01);
    slave_dump_data("Read", rx_buf, PKT_SIZE);

    printf("\nHOST: Read data from address 0x02\n");
    memset(rx_buf, 0, PKT_SIZE);
    slave_read(qspi, bus_width, rx_buf, PKT_SIZE, 0x02);
    slave_dump_data("Read", rx_buf, PKT_SIZE);

    aicos_free_align(0, tx_buf);
    aicos_free_align(0, rx_buf);
}

void test_qspi_send2slave(struct rt_qspi_device *qspi, int argc, char **argv)
{
    u8 *tx_buf, *rx_buf;
    u8 bus_width;
    unsigned long val;
    u32 cksum, *p;

    bus_width = 1;

    if (argc >= 2) {
        val = strtol(argv[1], NULL, 10);
        bus_width = (u8)val;
    }
    tx_buf = aicos_malloc_align(0, TEST_BUF_SIZE, CACHE_LINE_SIZE);
    rx_buf = aicos_malloc_align(0, TEST_BUF_SIZE, CACHE_LINE_SIZE);

    for (int i = 0; i < 100; i++) {
        printf("\nHOST: Write data to address 0, round %d\n", i);
        p = (void *)tx_buf;
        for (int j = 0; j < TEST_BUF_SIZE; j++) {
            //tx_buf[i] = 0xA << 4 | (i % 16);
            // tx_buf[i] = 0xA5;
            tx_buf[i] = (j + i) % 256;
        }
        tx_buf[0] = i;

        cksum = 0;
        for (int i = 0; i < TEST_BUF_SIZE / 4; i++) {
            cksum += *p;
            p++;
        }
        printf("ckxsum 0x%x\n", cksum);
        master_tx(qspi, bus_width, tx_buf, PKT_SIZE);

        rt_thread_mdelay(1000);
    }

    aicos_free_align(0, tx_buf);
    aicos_free_align(0, rx_buf);
}

