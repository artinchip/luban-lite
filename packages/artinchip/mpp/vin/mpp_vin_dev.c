// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020-2023 ArtInChip Technology Co., Ltd.
 * Authors:  Matteo <duanmt@artinchip.com>
 *
 * The device mode of MPP video in
 */

#include "aic_common.h"
#include "aic_errno.h"
#include "aic_log.h"
#include "aic_osal.h"

#include "mpp_vin.h"
#include "mpp_vin_dev.h"

#ifdef CONFIG_ARTINCHIP_MDI

#include "hal_mdi.h"
#include "mdi.h"

#include "artinchip_fb.h"
#include "display_support.h"

#ifdef CONFIG_ARTINCHIP_GE
#include "mpp_ge.h"
#endif

#define VIN_DEBUG_SHOW_FRAMERATE
#define VIN_DEBUG_DISP_ENABLE

struct mpp_vin_dev {
    struct mpp_size in_size;
    u32 in_stride;
    enum mpp_pixel_format out_fmt; /* only suport RGB565 & RGB888 */

    u32 frame_cnt;
    struct vin_video_buf vb;

    u32              ge_enable;
    u32              scale_enable;
#ifdef CONFIG_ARTINCHIP_GE
    u32              rot_angle;
    u32              ge_out_stride;
    struct mpp_size  ge_out_size;
    struct mpp_ge   *ge_dev;
    struct ge_bitblt ge_blt;
    dma_addr_t       fb_buf[2];
#endif

    s32 de_inited;
    struct aicfb_layer_data de_layer;
    struct aic_panel       *de_panel;

    aicos_mutex_t  lock;
    aicos_thread_t thid;
};

static struct mpp_vin_dev g_vin_dev = {0};

static char *g_vin_buf = NULL; /* Must 8Byte align */
static char *g_vin_buf_raw = NULL; /* Process the unaligned buf address */

static void mpp_mdi_update_out_cfg(void)
{
    struct mpp_buf *de = &g_vin_dev.de_layer.buf;
#ifdef CONFIG_ARTINCHIP_GE
    struct ge_bitblt *ge_blt = &g_vin_dev.ge_blt;
#endif

    /* Case 1: MDI -> DE, so out_cfg is decided by DE */
    if (!g_vin_dev.ge_enable)
        return;

    /* Case 2: MDI -> GE -> DE */
#ifdef CONFIG_ARTINCHIP_GE
    /* Case 2.1 Need scale. out_cfg is decided by DE */
    if (g_vin_dev.scale_enable) {
        g_vin_dev.ge_out_size   = de->size;
        g_vin_dev.ge_out_stride = de->stride[0];
    }
    /* Case 2.2 No scale, no rotate or rotate 180. out_cfg is same as input */
    else if (!g_vin_dev.rot_angle || g_vin_dev.rot_angle == MPP_ROTATION_180) {
        g_vin_dev.ge_out_size   = g_vin_dev.in_size;
        g_vin_dev.ge_out_stride = g_vin_dev.in_stride;
    }
    /* Case 2.3 No scale, rotate 90/270. out_cfg swap the width and height */
    else {
        g_vin_dev.ge_out_size.width  = g_vin_dev.in_size.height;
        g_vin_dev.ge_out_size.height = g_vin_dev.in_size.width;
        if (de->format == MPP_FMT_RGB_565)
            g_vin_dev.ge_out_stride = ALIGN_8B(g_vin_dev.in_size.height * 2);
        else
            g_vin_dev.ge_out_stride = ALIGN_8B(g_vin_dev.in_size.height * 3);
    }

    ge_blt->src_buf.size        = g_vin_dev.in_size;
    ge_blt->src_buf.stride[0]   = g_vin_dev.in_stride;
    ge_blt->src_buf.format      = de->format;
    ge_blt->dst_buf.crop_en     = 1;
    ge_blt->dst_buf.crop.width  = g_vin_dev.ge_out_size.width;
    ge_blt->dst_buf.crop.height = g_vin_dev.ge_out_size.height;
    ge_blt->dst_buf.stride[0]   = de->stride[0];
    ge_blt->dst_buf.format      = de->format;
    printf("GE: %d(%d) * %d -> %d * %d, canvas %d(%d) * %d\n",
           ge_blt->src_buf.size.width, ge_blt->src_buf.stride[0],
           ge_blt->src_buf.size.height,
           ge_blt->dst_buf.crop.width, ge_blt->dst_buf.crop.height,
           ge_blt->dst_buf.size.width, ge_blt->dst_buf.stride[0],
           ge_blt->dst_buf.size.height);
#endif
}

