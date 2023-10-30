/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DISP_COM_H_
#define _DISP_COM_H_

#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#include <rtdef.h>
#include <rthw.h>
#include <rtdevice.h>
#endif
#include <string.h>
#include <stdbool.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <artinchip_fb.h>
#include <mpp_types.h>
#include <aic_hal_dsi.h>
#include <aic_hal_rgb.h>

#include "panel/panel_com.h"
#include "disp_conf.h"

#define ALIGN_EVEN(x) ((x) & (~1))
#define ALIGN_8B(x) (((x) + (7)) & ~(7))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))
#define ALIGN_32B(x) (((x) + (31)) & ~(31))
#define ALIGN_64B(x) (((x) + (63)) & ~(63))
#define ALIGN_128B(x) (((x) + (127)) & ~(127))
#define ALIGN_1024B(x) (((x) + (1023)) & ~(1023))

#define PLL_DISP_FREQ_MIN   (360 * 1000 * 1000)

enum AIC_COM_TYPE {
    AIC_DE_COM   = 0x00,  /* display engine component */
    AIC_RGB_COM  = 0x01,  /* rgb component */
    AIC_LVDS_COM = 0x02,  /* lvds component */
    AIC_MIPI_COM = 0x03,  /* mipi dsi component */
    AIC_DBI_COM  = 0x04,  /* mipi dbi component */
};

struct panel_dbi_commands {
	const u8 *buf;
	size_t len;
};

struct spi_cfg {
	unsigned int qspi_mode;
	unsigned int vbp_num;
	unsigned int code1_cfg;
	unsigned int code[3];
};

struct panel_dbi {
    unsigned int type;
    unsigned int format;
    unsigned int first_line;
    unsigned int other_line;
    struct panel_dbi_commands commands;
    struct spi_cfg *spi;
};

struct panel_rgb {
    unsigned int mode;
    unsigned int format;
    unsigned int clock_phase;
    unsigned int data_order;
    unsigned int data_mirror;
};

enum lvds_mode {
    NS          = 0x0,
    JEIDA_24BIT = 0x1,
    JEIDA_18BIT = 0x2
};

enum lvds_link_mode {
    SINGLE_LINK0  = 0x0,
    SINGLE_LINK1  = 0x1,
    DOUBLE_SCREEN = 0x2,
    DUAL_LINK     = 0x3
};

struct panel_lvds {
    enum lvds_mode mode;
    enum lvds_link_mode link_mode;
};

struct panel_dsi {
    enum dsi_mode mode;
    enum dsi_format format;
    unsigned int lane_num;
};

struct aic_panel;

struct de_funcs {
    int (*set_mode)(struct aic_panel *panel);
    int (*clk_enable)(void);
    int (*clk_disable)(void);
    int (*timing_enable)(void);
    int (*timing_disable)(void);
    int (*wait_for_vsync)(void);
    int (*get_layer_config)(struct aicfb_layer_data *layer_data);
    int (*update_layer_config)(struct aicfb_layer_data *layer_data);
    int (*update_layer_config_list)(struct aicfb_config_lists *list);
    int (*get_alpha_config)(struct aicfb_alpha_config *alpha);
    int (*update_alpha_config)(struct aicfb_alpha_config *alpha);
    int (*get_ck_config)(struct aicfb_ck_config *ck);
    int (*update_ck_config)(struct aicfb_ck_config *ck);
    int (*set_display_prop)(struct aicfb_disp_prop *disp_prop);
    int (*get_display_prop)(struct aicfb_disp_prop *disp_prop);
    int (*set_ccm_config)(struct aicfb_ccm_config *ccm);
    int (*get_ccm_config)(struct aicfb_ccm_config *ccm);
    int (*set_gamma_config)(struct aicfb_gamma_config *gamma);
    int (*get_gamma_config)(struct aicfb_gamma_config *gamma);
};

struct di_funcs {
    enum AIC_COM_TYPE type;
    int (*clk_enable)(void);
    int (*clk_disable)(void);
    int (*enable)(void);
    int (*disable)(void);
    int (*attach_panel)(struct aic_panel *panel);
    int (*set_videomode)(const struct display_timing *timings, int enable);
    int (*send_cmd)(u32 dt, u32 vc, const u8 *data, u32 len);
};

struct platform_driver {
    const char *name;
    int component_type;
    int (*probe)(void);
    void (*remove)(void);
    union {
        struct di_funcs *di_funcs;
        struct de_funcs *de_funcs;
    };
};

struct aic_panel_callbacks {
    int (*di_enable)(void);
    int (*di_disable)(void);
    int (*di_send_cmd)(u32 dt, u32 vc, const u8 *data, u32 len);
    int (*di_set_videomode)(const struct display_timing *timings, int enable);
    int (*timing_enable)(void);
    int (*timing_disable)(void);
};

/* Each panel driver should define the follow functions. */
struct aic_panel_funcs {
    int (*prepare)(void);
    int (*enable)(struct aic_panel *panel);
    int (*disable)(struct aic_panel *panel);
    int (*unprepare)(void);
    int (*register_callback)(struct aic_panel *panel,
                struct aic_panel_callbacks *pcallback);
};

struct aic_panel {
    const char *name;
    struct aic_panel_funcs *funcs;
    struct aic_panel_callbacks callbacks;
    const struct display_timing *timings;

    union {
        struct panel_rgb  *rgb;
        struct panel_lvds *lvds;
        struct panel_dsi  *dsi;
        struct panel_dbi  *dbi;
    };

    int connector_type;
};

static inline void aic_delay_ms(u32 ms)
{
    aicos_udelay(ms * 1000);
}

static inline void aic_delay_us(u32 us)
{
    aicos_udelay(us);
}

extern struct platform_driver artinchip_rgb_driver;
extern struct platform_driver artinchip_lvds_driver;
extern struct platform_driver artinchip_dsi_driver;
extern struct platform_driver artinchip_dbi_driver;
extern struct platform_driver artinchip_de_driver;

#endif /* _DISP_COM_H_ */

