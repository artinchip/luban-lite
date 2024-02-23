/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>

#include "aic_common.h"
#include "artinchip_fb.h"
#include "mpp_fb.h"

#include "hal_adcim.h"
#include "hal_rtp.h"

#define AIC_POINT_NUM           5
#define AIC_CROSS_LENGTH        50
#define AIC_CROSS_WIDTH         25
#define AIC_CROSS_HEIGHT        25
#define AIC_BITS_TO_BYTE_RATE   8
#define AIC_CALI_ACCURACY       65536.0
#define AIC_DRAW_POINT_NUM      1000
#define AIC_CALI_MIN_INTERVAL   150
#define AIC_CALI_POINT_NUM      7
#define AIC_INVAILD_POINT_VAL   0xfff
#define AIC_RTP_PRESSURE_DET    1

#define AIC_POINTERCAL_PATH     "/data/config/rtp_pointercal"

static struct mpp_fb *g_fb = NULL;
static struct aicfb_screeninfo g_fb_info = {0};
static struct aic_rtp_dev g_rtp_dev = {0};
static int g_xres;
static int g_yres;
static bool g_draw_buf_sync = true;

static calibration g_cal = {
        .x = { 0 },
        .y = { 0 },
    };

static const char sopts[] = "cp:dh";
static const struct option lopts[] = {
    {"calibrate", no_argument, NULL, 'c'},
    {"points", required_argument, NULL, 'p'},
    {"draw", no_argument, NULL, 'd'},
    {"help",          no_argument, NULL, 'h'},
    {0, 0, 0, 0}
    };

static void cmd_rtp_usage(char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\t -c, --calibrate\tPlatform the screen calibration\n");
    printf("\t -p, --points\t\tSet the points for drawing, defalut is 1000\n");
    printf("\t -d, --draw\t\tDraw the shape\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example: %s -c\n", program);
}

static int test_get_fb_info(void)
{
    int ret = 0;

    g_fb = mpp_fb_open();
    if (!g_fb) {
        pr_err("mpp_fb_open error!!!!\n");
        return -1;
    }

    ret = mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &g_fb_info);
    if (ret < 0) {
        pr_err("ioctl() failed! errno: -%d\n", -ret);
        return -1;
    }

    pr_info("Screen width: %d, height: %d\n", g_fb_info.width,
            g_fb_info.height);

    g_xres = g_fb_info.width;
    g_yres = g_fb_info.height;

    return ret;
}

static int rtp_save_cali_param(calibration *cal)
{
    int cali_cnt;
    char cal_buf[sizeof(float) * AIC_CALI_POINT_NUM];
    int fd = open(AIC_POINTERCAL_PATH, O_WRONLY | O_CREAT);

    if (fd > 0) {
        for (cali_cnt = 0; cali_cnt < AIC_CALI_POINT_NUM; cali_cnt++) {
            memcpy(cal_buf + cali_cnt * sizeof(float), &cal->a[cali_cnt],
                   sizeof(float));
        }
        write(fd, cal_buf, AIC_CALI_POINT_NUM * sizeof(float));
        close(fd);
    } else {
        pr_err("open file failed!\n");
    }
    return 0;
}

/* Draw a grid, and each cell size: 200*200 */
static void rtp_draw_grid(void)
{
    u32 i, j;
    u8 *fb = g_fb_info.framebuffer;
    u8 rate = g_fb_info.bits_per_pixel / AIC_BITS_TO_BYTE_RATE;
    int stride = g_fb_info.stride;

    memset(fb, 0, g_fb_info.smem_len);
    for (i = 1; i * 200 < g_fb_info.height; i++)
        memset(fb + stride * (200 * i - 1), 0x30, stride);

    for (i = 0; i < g_fb_info.height; i++)
        for (j = 1; j * 200 < g_fb_info.width; j++)
            memset(fb + stride * i + 200 * rate * j - rate, 0x30, rate);

    aicos_dcache_clean_invalid_range(g_fb_info.framebuffer,
                                     g_fb_info.smem_len);
}

