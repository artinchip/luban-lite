/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "aic_reboot_reason.h"
#include "hal_wri.h"

/* Register of WRI */

#define WRI_RST_FLAG            (WRI_BASE + 0x0)
#define WRI_BOOT_INFO           (WRI_BASE + 0x100)
#define WRI_SYS_BAK             (WRI_BASE + 0x104)
#define WRI_VERSION             (WRI_BASE + 0xFFC)

#if defined(AIC_WRI_DRV_V12) || defined(AIC_WRI_DRV_V11) || defined(AIC_WRI_DRV_V10)
#define WRI_FLAG_CMP_RST        BIT(12)
#define WRI_FLAG_TSEN_RST       BIT(11)
#define WRI_FLAG_WDT_RST        BIT(10)
#define WRI_FLAG_DM_RST         BIT(9)
#define WRI_FLAG_EXT_RST        BIT(8)
#define WRI_FLAG_RTC_RST        BIT(1)
#define WRI_FLAG_SYS_POR        BIT(0)
#endif

#if defined(AIC_WRI_DRV_V13)
#define WRI_FLAG_SE_DM_NDM_RST      BIT(30)
#define WRI_FLAG_SE_WDOG_CPU_RST    BIT(29)
#define WRI_FLAG_SE_WDOG_SYS_RST    BIT(28)
#define WRI_FLAG_SC_DM_CPU_RST      BIT(27)
#define WRI_FLAG_SC_DM_NDM_RST      BIT(26)
#define WRI_FLAG_SC_WDOG_CPU_RST    BIT(25)
#define WRI_FLAG_SC_WDOG_SYS_RST    BIT(24)
#define WRI_FLAG_CS_DM_CPU_RST      BIT(23)
#define WRI_FLAG_CS_DM_NDM_RST      BIT(22)
#define WRI_FLAG_CS_WDOG_CPU_RST    BIT(21)
#define WRI_FLAG_CS_WDOG_SYS_RST    BIT(20)
#define WRI_FLAG_SP_DM_CPU_RST      BIT(19)
#define WRI_FLAG_SP_DM_NDM_RST      BIT(18)
#define WRI_FLAG_SP_WDOG_CPU_RST    BIT(17)
#define WRI_FLAG_SP_WDOG_SYS_RST    BIT(16)
#define WRI_FLAG_THS_RST            BIT(9)
#define WRI_FLAG_PIN_RST            BIT(8)
#define WRI_FLAG_RTC_POR            BIT(2)
#define WRI_FLAG_VDD11_SW_POR       BIT(1)
#define WRI_FLAG_VDD11_SP_POR       BIT(0)
#endif

#define WRI_REBOOT_REASON_MASK  GENMASK(7, 4)
#define WRI_REBOOT_REASON_SHIFT 4

struct reboot_info {
    u32 inited;
    enum aic_reboot_reason reason;
    enum aic_reboot_reason sw_reason;
    enum aic_warm_reset_type hw_reason;
};

static struct reboot_info g_last_reboot = {0};

#if defined(AIC_WRI_DRV_V12) || defined(AIC_WRI_DRV_V11) || defined(AIC_WRI_DRV_V10)
enum aic_warm_reset_type aic_wr_type_get(void)
{
    u32 val = 0;
    u16 wr_bit[WRI_TYPE_MAX] = {
        WRI_FLAG_SYS_POR, WRI_FLAG_RTC_RST, WRI_FLAG_EXT_RST, WRI_FLAG_DM_RST,
        WRI_FLAG_WDT_RST, WRI_FLAG_TSEN_RST, WRI_FLAG_CMP_RST};
    s32 i;

    if (g_last_reboot.inited)
        return g_last_reboot.hw_reason;

    val = readl(WRI_RST_FLAG);
    if (!val)
        return WRI_TYPE_POR;

    writel(val, WRI_RST_FLAG); /* clear the flag */
    for (i = WRI_TYPE_MAX - 1; i >= 0; i--) {
        if (val & wr_bit[i]) {
            g_last_reboot.hw_reason = (enum aic_warm_reset_type)i;
            return g_last_reboot.hw_reason;
        }
    }

    pr_warn("Invalid warm reset flag: %#x\n", val);
    return WRI_TYPE_POR;
}