static int mpp_mdi_request_buf(void)
{
    int i, imagesize = 0;
    char *video_buf = g_vin_buf;
    struct vin_video_buf *vb = &g_vin_dev.vb;
    struct mpp_buf *de = &g_vin_dev.de_layer.buf;

    if (g_vin_dev.ge_enable) {
        imagesize = de->size.height * de->stride[0];
        g_vin_dev.fb_buf[0] = (dma_addr_t)g_vin_buf;
        g_vin_dev.fb_buf[1] = (dma_addr_t)(g_vin_buf + imagesize);
        video_buf = g_vin_buf + imagesize * 2;
        printf("\nAllocated double framebuffer for DE: %#lx, %#lx\n",
               (long)g_vin_dev.fb_buf[0], (long)g_vin_dev.fb_buf[1]);
    }

    if (aic_mdi_req_buf(video_buf, VIN_RESERVED_MEM_SIZE - imagesize * 2, vb))
        return -1;

    printf("\nAllocated %d video buffer for MDI:\n", vb->num_buffers);
    printf("Buf   Plane[0]     size   Plane[1]     size\n");
    for (i = 0; i < vb->num_buffers; i++) {
        printf("%3d %#x %8d %#x %8d\n", i,
            vb->planes[i * VIN_MAX_PLANE_NUM].buf,
            vb->planes[i * VIN_MAX_PLANE_NUM].len,
            vb->planes[i * VIN_MAX_PLANE_NUM + 1].buf,
            vb->planes[i * VIN_MAX_PLANE_NUM + 1].len);
    }

    /* Queue all the buffer, prepare to accept DBI data */
    for (i = 0; i < vb->num_buffers; i++) {
        if (aic_mdi_q_buf(i) < 0)
            return -1;
    }
    return 0;
}

static void mpp_mdi_release_buf(void)
{
}

static dma_addr_t mpp_mdi_get_tail_buf(void)
{
    if (g_vin_dev.ge_enable)
        return g_vin_dev.fb_buf[1];
    else
        return g_vin_dev.vb.planes[(g_vin_dev.vb.num_buffers - 1) * VIN_MAX_PLANE_NUM].buf;
}

static int dbi_cmd_soft_reset(u8 code, u8 *data)
{
    aic_mdi_reset();
    return 0;
}

static int dbi_cmd_disp_on(u8 code, u8 *data)
{
    if (!g_vin_dev.de_inited) {
#ifdef VIN_DEBUG_DISP_ENABLE
        struct aicfb_layer_data *layer = &g_vin_dev.de_layer;
#endif

        /* 1. Prepare the video buffer & framebuffer */
        if (mpp_mdi_request_buf())
            return -1;

        /* 2.1 Open GE */
        if (g_vin_dev.ge_enable) {
#ifdef CONFIG_ARTINCHIP_GE
            g_vin_dev.ge_dev = mpp_ge_open();
            if (!g_vin_dev.ge_dev)
                return -1;
#endif
        }

#ifdef VIN_DEBUG_DISP_ENABLE
        /* 2.2 Open DE */
        display_panel_probe();
        display_interface_probe();
        display_engine_probe(0, 0);

        layer->enable = 1;
        /* Set a temporary buffer to DE, and the buffer should be valid */
        layer->buf.phy_addr[0] = mpp_mdi_get_tail_buf();
        display_ioctl(AICFB_UPDATE_LAYER_CONFIG, layer);
#endif
        g_vin_dev.de_inited = 1;
    }

#ifdef VIN_DEBUG_DISP_ENABLE
    /* 3. Open panel */
    display_ioctl(AICFB_ENABLE_PANEL, NULL);
#endif
    /* 4. Start the video buffer */
    aic_mdi_stream_on();

    /* 5. Start the MDI thread */
    aicos_thread_resume(g_vin_dev.thid);

    pr_info("Turn on DE\n");
    return 0;
}

