#include <string.h>
#include <finsh.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <drv_qspi.h>

#define USAGE \
    "qspidev help : Get this information.\n" \
    "qspidev attach <bus name> <dev name> : Attach device to QSPI bus.\n" \
    "qspidev init <name> <mode> <freq> : Initialize QSPI for device.\n" \
    "qspidev alloc <size> : Allocate buffer for test.\n" \
    "qspidev free <addr>  : Free buffer for test.\n" \
    "qspidev send <lines> <cmd> <addr> <dummy_cnt> <data addr> <data len>: Send data.\n" \
    "             lines: 111, 112, 114, 122, 144 (lines for cmd,addr,data)\n" \
    "             cmd  : hex string, e.g. 01\n" \
    "             addr : hex string, e.g. 000001\n" \
    "                    If address is not present, use \"-\" intead\n" \
    "             dummy_cycles: hex string, e.g. 02, dummy clock cycles\n" \
    "                    If dummy cycles is not present, use \"-\" intead\n" \
    "             data addr: hex string, e.g. 0x40000000\n" \
    "             data len : hex string, e.g. 0x800\n" \
    "qspidev sendhex <lines> <cmd> <addr> <dummy_cnt> <data hex string>: Send data.\n" \
    "             lines: 111, 112, 114, 122, 144 (lines for cmd,addr,data)\n" \
    "             cmd  : hex string, e.g. 01\n" \
    "             addr : hex string, e.g. 000001\n" \
    "                    If address is not present, use \"-\" intead\n" \
    "             dummy_cycles: hex string, e.g. 02, dummy clock cycles\n" \
    "                    If dummy cycles is not present, use \"-\" intead\n" \
    "             data hex string: hex string, e.g. 41 42 43 44 45\n" \
    "qspidev recv <lines> <cmd> <addr> <dummy_cnt> <data addr> <data len>: Send cmd and recieve data\n" \
    "             lines: 111, 112, 114, 122, 144 (lines for cmd,addr,data)\n" \
    "             cmd  : hex string, e.g. 01\n" \
    "             addr : hex string, e.g. 000001\n" \
    "                    If address is not present, use \"-\" intead\n" \
    "             dummy_cycles: hex string, e.g. 02, dummy clock cycles\n" \
    "                    If dummy cycles is not present, use \"-\" intead\n" \
    "             data addr: hex string, e.g. 0x40000000\n" \
    "             data len : hex string, e.g. 0x800\n" \
    "qspidev recvhex <lines> <cmd> <addr> <dummy_cnt> <data length>: Send cmd and recieve data, then print\n" \
    "             lines: 111, 112, 114, 122, 144 (lines for cmd,addr,data)\n" \
    "             cmd  : hex string, e.g. 01\n" \
    "             addr : hex string, e.g. 000001\n" \
    "                    If address is not present, use \"-\" intead\n" \
    "             dummy_cycles: hex string, e.g. 02, dummy clock cycles\n" \
    "                    If dummy cycles is not present, use \"-\" intead\n" \
    "             data len : hex string, e.g. 0x800\n" \
    "example:\n" \
    "qspidev attach qspi1 qtestdev\n" \
    "qspidev init qtestdev 3 50000000\n" \
    "qspidev recvhex 111 9f - 8 3\n" \
    "qspidev sendhex 111 ff\n" \
    "qspidev sendhex 111 13 000000\n" \
    "qspidev recvhex 111 03 000000 0 0x800\n"

static void qspi_usage(void)
{
    printf("%s", USAGE);
}

static struct rt_qspi_device *g_qspi;

static int test_qspi_attach(int argc, char **argv)
{
    char *bus_name, *dev_name;

    if (argc != 3) {
        qspi_usage();
        return -1;
    }
    bus_name = argv[1];
    dev_name = argv[2];

    /* Attach/Create dev to spi bus */
    aic_qspi_bus_attach_device(bus_name, dev_name, 0, 4, RT_NULL, RT_NULL);
    return 0;
}