enum aic_reboot_reason aic_judge_reboot_reason(enum aic_warm_reset_type hw,
                                               u32 sw)
{
    enum aic_reboot_reason r = (enum aic_reboot_reason)sw;

    /* First, check the software-triggered reboot */
    if (hw == WRI_TYPE_WDT) {
        printf("Reboot action: Watchdog-Reset, reason: ");

        switch (sw) {
        case REBOOT_REASON_UPGRADE:
            printf("Upgrade-Mode\n");
            break;
        case REBOOT_REASON_CMD_REBOOT:
            printf("Command-Reboot\n");
            break;
        case REBOOT_REASON_SW_LOCKUP:
            printf("Software-Lockup\n");
            break;
        case REBOOT_REASON_HW_LOCKUP:
            printf("Hardware-Lockup\n");
            break;
        case REBOOT_REASON_PANIC:
            printf("Kernel-Panic\n");
            break;
        case REBOOT_REASON_RAMDUMP:
            printf("Ramdump\n");
            break;
        default:
            printf("Unknown(%d)\n", r);
            break;
        }
        return r;
    }

    if (r == REBOOT_REASON_CMD_SHUTDOWN) {
            printf("Reboot reason: Command-Poweroff\n");
            return r;
    }

    if (r == REBOOT_REASON_SUSPEND) {
        printf("Reboot reason: Suspend\n");
        return r;
    }

    /* Second, check the hardware-triggered reboot */
    if (r == REBOOT_REASON_COLD) {
        if (hw == WRI_TYPE_POR) {
            printf("Startup reason: Power-On-Reset\n");
            return (enum aic_reboot_reason)sw;
        }

        printf("Reboot action: Warm-Reset, reason: ");
        switch (hw) {
        case WRI_TYPE_RTC:
            printf("RTC-Power-Down\n");
            r = REBOOT_REASON_RTC;
            break;
        case WRI_TYPE_EXT:
            printf("External-Reset\n");
            r = REBOOT_REASON_EXTEND;
            break;
        case WRI_TYPE_DM:
            printf("JTAG-Reset\n");
            r = REBOOT_REASON_JTAG;
            break;
        case WRI_TYPE_TSEN:
            printf("OTP-Reset\n");
            r = REBOOT_REASON_OTP;
            break;
        case WRI_TYPE_CMP:
            printf("Undervoltage-Reset\n");
            r = REBOOT_REASON_UNDER_VOL;
            break;
        default:
            printf("Unknown(%d)\n", hw);
            break;
        }
        return r;
    }

    pr_warn("Unknow reboot reason: %d - %d\n", hw, sw);
    return r;
}
#endif

#if defined(AIC_WRI_DRV_V13)
enum aic_warm_reset_type aic_wr_type_get(void)
{
    u32 val = 0;
    u32 wr_bit[WRI_TYPE_MAX] = {WRI_FLAG_VDD11_SP_POR,
                                WRI_FLAG_VDD11_SW_POR,
                                WRI_FLAG_RTC_POR,
                                WRI_FLAG_PIN_RST,
                                WRI_FLAG_THS_RST,
                                WRI_FLAG_SP_WDOG_SYS_RST,
                                WRI_FLAG_SP_WDOG_CPU_RST,
                                WRI_FLAG_SP_DM_NDM_RST,
                                WRI_FLAG_SP_DM_CPU_RST,
                                WRI_FLAG_CS_WDOG_SYS_RST,
                                WRI_FLAG_CS_WDOG_CPU_RST,
                                WRI_FLAG_CS_DM_NDM_RST,
                                WRI_FLAG_CS_DM_CPU_RST,
                                WRI_FLAG_SC_WDOG_SYS_RST,
                                WRI_FLAG_SC_WDOG_CPU_RST,
                                WRI_FLAG_SC_DM_NDM_RST,
                                WRI_FLAG_SC_DM_CPU_RST,
                                WRI_FLAG_SE_WDOG_SYS_RST,
                                WRI_FLAG_SE_WDOG_CPU_RST,
                                WRI_FLAG_SE_DM_NDM_RST
                                };
    s32 i;

    if (g_last_reboot.inited)
        return g_last_reboot.hw_reason;

    val = readl(WRI_RST_FLAG);
    if (!val)
        return WRI_TYPE_VDD11_SP_POR;

    writel(val, WRI_RST_FLAG); /* clear the flag */
    for (i = WRI_TYPE_MAX - 1; i >= 0; i--) {
        if (val & wr_bit[i]) {
            g_last_reboot.hw_reason = (enum aic_warm_reset_type)i;
            return g_last_reboot.hw_reason;
        }
    }