static int dbi_cmd_disp_off(u8 code, u8 *data)
{
    aicos_mutex_take(g_vin_dev.lock, AIC_OSAL_WAIT_FOREVER);

    if (!g_vin_dev.de_inited) {
        pr_err("Should init DE first.\n");
        return -1;
    }
    g_vin_dev.de_inited = 0;

    g_vin_dev.de_layer.enable = 0;
#ifdef VIN_DEBUG_DISP_ENABLE
    display_ioctl(AICFB_UPDATE_LAYER_CONFIG, &g_vin_dev.de_layer);
#endif

    aicos_mutex_give(g_vin_dev.lock);

    aic_mdi_stream_off();
    aicos_thread_suspend(g_vin_dev.thid);

    if (g_vin_dev.ge_enable) {
#ifdef CONFIG_ARTINCHIP_GE
        mpp_ge_close(g_vin_dev.ge_dev);
        g_vin_dev.ge_dev = NULL;
#endif
    }

    pr_info("Turn off DE\n");
    return 0;
}

static int dbi_cmd_width_set(u8 code, u8 *data)
{
    u16 start = dbi_byte2short(data[0], data[1]);
    u16 end = dbi_byte2short(data[2], data[3]);
    u16 size = 0;

    if (start >= end) {
        pr_err("Invalid width: %d -> %d\n", start, end);
        return -1;
    }

    size = end - start + 1;
    printf("Recv width: %d (%d - %d)\n", size, start, end);
    if (size > MDI_MAX_WIDTH) {
        pr_warn("Recv width %d is too large!\n", size);
        size = MDI_MAX_WIDTH;
    }
    aic_mdi_in_width_set(size);
    g_vin_dev.in_size.width = size;
    return 0;
}

static int dbi_cmd_height_set(u8 code, u8 *data)
{
    u16 start = dbi_byte2short(data[0], data[1]);
    u16 end = dbi_byte2short(data[2], data[3]);
    u16 size = 0;

    if (start >= end) {
        pr_err("Invalid height: %d -> %d\n", start, end);
        return -1;
    }

    size = end - start + 1;
    printf("Recv height: %d (%d - %d)\n", size, start, end);
    if (size > MDI_MAX_HEIGHT) {
        pr_warn("Recv height %d is too large!\n", size);
        size = MDI_MAX_HEIGHT;
    }
    aic_mdi_in_height_set(size);
    g_vin_dev.in_size.height = size;
    return 0;
}

