/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <rtconfig.h>
#ifdef RT_USING_FINSH
#include <rthw.h>
#include <rtthread.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <aic_core.h>

#include <finsh.h>

#include "artinchip_fb.h"
#include "mpp_fb.h"

/* Global macro and variables */

#ifndef LOG_TAG
#define LOG_TAG "de_test"
#endif

#define ERR(fmt, ...) aic_log(AIC_LOG_ERR, "E", fmt, ##__VA_ARGS__)
#define DBG(fmt, ...) aic_log(AIC_LOG_INFO, "I", fmt, ##__VA_ARGS__)

#define ALIGN_8B(x) (((x) + (7)) & ~(7))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))
#define ALIGN_32B(x) (((x) + (31)) & ~(31))
#define ALIGN_64B(x) (((x) + (63)) & ~(63))
#define ALIGN_128B(x) (((x) + (127)) & ~(127))
#define ALIGN_1024B(x) (((x) + (1023)) & ~(1023))

#define AICFB_VID_BUF_NUM   2

static const char sopts[] = "w:h:s:f:i:lu";
static const struct option lopts[] = {
    {"width",     required_argument, NULL, 'w'},
    {"height",    required_argument, NULL, 'h'},
    {"stride",    required_argument, NULL, 's'},
    {"format",    required_argument, NULL, 'f'},
    {"input",     required_argument, NULL, 'i'},
    {"list",            no_argument, NULL, 'l'},
    {"usage",       no_argument, NULL, 'u'},
    {0, 0, 0, 0}
};

struct video_data_format {
    enum mpp_pixel_format format;
    char f_str[16];
    int plane_num;
    int y_shift;
    int u_shift;
    int v_shift;
};

struct video_data_format g_vformat[] = {
    {MPP_FMT_YUV420P, "yuv420p", 3, 0, 2, 2},
    {MPP_FMT_YUV422P, "yuv422p", 3, 0, 1, 1},

    {MPP_FMT_NV12, "nv12", 2, 0, 1, 0},
    {MPP_FMT_NV21, "nv21", 2, 0, 1, 0},
    {MPP_FMT_NV16, "nv16", 2, 0, 0, 0},
    {MPP_FMT_NV61, "nv61", 2, 0, 0, 0},

    {MPP_FMT_YUYV, "yuyv", 1, 1, 0, 0},
    {MPP_FMT_YVYU, "yvyu", 1, 1, 0, 0},
    {MPP_FMT_UYVY, "uyvy", 1, 1, 0, 0},
    {MPP_FMT_VYUY, "vyuy", 1, 1, 0, 0},

    {MPP_FMT_YUV400, "yuv400", 1, 0, 0, 0},

    {MPP_FMT_YUV420_128x16_TILE, "yuv420_128x16", 2, 0, 1, 0},
    {MPP_FMT_YUV420_64x32_TILE,  "yuv420_64x32",  2, 0, 1, 0},
    {MPP_FMT_YUV422_128x16_TILE, "yuv422_128x16", 2, 0, 0, 0},
    {MPP_FMT_YUV422_64x32_TILE,  "yuv422_64x32",  2, 0, 0, 0},

    {MPP_FMT_MAX, "", 0, 0, 0, 0}
};

struct video_plane {
    void *buf;
    unsigned long phy_addr;
    int len;
};

struct video_buf {
    struct video_plane y;
    struct video_plane u;
    struct video_plane v;
};

struct aicfb_video_layer {
    int w;
    int h;
    int s;
    struct video_data_format *f;
    struct video_buf vbuf[AICFB_VID_BUF_NUM];
};

static struct aicfb_video_layer g_vlayer = {0};
static struct mpp_fb *g_mpp_fb = NULL;

/* Functions */