    pr_warn("Invalid warm reset flag: %#x\n", val);
    return WRI_TYPE_VDD11_SP_POR;
}

enum aic_reboot_reason aic_judge_reboot_reason(enum aic_warm_reset_type hw,
                                               u32 sw)
{
    enum aic_reboot_reason r = (enum aic_reboot_reason)sw;

    /* First, check the software-triggered reboot */
    if (hw == WRI_TYPE_SP_WDOG_SYS_RST || hw == WRI_TYPE_SP_WDOG_CPU_RST || hw == WRI_TYPE_CS_WDOG_SYS_RST || hw == WRI_TYPE_CS_WDOG_CPU_RST || hw == WRI_TYPE_SC_WDOG_SYS_RST || hw == WRI_TYPE_SC_WDOG_CPU_RST || hw == WRI_TYPE_SE_WDOG_SYS_RST || hw == WRI_TYPE_SE_WDOG_CPU_RST) {
            switch (hw) {
            case WRI_TYPE_SP_WDOG_SYS_RST:
                printf("Reboot action: SPSS Watchdog-system-Reset\n");
                break;
            case WRI_TYPE_SP_WDOG_CPU_RST:
                printf("Reboot action: SPSS Watchdog-CPU-Reset\n");
                break;
            case WRI_TYPE_CS_WDOG_SYS_RST:
                printf("Reboot action: CSYS Watchdog-system-Reset\n");
                break;
            case WRI_TYPE_CS_WDOG_CPU_RST:
                printf("Reboot action: CSYS Watchdog-CPU-Reset\n");
                break;
            case WRI_TYPE_SC_WDOG_SYS_RST:
                printf("Reboot action: SCSS Watchdog-system-Reset\n");
                break;
            case WRI_TYPE_SC_WDOG_CPU_RST:
                printf("Reboot action: SCSS Watchdog-CPU-Reset\n");
                break;
            case WRI_TYPE_SE_WDOG_SYS_RST:
                printf("Reboot action: SESS Watchdog-system-Reset\n");
                break;
            case WRI_TYPE_SE_WDOG_CPU_RST:
                printf("Reboot action: SESS Watchdog-CPU-Reset\n");
                break;
            default:
                printf("Reboot action: Unknown(%d)\n", hw);
                break;
                }

            switch (sw) {
            case REBOOT_REASON_UPGRADE:
                printf("Reboot reason: Upgrade-Mode\n");
                break;
            case REBOOT_REASON_SW_LOCKUP:
                printf("Reboot reason: Software-Lockup\n");
                break;
            case REBOOT_REASON_HW_LOCKUP:
                printf("Reboot reason: Hardware-Lockup\n");
                break;
            case REBOOT_REASON_PANIC:
                printf("Reboot reason: Kernel-Panic\n");
                break;
            case REBOOT_REASON_CS_CMD_REBOOT:
                printf("Reboot reason: CSYS Command-Reboot\n");
                break;
            case REBOOT_REASON_SC_CMD_REBOOT:
                printf("Reboot reason: SCSS Command-Reboot\n");
                break;
            case REBOOT_REASON_SP_CMD_REBOOT:
                printf("Reboot reason: SPSS Command-Reboot\n");
                break;
            case REBOOT_REASON_SE_CMD_REBOOT:
                printf("Reboot reason: SESS Command-Reboot\n");
                break;
            case REBOOT_REASON_RAMDUMP:
                printf("Ramdump\n");
                break;
            default:
                printf("Unknown(%d)\n", r);
                break;
            }
            return r;
        }

    if (r == REBOOT_REASON_CMD_SHUTDOWN) {
        printf("Reboot reason: Command-Poweroff\n");
        return r;
    }

    if (r == REBOOT_REASON_SUSPEND) {
        printf("Reboot reason: Suspend\n");
        return r;
    }

    /* Second, check the hardware-triggered reboot */
    if (r == REBOOT_REASON_COLD) {
        if (hw == WRI_TYPE_VDD11_SP_POR) {
            printf("Startup reason: SPSS Power-On-Reset\n");
            return (enum aic_reboot_reason)sw;
        }

        if (hw == WRI_TYPE_VDD11_SW_POR) {
            printf("Startup reason: SW Power-On-Reset\n");
            return (enum aic_reboot_reason)sw;
        }

        printf("Reboot action: Warm-Reset, reason: ");
        switch (hw) {
        case WRI_TYPE_RTC_POR:
            printf("Reboot reason: RTC-Power-Down\n");
            r = REBOOT_REASON_RTC_POR;
            break;
        case WRI_TYPE_PIN_RST:
            printf("Reboot reason: Extend-Reset\n");
            r = REBOOT_REASON_PIN_RST;
            break;
        case WRI_TYPE_THS_RST:
            printf("Reboot reason: OTP-Reset\n");
            r = REBOOT_REASON_THS_RST;
            break;
        case WRI_TYPE_SP_DM_NDM_RST:
            printf("Reboot reason: SPSS NDM Reset\n");
            r = REBOOT_REASON_SP_DM_NDM_RST;
            break;
        case WRI_TYPE_SP_DM_CPU_RST:
            printf("Reboot reason: SPSS CPU Reset\n");
            r = REBOOT_REASON_SP_DM_CPU_RST;
            break;
        case WRI_TYPE_CS_DM_NDM_RST:
            printf("Reboot reason: CSYS NDM Reset\n");
            r = REBOOT_REASON_CS_DM_NDM_RST;
            break;
        case WRI_TYPE_CS_DM_CPU_RST:
            printf("Reboot reason: CSYS CPU Reset\n");
            r = REBOOT_REASON_CS_DM_CPU_RST;
            break;
        case WRI_TYPE_SC_DM_NDM_RST:
            printf("Reboot reason: SCSS NDM Reset\n");
            r = REBOOT_REASON_SC_DM_NDM_RST;
            break;
        case WRI_TYPE_SC_DM_CPU_RST:
            printf("Reboot reason: SCSS CPU Reset\n");
            r = REBOOT_REASON_SC_DM_CPU_RST;
            break;
        case WRI_TYPE_SE_DM_NDM_RST:
            printf("Reboot reason: SESS NDM Reset\n");
            r = REBOOT_REASON_SE_DM_NDM_RST;
            break;
        default:
            printf("Unknown(%d)\n", hw);
            break;
        }
        return r;
    }

    pr_warn("Unknown reboot reason: %d - %d\n", hw, sw);
    return r;
}
#endif

