/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <aic_hal_ge.h>
#include "disp_com.h"
#ifdef AIC_FB_ROTATE_EN
#include <aic_drv_ge.h>
#endif
#if defined(KERNEL_RTTHREAD)
#include <drivers/pm.h>
#endif

#undef pr_debug
#ifdef AIC_FB_DRV_DEBUG
#define pr_debug    pr_info
#else
#define pr_debug(fmt, ...)
#endif

#define AICFB_ON    1
#define AICFB_OFF   0

struct aicfb_info
{
    struct platform_driver *de;
    struct platform_driver *di;
    struct aic_panel *panel;

#if defined(KERNEL_RTTHREAD)
    struct rt_device device;
#endif

    u32 width;
    u32 height;

    u32 fb_rotate;

    void *fb_start;
    u32 stride;
    size_t fb_size;
    u32 bits_per_pixel;
};

static void aicfb_enable_clk(struct aicfb_info *fbi, u32 on);
static void aicfb_enable_panel(struct aicfb_info *fbi, u32 on);

static struct aicfb_info *g_aicfb_info;
static bool aicfb_probed = false;

static inline struct aicfb_info *aicfb_get_drvdata(void)
{
    return g_aicfb_info;
}

static inline void aicfb_set_drvdata(struct aicfb_info *fbi)
{
    g_aicfb_info = fbi;
}

static void *aicfb_malloc_align(size_t size, size_t align)
{
    size_t fb_size;
    void *fb_start;

    fb_size = size + align - 1;

    fb_start = aicos_malloc(MEM_CMA, fb_size);
    if (!fb_start) {
        pr_err("alloc fb0 failed\n");
        return NULL;
    }

    return (void *)ALIGN_UP((uintptr_t)fb_start, align);
}

#ifdef AIC_FB_ROTATE_EN
static int aicfb_rotate(struct aicfb_info *fbi, struct aicfb_layer_data *layer,
                u32 buf_id)
{
    struct ge_bitblt blt = {0};
    struct aic_ge_client *client = NULL;

    /* source buffer */
    blt.src_buf.buf_type = MPP_PHY_ADDR;
    blt.src_buf.phy_addr[0] = (uintptr_t)fbi->fb_start + fbi->fb_size * buf_id;
    blt.src_buf.stride[0] = fbi->stride;
    blt.src_buf.size.width = fbi->width;
    blt.src_buf.size.height = fbi->height;
    blt.src_buf.format = layer->buf.format;

    /* destination buffer */
    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = layer->buf.phy_addr[0];
    blt.dst_buf.stride[0] = layer->buf.stride[0];
    blt.dst_buf.size.width = layer->buf.size.width;
    blt.dst_buf.size.height = layer->buf.size.height;
    blt.dst_buf.format = layer->buf.format;

    switch (fbi->fb_rotate) {
    case 0:
        blt.ctrl.flags = 0;
        break;
    case 90:
        blt.ctrl.flags = MPP_ROTATION_90;
        break;
    case 180:
        blt.ctrl.flags = MPP_ROTATION_180;
        break;
    case 270:
        blt.ctrl.flags = MPP_ROTATION_270;
        break;
    default:
        pr_err("Invalid rotation degree\n");
        return -EINVAL;
    };

    return hal_ge_control(client, IOC_GE_BITBLT, &blt);
}
#endif

static int aicfb_pan_display(struct aicfb_info *fbi, u32 buf_id)
{
    struct aicfb_layer_data layer = {0};
    struct platform_driver *de = fbi->de;

#if !defined(AIC_PAN_DISPLAY) && !defined(AIC_FB_ROTATE_EN)
    pr_err("pan display do not enabled\n");
    return -EINVAL;
#endif

    layer.layer_id = AICFB_LAYER_TYPE_UI;
    layer.rect_id = 0;
    de->de_funcs->get_layer_config(&layer);

    layer.enable = 1;
    layer.buf.phy_addr[0] = (uintptr_t)fbi->fb_start + fbi->fb_size * buf_id;

#ifdef AIC_FB_ROTATE_EN
    if (fbi->fb_rotate)
    {
        layer.buf.phy_addr[0] += fbi->fb_size * AIC_FB_DRAW_BUF_NUM;

        aicfb_rotate(fbi, &layer, buf_id);
    }
#endif

    de->de_funcs->update_layer_config(&layer);
    return 0;
}