static void test_draw_a_point(u32 cnt, struct aic_rtp_event *e,
                              calibration *cal)
{
    u32 pos = 0;
    u8 *buf = NULL;
    int panel_x = 0;
    int panel_y = 0;
    int a[7] = {0};
    u8 rate = g_fb_info.bits_per_pixel / AIC_BITS_TO_BYTE_RATE;

    panel_x = AIC_RTP_MAX_VAL - e->x;
    panel_y = AIC_RTP_MAX_VAL - e->y;
    panel_x = (panel_x * g_fb_info.width) / AIC_RTP_MAX_VAL;
    panel_y = (panel_y * g_fb_info.height) / AIC_RTP_MAX_VAL;

    if (cal->a[6]) {
            memcpy(a, cal->a, sizeof(a));
            panel_x = (panel_x * a[1] + panel_y * a[2] + a[0]) / a[6];
            panel_y = (panel_x * a[4] + panel_y * a[5] + a[3]) / a[6];
        }
    printf("%d: X %d/%d, Y %d/%d, Press %d\n", cnt,
           panel_x, e->x, panel_y, e->y, e->pressure);

    pos = panel_y * g_fb_info.stride + panel_x * rate;
    if (pos < g_fb_info.smem_len) {
        buf = g_fb_info.framebuffer + pos;
        memset(buf, 0xFF, 4);
        buf -= pos % CACHE_LINE_SIZE;
        aicos_dcache_clean_invalid_range(buf, CACHE_LINE_SIZE);
        return;
    }
    pr_err("Invalid position: %d\n", pos);
}

/* Draw a cross, and each line size: 50 */
static void rtp_draw_cross(calibration *cal, int index, char *name, int y,
                           int x)
{
    u32 i;
    u8 *fb = g_fb_info.framebuffer;
    u8 rate = g_fb_info.bits_per_pixel / AIC_BITS_TO_BYTE_RATE;
    int length = AIC_CROSS_LENGTH;

    memset(fb, 0, g_fb_info.smem_len);
    memset(fb + g_fb_info.stride * (y + length / 2) + rate * x, 0xFF, rate * length);

    for (i = 0; i < length; i++)
         memset(fb + g_fb_info.stride * (y + i) + rate * (x + length / 2) , 0xFF, rate);

    cal->xfb[index] = x + length / 2;
    cal->yfb[index] = y + length / 2;
    printf("%s : X = %4d Y = %4d\n", name, cal->xfb[index], cal->yfb[index]);

    aicos_dcache_clean_invalid_range(g_fb_info.framebuffer,
                                     g_fb_info.smem_len);

    return;
}

static int rtp_perform_calibration(calibration *cal)
{
    int j;
    float n, x, y, x2, y2, xy, z, zx, zy;
    float det, a, b, c, e, f, i;
    float scaling = AIC_CALI_ACCURACY;

    /* Get sums for matrix */
    n = x = y = x2 = y2 = xy = 0;
    for (j = 0; j < AIC_POINT_NUM; j++) {
        n += 1.0;
        x += (float)cal->x[j];
        y += (float)cal->y[j];
        x2 += (float)(cal->x[j] * cal->x[j]);
        y2 += (float)(cal->y[j] * cal->y[j]);
        xy += (float)(cal->x[j] * cal->y[j]);
    }

    /* Get determinant of matrix -- check if determinant is too small */
    det = n * (x2 * y2 - xy * xy) + x * (xy * y - x * y2) + y * (x * xy - y * x2);
    if (det < 0.1 && det > -0.1) {
        printf("ts_calibrate: determinant is too small -- %f\n", det);
        return 0;
    }

    /* Get elements of inverse matrix */
    a = (x2 * y2 - xy * xy) / det;
    b = (xy * y - x * y2) / det;
    c = (x * xy - y * x2) / det;
    e = (n * y2 - y * y) / det;
    f = (x * y - n * xy) / det;
    i = (n * x2 - x * x) / det;

    /* Get sums for x calibration */
    z = zx = zy = 0;
    for (j = 0; j < AIC_POINT_NUM; j++) {
        z += (float)cal->xfb[j];
        zx += (float)(cal->xfb[j] * cal->x[j]);
        zy += (float)(cal->xfb[j] * cal->y[j]);
    }

    /* Now multiply out to get the calibration for framebuffer x coord */
    cal->a[0] = (int)((a * z + b * zx + c * zy) * (scaling));
    cal->a[1] = (int)((b * z + e * zx + f * zy) * (scaling));
    cal->a[2] = (int)((c * z + f * zx + i * zy) * (scaling));

    /* Get sums for y calibration */
    z = zx = zy = 0;
    for (j = 0; j < AIC_POINT_NUM; j++) {
        z += (float)cal->yfb[j];
        zx += (float)(cal->yfb[j] * cal->x[j]);
        zy += (float)(cal->yfb[j] * cal->y[j]);
    }

    /* Now multiply out to get the calibration for framebuffer y coord */
    cal->a[3] = (int)((a * z + b * zx + c * zy) * (scaling));
    cal->a[4] = (int)((b * z + e * zx + f * zy) * (scaling));
    cal->a[5] = (int)((c * z + f * zx + i * zy) * (scaling));

    /* If we got here, we're OK, so assign scaling to a[6] and return */
    cal->a[6] = (int)scaling;

    rtp_save_cali_param(cal);

    return 1;
}

