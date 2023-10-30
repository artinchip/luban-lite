#ifndef _AIC_QSPI_SLAVE_
#define _AIC_QSPI_SLAVE_

/* Write flow:
 * 1BYTE 3BYTE   512
 * 1. WRITE ADDRESS DATA
 *
 * Read flow:
 * 1. LOAD  ADDRESS
 * 2. GET_STATUS (Slave preparing data, should read N times)
 * 3. Read DATA ()
 */

#define MEM_CMD_WRITE 0x10
#define MEM_CMD_LOAD  0x20
#define MEM_CMD_STATUS 0x21
#define MEM_CMD_READ  0x22

#define WRITE_STATUS_VAL 0xF0F0F0F0
#define LOAD_STATUS_VAL  0xF1F1F1F1

#define TEST_BUF_SIZE (512 * 1024)

/* All data should be 4 bytes aligned. */
#define CMD_SIZE 4
#define STATUS_SIZE 4
#define PKT_SIZE TEST_BUF_SIZE
// #define PKT_SIZE 256

int test_qspi_slave_controller_init(u32 id, u32 bus_width,
                                    qspi_slave_async_cb cb, void *priv,
                                    qspi_slave_handle *h);
void test_qspi_slave_controller_deinit(qspi_slave_handle *h);
#endif
