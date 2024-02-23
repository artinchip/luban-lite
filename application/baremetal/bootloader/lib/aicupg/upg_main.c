/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <string.h>
#include <usbdescriptors.h>
#include <usbdevice.h>
#include <usb_drv.h>
#include <usbupg.h>
#include <aic_core.h>
#include <aicupg.h>
#include "upg_internal.h"

#define false 0
#define true  1

struct upg_internal upg_info = {
    .cur_cmd = NULL,
    .dev_type = UPG_DEV_TYPE_RAM,
    .dev_id = 0,
    .cfg = {
        .mode = 0,
    }
};

static int check_cmd_header(struct cmd_header *h)
{
    u32 sum;

    if (h->magic != UPG_CMD_HEADER_MAGIC)
        return false;
    if (h->protocol != UPG_PROTO_TYPE)
        return false;
    if (h->version != UPG_PROTO_VERSION)
        return false;

    sum = 0;
    sum += h->magic;
    sum += ((h->reserved << 24) | (h->command << 16) | (h->version << 8) |
            h->protocol);
    sum += h->data_length;
    if (sum != h->checksum)
        return false;

    return true;
}

void aicupg_gen_resp(struct resp_header *h, u8 cmd, u8 sts, u32 len)
{
    u32 sum;

    h->magic = UPG_CMD_RESP_MAGIC;
    h->protocol = UPG_PROTO_TYPE;
    h->version = UPG_PROTO_VERSION;
    h->command = cmd;
    h->status = sts;
    h->data_length = len;

    sum = 0;
    sum += h->magic;
    sum += ((h->status << 24) | (h->command << 16) | (h->version << 8) |
            h->protocol);
    sum += h->data_length;
    h->checksum = sum;
}

s32 aicupg_set_upg_cfg(struct upg_cfg *cfg)
{
    if (!cfg) {
        pr_info("Invalide parameter.\n");
        return -1;
    }

    memcpy(&upg_info.cfg, cfg, sizeof(*cfg));
    pr_debug("%s, mode = %d\n", __func__, upg_info.cfg.mode);

    return 0;
}

s32 aicupg_initialize(struct upg_init *param)
{
    upg_info.init.mode = param->mode;
    return 0;
}

s32 aicupg_get_upg_mode(void)
{
    return (s32)upg_info.cfg.mode;
}

void set_current_command(struct upg_cmd *cmd)
{
    upg_info.cur_cmd = cmd;
}

struct upg_cmd *get_current_command(void)
{
    return upg_info.cur_cmd;
}

enum upg_cmd_state get_current_command_state(void)
{
    if (upg_info.cur_cmd)
        return upg_info.cur_cmd->state;
    return CMD_STATE_IDLE;
}

void set_current_device_type(enum upg_dev_type type)
{
    upg_info.dev_type = type;
}

enum upg_dev_type get_current_device_type(void)
{
    return upg_info.dev_type;
}

const char *get_current_device_name(enum upg_dev_type type)
{
    char *dev_list[] = {
        "RAM",
        "MMC",
        "SPI_NAND",
        "SPI_NOR",
        "RAW_NAND",
        "UNKNOWN",
    };

    return dev_list[type];
}

void set_current_device_id(int id)
{
    upg_info.dev_id = id;
}

int get_current_device_id(void)
{
    return upg_info.dev_id;
}

static struct upg_cmd *find_command(struct cmd_header *h)
{
    struct upg_cmd *cmd = NULL;

    cmd = find_basic_command(h);
    if (cmd)
        return cmd;

    /* Not basic command, maybe it is FWC relative command. */
    cmd = find_fwc_command(h);
    return cmd;
}

s32 aicupg_data_packet_write(u8 *data, s32 len)
{
    struct cmd_header h;
    struct upg_cmd *cmd;
    u32 clen;

    clen = 0;
    if (len >= sizeof(struct cmd_header))
        memcpy(&h, data, sizeof(struct cmd_header));

    if ((len >= sizeof(struct cmd_header)) &&
        (check_cmd_header(&h) == true)) {
        /* Command start packet, find the command handler */
        cmd = find_command(&h);
        set_current_command(cmd);
        if (cmd)
            cmd->start(cmd, h.data_length);
        clen = sizeof(struct cmd_header);
    }

    /* Maybe this packet is cmd_header only */
    if (clen == len)
        return clen;
    /* There is command data */
    cmd = get_current_command();
    if (cmd && cmd->write_input_data)
        clen += cmd->write_input_data(cmd, data, len - clen);

    /* End CMD after CSW is sent */
    if (get_current_command_state() == CMD_STATE_END)
        cmd->end(cmd);

    pr_debug("%s, l: %d\n", __func__, __LINE__);
    return clen;
}

s32 aicupg_data_packet_read(u8 *data, s32 len)
{
    struct upg_cmd *cmd;
    s32 rlen = 0;

    /* Host read data from device */
    cmd = get_current_command();
    if (cmd && cmd->read_output_data)
        rlen = cmd->read_output_data(cmd, data, len);

    /* End CMD before CSW is sent */
    if (get_current_command_state() == CMD_STATE_END)
        cmd->end(cmd);

    return rlen;
}