static int dbi_cmd_pixel_fmt_set(u8 code, u8 *data)
{
    u8 dpi = getbits(DBI_CMD_DPI_MASK, DBI_CMD_DPI_SHIFT, data[0]);
    u8 dbi = getbits(DBI_CMD_DBI_MASK, DBI_CMD_DBI_SHIFT, data[0]);
    u8 flag = getbit(DBI_CMD_DBI_FMT_FLAG, data[0]);
    struct mpp_buf *de = &g_vin_dev.de_layer.buf;
    char *bits[] = {"", "", "", "", "", "16bits", "18bits", "24bits"};
    u32 imagesize = 0;

    if ((dpi != DPI_RGB_IF_16BIT) && (dpi != DPI_RGB_IF_24BIT)) {
        pr_err("Unsupported DPI format %d\n", dpi);
        return -1;
    }

    if (dbi < DBI_MCU_IF_16BIT) {
        pr_err("Unsupported DBI format %d\n", dbi);
        return -1;
    }

    aicos_mutex_take(g_vin_dev.lock, AIC_OSAL_WAIT_FOREVER);

    if (dbi == DBI_MCU_IF_16BIT)
        g_vin_dev.in_stride = ALIGN_8B(g_vin_dev.in_size.width * 2);
    else
        g_vin_dev.in_stride = ALIGN_8B(g_vin_dev.in_size.width * 3);

    if (dpi == DPI_RGB_IF_16BIT) {
        de->format = MPP_FMT_RGB_565;
        de->stride[0] = ALIGN_8B(de->size.width * 2);
        imagesize = de->stride[0] * de->size.height;
    } else {
        de->format = MPP_FMT_RGB_888;
        de->stride[0] = ALIGN_8B(de->size.width * 3);
        imagesize = de->stride[0] * de->size.height;
    }

    g_vin_dev.out_fmt = de->format;
    mpp_mdi_update_out_cfg();

    aicos_mutex_give(g_vin_dev.lock);
    printf("Recv DBI %s(%d) -> DPI %s(%d), Flag %d\n",
            bits[dbi], dbi, bits[dpi], dpi, flag);

    aic_mdi_in_fmt_set(dbi, flag);
    /* Consider the MDI output is DE temporary.
     * The output should changed to GE after CMD DBI_CMD_AIC_GE_CTL */
    aic_mdi_out_pixel_set(de->stride[0], imagesize);
    return 0;
}

static int dbi_cmd_bright_set(u8 code, u8 *data)
{
    pr_info("Unsupported cmd %#x\n", code);
    return 0;
}

static int dbi_cmd_ge_ctl(u8 code, u8 *data)
{
    u8 rotate = data[0] & DBI_CMD_GE_ROT_MASK;
    u32 *flag = &g_vin_dev.ge_blt.ctrl.flags;

    *flag = 0;
    if (data[0] & DBI_CMD_GE_H_FLIP)
        *flag |= MPP_FLIP_H;
    if (data[0] & DBI_CMD_GE_V_FLIP)
        *flag |= MPP_FLIP_V;
    if (data[0] & DBI_CMD_GE_SCALE)
        g_vin_dev.scale_enable = 1;

    *flag |= rotate;
    g_vin_dev.rot_angle = rotate;
    g_vin_dev.ge_enable = 1;
    printf("Recv GE ctl: H flip %s, V flip %s, Scale %s, Rotate %d\n",
            *flag & MPP_FLIP_H ? "enable" : "disable",
            *flag & MPP_FLIP_V ? "enable" : "disable",
            g_vin_dev.scale_enable ? "enable" : "disable", rotate * 90);

    mpp_mdi_update_out_cfg();
    aic_mdi_out_pixel_set(g_vin_dev.in_stride,
                          g_vin_dev.in_stride * g_vin_dev.in_size.height);
    return 0;
}

static int dbi_cmd_fr_set(u8 code, u8 *data)
{
    u8 fr = getbits(DBI_CMD_FR_MASK, DBI_CMD_FR_SHIFT, data[0]);
    u32 pixclk = fr;
    struct display_timing *t = g_vin_dev.de_panel->timing;

    aicos_mutex_take(g_vin_dev.lock, AIC_OSAL_WAIT_FOREVER);
    pixclk *= (t->hactive + t->hfront_porch + t->hback_porch + t->hsync_len) *
              (t->vactive + t->vfront_porch + t->vback_porch + t->vsync_len);
    if (pixclk > MDI_MAX_PIXCLK) {
        pr_info("pixclk %d is too large\n", pixclk);
        pixclk = MDI_MAX_PIXCLK;
    }
    t->pixelclock = pixclk;
    aicos_mutex_give(g_vin_dev.lock);

    printf("Recv framerate %d, pixclk should be %d\n", fr, pixclk);
    return 0;
}