static void usage(char *program)
{
    printf("Usage: %s [options]: \n", program);
    printf("\t -w, --width\t\tneed an integer argument, default is 176\n");
    printf("\t -h, --height\t\tneed an integer argument, default is 144\n");
    printf("\t -s, --stride\t\tvideo stride, just tile format need\n");
    printf("\t -f, --format\t\tvideo format, yuv420p etc\n");
    printf("\t -i, --input\t\tneed a file name \n");
    printf("\t -l, --list\t\tlist the supported formats\n");
    printf("\t -u, --usage \n");
    printf("\n");
    printf("Example: %s -w 176 -h 144 -f yuv420p -i my.yuv\n", program);
}

static void format_list(char *program)
{
    printf("%s support the following formats:\n", program);
    printf("\t yuv420p\n");
    printf("\t yuv422p\n");
    printf("\t yuv400\n");
    printf("\t nv12\n");
    printf("\t nv21\n");
    printf("\t nv16\n");
    printf("\t nv61\n");
    printf("\t yuyv\n");
    printf("\t yvyu\n");
    printf("\t uyvy\n");
    printf("\t vyuy\n");
    printf("\t yuv420_128x16\n");
    printf("\t yuv420_64x32\n");
    printf("\t yuv422_128x16\n");
    printf("\t yuv422_64x32\n");
    printf("\n");
}

static long long int str2int(char *_str)
{
    if (_str == NULL) {
        pr_err("The string is empty!\n");
        return -1;
    }

    if (strncmp(_str, "0x", 2))
        return atoi(_str);
    else
        return strtoll(_str, NULL, 16);
}

static inline bool format_invalid(enum mpp_pixel_format format)
{
    if (format == MPP_FMT_MAX)
        return true;

    return false;
}

static inline bool is_packed_format(enum mpp_pixel_format format)
{
    switch (format) {
    case MPP_FMT_YUYV:
    case MPP_FMT_YVYU:
    case MPP_FMT_UYVY:
    case MPP_FMT_VYUY:
        return true;
    default:
        break;
    }
    return false;
}

static inline bool is_tile_format(enum mpp_pixel_format format)
{
    switch (format) {
    case MPP_FMT_YUV420_128x16_TILE:
    case MPP_FMT_YUV420_64x32_TILE:
    case MPP_FMT_YUV422_128x16_TILE:
    case MPP_FMT_YUV422_64x32_TILE:
        return true;
    default:
        break;
    }
    return false;
}

static inline bool is_plane_format(enum mpp_pixel_format format)
{
    switch (format) {
    case MPP_FMT_YUV420P:
    case MPP_FMT_YUV422P:
    case MPP_FMT_YUV400:
    case MPP_FMT_NV12:
    case MPP_FMT_NV21:
    case MPP_FMT_NV16:
    case MPP_FMT_NV61:
        return true;
    default:
        break;
    }
    return false;
}

static inline bool is_2plane(enum mpp_pixel_format format)
{
	switch (format) {
	case MPP_FMT_NV12:
	case MPP_FMT_NV21:
	case MPP_FMT_NV16:
	case MPP_FMT_NV61:
		return true;
	default:
		break;
	}
	return false;
}

static inline bool is_tile_16_align(enum mpp_pixel_format format)
{
    switch (format) {
    case MPP_FMT_YUV420_128x16_TILE:
    case MPP_FMT_YUV422_128x16_TILE:
        return true;
    default:
        break;
    }
    return false;
}

static inline bool is_tile_32_align(enum mpp_pixel_format format)
{
    switch (format) {
    case MPP_FMT_YUV420_64x32_TILE:
    case MPP_FMT_YUV422_64x32_TILE:
        return true;
    default:
        break;
    }
    return false;
}

static int tile_format_size(struct aicfb_video_layer *vlayer, int shift)
{
    int size = -1;

    if (is_tile_16_align(vlayer->f->format))
        size = vlayer->s * ALIGN_16B(vlayer->h >> shift);

    if (is_tile_32_align(vlayer->f->format))
        size = vlayer->s * ALIGN_32B(vlayer->h >> shift);

    return size;
}

static int aicfb_open(void)
{
    if (g_mpp_fb)
        return 0;

    g_mpp_fb = mpp_fb_open();
    if (!g_mpp_fb) {
        ERR("open mpp fb failed\n");
        return -1;
    }

    return 0;
}

