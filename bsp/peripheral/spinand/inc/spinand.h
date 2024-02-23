/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#ifndef __SPINAND_H__
#define __SPINAND_H__

#include <aic_core.h>

#define SPINAND_SUCCESS     0 /**< success */
#define SPINAND_ERR         1 /**< not found or not supported */
#define SPINAND_ERR_TIMEOUT 2 /**< timeout error */
#define SPINAND_ERR_ECC     3 /**< ecc check error */

#define SPINAND_DIE_ID0 (0)
#define SPINAND_DIE_ID1 (1)

#undef pr_debug
#ifdef AIC_SPINAND_DRV_DEBUG
#define pr_debug pr_info
#else
#define pr_debug(fmt, ...)
#endif

/**
* struct nand_bbt - bad block table object
* @cache: in memory BBT cache
*/
struct nand_bbt {
    u8 *cache;
};

struct aic_spinand;

/* SPI NAND flash information */
struct aic_spinand_info {
    u16 devid;
    u16 page_size;
    u16 oob_size;
    u16 block_per_lun;
    u16 pages_per_eraseblock;
    u8 planes_per_lun;
    u8 is_die_select;
    const char *sz_description;
    struct spi_nand_cmd_cfg *cmd;
    int (*get_status)(struct aic_spinand *flash, u8 status);
};
typedef struct aic_spinand_info *aic_spinand_info_t;

#define DEVID(x)    (u16)(x)
#define PAGESIZE(x) (u16)(x)
#define OOBSIZE(x)  (u16)(x)
#define BPL(x) (u16)(x)
#define PPB(x) (u16)(x)
#define PLANENUM(x) (u8)(x)
#define DIE(x) (u16)(x)

#define SPINAND_MAX_ID_LEN 4

struct spinand_id {
    u8 data[SPINAND_MAX_ID_LEN];
};

struct aic_spinand {
    const struct aic_spinand_info *info;
    struct spinand_id id;
    void *user_data;
    void *lock;
    u8 use_continuous_read;
    u8 qspi_dl_width;
    u8 IsInited;
    u8 *databuf;
    u8 *oobbuf;
    struct nand_bbt bbt;
};
typedef struct aic_spinand *aic_spinand_t;

int spinand_read_id_op(struct aic_spinand *flash, u8 *id);
int spinand_block_erase(struct aic_spinand *flash, u16 blk);
int spinand_flash_init(struct aic_spinand *flash);
int spinand_read_page(struct aic_spinand *flash, u32 page, u8 *data,
                      u32 data_len, u8 *spare, u32 spare_len);
int spinand_block_isbad(struct aic_spinand *flash, u16 blk);
int spinand_continuous_read(struct aic_spinand *flash, u32 page, u8 *data,
                            u32 size);
int spinand_write_page(struct aic_spinand *flash, u32 page, const u8 *data,
                       u32 data_len, const u8 *spare, u32 spare_len);
int spinand_block_markbad(struct aic_spinand *flash, u16 blk);
int spinand_config_set(struct aic_spinand *flash, u8 mask, u8 val);
int spinand_erase(struct aic_spinand *flash, u32 offset, u32 size);
int spinand_read(struct aic_spinand *flash, u8 *addr, u32 offset, u32 size);
int spinand_write(struct aic_spinand *flash, u8 *addr, u32 offset, u32 size);

#ifdef AIC_SPINAND_CONT_READ

#define SPINAND_CMD_READ_FROM_CACHE_X4_CONT_CFG       \
    {                                                 \
        SPINAND_CMD_READ_FROM_CACHE_X4, 1, 0, 0, 4, 4 \
    }
#endif

/*
 * Standard SPI-NAND flash commands
 */
#define SPINAND_CMD_RESET                   0xff
#define SPINAND_CMD_GET_FEATURE             0x0f
#define SPINAND_CMD_SET_FEATURE             0x1f
#define SPINAND_CMD_PAGE_READ               0x13
#define SPINAND_CMD_READ_PAGE_CACHE_RDM     0x30
#define SPINAND_CMD_READ_PAGE_CACHE_LAST    0x3f
#define SPINAND_CMD_READ_FROM_CACHE         0x03
#define SPINAND_CMD_READ_FROM_CACHE_FAST    0x0b
#define SPINAND_CMD_READ_FROM_CACHE_X2      0x3b
#define SPINAND_CMD_READ_FROM_CACHE_DUAL_IO 0xbb
#define SPINAND_CMD_READ_FROM_CACHE_X4      0x6b
#define SPINAND_CMD_READ_FROM_CACHE_QUAD_IO 0xeb
#define SPINAND_CMD_BLK_ERASE               0xd8
#define SPINAND_CMD_PROG_EXC                0x10
#define SPINAND_CMD_PROG_LOAD               0x02
#define SPINAND_CMD_PROG_LOAD_RDM_DATA      0x84
#define SPINAND_CMD_PROG_LOAD_X4            0x32
#define SPINAND_CMD_PROG_LOAD_RDM_DATA_X4   0x34
#define SPINAND_CMD_READ_ID                 0x9f
#define SPINAND_CMD_WR_DISABLE              0x04
#define SPINAND_CMD_WR_ENABLE               0x06
#define SPINAND_CMD_END                     0x0

