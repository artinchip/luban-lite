#ifndef _AIC_HAL_CAN_H_
#define _AIC_HAL_CAN_H_

#include <aic_core.h>

/* Register address */
#define CAN_MODE_REG                       0x0000
#define CAN_MCR_REG                        0x0004
#define CAN_STAT_REG                       0x0008
#define CAN_INTR_REG                       0x000C
#define CAN_INTEN_REG                      0x0010
#define CAN_BTR0_REG                       0x0018
#define CAN_BTR1_REG                       0x001C
#define CAN_ARBLOST_REG                    0x002C
#define CAN_ERRCODE_REG                    0x0030
#define CAN_ERRWT_REG                      0x0034
#define CAN_RXERR_REG                      0x0038
#define CAN_TXERR_REG                      0x003C
#define CAN_BUF0_REG                       0x0040
#define CAN_BUF1_REG                       0x0044
#define CAN_BUF2_REG                       0x0048
#define CAN_BUF3_REG                       0x004C
#define CAN_BUF4_REG                       0x0050
#define CAN_BUF5_REG                       0x0054
#define CAN_RXCODE0_REG                    0x0040
#define CAN_RXCODE1_REG                    0x0044
#define CAN_RXCODE2_REG                    0x0048
#define CAN_RXCODE3_REG                    0x004C
#define CAN_RXMASK0_REG                    0x0050
#define CAN_RXMASK1_REG                    0x0054
#define CAN_RXMASK2_REG                    0x0058
#define CAN_RXMASK3_REG                    0x005C
#define CAN_RXC_REG                        0x0074
#define CAN_RSADDR_REG                     0x0078
#define CAN_RXFIFO_REG                     0x0080
#define CAN_TXBRO_REG                      0x0180
#define CAN_VERSION_REG                    0x0FFC

/* Register bit filed */
/* Mode register */
#define CAN_MODE_SLEEP                     BIT(4)
#define CAN_MODE_WAKEUP                    (0 << 4)
#define CAN_MODE_FILTER_SINGLE             BIT(3)
#define CAN_MODE_FILTER_DUAL               (0 << 3)
#define CAN_MODE_SELFTEST                  BIT(2)
#define CAN_MODE_LISTEN                    BIT(1)
#define CAN_MODE_RST                       BIT(0)
#define CAN_MODE_NORMAL			   0
#define CAN_MODE_MASK                      0x17

/* Control reg, write only */
#define CAN_MCR_SELFREQ                    BIT(4)
#define CAN_MCR_CLR_OVF                    BIT(3)
#define CAN_MCR_RXB_REL                    BIT(2)
#define CAN_MCR_ABORTREQ                   BIT(1)
#define CAN_MCR_TXREQ                      BIT(0)

/* Status reg, read only */
#define CAN_STAT_BUS                       BIT(7)
#define CAN_STAT_ERR                       BIT(6)
#define CAN_STAT_TX                        BIT(5)
#define CAN_STAT_RX                        BIT(4)
#define CAN_STAT_TXC                       BIT(3)
#define CAN_STAT_TXB                       BIT(2)
#define CAN_STAT_OVF                       BIT(1)
#define CAN_STAT_RXB                       BIT(0)

/* interrupt flag reg */
#define CAN_INTR_ERRB                      BIT(7)
#define CAN_INTR_ARBLOST                   BIT(6)
#define CAN_INTR_ERRP                      BIT(5)
#define CAN_INTR_WAKEUP                    BIT(4)
#define CAN_INTR_OVF                       BIT(3)
#define CAN_INTR_ERRW                      BIT(2)
#define CAN_INTR_TX                        BIT(1)
#define CAN_INTR_RX                        BIT(0)

/* interrupt enable reg */
#define CAN_INTEN_ERRB                     BIT(7)
#define CAN_INTEN_ARBLOST                  BIT(6)
#define CAN_INTEN_ERRP                     BIT(5)
#define CAN_INTEN_WAKEUP                   BIT(4)
#define CAN_INTEN_OVF                      BIT(3)
#define CAN_INTEN_ERRW                     BIT(2)
#define CAN_INTEN_TXI                      BIT(1)
#define CAN_INTEN_RXI                      BIT(0)

/* btr0 reg */
#define CAN_BTR0_SJW_MASK                  GENMASK(7, 6)
#define CAN_BTR0_BRP_MASK                  (0x3F)

/* btr1 reg */
#define CAN_BTR1_SAM_MASK                  BIT(7)
#define CAN_BTR1_TS2_MASK                  GENMASK(6, 4)
#define CAN_BTR1_TS1_MASK                  (0xF)

/* arb lost reg */
#define CAN_ARBLOST_CAP_MASK               (0x1F)

/* error code reg */
#define CAN_ERRCODE_ERRTYPE_MASK           GENMASK(7, 6)
#define CAN_ERRCODE_ERRTYPE_BIT            (0x0 << 6)
#define CAN_ERRCODE_ERRTYPE_FORMAT         (0x1 << 6)
#define CAN_ERRCODE_ERRTYPE_STUFF          (0x2 << 6)
#define CAN_ERRCODE_ERRTYPE_OTHER          (0x3 << 6)
#define CAN_ERRCODE_DIR                    BIT(5)
#define CAN_ERRCODE_SEGCODE_MASK           (0x1f)
#define CAN_ERRCODE_ACK_SLOT               (0x19)