int aicfb_ioctl(int cmd, void *args)
{
    struct aicfb_info *fbi = aicfb_get_drvdata();

    switch (cmd)
    {
    case AICFB_WAIT_FOR_VSYNC:
        return fbi->de->de_funcs->wait_for_vsync();

    case AICFB_GET_SCREENREG:
    {
        if (!fbi->di->di_funcs->read_cmd) {
            pr_err("display interface do not supports AICFB_GET_SCREENREG\n");
            return -EINVAL;
        }

        return fbi->di->di_funcs->read_cmd(*(u32 *)args);
    }
    case AICFB_GET_SCREENINFO:
    {
        const struct display_timing *timing = fbi->panel->timings;
        struct aicfb_screeninfo *info;

        info = (struct aicfb_screeninfo *) args;
        info->format = AICFB_FORMAT;
        info->bits_per_pixel = fbi->bits_per_pixel;
        info->stride = fbi->stride;
        info->framebuffer = (unsigned char *)fbi->fb_start;
        info->width = timing->hactive;
        info->height = timing->vactive;
        info->smem_len = fbi->fb_size;
        break;
    }
    case AICFB_PAN_DISPLAY:
        return aicfb_pan_display(fbi, *(int *)args);

    case AICFB_POWERON:
    {
        struct aicfb_layer_data layer = {0};
        struct platform_driver *de = fbi->de;

        aicfb_enable_clk(fbi, AICFB_ON);

        layer.layer_id = AICFB_LAYER_TYPE_UI;
        layer.rect_id = 0;
        de->de_funcs->get_layer_config(&layer);

        layer.enable = 1;
        de->de_funcs->update_layer_config(&layer);

        aicfb_enable_panel(fbi, AICFB_ON);
        break;
    }
    case AICFB_POWEROFF:
    {
        struct aicfb_layer_data layer = {0};
        struct platform_driver *de = fbi->de;

        aicfb_enable_panel(fbi, AICFB_OFF);

        layer.layer_id = AICFB_LAYER_TYPE_UI;
        layer.rect_id = 0;
        de->de_funcs->get_layer_config(&layer);

        layer.enable = 0;
        de->de_funcs->update_layer_config(&layer);

        aicfb_enable_clk(fbi, AICFB_OFF);
        break;
    }
    case AICFB_GET_LAYER_CONFIG:
        return fbi->de->de_funcs->get_layer_config(args);

    case AICFB_UPDATE_LAYER_CONFIG:
        return fbi->de->de_funcs->update_layer_config(args);

    case AICFB_UPDATE_LAYER_CONFIG_LISTS:
        return fbi->de->de_funcs->update_layer_config_list(args);

    case AICFB_GET_ALPHA_CONFIG:
        return fbi->de->de_funcs->get_alpha_config(args);

    case AICFB_UPDATE_ALPHA_CONFIG:
        return fbi->de->de_funcs->update_alpha_config(args);

    case AICFB_GET_CK_CONFIG:
        return fbi->de->de_funcs->get_ck_config(args);

    case AICFB_UPDATE_CK_CONFIG:
        return fbi->de->de_funcs->update_ck_config(args);

    case AICFB_SET_DISP_PROP:
        return fbi->de->de_funcs->set_display_prop(args);

    case AICFB_GET_DISP_PROP:
    {
        s32 ret = 0;
        struct aicfb_disp_prop prop;

        ret = fbi->de->de_funcs->get_display_prop(&prop);
        if (ret)
            return ret;

        memcpy(args, &prop, sizeof(struct aicfb_disp_prop));
        break;
    }
    case AICFB_GET_CCM_CONFIG:
        return fbi->de->de_funcs->get_ccm_config(args);

    case AICFB_SET_CCM_CONFIG:
        return fbi->de->de_funcs->set_ccm_config(args);

    case AICFB_SET_GAMMA_CONFIG:
        return fbi->de->de_funcs->set_gamma_config(args);

    case AICFB_GET_GAMMA_CONFIG:
        return fbi->de->de_funcs->get_gamma_config(args);

    default:
        pr_err("Invalid ioctl cmd %#x\n", cmd);
        return -EINVAL;
    }
    return 0;
}