static int dbi_cmd_porch_set(u8 code, u8 *data)
{
    struct display_timing *t = g_vin_dev.de_panel->timing;

    aicos_mutex_take(g_vin_dev.lock, AIC_OSAL_WAIT_FOREVER);
    t->vfront_porch = data[0] & DBI_CMD_VP_MASK;
    t->vback_porch  = data[1] & DBI_CMD_VP_MASK;
    t->hfront_porch = data[2];
    t->hback_porch  = data[3];
    t->hsync_len    = 2;
    t->vsync_len    = 2;
    aicos_mutex_give(g_vin_dev.lock);

    printf("Recv blank porch:\n\t hfp %d, hbp %d, vfp %d, vbp %d\n",
            t->hfront_porch, t->hback_porch, t->vfront_porch, t->vback_porch);

    return 0;
}

static int dbi_cmd_unsupported(u8 code, u8 *data)
{
    pr_info("Unsupported cmd %#x\n", code);
    return 0;
}

static struct aic_dbi_cmd g_dbi_cmds[] = {
    {DBI_CMD_SOFT_RESET,      0, "Soft reset",      dbi_cmd_soft_reset},
    {DBI_CMD_SLEEP_OUT,       0, "Sleep out",       dbi_cmd_unsupported},
    {DBI_CMD_DISP_OFF,        0, "Display off",     dbi_cmd_disp_off},
    {DBI_CMD_DISP_ON,         0, "Display on",      dbi_cmd_disp_on},
    {DBI_CMD_COL_ADDR_SET,    4, "Column addr set", dbi_cmd_width_set},
    {DBI_CMD_PAGE_ADDR_SET,   4, "Page addr set",   dbi_cmd_height_set},
    {DBI_CMD_PIXEL_FMT,       1, "Pixel format",    dbi_cmd_pixel_fmt_set},
    {DBI_CMD_BRIGHTNESS,      1, "Brightness set",  dbi_cmd_bright_set},
    {DBI_CMD_AIC_GE_CTL,      1, "AIC GE control",  dbi_cmd_ge_ctl},
    {DBI_CMD_FR_CTL,          2, "Frame rate",      dbi_cmd_fr_set},
    {DBI_CMD_BLANK_PORCH_CTL, 4, "Blank porch",     dbi_cmd_porch_set},

    {0x33, 6,  "ili94 Ver Scr",     dbi_cmd_unsupported},
    {0x35, 1,  "ili94 Tear ON",     dbi_cmd_unsupported},
    {0x36, 1,  "ili94 Mem Acc",     dbi_cmd_unsupported},
    {0x37, 2,  "ili94 Ver Addr",    dbi_cmd_unsupported},
    {0x44, 2,  "ili94 Wr Tear",     dbi_cmd_unsupported},
    {0x50, 0,  "ili94 cmd",         dbi_cmd_unsupported},
    {0xc0, 2,  "ili94 Pwr ctl1",    dbi_cmd_unsupported},
    {0xc1, 1,  "ili94 Pwr ctl2",    dbi_cmd_unsupported},
    {0xc5, 3,  "ili94 VCOM ctl",    dbi_cmd_unsupported},
    {0xb4, 1,  "ili94 Disp Inv",    dbi_cmd_unsupported},
    {0xb6, 2,  "ili94 Disp Func",   dbi_cmd_unsupported}, // data len is 3?
    {0xb7, 1,  "ili94 Entry Mod",   dbi_cmd_unsupported},
    {0xbe, 2,  "ili94 HS Lane",     dbi_cmd_unsupported},
    {0xe0, 15, "ili94 Pos GAM",     dbi_cmd_unsupported},
    {0xe1, 15, "ili94 Neg GAM",     dbi_cmd_unsupported},
    {0xe9, 1,  "ili94 Set Img",     dbi_cmd_unsupported},
    {0xec, 4,  "ili94 cmd",         dbi_cmd_unsupported},
    {0xf4, 3,  "ili94 cmd",         dbi_cmd_unsupported},
    {0xf7, 4,  "ili94 Adj ctl",     dbi_cmd_unsupported},

    {0xe7, 1,  "ST77903 cmd",       dbi_cmd_unsupported},
    {0xf0, 1,  "ST77903 cmd",       dbi_cmd_unsupported},
};

