#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <upg_detect.h>

s32 upg_boot_pin_check(void)
{
    unsigned int pin, g, p, value, cnt, sts;

    hal_clk_enable_deassertrst_iter(CLK_GPIO);
    pin = hal_gpio_name2pin(AIC_BOOTLOADER_UPGBOOT_PIN);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, 1);
    hal_gpio_set_bias_pull(g, p, PIN_PULL_UP);
    hal_gpio_direction_input(g, p);

    sts = 0;
    for (cnt = 4; cnt > 0; cnt--) {
        hal_gpio_get_value(g, p, &value);
        sts += value;
    }

    if (sts == 0)
        return UPG_DETECT_REASON_PIN;
    return 0;
}