#if defined(AIC_WRI_DRV_V12)

void aic_set_reboot_reason(enum aic_reboot_reason r)
{
    u32 cur = 0;
    u8 reason_num = WRI_REBOOT_REASON_MASK >> WRI_REBOOT_REASON_SHIFT;

    cur = readl(WRI_BOOT_INFO);
     /* If it's valid already, so ignore the current request */
    if (cur & WRI_REBOOT_REASON_MASK)
        return;

    writel_bits(r, WRI_REBOOT_REASON_MASK, WRI_REBOOT_REASON_SHIFT,
                WRI_BOOT_INFO);

    if (r <= reason_num)
        pr_debug("Set reboot reason %d\n", r);
}

enum aic_reboot_reason aic_get_reboot_reason(void)
{
    u32 val = 0;
    enum aic_warm_reset_type hw = aic_wr_type_get();

    if (g_last_reboot.inited)
        return g_last_reboot.reason;

    val = readl_bits(WRI_REBOOT_REASON_MASK, WRI_REBOOT_REASON_SHIFT,
                     WRI_BOOT_INFO);
    if (val)
        aic_set_reboot_reason(REBOOT_REASON_COLD);

    pr_debug("Last reboot info: hw %d, sw %d\n", hw, val);
    g_last_reboot.reason = aic_judge_reboot_reason(hw, val);
    g_last_reboot.inited = 1;
    return g_last_reboot.reason;
}

/* Convert and print the time info from a GTC counter.
   It's useful when print the time before console is ready. */
void aic_show_gtc_time(char *tag, u32 val)
{
    u32 sec, msec;
    u32 cnt = val;

    if (!cnt)
        cnt = readl(GTC_BASE + 0x8);

    sec = cnt / 4000000;
    msec = (cnt % 4000000) / 4 / 1000;
    printf("[%s] time: %d.%03d sec\n", tag ? tag : "GTC", sec, msec);
}

void aic_show_startup_time(void)
{
    u32 cnt = readl(GTC_BASE + 0x8);

    aic_show_gtc_time("Startup", cnt);
}

#endif
