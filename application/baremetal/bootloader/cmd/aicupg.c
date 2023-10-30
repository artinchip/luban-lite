/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <console.h>
#include <aic_core.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <boot_param.h>
#include <usb_drv.h>
#include <usbhost.h>
#include <config_parse.h>
#include <aicupg.h>
#include <fatfs.h>
#include <mmc.h>
#include <hal_syscfg.h>
#include <upg_uart.h>

#define AICUPG_HELP                                \
    "ArtInChip upgrading command:\n"               \
    "aicupg [devtype] [interface]\n"               \
    "  - devtype: should be usb, uart, mmc, fat\n" \
    "  - interface: specify the controller id\n"   \
    "e.g.\n"                                       \
    "  aicupg usb 0\n"                             \
    "  aicupg mmc 1\n"                             \
    "when devtype is fat: \n"                      \
    "aicupg [devtype] [blkdev] [interface]\n"      \
    "- blkdev: should be udisk,mmc \n"             \
    "e.g.: \n"                                     \
    "  aicupg fat udisk 0\n"                       \
    "  aicupg fat mmc 1\n"
static void aicupg_help(void)
{
    puts(AICUPG_HELP);
}

#define AICUPG_ARGS_MAX 4
extern struct usb_device usbupg_device;

static int ctrlc(void)
{
    switch (getchar()) {
        case 0x03:              /* ^C - Control C */
            return 1;
        default:
            break;
    }

    return 0;
}

#if defined(AIC_BOOTLOADER_FATFS_SUPPORT)
static int image_header_check(struct image_header_pack *header)
{
    /*check header*/
    if ((strcmp(header->hdr.magic, "AIC.FW") != 0)) {
        pr_err("Error:image check failed, maybe not have a image in media!\n");
        return -1;
    }

    return 0;
}
#endif

static int do_uart_protocol_upg(int intf)
{
    int ret = 0;

#if defined(AICUPG_UART_ENABLE)
    aic_upg_uart_init(intf);
    while (1) {
        if (ctrlc())
            break;
        aic_upg_uart_loop();
    }
#endif

    return ret;
}

static int do_usb_protocol_upg(int intf)
{
    int ret = 0;

#if defined(AICUPG_USB_ENABLE)
#ifndef AIC_SYSCFG_DRV_V12
    syscfg_usb_phy0_sw_host(0);
#endif
    aic_udc_init(&usbupg_device);
    while (1) {
        if (ctrlc())
            break;
        aic_udc_state_loop();
    }
#endif

    return ret;
}

static int do_sdcard_upg(int intf)
{
    int ret = 0;

    return ret;
}

static int do_fat_upg(int intf, char *const blktype)
{

    int ret = 0;
#if defined(AIC_BOOTLOADER_FATFS_SUPPORT)
    struct image_header_pack *hdrpack;
    char image_name[IMG_NAME_MAX_SIZ] = {0};
    char protection[PROTECTION_PARTITION_LEN] = {0};
    ulong actread;
    char *file_buf;

    hdrpack = (struct image_header_pack *)aicos_malloc_align(0, sizeof(struct image_header_pack), CACHE_LINE_SIZE);
    if (!hdrpack) {
        pr_err("Error, malloc hdrpack failed.\n");
        return -1;
    }
    memset((struct image_header_pack *)hdrpack, 0, sizeof(struct image_header_pack));

    file_buf = (char *)aicos_malloc_align(0, 1024, CACHE_LINE_SIZE);
    if (!file_buf) {
        pr_err("Error, malloc buf failed.\n");
        goto err;
    }
    memset((void *)file_buf, 0, 1024);

    if (!strcmp(blktype, "udisk")) {
        /*usb init*/
#if defined(AICUPG_UDISK_ENABLE)
        if (usbh_initialize(intf) < 0) {
            pr_err("usbh init failed!\n");
            goto err;
        }

        ret = aic_fat_set_blk_dev(intf, BLK_DEV_TYPE_MSC);
        if (ret) {
            pr_err("set blk dev failed.\n");
            goto err;
        }
#endif
    } else if (!strcmp(blktype, "mmc")) {
#if defined(AICUPG_SDCARD_ENABLE)
        ret = mmc_init(intf);
        if (ret) {
            printf("sdmc %d init failed.\n", intf);
            goto err;
        }

        ret = aic_fat_set_blk_dev(intf, BLK_DEV_TYPE_MMC);
        if (ret) {
            pr_err("set blk dev failed.\n");
            goto err;
        }
#endif
    } else {
        goto err;
    }

    ret = aic_fat_read_file("bootcfg.txt", (void *)file_buf, 0, 1024, &actread);
    if (actread == 0 || ret) {
        printf("Error:read file bootcfg.txt failed!\n");
        goto err;
    }

    ret = boot_cfg_get_image(file_buf, actread, image_name, IMG_NAME_MAX_SIZ);
    if (ret) {
        pr_err("get bootcfg.txt image name failed!\n");
        goto err;
    }

    ret = boot_cfg_get_protection(file_buf, actread, protection, PROTECTION_PARTITION_LEN);
    if (ret)
        pr_warn("No protected partition.\n");
    else
        pr_info("Protected=%s\n", protection);

    ret = aic_fat_read_file(image_name, (void *)hdrpack, 0, sizeof(struct image_header_pack), &actread);
    if (actread != sizeof(struct image_header_pack) || ret) {
        printf("Error:read file %s failed!\n", image_name);
        goto err;
    }

    /*check header*/
    ret = image_header_check(hdrpack);
    if (ret) {
        pr_err("check image header failed!\n");
        goto err;
    }

    /*write data to media*/
    ret = aicupg_fat_write(image_name, protection, &hdrpack->hdr);
    if (ret == 0) {
        pr_err("fat write data failed!\n");
        goto err;
    }

    aicos_free_align(0, hdrpack);
    aicos_free_align(0, file_buf);
    return 0;

err:
    if (hdrpack)
        aicos_free_align(0, hdrpack);
    if (file_buf)
        aicos_free_align(0, file_buf);
#endif
    return ret;
}

static int do_aicupg(int argc, char *argv[])
{
    char *devtype = NULL;
    int intf, ret = 0;

    aic_get_reboot_reason();

    if ((argc < 3) || (argc > AICUPG_ARGS_MAX))
        goto help;
    devtype = argv[1]; /* mmc  usb fat */
    if (argc >= 4 && argv[3])
        intf = strtol(argv[3], NULL, 0);
    else
        intf = strtol(argv[2], NULL, 0);

    if (devtype == NULL)
        goto help;
    if (!strcmp(devtype, "uart"))
        ret = do_uart_protocol_upg(intf);
    if (!strcmp(devtype, "usb"))
        ret = do_usb_protocol_upg(intf);
    if (!strcmp(devtype, "mmc"))
        ret = do_sdcard_upg(intf);
    if (!strcmp(devtype, "fat"))
        ret = do_fat_upg(intf, argv[2]);

    return ret;

help:
    aicupg_help();
    return ret;
}

CONSOLE_CMD(aicupg, do_aicupg, "Upgrading mode command.");