/* Calculate the average value of multiple points triggered by one click as
 * the calibration point. Among them, the calibration point is the touch
 * screen coordinate system */
static void rtp_get_valid_point(calibration *cal, int index,
                                      struct aic_rtp_event *e)
{
    int x=0, y=0;
    int cnt = 0;
    u32 tp_x = 0, tp_y = 0;
    int sum_x =0;
    int sum_y = 0;
    int ret = 0;
    u64 start_us, end_us;

    start_us = aic_get_time_us();
    do {
redocalibration:
        memset(e, 0, sizeof(struct aic_rtp_event));
        ret = hal_rtp_ebuf_read(&g_rtp_dev.ebuf, e);
        end_us = aic_get_time_us();

        if (ret < 0)
            continue;
        if (e->x == AIC_INVAILD_POINT_VAL && e->y == AIC_INVAILD_POINT_VAL)
            continue;
        if (e->x > 0 || e->y > 0) {
            start_us = aic_get_time_us();
            x = e->x;
            y = e->y;
            sum_x += x;
            sum_y += y;
            cnt++;
        }
    } while (abs(end_us - start_us) < 600000);

    if (x == 0) {
        start_us = aic_get_time_us();
        goto redocalibration;
    }

    x = sum_x /cnt;
    y = sum_y /cnt;

    /* ADC value converted to touch panel's coordinate value */
    tp_x = AIC_RTP_MAX_VAL - x;
    tp_y = AIC_RTP_MAX_VAL - y;
    tp_x = (tp_x * g_fb_info.width) / AIC_RTP_MAX_VAL;
    tp_y = (tp_y * g_fb_info.height) / AIC_RTP_MAX_VAL;
    cal->x[index] = tp_x;
    cal->y[index] = tp_y;

    printf("Calibration: X = %d, Y = %d\n", tp_x, tp_y);

    return;
}

void rtp_calibrate(calibration *cal, struct aic_rtp_event *e)
{
    int length = AIC_CROSS_LENGTH;
    int width = AIC_CROSS_WIDTH;
    int height = AIC_CROSS_HEIGHT;

    memset(cal, 0, sizeof(&cal));

    rtp_draw_cross(cal, 0, "Top left", height, width);
    rtp_get_valid_point(cal, 0, e);

    rtp_draw_cross(cal, 1, "Top right", height, g_xres - width - length);
    rtp_get_valid_point(cal, 1, e);

    rtp_draw_cross(cal, 2, "Bot right", g_yres - height - length,
                   g_xres - width - length);
    rtp_get_valid_point(cal, 2, e);

    rtp_draw_cross(cal, 3, "Bot left", g_yres - height - length, width);
    rtp_get_valid_point(cal, 3, e);

    rtp_draw_cross(cal, 4, "Center", (g_yres - length) / 2,
                   (g_xres - length) / 2);
    rtp_get_valid_point(cal, 4, e);

    memset(g_fb_info.framebuffer, 0, g_fb_info.smem_len);
    rtp_perform_calibration(cal);

    return;
}

