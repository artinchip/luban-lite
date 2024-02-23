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
#include <driver.h>
#include <heap.h>
#include <board.h>
#include <uart.h>
#include <console.h>
#include <aic_osal.h>
#include <boot_param.h>
#include <hal_efuse.h>
#include <hal_syscfg.h>
#include <aic_hal.h>
#include <aic_time.h>
#include <boot_time.h>
#include <aic_clk_id.h>
#include <upg_uart.h>
#include <hal_syscfg.h>
#include <xspi_psram.h>
#include <usb_drv.h>
#include <usbhost.h>
#include <usbdevice.h>
#include <boot_rom.h>
#include <hal_axicfg.h>

extern size_t __heap_start;
extern size_t __heap_end;

extern void aic_board_pinmux_init(void);
extern void aic_board_sysclk_init(void);
extern void stdio_set_uart(int id);
extern int drv_dma_init(void);

#define MAX_HEAP_SIZE_IN_BOOT 0x800000

#ifdef AIC_BOOTLOADER_AXICFG_SUPPORT
struct hal_axicfg_table axi_cfg_table[HAL_AXICFG_PORT_MAX] = {
    { .enable = AXICFG_CPU_EN, .priority = (u8)AIC_AXICFG_PORT_CPU_PRIO },
    { .enable = AXICFG_AHB_EN, .priority = (u8)AIC_AXICFG_PORT_AHB_PRIO },
    { .enable = AXICFG_DE_EN, .priority = (u8)AIC_AXICFG_PORT_DE_PRIO },
    { .enable = AXICFG_GE_EN, .priority = (u8)AIC_AXICFG_PORT_GE_PRIO },
    { .enable = AXICFG_VE_EN, .priority = (u8)AIC_AXICFG_PORT_VE_PRIO },
    { .enable = AXICFG_DVP_EN, .priority = (u8)AIC_AXICFG_PORT_DVP_PRIO },
    { .enable = AXICFG_CE_EN, .priority = (u8)AIC_AXICFG_PORT_CE_PRIO },
};
#endif

static int board_init(enum boot_device bd)
{
    size_t heap_size, heap_start;
    int cons_uart;

    /* target/<chip>/<board>/board.c */
    aic_board_sysclk_init();
    aic_board_pinmux_init();
    boot_time_trace("Clock and pinmux done");


#ifdef AIC_BOOTLOADER_AXICFG_SUPPORT
    for (int i = 0; i < HAL_AXICFG_PORT_MAX; i++) {
        if (axi_cfg_table[i].enable) {
            hal_axicfg_module_init(i, axi_cfg_table[i].priority);
        }
    }
#endif

    heap_size = ((size_t)&__heap_end) - ((size_t)&__heap_start);

    if (bd != BD_UDISK && bd != BD_SDFAT32 && heap_size > 0x200000)
            heap_size = 0x200000;

    heap_start = (size_t)&__heap_end - heap_size;

    heap_init((void *)heap_start, heap_size);
    boot_time_trace("Heap init done");

    cons_uart = AIC_BOOTLOADER_CONSOLE_UART;
    uart_init(cons_uart);
    stdio_set_uart(cons_uart);
    boot_time_trace("Console UART ready");

    return 0;
}

void show_banner(void)
{
    printf("\ntinySPL [Built on %s %s]\n", __DATE__, __TIME__);
}

int main(void)
{
    enum boot_device bd;
    int ctrlc = -1;
    s32 id = -1;
    s32 __attribute__((unused)) ret = -1;

    boot_time_trace("Enter main");

#ifdef AIC_SID_DRV
    hal_efuse_init();
#endif
#ifdef AIC_SYSCFG_DRV
    hal_syscfg_probe();
#endif

    bd = aic_get_boot_device();
    board_init(bd);
#ifdef AIC_DMA_DRV
    drv_dma_init();
#endif

    show_banner();
    boot_time_trace("Banner shown");

    console_init();
    console_set_usrname("aic");

    ctrlc = console_get_ctrlc();
    if (ctrlc < 0) {
#if defined(AICUPG_UDISK_ENABLE)
        id = usbh_get_connect_id();
        boot_time_trace("UDISK checked");
        if (id < 0) {
            pr_err("Not find udisk.\n");
        } else {
            if (id == 0)
                ret = console_run_cmd("aicupg fat udisk 0");
            else if (id == 1)
                ret = console_run_cmd("aicupg fat udisk 1");
            if (!ret)
                console_loop();
        }

#endif
        /*
         * Set bootcmd string to console, bootloader will check and run bootcmd
         * first in console_loop
         *
         * For the detail of boot command, please reference to cmd/
         */
        switch (bd) {
            case BD_USB:
#if defined(AICUPG_USB_ENABLE)
                usbd_connection_check_start();
                if (usbd_connection_check_status()) {
                    usbd_connection_check_end();
                    console_set_bootcmd("aicupg usb 0");
                    break;
                }
#endif
                console_set_bootcmd("aicupg uart 0");
                break;
            case BD_UDISK:
                if (id == 0)
                    console_set_bootcmd("aicupg fat udisk 0");
                else if (id == 1)
                    console_set_bootcmd("aicupg fat udisk 1");
                break;
            case BD_SDFAT32:
                console_set_bootcmd("aicupg fat mmc 1");
                break;
            case BD_SDMC0:
            case BD_SDMC1:
                console_set_bootcmd("mmc_boot");
                break;
            case BD_SPINOR:
#ifdef AIC_BOOTLOADER_CMD_XIP_BOOT
                console_set_bootcmd("xip_boot");
#elif defined(AIC_BOOTLOADER_CMD_NOR_BOOT)
                console_set_bootcmd("nor_boot");
#endif
                break;
            case BD_SPINAND:
                console_set_bootcmd("nand_boot");
                break;
            default:
                break;
        }
    }
    /*
     * In the loop:
     *  1. Bootloader first will try to run the "bootcmd", to boot application
     *  2. If bootloader failure to boot application, it will stay in console
     *     loop, user/developer can run built-in commands to do some jobs.
     */
    console_loop();

    while (1) {
        /* Should not run to here. */
    };
    return 0;
}