static int test_qspi_init(int argc, char **argv)
{
    struct rt_qspi_configuration qspi_cfg;
    struct rt_device *dev;
    char *name;
    int ret = 0;

    if (argc != 4) {
        printf("Argument error, please see help information.\n");
        return -1;
    }
    name = argv[1];

    g_qspi = (struct rt_qspi_device *)rt_device_find(name);
    if (!g_qspi) {
        printf("Failed to get device in name %s\n", name);
        return -1;
    }

    dev = (struct rt_device *)g_qspi;
    if (dev->type != RT_Device_Class_SPIDevice) {
        g_qspi = NULL;
        printf("%s is not SPI device.\n", name);
        return -1;
    }
    qspi_cfg.qspi_dl_width = 4;
    qspi_cfg.parent.mode = atol(argv[2]);
    qspi_cfg.parent.max_hz = atol(argv[3]);
    ret = rt_qspi_configure(g_qspi, &qspi_cfg);
    if (ret < 0) {
        printf("qspi configure failure.\n");
        return ret;
    }
    return 0;
}

static bool qspi_lines_check(uint32_t line)
{
    switch (line) {
        case 111: /* cmd 1 line, addr 1 line, data 1 line */
        case 112: /* cmd 1 line, addr 1 line, data 2 line */
        case 122: /* cmd 1 line, addr 2 line, data 2 line */
        case 114: /* cmd 1 line, addr 1 line, data 4 line */
        case 144: /* cmd 1 line, addr 4 line, data 4 line */
            return true;
        default:
            return false;
    }
    return true;
}

static void test_qspi_sendhex(int argc, char **argv)
{
    char *pl;
    uint32_t line = 0, addrsiz = 0, addr = 0, dmycyc = 0;
    unsigned long i, data_len, align_len;
    uint8_t cmd, *data;
    struct rt_qspi_message msg;
    rt_size_t ret;

    if (!g_qspi) {
        printf("QSPI device is not init yet.\n");
        return;
    }
    if (argc < 3) {
        printf("Argument is not correct, please see help for more information.\n");
        return;
    }
    pl = argv[1];
    line = strtol(pl, NULL, 10);
    if (qspi_lines_check(line) == false) {
        qspi_usage();
        return;
    }
    cmd = (uint8_t)strtol(argv[2], NULL, 16);
    addrsiz = 0;
    addr = 0;
    if (argc >= 4) {
        if (rt_memcmp(argv[3], "-", 1)) {
            addrsiz = (strlen(argv[3]) + 1) >> 1;
            addr = strtol(argv[3], NULL, 16);
        } else {
            addrsiz = 0;
            addr = 0;
        }
    }
    dmycyc = 0;
    data_len = 0;
    if (argc >= 5) {
        if (rt_memcmp(argv[4], "-", 1))
            dmycyc = strtol(argv[4], NULL, 10);
        data_len = argc - 5;
    }
    data = RT_NULL;
    printf("data len %ld\n", data_len);
    if (data_len) {
        align_len = roundup(data_len, CACHE_LINE_SIZE);
        data = aicos_malloc_align(0, align_len, CACHE_LINE_SIZE);
        for (i = 5; i < argc; i++) {
            data[i - 5] = (uint8_t)strtol(argv[i], NULL, 16);
            printf("%s -> %02x\n", argv[i], data[i - 5]);
        }
    }
    rt_memset(&msg, 0, sizeof(msg));
    msg.instruction.content = cmd;
    msg.instruction.qspi_lines = pl[0] - '0';
    if (addrsiz) {
        msg.address.content = addr;
        msg.address.size = addrsiz;
        msg.address.qspi_lines = pl[1] - '0';
    }
    msg.dummy_cycles = dmycyc;
    if (data) {
        msg.qspi_data_lines = pl[2] - '0';
        msg.parent.send_buf = (void *)data;
        msg.parent.length = data_len;
    }
    msg.parent.cs_take = 1;
    msg.parent.cs_release = 1;
    rt_spi_take_bus((struct rt_spi_device *)g_qspi);
    ret = rt_qspi_transfer_message(g_qspi, &msg);
    if (ret != data_len) {
        printf("Send data failed. ret 0x%x\n", (int)ret);
    }
    rt_spi_release_bus((struct rt_spi_device *)g_qspi);
    if (data)
        aicos_free_align(0, data);
}