static int vidbuf_request_one(struct video_plane *plane, int len)
{
    plane->len = len;

    plane->buf = aicos_malloc(MEM_CMA, len + 1023);
    if (!plane->buf) {
        ERR("memory alloc failed, need %d bytes", len);
        return -1;
    }

    plane->phy_addr = ALIGN_1024B((unsigned long)plane->buf);
    DBG("alloc phy_addr 0x%x\n", plane->phy_addr);

    return 0;
}

static int vidbuf_request(struct aicfb_video_layer *vlayer)
{
    int i, j;
    int y_frame = vlayer->w * vlayer->h;

    /* Prepare two group buffer for video player,
       and each group has three planes: y, u, v. */
    for (i = 0; i < AICFB_VID_BUF_NUM; i++) {
        struct video_plane *p = (struct video_plane *)&vlayer->vbuf[i];
        int *shift = &vlayer->f->y_shift;
        for (j = 0; j < vlayer->f->plane_num; j++, p++) {
            if (is_tile_format(vlayer->f->format))
                vidbuf_request_one(p, tile_format_size(vlayer, shift[j]));

            if (is_packed_format(vlayer->f->format))
                vidbuf_request_one(p, y_frame << shift[j]);

            if (is_plane_format(vlayer->f->format))
                vidbuf_request_one(p, y_frame >> shift[j]);
        }
    }

    return 0;
}

static void vidbuf_release(struct aicfb_video_layer *vlayer)
{
    int i, j;

    for (i = 0; i < AICFB_VID_BUF_NUM; i++) {
        struct video_plane *p = (struct video_plane *)&vlayer->vbuf[i];
        for (j = 0; j < vlayer->f->plane_num; j++, p++)
            aicos_free(MEM_CMA, p->buf);
    }
}