#define SPINAND_CMD_GET_FEATURE_CFG            \
    {                                          \
        SPINAND_CMD_GET_FEATURE, 1, 1, 1, 0, 1 \
    }
#define SPINAND_CMD_SET_FEATURE_CFG            \
    {                                          \
        SPINAND_CMD_SET_FEATURE, 1, 1, 1, 0, 1 \
    }
#define SPINAND_CMD_WR_ENABLE_CFG            \
    {                                        \
        SPINAND_CMD_WR_ENABLE, 1, 0, 0, 0, 0 \
    }
#define SPINAND_CMD_WR_DISABLE_CFG            \
    {                                         \
        SPINAND_CMD_WR_DISABLE, 1, 0, 0, 0, 0 \
    }
#define SPINAND_CMD_PROG_EXC_CFG            \
    {                                       \
        SPINAND_CMD_PROG_EXC, 1, 3, 1, 0, 0 \
    }
#define SPINAND_CMD_PAGE_READ_CFG            \
    {                                        \
        SPINAND_CMD_PAGE_READ, 1, 3, 1, 0, 0 \
    }
#define SPINAND_CMD_BLK_ERASE_CFG            \
    {                                        \
        SPINAND_CMD_BLK_ERASE, 1, 3, 1, 0, 0 \
    }
#define SPINAND_CMD_READ_ID_CFG            \
    {                                      \
        SPINAND_CMD_READ_ID, 1, 0, 1, 1, 1 \
    }
#define SPINAND_CMD_RESET_CFG            \
    {                                    \
        SPINAND_CMD_RESET, 1, 0, 0, 0, 0 \
    }

struct spi_nand_cmd_cfg {
    u8 opcode;
    u8 opcode_bits;
    u8 addr_bytes;
    u8 addr_bits;
    u8 dummy_bytes;
    u8 data_bits;
};

/* feature registers */
#define REG_BLOCK_LOCK 0xa0
#define REG_DIE_SELECT 0xd0

/* configuration register */
#define REG_CFG         0xb0
#define CFG_OTP_ENABLE  BIT(6)
#define CFG_ECC_ENABLE  BIT(4)
#define CFG_BUF_ENABLE  BIT(3)
#define CFG_QUAD_ENABLE BIT(0)

/* status register */
#define REG_STATUS          0xc0
#define STATUS_BUSY         BIT(0)
#define STATUS_ERASE_FAILED BIT(2)
#define STATUS_PROG_FAILED  BIT(3)

#define STATUS_ECC_MASK             GENMASK(5, 4)
#define STATUS_ECC_NO_BITFLIPS      (0 << 4)
#define STATUS_ECC_HAS_1_4_BITFLIPS (1 << 4)
#define STATUS_ECC_UNCOR_ERROR      (2 << 4)

#ifdef SPI_NAND_WINBOND
extern const struct spinand_manufacturer winbond_spinand_manufacturer;
#endif
#ifdef SPI_NAND_XTX
extern const struct spinand_manufacturer xtx_spinand_manufacturer;
#endif
#ifdef SPI_NAND_GIGADEVICE
extern const struct spinand_manufacturer gigadevice_spinand_manufacturer;
#endif
#ifdef SPI_NAND_FORESEE
extern const struct spinand_manufacturer foresee_spinand_manufacturer;
#endif
#ifdef SPI_NAND_TOSHIBA
extern const struct spinand_manufacturer toshiba_spinand_manufacturer;
#endif
#ifdef SPI_NAND_MACRONIX
extern const struct spinand_manufacturer macronix_spinand_manufacturer;
#endif
#ifdef SPI_NAND_ZETTA
extern const struct spinand_manufacturer zetta_spinand_manufacturer;
#endif
#ifdef SPI_NAND_DOSILICON
extern const struct spinand_manufacturer dosilicon_spinand_manufacturer;
#endif
#ifdef SPI_NAND_ETRON
extern const struct spinand_manufacturer etron_spinand_manufacturer;
#endif
#ifdef SPI_NAND_MICRON
extern const struct spinand_manufacturer micron_spinand_manufacturer;
#endif
#ifdef SPI_NAND_ZBIT
extern const struct spinand_manufacturer zbit_spinand_manufacturer;
#endif
#ifdef SPI_NAND_ESMT
extern const struct spinand_manufacturer esmt_spinand_manufacturer;
#endif
#ifdef SPI_NAND_UMTEK
extern const struct spinand_manufacturer umtek_spinand_manufacturer;
#endif
#ifdef SPI_NAND_QUANXING
extern const struct spinand_manufacturer quanxing_spinand_manufacturer;
#endif

extern struct spi_nand_cmd_cfg cmd_cfg_table[];

const struct aic_spinand_info *
spinand_match_and_init(u8 devid, const struct aic_spinand_info *table,
                       u32 table_size);
int aic_spinand_transfer_message(struct aic_spinand *flash,
                                 struct spi_nand_cmd_cfg *cfg, u32 addr,
                                 u8 *sendBuff, u8 *recvBuff, u32 DataCount);

#endif /* __SPINAND_H__ */