static s32 dbi_cmd_process(u8 code)
{
    u8 i, ret;
    u8 data[DBI_DAT_MAX_LEN] = {0};
    struct aic_dbi_cmd *cmd = g_dbi_cmds;

    for (i = 0; i < ARRAY_SIZE(g_dbi_cmds); i++, cmd++) {
        if (code != cmd->code)
            continue;

        pr_debug("Recv cmd %#x (%s)\n", cmd->code, cmd->name);

        if (!cmd->data_len)
            return cmd->proc(code, NULL);

        ret = aich_mdi_rd_fifo(data, cmd->data_len);
        if (!ret || ret != cmd->data_len) {
            pr_warn("CMD %#x (%s): Failed to read the data %d/%d\n",
                    cmd->code, cmd->name, ret, cmd->data_len);
            return -1;
        }

        return cmd->proc(code, data);
    }
    pr_warn("unknown command %#x\n", code);
    aich_mdi_rd_fifo(data, DBI_DAT_MAX_LEN);
    return -1;
}

#ifdef CONFIG_ARTINCHIP_GE
static int mpp_mdi_preprocess(s32 index, s32 pingpang)
{
    s32 ret = 0;
    struct ge_bitblt *ge_blt = &g_vin_dev.ge_blt;

    ge_blt->src_buf.phy_addr[0] = g_vin_dev.vb.planes[index * VIN_MAX_PLANE_NUM].buf;
    ge_blt->dst_buf.phy_addr[0] = g_vin_dev.fb_buf[pingpang];

    ret = mpp_ge_bitblt(g_vin_dev.ge_dev, ge_blt);
    if (ret < 0) {
        pr_err("GE bitblt failed！\n");
        return ret;
    }
    ret = mpp_ge_emit(g_vin_dev.ge_dev);
    if (ret < 0) {
        pr_err("GE emit failed！\n");
        return ret;
    }
    ret = mpp_ge_sync(g_vin_dev.ge_dev);
    if (ret < 0) {
        pr_err("GE sync failed!\n");
        return ret;
    }
    return 0;
}
#endif

#ifdef VIN_DEBUG_SHOW_FRAMERATE
#define NS_PER_SEC      1000000000

static void show_fps(struct timespec *start, struct timespec *end, int cnt)
{
    double diff;

    if (end->tv_nsec < start->tv_nsec) {
        diff = (double)(NS_PER_SEC + end->tv_nsec - start->tv_nsec)/NS_PER_SEC;
        diff += end->tv_sec - 1 - start->tv_sec;
    } else {
        diff = (double)(end->tv_nsec - start->tv_nsec)/NS_PER_SEC;
        diff += end->tv_sec - start->tv_sec;
    }

    printf("MDI frame rate: %.1f (%d / %.1fs)\n", (double)cnt/diff, cnt, diff);
}
#endif

static void mpp_mdi_thread(void *arg)
{
    u32 cnt = 0, index = 0;
    struct vin_video_buf *vb = &g_vin_dev.vb;
    struct aicfb_layer_data *layer = &g_vin_dev.de_layer;
#ifdef CONFIG_ARTINCHIP_GE
    s32 pingpang = 1;
#endif

#ifdef VIN_DEBUG_SHOW_FRAMERATE
    struct timespec begin, now;

    clock_gettime(CLOCK_MONOTONIC, &begin);
#endif
    while (1) {
        if (!g_vin_dev.de_inited) {
            aos_msleep(10);
            continue;
        }

        cnt++;
        if ((g_vin_dev.frame_cnt) && (cnt >= g_vin_dev.frame_cnt))
                break;

        if (aic_mdi_dq_buf(&index) < 0)
            break;

        if (g_vin_dev.ge_enable) {
#ifdef CONFIG_ARTINCHIP_GE
            pingpang = !pingpang;
            if (mpp_mdi_preprocess(index, pingpang))
                break;
#endif
        }

        aicos_mutex_take(g_vin_dev.lock, AIC_OSAL_WAIT_FOREVER);
        layer->enable = 1;
        if (g_vin_dev.ge_enable)
            layer->buf.phy_addr[0] = g_vin_dev.fb_buf[pingpang];
        else
            layer->buf.phy_addr[0] = vb->planes[index * VIN_MAX_PLANE_NUM].buf;
#ifdef VIN_DEBUG_DISP_ENABLE
        display_ioctl(AICFB_UPDATE_LAYER_CONFIG, &g_vin_dev.de_layer);
        display_ioctl(AICFB_WAIT_FOR_VSYNC, NULL);
#endif
        aicos_mutex_give(g_vin_dev.lock);

        aic_mdi_q_buf(index);

#ifdef VIN_DEBUG_SHOW_FRAMERATE
        if (cnt && (cnt % 1000 == 0)) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            show_fps(&begin, &now, cnt);
            clock_gettime(CLOCK_MONOTONIC, &begin);
            cnt = 0;
        }
#endif
    }
