/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: weilin.peng@artinchip.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <board.h>
#include <hal_syscfg.h>
#include <aic_core.h>
#include <aic_drv_bare.h>
#include <artinchip_fb.h>
#include "aic_hal_ve.h"
#include "aic_reboot_reason.h"
#include <aic_log.h>
#ifdef AIC_DMA_DRV
#include "drv_dma.h"
#endif

#ifdef LPKG_USING_DFS
#include <dfs.h>
#include <dfs_fs.h>
#ifdef LPKG_USING_DFS_ELMFAT
#include <dfs_elm.h>
#endif
#ifdef LPKG_USING_DFS_ROMFS
#include <dfs_romfs.h>
#endif
#endif

#ifdef AIC_SDMC_DRV
#include "mmc.h"
#endif

#ifdef AIC_LVGL_DEMO
#include "lvgl.h"

extern void lv_port_disp_init(void);
extern void lv_port_indev_init(void);
extern void lv_user_gui_init(void);
#endif

void show_banner(void)
{
    printf("%s\n", BANNER);
    printf("Welcome to ArtInChip Luban-Lite %d.%d [Baremetal - Built on %s %s]\n",
               LL_VERSION, LL_SUBVERSION, __DATE__, __TIME__);
}

static int board_init(void)
{
    int cons_uart;

    aic_hw_board_init();

    hal_syscfg_probe();

    aicos_local_irq_enable();

    cons_uart = AIC_BAREMETAL_CONSOLE_UART;
    uart_init(cons_uart);
    stdio_set_uart(cons_uart);

    show_banner();

    return 0;
}

#if LV_USE_LOG
static void lv_rt_log(const char *buf)
{
    printf("%s\n", buf);
}
#endif /* LV_USE_LOG */

extern void lwip_test_example_main_loop(void * data);
int main(void)
{
    board_init();

#ifdef AIC_DMA_DRV
    drv_dma_init();
#endif

#ifdef TLSF_MEM_HEAP
    aic_tlsf_heap_test();
#endif

#ifdef LPKG_USING_DFS
    dfs_init();
#ifdef LPKG_USING_DFS_ROMFS
    dfs_romfs_init();
#endif
#ifdef LPKG_USING_DFS_ELMFAT
    elm_init();
#endif
#endif

#ifdef LPKG_USING_DFS_ROMFS
    if (dfs_mount(NULL, "/", "rom", 0, &romfs_root) < 0)
        pr_err("Failed to mount romfs\n");
#endif

#ifdef AIC_SDMC_DRV
    mmc_init(1);
    sdcard_hotplug_init();
#endif

#if defined(LPKG_USING_DFS_ELMFAT) && defined(AIC_SDMC_DRV)
    if (dfs_mount("sdmc", "/sdcard", "elm", 0, (const void *)SDMC_DISK) < 0)
        pr_err("Failed to mount sdmc with FatFS\n");
#endif

#ifdef AIC_SPINAND_DRV
    mtd_probe();
#endif

#if defined(LPKG_USING_DFS_ELMFAT) && defined(AIC_SPINAND_DRV)
    if (dfs_mount("rodata", "/rodata", "elm", 0,(const void *)SPINAND_DISK) < 0)
        pr_err("Failed to mount spinand with FatFS\n");
#endif

#ifdef AIC_DISPLAY_DRV
    aicfb_probe();
#endif
#ifdef AIC_GE_DRV
    hal_ge_init();
#endif
#ifdef AIC_VE_DRV
    hal_ve_probe();
#endif
#ifdef AIC_WRI_DRV
    aic_get_reboot_reason();
    aic_show_startup_time();
#endif
#ifdef LPKG_LWIP_EXAMPLES
    /* LwIP test loop */
    lwip_test_example_main_loop(NULL);
#endif

#ifdef AIC_LVGL_DEMO
#if LV_USE_LOG
    lv_log_register_print_cb(lv_rt_log);
#endif /* LV_USE_LOG */
    lv_init();
    lv_port_disp_init();
    lv_user_gui_init();

    while(1)
    {
        lv_task_handler();
    }
#endif /* AIC_LVGL_DEMO */
    #ifdef AIC_VE_TEST
    extern int  do_pic_dec_test(int argc, char **argv);
    /* Main loop */
    aicos_mdelay(2000);
    while (1) {
        do_pic_dec_test(0,NULL);
        //break;
    }
    #else
    while (1) {
        break;
    }
    #endif

#ifdef AIC_CONSOLE_BARE_DRV
    /* Console shell loop */
    console_init();
    console_loop();
#endif

    return 0;
}
