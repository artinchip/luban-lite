/*
 * Copyright (C) 2023 ArtInChip Technology Co.,Ltd
 * Author: Dehuang Wu <dehuang.wu@artinchip.com>
 */
#ifndef __BOOT_PARAM_H__
#define __BOOT_PARAM_H__

#define set_boot_device(var, device) \
    {                                \
        (var) &= ~(0xF);             \
        (var) |= (device) & (0xF);   \
    }

#define set_boot_reason(var, reason)    \
    {                                   \
        (var) &= ~(0xF << 4);           \
        (var) |= (reason) & (0xF << 8); \
    }

#define get_boot_device(var)     ((var)&0xF)
#define get_boot_controller(var) (((var) >> 4) & 0xF)
#define get_boot_reason(var)     (((var) >> 8) & 0xF)
#define get_boot_image_id(var)   (((var) >> 12) & 0xF)

union boot_params {
    /*
	 * Save registers. Later's code use these register value to:
	 * 1. Get parameters from previous boot stage
	 * 2. Return to Boot ROM in some condition
	 *
	 *      a0 is boot param
	 *      a1 is private data address
	 */
    unsigned long regs[22];
    struct {
        unsigned long a[8];  /* a0 ~ a7 */
        unsigned long s[12]; /* s0 ~ s11 */
        unsigned long sp;
        unsigned long ra;
    } r;
};

enum boot_reason {
    BR_COLD_BOOT = 0,
    BR_WARM_BOOT,
};

enum boot_device {
    BD_NONE,
    BD_SDMC0,
    BD_SDMC1,
    BD_SDMC2,
    BD_SPINOR,
    BD_SPINAND,
    BD_SDFAT32,
    BD_USB,
    BD_UDISK,
};

enum boot_controller {
    BC_NONE,
    BC_SDMC0,
    BC_SDMC1,
    BC_SDMC2,
    BC_SPI0,
    BC_SPI1,
    BC_USB,
};

#define BD_BOOTROM BD_USB

enum boot_reason aic_get_boot_reason(void);
enum boot_device aic_get_boot_device(void);
enum boot_controller aic_get_boot_controller(void);
int aic_get_boot_image_id(void);
unsigned long aic_timer_get_us(void);

#endif /* __BOOT_PARAM_H__ */