int aicupg_get_fwc_attr(struct fwc_info *fwc)
{
    int attr = 0;

    if (!fwc)
        return 0;

    if (strstr(fwc->meta.attr, "required"))
        attr |= FWC_ATTR_REQUIRED;
    else if (strstr(fwc->meta.attr, "optional"))
        attr |= FWC_ATTR_OPTIONAL;

    if (strstr(fwc->meta.attr, "run"))
        attr |= FWC_ATTR_ACTION_RUN;
    else if (strstr(fwc->meta.attr, "burn"))
        attr |= FWC_ATTR_ACTION_BURN;

    if (strstr(fwc->meta.attr, "block"))
        attr |= FWC_ATTR_DEV_BLOCK;
    else if (strstr(fwc->meta.attr, "mtd"))
        attr |= FWC_ATTR_DEV_MTD;
    else if (strstr(fwc->meta.attr, "ubi"))
        attr |= FWC_ATTR_DEV_UBI;
    else if (strstr(fwc->meta.attr, "uffs"))
        attr |= FWC_ATTR_DEV_UFFS;

    return attr;
}

/*
 * Init fwc and config fwc->meta
 */
void fwc_meta_config(struct fwc_info *fwc, struct fwc_meta *pmeta)
{
    memset((void *)fwc, 0, sizeof(struct fwc_info));
    memcpy(&fwc->meta, pmeta, sizeof(struct fwc_meta));
}

/*
 * Get memory type by header
 * - Determine the memory type of the current image
 */
static enum upg_dev_type media_type_get(struct image_header_upgrade *header)
{
    static enum upg_dev_type type;

    pr_debug("%s, %s\n", __func__, header->media_type);
    if (strcmp(header->media_type, "mmc") == 0)
        type = UPG_DEV_TYPE_MMC;
    else if (strcmp(header->media_type, "spi-nand") == 0)
        type = UPG_DEV_TYPE_SPI_NAND;
    else if (strcmp(header->media_type, "spi-nor") == 0)
        type = UPG_DEV_TYPE_SPI_NOR;
    else
        type = UPG_DEV_TYPE_UNKNOWN;

    return type;
}

/*
 * Prepare write data
 * - Select function based on type
 */
s32 media_device_prepare(struct fwc_info *fwc, struct image_header_upgrade
        *header)
{
    enum upg_dev_type type;
    s32 ret = 0;

    /* get device type */
    type = media_type_get(header);

    /* config upg_info */
    set_current_device_type(type);
    set_current_device_id(header->media_dev_id);
    switch (type) {
#if defined(AICUPG_MMC_ARTINCHIP)
        case UPG_DEV_TYPE_MMC:
            ret = mmc_fwc_prepare(fwc, header->media_dev_id);
            break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NAND:
            ret = nand_fwc_prepare(fwc, header->media_dev_id);
            break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NOR:
            ret = nor_fwc_prepare(fwc, header->media_dev_id);
            break;
#endif
        default:
            pr_err("device type is not support!...\n");
            ret = -1;
            break;
    }

    return ret;
}

/*
 * Start write data
 * - Select function based on type
 */
void media_data_write_start(struct fwc_info *fwc)
{
    enum upg_dev_type type;

    type = get_current_device_type();
    switch (type) {
#if defined(AICUPG_MMC_ARTINCHIP)
        case UPG_DEV_TYPE_MMC:
            mmc_fwc_start(fwc);
            break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NAND:
            nand_fwc_start(fwc);
            break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NOR:
            nor_fwc_start(fwc);
            break;
#endif
        default:
            pr_err("device type is not support!...\n");
            break;
    }
}

/*
 * Write data to memory device
 * - Make the data size into whole block
 * - Select function based on type
 */
s32 media_data_write(struct fwc_info *fwc, u8 *buf, u32 len)
{
    enum upg_dev_type type;
    s32 ret, len_to_write;

    type = get_current_device_type();
    if (len % fwc->block_size)
        len_to_write = len + fwc->block_size - (len % fwc->block_size);
    else
        len_to_write = len;

    switch (type) {
#if defined(AICUPG_MMC_ARTINCHIP)
        case UPG_DEV_TYPE_MMC:
            ret = mmc_fwc_data_write(fwc, buf, len_to_write);
            break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NAND:
            ret = nand_fwc_data_write(fwc, buf, len_to_write);
            break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NOR:
            ret = nor_fwc_data_write(fwc, buf, len_to_write);
            break;
#endif
        default:
            ret = 0;
            pr_err("device type is not support!...\n");
            break;
    }

    /* The size of the data we actually write is len */
    if (ret != len_to_write)
        ret = 0;
    else
        ret = len;

    return ret;
}

/*
 * End write data
 * - Select function based on type
 */
void media_data_write_end(struct fwc_info *fwc)
{
    enum upg_dev_type type;

    type = get_current_device_type();
    switch (type) {
#if defined(AICUPG_MMC_ARTINCHIP)
        case UPG_DEV_TYPE_MMC:
            mmc_fwc_data_end(fwc);
            break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NAND:
            nand_fwc_data_end(fwc);
            break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NOR:
            nor_fwc_data_end(fwc);
            break;
#endif
        default:
            pr_err("device type is not support!...\n");
            break;
    }
}