#if defined(KERNEL_RTTHREAD)
#ifdef RT_USING_PM
static int aicfb_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct aicfb_info *fbi = device->user_data;
    struct platform_driver *de = fbi->de;

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
    {
        struct aicfb_layer_data layer = {0};

        aicfb_enable_panel(fbi, AICFB_OFF);

        layer.layer_id = AICFB_LAYER_TYPE_UI;
        layer.rect_id = 0;
        de->de_funcs->get_layer_config(&layer);

        layer.enable = 0;
        de->de_funcs->update_layer_config(&layer);

        aicfb_enable_clk(fbi, AICFB_OFF);
    }
        break;
    default:
        break;
    }

    return 0;
}

static void aicfb_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct aicfb_info *fbi = device->user_data;
    struct platform_driver *de = fbi->de;

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
    {
        struct aicfb_layer_data layer = {0};

        aicfb_enable_clk(fbi, AICFB_ON);

        layer.layer_id = AICFB_LAYER_TYPE_UI;
        layer.rect_id = 0;
        de->de_funcs->get_layer_config(&layer);

        layer.enable = 1;
        de->de_funcs->update_layer_config(&layer);
        aicfb_enable_panel(fbi, AICFB_ON);
    }
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops aicfb_pm_ops =
{
    SET_DEVICE_PM_OPS(aicfb_suspend, aicfb_resume)
    NULL,
};
#endif /* RT_USING_PM */

rt_err_t aicfb_control(rt_device_t dev, int cmd, void *args)
{
    struct aicfb_info *fbi = aicfb_get_drvdata();
    int command;

    switch (cmd)
    {
    case RTGRAPHIC_CTRL_WAIT_VSYNC:
        command = AICFB_WAIT_FOR_VSYNC;
        break;
    case RTGRAPHIC_CTRL_GET_INFO:
    {
        const struct display_timing *timing = fbi->panel->timings;
        struct rt_device_graphic_info *info;

        info = (struct rt_device_graphic_info *) args;
        info->pixel_format = AICFB_FORMAT;
        info->bits_per_pixel = fbi->bits_per_pixel;
        info->pitch = (rt_uint16_t)fbi->stride;
        info->framebuffer = (rt_uint8_t*)fbi->fb_start;
        info->width = (rt_uint16_t)timing->hactive;
        info->height = (rt_uint16_t)timing->vactive;
        info->smem_len = fbi->fb_size;

        return RT_EOK;
    }
    case RTGRAPHIC_CTRL_PAN_DISPLAY:
        command = AICFB_PAN_DISPLAY;
        break;
    case RTGRAPHIC_CTRL_POWERON:
        command = AICFB_POWERON;
        break;
    case RTGRAPHIC_CTRL_POWEROFF:
        command = AICFB_POWEROFF;
        break;
    default:
        command = cmd;
        break;
    }

    return aicfb_ioctl(command, args);
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops aicfb_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    aicfb_control
};
#endif /* RT_USING_DEVICE_OPS */

static int aic_rt_fb_device_init(struct aicfb_info *fbi)
{
    struct rt_device *device = &fbi->device;
    int ret;

    device->type = RT_Device_Class_Graphic;

#ifdef RT_USING_DEVICE_OPS
    device->ops = &aicfb_ops;
#else
    device->init = RT_NULL;
    device->open = RT_NULL;
    device->close = RT_NULL;
    device->read = RT_NULL;
    device->write = RT_NULL;
    device->control = aicfb_control;
#endif /* RT_USING_DEVICE_OPS */

    device->user_data = fbi;

    /* register to device manager */
    ret = rt_device_register(device, "aicfb", RT_DEVICE_FLAG_RDWR);
    if(ret != RT_EOK)
    {
        pr_err("aicfb registered fail!\n");
        return ret;
    }

#ifdef RT_USING_PM
    rt_pm_device_register(device, &aicfb_pm_ops);
#endif
    return RT_EOK;
}
#endif /* KERNEL_RTTHREAD */