static void test_qspi_recvhex(int argc, char **argv)
{
    char *pl;
    uint32_t line = 0, addrsiz = 0, addr = 0, dmycyc = 0;
    unsigned long i, data_len, align_len;
    uint8_t *data, cmd;
    struct rt_qspi_message msg;

    if (!g_qspi) {
        printf("QSPI device is not init yet.\n");
        return;
    }
    if (argc < 3) {
        printf("Argument is not correct, please see help for more information.\n");
        return;
    }
    pl = argv[1];
    line = strtol(pl, NULL, 10);
    if (qspi_lines_check(line) == false) {
        qspi_usage();
        return;
    }
    cmd = (uint8_t)strtol(argv[2], NULL, 16);
    addrsiz = 0;
    if (argc >= 4) {
        if (rt_memcmp(argv[3], "-", 1)) {
            addrsiz = (strlen(argv[3]) + 1) >> 1;
            addr = strtol(argv[3], NULL, 16);
        } else {
            addrsiz = 0;
            addr = 0;
        }
    }
    dmycyc = 0;
    if (argc >= 5) {
        if (rt_memcmp(argv[4], "-", 1))
            dmycyc = strtol(argv[4], NULL, 16);
    }
    data_len = 0;
    if (argc >= 6) {
        data_len = strtol(argv[5], NULL, 16);
    }
    data = RT_NULL;
    if (data_len > 0) {
        align_len = roundup(data_len, CACHE_LINE_SIZE);
        data = aicos_malloc_align(0, align_len, CACHE_LINE_SIZE);
    }
    rt_memset(&msg, 0, sizeof(msg));
    msg.instruction.content = cmd;
    msg.instruction.qspi_lines = pl[0] - '0';
    if (addrsiz) {
        msg.address.content = addr;
        msg.address.size = addrsiz;
        msg.address.qspi_lines = pl[1] - '0';
    }
    msg.dummy_cycles = dmycyc;
    msg.qspi_data_lines = pl[2] - '0';
    msg.parent.recv_buf = (void *)data;
    msg.parent.length = data_len;
    msg.parent.cs_take = 1;
    msg.parent.cs_release = 1;
    rt_spi_take_bus((struct rt_spi_device *)g_qspi);
    rt_qspi_transfer_message(g_qspi, &msg);
    rt_spi_release_bus((struct rt_spi_device *)g_qspi);

    if (data) {
        printf("\n");
        for (i = 0; i < data_len; i++) {
            if (i && (i % 16) == 0)
                printf("\n");
            printf("%02x ", data[i]);
        }
        printf("\n");
        aicos_free_align(0, data);
    }
}

static void test_qspi_send(int argc, char **argv)
{
    char *pl;
    uint32_t line = 0, addrsiz = 0, addr = 0, dmycyc = 0;
    unsigned long data, data_len;
    uint8_t cmd;
    rt_size_t ret;
    struct rt_qspi_message msg;

    if (!g_qspi) {
        printf("QSPI device is not init yet.\n");
        return;
    }
    if (argc < 3) {
        printf("Argument is not correct, please see help for more information.\n");
        return;
    }
    pl = argv[1];
    line = strtol(pl, NULL, 10);
    if (qspi_lines_check(line) == false) {
        qspi_usage();
        return;
    }
    cmd = (uint8_t)strtol(argv[2], NULL, 16);
    addrsiz = 0;
    addr = 0;
    if (argc >= 4) {
        if (rt_memcmp(argv[3], "-", 1)) {
            addrsiz = (strlen(argv[3]) + 1) >> 1;
            addr = strtol(argv[3], NULL, 16);
        } else {
            addrsiz = 0;
            addr = 0;
        }
    }
    dmycyc = 0;
    if (argc >= 5) {
        dmycyc = strtol(argv[4], NULL, 16);
    }
    data = 0;
    if (argc >= 6) {
        data = strtol(argv[5], NULL, 16);
    }
    data_len = 0;
    if (argc >= 7) {
        data_len = strtol(argv[6], NULL, 16);
    }
    rt_memset(&msg, 0, sizeof(msg));
    msg.instruction.content = cmd;
    msg.instruction.qspi_lines = pl[0] - '0';
    if (addrsiz) {
        msg.address.content = addr;
        msg.address.size = addrsiz;
        msg.address.qspi_lines = pl[1] - '0';
    }
    msg.dummy_cycles = dmycyc;
    if (data) {
        msg.qspi_data_lines = pl[2] - '0';
        msg.parent.send_buf = (void *)data;
        msg.parent.length = data_len;
    }
    msg.parent.cs_take = 1;
    msg.parent.cs_release = 1;
    rt_spi_take_bus((struct rt_spi_device *)g_qspi);
    ret = rt_qspi_transfer_message(g_qspi, &msg);
    if (ret != data_len) {
        printf("Send data failed. ret 0x%x\n", (int)ret);
    }
    rt_spi_release_bus((struct rt_spi_device *)g_qspi);
}