#ifdef VIN_DEBUG_SHOW_FRAMERATE
    clock_gettime(CLOCK_MONOTONIC, &now);
    show_fps(&begin, &now, cnt);
#endif

    mpp_mdi_release_buf();
    mpp_vin_dev_deinit();
    pr_info("MDI thread exit!\n");
}

static int mpp_vin_malloc_align(void)
{
    g_vin_buf_raw = (char *)aicos_malloc(MEM_CMA, VIN_RESERVED_MEM_SIZE + 8);
    if (!g_vin_buf_raw) {
        pr_err("Failed to malloc %d buffer\n", VIN_RESERVED_MEM_SIZE);
        return -1;
    }
    if ((long)g_vin_buf_raw % 8)
        g_vin_buf = (char *)((long)(g_vin_buf_raw + 7) & (~0x7));
    else
        g_vin_buf = g_vin_buf_raw;

    memset(g_vin_buf, 0, VIN_RESERVED_MEM_SIZE);
    pr_debug("MPP VIN buffer base: %#lx\n", (long)g_vin_buf);
    return 0;
}

int mpp_vin_dev_init(u32 cnt)
{
    struct mpp_buf *de = &g_vin_dev.de_layer.buf;
#ifdef  CONFIG_ARTINCHIP_GE
    struct ge_bitblt *ge_blt = &g_vin_dev.ge_blt;
#endif

    if (aic_mdi_open(dbi_cmd_process))
        return -1;

    if (g_vin_buf) {
        pr_info("MDI buffer is already alloced: %#lx\n", (long)g_vin_buf);
        return 0;
    }

    if (mpp_vin_malloc_align())
        return -1;

    g_vin_dev.de_panel = aic_open_panel();
    if (!g_vin_dev.de_panel)
        return -1;

    g_vin_dev.de_layer.layer_id = AICFB_LAYER_TYPE_UI;
    de->buf_type    = MPP_PHY_ADDR;
    de->format      = MPP_FMT_RGB_888;
    de->size.width  = APP_FB_WIDTH;
    de->size.height = APP_FB_HEIGHT;
    de->stride[0]   = ALIGN_8B(APP_FB_WIDTH * 3);
#ifdef CONFIG_ARTINCHIP_GE
    ge_blt->src_buf.buf_type  = MPP_PHY_ADDR;
    ge_blt->dst_buf.buf_type  = MPP_PHY_ADDR;
    ge_blt->dst_buf.size      = de->size;
    ge_blt->dst_buf.stride[0] = de->stride[0];
    ge_blt->dst_buf.format    = de->format;
#endif

    g_vin_dev.frame_cnt = cnt;
    g_vin_dev.lock = aicos_mutex_create();

    g_vin_dev.thid = aicos_thread_create("mpp_mdi", 4096, AOS_DEFAULT_APP_PRI,
                                         mpp_mdi_thread, NULL);
    if (g_vin_dev.thid == NULL) {
        pr_err("Failed to create MDI thread\n");
        return -1;
    }
    aicos_thread_suspend(g_vin_dev.thid);

    return 0;
}

void mpp_vin_dev_deinit(void)
{
    if (g_vin_buf) {
        aicos_free(MEM_CMA, g_vin_buf_raw);
        g_vin_buf = NULL;
    }

    aic_mdi_close();
}

#endif