static struct platform_driver *drivers[] = {
#ifdef AIC_DISP_DE_DRV
    &artinchip_de_driver,
#endif
#ifdef AIC_DISP_RGB
    &artinchip_rgb_driver,
#endif
#ifdef AIC_DISP_LVDS
    &artinchip_lvds_driver,
#endif
#ifdef AIC_DISP_MIPI_DSI
    &artinchip_dsi_driver,
#endif
#ifdef AIC_DISP_MIPI_DBI
    &artinchip_dbi_driver,
#endif
};

static struct platform_driver *
aicfb_find_component(struct platform_driver **drv, int id, int len)
{
    struct platform_driver **driver = drv;
    int ret, i;

    for (i = 0; i < len; i++)
    {
        if (driver[i]->component_type == id)
            break;
    }

    if (i >= len || !driver[i] || !driver[i]->probe)
        return NULL;

    ret = driver[i]->probe();
    if (ret)
        return NULL;

    pr_debug("find component: %s\n", driver[i]->name);

    return driver[i];
}

static void aicfb_get_panel_info(struct aicfb_info *fbi)
{
    struct platform_driver *de = fbi->de;
    struct platform_driver *di = fbi->di;
    struct aic_panel *panel = fbi->panel;

    if(de->de_funcs->set_mode)
        de->de_funcs->set_mode(panel);

    if(di->di_funcs->attach_panel)
        di->di_funcs->attach_panel(panel);
}

static void aicfb_register_panel_callback(struct aicfb_info *fbi)
{
    struct platform_driver *de = fbi->de;
    struct platform_driver *di = fbi->di;
    struct aic_panel *panel = fbi->panel;
    struct aic_panel_callbacks cb;

    cb.di_enable = di->di_funcs->enable;
    cb.di_disable = di->di_funcs->disable;
    cb.di_send_cmd = di->di_funcs->send_cmd;
    cb.di_set_videomode = di->di_funcs->set_videomode;
    cb.timing_enable = de->de_funcs->timing_enable;
    cb.timing_disable = de->de_funcs->timing_disable;
    panel->funcs->register_callback(panel, &cb);
}

static inline int aicfb_format_bpp(enum mpp_pixel_format format)
{
    switch (format) {
    case MPP_FMT_ARGB_8888:
    case MPP_FMT_ABGR_8888:
    case MPP_FMT_RGBA_8888:
    case MPP_FMT_BGRA_8888:
    case MPP_FMT_XRGB_8888:
    case MPP_FMT_XBGR_8888:
    case MPP_FMT_RGBX_8888:
    case MPP_FMT_BGRX_8888:
        return 32;
    case MPP_FMT_RGB_888:
    case MPP_FMT_BGR_888:
        return 24;
    case MPP_FMT_ARGB_1555:
    case MPP_FMT_ABGR_1555:
    case MPP_FMT_RGBA_5551:
    case MPP_FMT_BGRA_5551:
    case MPP_FMT_RGB_565:
    case MPP_FMT_BGR_565:
    case MPP_FMT_ARGB_4444:
    case MPP_FMT_ABGR_4444:
    case MPP_FMT_RGBA_4444:
    case MPP_FMT_BGRA_4444:
        return 16;
    default:
        break;
    }
    return 32;
}

static void aicfb_fb_info_setup(struct aicfb_info *fbi)
{
    u32 stride, bpp;
    u32 active_w;
    u32 active_h;
    size_t fb_size;

    bpp = aicfb_format_bpp(AICFB_FORMAT);

#ifdef AIC_FB_ROTATE_EN
    fbi->fb_rotate = AIC_FB_ROTATE_DEGREE;
#else
    fbi->fb_rotate = 0;
#endif

    if (fbi->fb_rotate == 90 || fbi->fb_rotate == 270)
    {
        active_w = fbi->panel->timings->vactive;
        active_h = fbi->panel->timings->hactive;
    }
    else
    {
        active_w = fbi->panel->timings->hactive;
        active_h = fbi->panel->timings->vactive;
    }

    stride = ALIGN_8B(active_w * bpp / 8);
    fb_size = active_h * stride;

    fbi->width = active_w;
    fbi->height = active_h;

    fbi->bits_per_pixel = bpp;
    fbi->stride = stride;
    fbi->fb_size = fb_size;
}

