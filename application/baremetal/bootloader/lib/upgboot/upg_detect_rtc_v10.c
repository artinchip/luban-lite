#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <upg_detect.h>

#define RTC_CMU_REG               ((void *)0x18020908)
#define RTC_CMU_BUS_EN_MSK        (0x1000)

#define BASE_RTC                  ((void *)0x19030000)
#define RTC_WRITE_KEY_REG         (BASE_RTC + 0x0FC)
#define RTC_WRITE_KEY_VALUE       (0xAC)

#define RTC_BOOTINFO1_REG         (BASE_RTC + 0x100)
#define BOOTINFO1_REASON_OFF      (4)
#define BOOTINFO1_REASON_MSK      (0xF << 4)
#define RTC_REBOOT_REASON_UPGRADE (4)

s32 upg_reg_flag_check(void)
{
	u32 val;

    hal_clk_enable_deassertrst_iter(CLK_RTC);

	val = readl((void *)RTC_BOOTINFO1_REG);
	val = (val & BOOTINFO1_REASON_MSK) >> BOOTINFO1_REASON_OFF;

    if (val == RTC_REBOOT_REASON_UPGRADE)
        return UPG_DETECT_REASON_SOFT;

    return 0;
}

void upg_reg_flag_clear(void)
{
	u32 val;

	writel(RTC_WRITE_KEY_VALUE, RTC_WRITE_KEY_REG);
	val = readl(RTC_BOOTINFO1_REG);
	val &= ~BOOTINFO1_REASON_MSK;
	writel(val, RTC_BOOTINFO1_REG);
	writel(0, RTC_WRITE_KEY_REG);
}