static int set_ui_layer_alpha(int val)
{
    int ret = 0;
    struct aicfb_alpha_config alpha = {0};

    alpha.layer_id = 1;
    alpha.enable = 1;
    alpha.mode = 1;
    alpha.value = val;
    ret = mpp_fb_ioctl(g_mpp_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    if (ret < 0)
        ERR("ioctl update alpha config failed!\n");

    return ret;
}

static void video_layer_set(struct aicfb_video_layer *vlayer, int index)
{
    struct aicfb_layer_data layer = {0};
    struct video_buf *vbuf = &vlayer->vbuf[index];

    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    layer.enable = 1;
    layer.pos.x = 10;
    layer.pos.y = 10;
    layer.scale_size.width = vlayer->w * 3;
    layer.scale_size.height = vlayer->h * 3;
    layer.buf.size.width = vlayer->w;
    layer.buf.size.height = vlayer->h;
    layer.buf.format = vlayer->f->format;
    layer.buf.buf_type = MPP_PHY_ADDR;
    layer.buf.phy_addr[0] = vbuf->y.phy_addr;
    layer.buf.phy_addr[1] = vbuf->u.phy_addr;
    layer.buf.phy_addr[2] = vbuf->v.phy_addr;

    if (is_packed_format(vlayer->f->format))
        layer.buf.stride[0] = vlayer->w << 1;

    if (is_tile_format(vlayer->f->format)) {
        layer.buf.stride[0] = vlayer->s;
        layer.buf.stride[1] = vlayer->s;
    }

    if (is_plane_format(vlayer->f->format)) {
        layer.buf.stride[0] = vlayer->w;
		layer.buf.stride[1] = is_2plane(vlayer->f->format) ?
					vlayer->w : vlayer->w >> 1;
        layer.buf.stride[2] = vlayer->w >> 1;
    }

    if (mpp_fb_ioctl(g_mpp_fb, AICFB_UPDATE_LAYER_CONFIG, &layer) < 0)
        ERR("ioctl update layer config failed!\n");
}

static void video_layer_disable(void)
{
    struct aicfb_layer_data layer = {0};

    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    layer.enable = 0;

    if (mpp_fb_ioctl(g_mpp_fb, AICFB_UPDATE_LAYER_CONFIG, &layer) < 0)
        ERR("ioctl update layer config failed!\n");
}

static void vidbuf_cpu_begin(struct video_buf *vbuf)
{

}

static void vidbuf_cpu_end(struct video_buf *vbuf)
{
    int i;
    struct video_plane *p = (struct video_plane *)vbuf;

    for (i = 0; i < g_vlayer.f->plane_num; i++, p++)
        aicos_dcache_clean_invalid_range((unsigned long *)p->phy_addr, p->len);
}

static int vidbuf_read(struct aicfb_video_layer *vlayer, int index, int fd)
{
    int i, ret = 0;
    static int frame_cnt = 0;
    struct video_plane *p = (struct video_plane *)&vlayer->vbuf[index];

    if (frame_cnt == 0)
        lseek(fd, 0, SEEK_SET);

    for (i = 0; i < vlayer->f->plane_num; i++, p++) {
        DBG("Frame %d - %d, len %d\n", frame_cnt, i, p->len);
        ret = read(fd, (void *)p->phy_addr, p->len);
        if (ret != p->len) {
            ERR("read(%d) return %d.\n", p->len, ret);
            return -1;
        }
    }
    frame_cnt++;

    return ret;
}

static int format_parse(char *str)
{
    int i;

    for (i = 0; g_vformat[i].format != MPP_FMT_MAX; i++) {
        if (strncmp(g_vformat[i].f_str, str, strlen(str)) == 0)
            return i;
    }

    ERR("Invalid format: %s\n", str);
    return 0;
}

static int get_file_size(const char *path)
{
    struct stat st;

    stat(path, &st);

    DBG("%s size: %d\n", path, st.st_size);

    return st.st_size;
}

static void test_de_video_layer(int argc, char **argv)
{
    int c, ret = 0;
    int vid_fd = -1;
    int fsize = 0;
    int index = 0;

    g_vlayer.w = 176;
    g_vlayer.h = 144;

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'w':
            g_vlayer.w = str2int(optarg);
            break;
        case 'h':
            g_vlayer.h = str2int(optarg);
            break;
        case 's':
            g_vlayer.s = str2int(optarg);
            DBG("stride: %d\n", g_vlayer.s);
            break;
        case 'f':
            g_vlayer.f = &g_vformat[format_parse(optarg)];
            if (format_invalid(g_vlayer.f->format))
                return format_list(argv[0]);
            DBG("format: %d\n", g_vlayer.f->format);
            break;
        case 'i':
            vid_fd = open(optarg, O_RDONLY);
            if (vid_fd < 0) {
                ERR("Failed to open %s.\n", optarg);
                return;
            }
            fsize = get_file_size(optarg);
            DBG("open(%s) fd %d, size %d\n", optarg, vid_fd, fsize);
            break;
        case 'l':
            format_list(argv[0]);
            return;
        case 'u':
            usage(argv[0]);
            return;
        default:
            usage(argv[0]);
            return;
        }
    }

    if (is_tile_format(g_vlayer.f->format) && (g_vlayer.s == 0)) {
        ERR("YUV tile format need a stride\n");
        return;
    }

    if (aicfb_open())
        goto out;

    set_ui_layer_alpha(28);
    vidbuf_request(&g_vlayer);

    do {
        struct video_buf *vbuf = &g_vlayer.vbuf[index];

        vidbuf_cpu_begin(vbuf);
        ret = vidbuf_read(&g_vlayer, index, vid_fd);
        vidbuf_cpu_end(vbuf);
        if (ret < 0)
            break;

        video_layer_set(&g_vlayer, index);
        index = !index;
        aicos_msleep(40);
        if (lseek(vid_fd, 0, SEEK_CUR) == fsize)
            break;
    } while (1);

    aicos_msleep(1000);

    video_layer_disable();
    vidbuf_release(&g_vlayer);

out:
    if (vid_fd > 0)
        close(vid_fd);
}
MSH_CMD_EXPORT_ALIAS(test_de_video_layer, test_video_layer, de video layer test);
#endif /* RT_USING_FINSH */