static int test_rtp_init(void)
{
    if (hal_adcim_probe())
        return -1;

    if (hal_rtp_clk_init())
        return -1;

    g_rtp_dev.x_plate = AIC_RTP_X_PLATE;
    g_rtp_dev.y_plate = AIC_RTP_Y_PLATE;
    g_rtp_dev.mode = RTP_MODE_AUTO2;
    g_rtp_dev.max_press = AIC_RTP_MAX_PRESSURE;
    g_rtp_dev.smp_period = AIC_RTP_PERIOD_MS;
    g_rtp_dev.pressure_det = AIC_RTP_PRESSURE_DET;
    g_rtp_dev.pdeb = AIC_RTP_PDEB;
    g_rtp_dev.delay = AIC_RTP_DELAY;

    aicos_request_irq(RTP_IRQn, hal_rtp_isr, 0, NULL, NULL);
    hal_rtp_enable(&g_rtp_dev, 1);
    hal_rtp_int_enable(&g_rtp_dev, 1);
    hal_rtp_auto_mode(&g_rtp_dev);
    return 0;
}

static void ts_rtp_deinit(void)
{
    hal_rtp_int_enable(&g_rtp_dev, 0);
    hal_rtp_enable(&g_rtp_dev, 0);
}

static void rtp_draw(int max, struct aic_rtp_event *e, calibration *cal)
{
    u32 ret, cnt = 0;
    static u32 last_time = 0;

    rtp_draw_grid();

    char cal_buf[sizeof(float) * AIC_CALI_POINT_NUM];
    int cali_cnt;

    int fd = open(AIC_POINTERCAL_PATH, O_RDONLY);
    if (fd >= 0) {
        read(fd, cal_buf, AIC_CALI_POINT_NUM * sizeof(float));
        for (cali_cnt = 0; cali_cnt < AIC_CALI_POINT_NUM; cali_cnt++) {
            cal->a[cali_cnt] = *(int *)(cal_buf + cali_cnt * sizeof(float));
        }
        close(fd);
    }

    pr_info("Try to read %d points from RTP ...\n", max);

    do {
        memset(e, 0, sizeof(struct aic_rtp_event));
        ret = hal_rtp_ebuf_read(&g_rtp_dev.ebuf, e);
        if (ret < 0)
            continue;
        if (e->x == AIC_INVAILD_POINT_VAL && e->y == AIC_INVAILD_POINT_VAL)
            continue;
        if (e->x > 0 || e->y > 0) {
            if (e->timestamp == last_time) {
                continue;
            }
            last_time = e->timestamp;
            test_draw_a_point(cnt, e, cal);
            cnt++;
        }
    } while (cnt < max);
}

static int cmd_test_rtp_draw(int argc, char *argv[])
{
    int c;
    struct aic_rtp_event e = {0};
    static int draw_point_num = AIC_DRAW_POINT_NUM;

    if (argc < 2) {
        cmd_rtp_usage(argv[0]);
        return 0;
    }

    if (test_get_fb_info())
        return -1;

    if (test_rtp_init())
        return -1;

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'c':
            rtp_calibrate(&g_cal, &e);
            break;
        case 'p':
            draw_point_num = atoi(optarg);
            break;
        case 'd':
            g_draw_buf_sync = true;
            rtp_draw(draw_point_num, &e, &g_cal);
            break;
        case 'h':
        default:
            cmd_rtp_usage(argv[0]);
            return 0;
        }
    }

    if (g_fb)
        mpp_fb_close(g_fb);

    ts_rtp_deinit();
    return 0;
}

CONSOLE_CMD(test_rtp_draw, cmd_test_rtp_draw, "RTP test example");
