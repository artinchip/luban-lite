#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <upg_detect.h>

__WEAK s32 upg_reg_flag_check(void)
{
    return 0;
}

__WEAK void upg_reg_flag_clear(void)
{
}

s32 upg_mode_detect(void)
{
    s32 upgmode = 0;

    upgmode = upg_reg_flag_check();
    if (upgmode == 0)
        upgmode = upg_boot_pin_check();
    if (upgmode)
        upg_reg_flag_clear();
    return upgmode;
}