/* buf0 reg */
#define CAN_BUF0_MSG_EFF_FLAG              BIT(7)
#define CAN_BUF0_MSG_RTR_FLAG              BIT(6)
#define CAN_BUF0_MSG_EFF_SHIFT             7
#define CAN_BUF0_MSG_RTR_SHIFT             6

#define CAN_TSEG_MIN                       7
#define CAN_ERRP_THRESHOLD                 127

typedef enum {
    ACTIVE_STATUS,
    WARNING_STATUS,
    PASSIVE_STATUS,
    BUS_OFF,
} can_state_t;

struct can_bittiming_const {
    u32 sync_seg;
    u32 tseg1_min;
    u32 tseg1_max;
    u32 tseg2_min;
    u32 tseg2_max;
    u32 sjw_max;
    u32 brp_min;
    u32 brp_max;
    u32 brp_inc;
};

typedef enum {
    CAN_NORMAL_MODE,
    CAN_RESET_MODE      = BIT(0),
    CAN_LISTEN_MODE     = BIT(1),
    CAN_SELFTEST_MODE   = BIT(2),
    CAN_SLEEP_MODE      = BIT(4),
} can_mode_t;

typedef enum {
    TX_REQ          = BIT(0),
    ABORT_REQ       = BIT(1),
    RXB_REL_REQ     = BIT(2),
    CLR_OF_REQ      = BIT(3),
    SELF_REQ        = BIT(4),
} can_op_req_t;

typedef struct {
    u8 code;
    char *msg;
} bus_err_msg_t;

typedef struct can_status {
    can_state_t current_state;
    u32    recverrcnt;
    u32    snderrcnt;
    u32    rxovercnt;
    u32    arblostcnt;
    u32    biterrcnt;
    u32    formaterrcnt;
    u32    stufferrcnt;
    u32    othererrcnt;
    u32    recvpkgcnt;
    u32    sndpkgcnt;
    u32    rxerr;
    u32    txerr;
} can_status_t;

typedef struct {
    u32    id;
    u8     rtr;
    u8     ide;
    u8     dlc;
    u8     data[8];
} can_msg_t;

typedef struct can_handle can_handle;
struct can_handle {
	unsigned long can_base;
	u32 irq_num;
	u32 clk_id;
	u32 idx;
	void (*callback)(can_handle * phandle, void *arg);
	void *arg;
	u32 baudrate;
	can_msg_t msg;
	can_status_t status;
};

typedef enum {
    FILTER_CLOSE        = 0,
    SINGLE_FILTER_MODE  = 1,
    DUAL_FILTER_MODE    = 2,
} can_filter_mode_t;

typedef union {
    struct single_filter_std {
        u16    id_filter;
        u8     rtr_filter;
        u8     data0_filter;
        u8     data1_filter;
    } sfs;

    struct single_filter_ext {
        u32    id_filter;
        u8     rtr_filter;
    } sfe;

    struct dual_filter_std {
        u16    id_filter0;
        u8     rtr_filter0;
        u8     data0_filter0;
        u16    id_filter1;
        u8     rtr_filter1;
    } dfs;

    struct dual_filter_ext {
        u16    id_filter0;
        u16    id_filter1;
    } dfe;
} can_filter_t;

typedef struct can_filter_config {
    can_filter_mode_t   filter_mode;
    /* is_eff indicates whether the filter is used to filter extended frame */
    u8                  is_eff;
    can_filter_t        rxcode;
    can_filter_t        rxmask;
} can_filter_config_t;

static inline void hal_can_enable_interrupt(can_handle *phandle)
{
	writel(0xFF, phandle->can_base + CAN_INTEN_REG);
}

static inline void hal_can_disable_interrupt(can_handle *phandle)
{
	writel(0, phandle->can_base + CAN_INTEN_REG);
}

#define CAN_IOCTL_SET_MODE          1
#define CAN_IOCTL_SET_FILTER        2
#define CAN_IOCTL_SET_BAUDRATE      4
#define CAN_IOCTL_GET_BAUDRATE      8

#define CAN_EVENT_RX_IND         0x01    /* Rx indication */
#define CAN_EVENT_TX_DONE        0x02    /* Tx complete   */
#define CAN_EVENT_TX_FAIL        0x03    /* Tx fail   */
#define CAN_EVENT_RXOF_IND       0x06    /* Rx overflow */

int hal_can_init(can_handle *phandle, u32 can_idx);
void hal_can_uninit(can_handle *phandle);
void hal_can_send_frame(can_handle *phandle, can_msg_t * msg, can_op_req_t req);
void hal_can_receive_frame(can_handle *phandle, can_msg_t * msg);
int hal_can_ioctl(can_handle *phandle, int cmd, void *arg);
int hal_can_attach_callback(can_handle *phandle, void *callback, void *arg);
void hal_can_detach_callback(can_handle *phandle);
irqreturn_t hal_can_isr_handler(int irq_num, void *arg);

#endif