static void fb_color_block(struct aicfb_info *fbi)
{
#ifdef AIC_DISP_COLOR_BLOCK
    u32 width, height;
    u32 i, j, index;
    unsigned char color[5][3] = {
        { 0x00, 0x00, 0xFF },
        { 0x00, 0xFF, 0x00 },
        { 0xFF, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0xFF, 0xFF, 0xFF },
    };
    unsigned char *pos = (unsigned char *)fbi->fb_start;

    width = fbi->panel->timings->hactive;
    height = fbi->panel->timings->vactive;

    switch (fbi->bits_per_pixel) {
#if defined(AICFB_ARGB8888) || defined(AICFB_ABGR8888) || defined(AICFB_XRGB8888)
    case 32:
    {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                index = (i / 100 + j / 100) % 5;

                *(pos++) = color[index][0];
                *(pos++) = color[index][1];
                *(pos++) = color[index][2];
                *(pos++) = 0;
            }
        }
        break;
    }
#endif
#if defined(AICFB_RGB888)
    case 24:
    {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                index = (i / 100 + j / 100) % 5;

                *(pos++) = color[index][0];
                *(pos++) = color[index][1];
                *(pos++) = color[index][2];
            }
        }
        break;
    }
#endif
#if defined(AICFB_RGB565)
    case 16:
    {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                index = (i / 100 + j / 100) % 5;

                *(pos++) = (color[index][0] >> 3) | ((color[index][1] & 0x1c) << 3);
                *(pos++) = ((color[index][1] & 0xe0) >> 5) | (color[index][2] & 0xf8);
            }
        }
        break;
    }
#endif
    default:
        *pos = color[0][0];
        index = AICFB_FORMAT;
        i = width;
        j = height;

        pr_info("format(%d) do not support %dx%d color block.\n", index, i, j);
        return;
    }

    aicos_dcache_clean_range((unsigned long *)fbi->fb_start, fbi->fb_size);
#endif
}

static void aicfb_enable_clk(struct aicfb_info *fbi, u32 on)
{
    struct platform_driver *de = fbi->de;
    struct platform_driver *di = fbi->di;

    if (on == AICFB_ON)
    {
        de->de_funcs->clk_enable();
        di->di_funcs->clk_enable();
    }
    else
    {
        di->di_funcs->clk_disable();
        de->de_funcs->clk_disable();
    }
}

static void aicfb_update_alpha(struct aicfb_info *fbi)
{
    struct platform_driver *de = fbi->de;
    struct aicfb_alpha_config alpha = {0};

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    de->de_funcs->get_alpha_config(&alpha);

    de->de_funcs->update_alpha_config(&alpha);
}

static void aicfb_update_layer(struct aicfb_info *fbi)
{
    struct platform_driver *de = fbi->de;
    struct aic_panel *panel = fbi->panel;
    struct aicfb_layer_data layer = {0};

    layer.layer_id = AICFB_LAYER_TYPE_UI;
    layer.rect_id = 0;
    de->de_funcs->get_layer_config(&layer);

    layer.enable = 1;

    switch (fbi->fb_rotate)
    {
    case 0:
        layer.buf.size.width = panel->timings->hactive;
        layer.buf.size.height = panel->timings->vactive;
        layer.buf.stride[0] = fbi->stride;
        layer.buf.phy_addr[0] = (uintptr_t)fbi->fb_start;
        layer.buf.format = AICFB_FORMAT;
        break;
#ifdef AIC_FB_ROTATE_EN
    case 90:
    case 270:
    {
        unsigned int stride = ALIGN_8B(fbi->height * fbi->bits_per_pixel / 8);

        layer.buf.phy_addr[0] = (uintptr_t)fbi->fb_start + fbi->fb_size * AIC_FB_DRAW_BUF_NUM;
        layer.buf.size.width = panel->timings->hactive;
        layer.buf.size.height = panel->timings->vactive;
        layer.buf.stride[0] = stride;
        break;
    }
    case 180:
        pr_info("rotate 180\n");
        layer.buf.phy_addr[0] = (uintptr_t)fbi->fb_start + fbi->fb_size * AIC_FB_DRAW_BUF_NUM;
        layer.buf.size.width = panel->timings->hactive;
        layer.buf.size.height = panel->timings->vactive;
        layer.buf.stride[0] = fbi->stride;
        break;
#endif
    default:
        pr_err("Invalid rotation degree\n");
        return;
    }

    layer.buf.crop_en = 0;
    layer.buf.format = AICFB_FORMAT;
    layer.buf.flags = 0;

    de->de_funcs->update_layer_config(&layer);
}