static void test_qspi_recv(int argc, char **argv)
{
    char *pl;
    uint32_t line = 0, addrsiz = 0, addr = 0, dmycyc = 0;
    unsigned long data, data_len;
    uint8_t cmd;
    rt_size_t ret;
    struct rt_qspi_message msg;

    if (!g_qspi) {
        printf("QSPI device is not init yet.\n");
        return;
    }
    if (argc < 3) {
        printf("Argument is not correct, please see help for more information.\n");
        return;
    }
    pl = argv[1];
    line = strtol(pl, NULL, 10);
    if (qspi_lines_check(line) == false) {
        qspi_usage();
        return;
    }
    cmd = (uint8_t)strtol(argv[2], NULL, 16);
    addrsiz = 0;
    if (argc >= 4) {
        if (rt_memcmp(argv[3], "-", 1)) {
            addrsiz = (strlen(argv[3]) + 1) >> 1;
            addr = strtol(argv[3], NULL, 16);
        } else {
            addrsiz = 0;
            addr = 0;
        }
    }
    dmycyc = 0;
    if (argc >= 5) {
        dmycyc = strtol(argv[4], NULL, 16);
    }
    data = 0;
    if (argc >= 6) {
        data = strtol(argv[5], NULL, 16);
    }
    data_len = 0;
    if (argc >= 7) {
        data_len = strtol(argv[6], NULL, 16);
    }
    rt_memset(&msg, 0, sizeof(msg));
    msg.instruction.content = cmd;
    msg.instruction.qspi_lines = pl[0] - '0';
    if (addrsiz) {
        msg.address.content = addr;
        msg.address.size = addrsiz;
        msg.address.qspi_lines = pl[1] - '0';
    }
    msg.dummy_cycles = dmycyc;
    msg.qspi_data_lines = pl[2] - '0';
    msg.parent.recv_buf = (void *)data;
    msg.parent.length = data_len;
    msg.parent.cs_take = 1;
    msg.parent.cs_release = 1;
    rt_spi_take_bus((struct rt_spi_device *)g_qspi);
    ret = rt_qspi_transfer_message(g_qspi, &msg);
    if (ret != data_len) {
        printf("Recv data failed. ret 0x%x\n", (int)ret);
    }
    rt_spi_release_bus((struct rt_spi_device *)g_qspi);
}

extern void test_qspi_slaverw(struct rt_qspi_device *qspi, int argc, char **argv);
extern void test_qspi_send2slave(struct rt_qspi_device *qspi, int argc, char **argv);
static void cmd_test_qspi(int argc, char **argv)
{
    unsigned long size = 0, addr = 0, align_len;

    if (argc < 2)
        goto help;

    if (!rt_strcmp(argv[1], "help")) {
        goto help;
    } else if (!rt_strcmp(argv[1], "attach")) {
        test_qspi_attach(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "init")) {
        test_qspi_init(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "alloc")) {
        size = strtol(argv[2], NULL, 0);
        align_len = roundup(size, CACHE_LINE_SIZE);
        printf("0x%08lx\n", (unsigned long)aicos_malloc_align(0, align_len, CACHE_LINE_SIZE));
        printf("size %lu\n", align_len);
        return;
    } else if (!rt_strcmp(argv[1], "free")) {
        addr = strtol(argv[2], NULL, 0);
        aicos_free_align(0, (void *)addr);
        printf("free 0x%lx\n", addr);
        return;
    } else if (!rt_strcmp(argv[1], "sendhex")) {
        test_qspi_sendhex(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "recvhex")) {
        test_qspi_recvhex(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "send")) {
        test_qspi_send(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "recv")) {
        test_qspi_recv(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "slaverw")) {
#ifdef AIC_QSPI_DRV_V11
        if (!g_qspi) {
            printf("QSPI device is not init yet.\n");
            return;
        }
        test_qspi_slaverw(g_qspi, argc - 1, &argv[1]);
        return;
#endif
    } else if (!rt_strcmp(argv[1], "send2slave")) {
#ifdef AIC_QSPI_DRV_V11
        if (!g_qspi) {
            printf("QSPI device is not init yet.\n");
            return;
        }
        test_qspi_send2slave(g_qspi, argc - 1, &argv[1]);
        return;
#endif
    }
help:
    qspi_usage();
}

MSH_CMD_EXPORT_ALIAS(cmd_test_qspi, qspidev, Test QSPI);