static void aicfb_enable_panel(struct aicfb_info *fbi, u32 on)
{
    struct aic_panel *panel = fbi->panel;

    if (on == AICFB_ON)
    {
        panel->funcs->prepare();
        panel->funcs->enable(panel);
    }
    else
    {
        panel->funcs->disable(panel);
        panel->funcs->unprepare();
    }

}

static inline size_t aicfb_calc_fb_size(struct aicfb_info *fbi)
{
    unsigned int draw_buf_num = 0;
    unsigned int disp_buf_num = 0;

#ifdef AIC_FB_ROTATE_EN
    draw_buf_num = AIC_FB_DRAW_BUF_NUM;
#else
    draw_buf_num = 0;
#endif

#ifdef AIC_PAN_DISPLAY
    disp_buf_num = 2;
#else
    disp_buf_num = 1;
#endif

     return fbi->fb_size * (disp_buf_num + draw_buf_num);
}

int aicfb_probe(void)
{
    struct aicfb_info *fbi;
    size_t fb_size;
    int ret = -EINVAL;

    if (aicfb_probed)
        return 0;

    fbi = aicos_malloc(0, sizeof(*fbi));
    if (!fbi)
    {
        pr_err("alloc fb_info failed\n");
        return -ENOMEM;
    }

    fbi->de = aicfb_find_component(drivers, AIC_DE_COM, ARRAY_SIZE(drivers));
    if (!fbi->de)
    {
        pr_err("failed to find de component\n");
        goto err;
    }

    fbi->di = aicfb_find_component(drivers, AIC_DI_TYPE, ARRAY_SIZE(drivers));
    if (!fbi->di)
    {
        pr_err("failed to find di component\n");
        goto err;
    }

    fbi->panel = aic_find_panel(AIC_DI_TYPE);
    if (!fbi->panel)
    {
        pr_err("failed to find panel component\n");
        goto err;
    }

    aicfb_fb_info_setup(fbi);

    fb_size = aicfb_calc_fb_size(fbi);
    /* fb_start must be cache line align */
    fbi->fb_start = aicfb_malloc_align(fb_size, CACHE_LINE_SIZE);
    if (!fbi->fb_start)
    {
        ret = -ENOMEM;
        goto err;
    }
    pr_info("fb0 allocated at 0x%x\n", (u32)(uintptr_t)fbi->fb_start);

    fb_color_block(fbi);

    aicfb_get_panel_info(fbi);
    aicfb_register_panel_callback(fbi);

#if defined(KERNEL_RTTHREAD)
    ret = aic_rt_fb_device_init(fbi);
    if(ret != RT_EOK)
        goto err;
#endif

    aicfb_probed = true;
    aicfb_set_drvdata(fbi);

    aicfb_enable_clk(fbi, AICFB_ON);
    aicfb_update_alpha(fbi);
    aicfb_update_layer(fbi);
#if defined(AIC_DISP_COLOR_BLOCK)
    aicfb_enable_panel(fbi, AICFB_ON);
#endif
    return 0;

err:
    free(fbi);
    return ret;
}
#if defined(KERNEL_RTTHREAD)
INIT_DEVICE_EXPORT(aicfb_probe);
#endif

void aicfb_remove(void)
{
    struct aicfb_info *fbi = aicfb_get_drvdata();

    aicfb_probed = false;
    free(fbi);
}

