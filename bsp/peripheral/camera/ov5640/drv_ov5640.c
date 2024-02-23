/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#define LOG_TAG     "ov5640"

#include <drivers/i2c.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "aic_hal_clk.h"
#include "mpp_types.h"
#include "mpp_vin.h"

#include "drv_camera.h"

/* Format configuration of OV5640 */
#define OV5640_MODE             OV5640_MODE_VGA_640_480
#define OV5640_BUS_TYPE         MEDIA_BUS_PARALLEL
#define OV5640_CODE             MEDIA_BUS_FMT_UYVY8_2X8
#define OV5640_DVP_BUS_WIDTH    8
#define OV5640_IIC_TIMEOUT      3000    // 3 sec
#define OV5640_AUTO_EXPOSURE    1
#define OV5640_AUTO_GAIN        1
#define OV5640_CSI2_DATA_LANES  2

/* The clk source is decided by board design */
#ifdef AIC_CHIP_D13X
#define OV5640_CLK_SRC          CLK_OUT1
#define OV5640_FPS              OV5640_15_FPS
#else
#define OV5640_CLK_SRC          CLK_OUT1
#define OV5640_FPS              OV5640_24_FPS
#endif

/* min/typical/max system clock (xclk) frequencies */
#define OV5640_XCLK_MIN     6000000
#define OV5640_XCLK_MAX     54000000

#define OV5640_DEFAULT_SLAVE_ID         0x3c

#define OV5640_REG_SYS_RESET02          0x3002
#define OV5640_REG_SYS_CLOCK_ENABLE02   0x3006
#define OV5640_REG_SYS_CTRL0            0x3008
#define OV5640_REG_SYS_CTRL0_SW_PWDN    0x42
#define OV5640_REG_SYS_CTRL0_SW_PWUP    0x02
#define OV5640_REG_CHIP_ID              0x300a
#define OV5640_REG_IO_MIPI_CTRL00       0x300e
#define OV5640_REG_PAD_OUTPUT_ENABLE01  0x3017
#define OV5640_REG_PAD_OUTPUT_ENABLE02  0x3018
#define OV5640_REG_PAD_OUTPUT00         0x3019
#define OV5640_REG_OUTPUT_DRV_CAP       0x302c
#define OV5640_REG_SYSTEM_CONTROL1      0x302e
#define OV5640_REG_SC_PLL_CTRL0         0x3034
#define OV5640_REG_SC_PLL_CTRL1         0x3035
#define OV5640_REG_SC_PLL_CTRL2         0x3036
#define OV5640_REG_SC_PLL_CTRL3         0x3037
#define OV5640_REG_SLAVE_ID             0x3100
#define OV5640_REG_SCCB_SYS_CTRL1       0x3103
#define OV5640_REG_SYS_ROOT_DIVIDER     0x3108
#define OV5640_REG_AWB_R_GAIN           0x3400
#define OV5640_REG_AWB_G_GAIN           0x3402
#define OV5640_REG_AWB_B_GAIN           0x3404
#define OV5640_REG_AWB_MANUAL_CTRL      0x3406
#define OV5640_REG_AEC_PK_EXPOSURE_HI   0x3500
#define OV5640_REG_AEC_PK_EXPOSURE_MED  0x3501
#define OV5640_REG_AEC_PK_EXPOSURE_LO   0x3502
#define OV5640_REG_AEC_PK_MANUAL        0x3503
#define OV5640_REG_AEC_PK_REAL_GAIN     0x350a
#define OV5640_REG_AEC_PK_VTS           0x350c
#define OV5640_REG_TIMING_DVPHO         0x3808
#define OV5640_REG_TIMING_DVPVO         0x380a
#define OV5640_REG_TIMING_HTS           0x380c
#define OV5640_REG_TIMING_VTS           0x380e
#define OV5640_REG_TIMING_TC_REG20      0x3820
#define OV5640_REG_TIMING_TC_REG21      0x3821
#define OV5640_REG_AEC_CTRL00           0x3a00
#define OV5640_REG_AEC_B50_STEP         0x3a08
#define OV5640_REG_AEC_B60_STEP         0x3a0a
#define OV5640_REG_AEC_CTRL0D           0x3a0d
#define OV5640_REG_AEC_CTRL0E           0x3a0e
#define OV5640_REG_AEC_CTRL0F           0x3a0f
#define OV5640_REG_AEC_CTRL10           0x3a10
#define OV5640_REG_AEC_CTRL11           0x3a11
#define OV5640_REG_AEC_CTRL1B           0x3a1b
#define OV5640_REG_AEC_CTRL1E           0x3a1e
#define OV5640_REG_AEC_CTRL1F           0x3a1f
#define OV5640_REG_HZ5060_CTRL00        0x3c00
#define OV5640_REG_HZ5060_CTRL01        0x3c01
#define OV5640_REG_SIGMADELTA_CTRL0C    0x3c0c
#define OV5640_REG_FRAME_CTRL01         0x4202
#define OV5640_REG_FORMAT_CONTROL00     0x4300
#define OV5640_REG_VFIFO_HSIZE          0x4602
#define OV5640_REG_VFIFO_VSIZE          0x4604
#define OV5640_REG_JPG_MODE_SELECT      0x4713
#define OV5640_REG_CCIR656_CTRL00       0x4730
#define OV5640_REG_POLARITY_CTRL00      0x4740
#define OV5640_REG_MIPI_CTRL00          0x4800
#define OV5640_REG_DEBUG_MODE           0x4814
#define OV5640_REG_ISP_FORMAT_MUX_CTRL  0x501f
#define OV5640_REG_PRE_ISP_TEST_SET1    0x503d
#define OV5640_REG_SDE_CTRL0            0x5580
#define OV5640_REG_SDE_CTRL1            0x5581
#define OV5640_REG_SDE_CTRL3            0x5583
#define OV5640_REG_SDE_CTRL4            0x5584
#define OV5640_REG_SDE_CTRL5            0x5585
#define OV5640_REG_AVG_READOUT          0x56a1

enum ov5640_mode_id {
    OV5640_MODE_QCIF_176_144 = 0,
    OV5640_MODE_QVGA_320_240,
    OV5640_MODE_VGA_640_480,
    OV5640_MODE_NTSC_720_480,
    OV5640_MODE_PAL_720_576,
    OV5640_MODE_XGA_1024_768,
    OV5640_MODE_720P_1280_720,
    OV5640_MODE_1080P_1920_1080,
    OV5640_MODE_QSXGA_2592_1944,
    OV5640_NUM_MODES,
};

enum ov5640_frame_rate {
    OV5640_15_FPS = 0,
    OV5640_24_FPS,
    OV5640_30_FPS,
    OV5640_60_FPS,
    OV5640_NUM_FRAMERATES,
};

enum ov5640_format_mux {
    OV5640_FMT_MUX_YUV422 = 0,
    OV5640_FMT_MUX_RGB,
    OV5640_FMT_MUX_DITHER,
    OV5640_FMT_MUX_RAW_DPC,
    OV5640_FMT_MUX_SNR_RAW,
    OV5640_FMT_MUX_RAW_CIP,
};

struct ov5640_pixfmt {
    u32 code;
    u32 colorspace;
};

#if 0 // Unused now
static const struct ov5640_pixfmt ov5640_formats[] = {
    { MEDIA_BUS_FMT_JPEG_1X8,       MEDIA_COLORSPACE_JPEG, },
    { MEDIA_BUS_FMT_UYVY8_2X8,      MEDIA_COLORSPACE_SRGB, },
    { MEDIA_BUS_FMT_YUYV8_2X8,      MEDIA_COLORSPACE_SRGB, },
    { MEDIA_BUS_FMT_RGB565_2X8_LE,  MEDIA_COLORSPACE_SRGB, },
    { MEDIA_BUS_FMT_RGB565_2X8_BE,  MEDIA_COLORSPACE_SRGB, },
    { MEDIA_BUS_FMT_SBGGR8_1X8,     MEDIA_COLORSPACE_SRGB, },
    { MEDIA_BUS_FMT_SGBRG8_1X8,     MEDIA_COLORSPACE_SRGB, },
    { MEDIA_BUS_FMT_SGRBG8_1X8,     MEDIA_COLORSPACE_SRGB, },
    { MEDIA_BUS_FMT_SRGGB8_1X8,     MEDIA_COLORSPACE_SRGB, },
};
#endif

static const int ov5640_framerates[] = {
    [OV5640_15_FPS] = 15,
    [OV5640_24_FPS] = 24,
    [OV5640_30_FPS] = 30,
    [OV5640_60_FPS] = 60,
};

/*
 * Image size under 1280 * 960 are SUBSAMPLING
 * Image size upper 1280 * 960 are SCALING
 */
enum ov5640_downsize_mode {
    SUBSAMPLING,
    SCALING,
};

struct reg_value {
    u16 reg_addr;
    u8 val;
    u8 mask;
    u32 delay_ms;
};

struct ov5640_mode_info {
    enum ov5640_mode_id id;
    enum ov5640_downsize_mode dn_mode;
    u32 hact;
    u32 htot;
    u32 vact;
    u32 vtot;
    const struct reg_value *reg_data;
    u32 reg_data_size;
    u32 max_fps;
};

struct ov5640_dev {
    struct rt_device dev;
    struct rt_i2c_bus_device *iic;
    u32 xclk_freq;
    u32 rst_pin;
    u32 pwdn_pin;

    struct mpp_video_fmt fmt;
    bool upside_down;

    const struct ov5640_mode_info *current_mode;
    const struct ov5640_mode_info *last_mode;
    enum ov5640_frame_rate current_fr;

    u32 prev_sysclk, prev_hts;
    u32 ae_low, ae_high, ae_target;

    bool pending_mode_change;
    bool streaming;
};

static struct ov5640_dev g_ov5640_dev = {0};

/*
 * FIXME: all of these register tables are likely filled with
 * entries that set the register to their power-on default values,
 * and which are otherwise not touched by this driver. Those entries
 * should be identified and removed to speed register load time
 * over i2c.
 */
/* YUV422 UYVY VGA@30fps */
static const struct reg_value ov5640_init_setting_30fps_VGA[] = {
    {0x3103, 0x11, 0, 0}, {0x3008, 0x82, 0, 5}, {0x3008, 0x42, 0, 0},
    {0x3103, 0x03, 0, 0}, {0x3630, 0x36, 0, 0},
    {0x3631, 0x0e, 0, 0}, {0x3632, 0xe2, 0, 0}, {0x3633, 0x12, 0, 0},
    {0x3621, 0xe0, 0, 0}, {0x3704, 0xa0, 0, 0}, {0x3703, 0x5a, 0, 0},
    {0x3715, 0x78, 0, 0}, {0x3717, 0x01, 0, 0}, {0x370b, 0x60, 0, 0},
    {0x3705, 0x1a, 0, 0}, {0x3905, 0x02, 0, 0}, {0x3906, 0x10, 0, 0},
    {0x3901, 0x0a, 0, 0}, {0x3731, 0x12, 0, 0}, {0x3600, 0x08, 0, 0},
    {0x3601, 0x33, 0, 0}, {0x302d, 0x60, 0, 0}, {0x3620, 0x52, 0, 0},
    {0x371b, 0x20, 0, 0}, {0x471c, 0x50, 0, 0}, {0x3a13, 0x43, 0, 0},
    {0x3a18, 0x00, 0, 0}, {0x3a19, 0xf8, 0, 0}, {0x3635, 0x13, 0, 0},
    {0x3636, 0x03, 0, 0}, {0x3634, 0x40, 0, 0}, {0x3622, 0x01, 0, 0},
    {0x3c01, 0xa4, 0, 0}, {0x3c04, 0x28, 0, 0}, {0x3c05, 0x98, 0, 0},
    {0x3c06, 0x00, 0, 0}, {0x3c07, 0x08, 0, 0}, {0x3c08, 0x00, 0, 0},
    {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
    {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0}, {0x3814, 0x31, 0, 0},
    {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
    {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0}, {0x3804, 0x0a, 0, 0},
    {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0}, {0x3807, 0x9b, 0, 0},
    {0x3810, 0x00, 0, 0},
    {0x3811, 0x10, 0, 0}, {0x3812, 0x00, 0, 0}, {0x3813, 0x06, 0, 0},
    {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3708, 0x64, 0, 0},
    {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x03, 0, 0},
    {0x3a03, 0xd8, 0, 0}, {0x3a08, 0x01, 0, 0}, {0x3a09, 0x27, 0, 0},
    {0x3a0a, 0x00, 0, 0}, {0x3a0b, 0xf6, 0, 0}, {0x3a0e, 0x03, 0, 0},
    {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x03, 0, 0}, {0x3a15, 0xd8, 0, 0},
    {0x4001, 0x02, 0, 0}, {0x4004, 0x02, 0, 0}, {0x3000, 0x00, 0, 0},
    {0x3002, 0x1c, 0, 0}, {0x3004, 0xff, 0, 0}, {0x3006, 0xc3, 0, 0},
    {0x302e, 0x08, 0, 0}, {0x4300, 0x3f, 0, 0},
    {0x501f, 0x00, 0, 0}, {0x4407, 0x04, 0, 0},
    {0x440e, 0x00, 0, 0}, {0x460b, 0x35, 0, 0}, {0x460c, 0x22, 0, 0},
    {0x4837, 0x0a, 0, 0}, {0x3824, 0x02, 0, 0},
    {0x5000, 0xa7, 0, 0}, {0x5001, 0xa3, 0, 0}, {0x5180, 0xff, 0, 0},
    {0x5181, 0xf2, 0, 0}, {0x5182, 0x00, 0, 0}, {0x5183, 0x14, 0, 0},
    {0x5184, 0x25, 0, 0}, {0x5185, 0x24, 0, 0}, {0x5186, 0x09, 0, 0},
    {0x5187, 0x09, 0, 0}, {0x5188, 0x09, 0, 0}, {0x5189, 0x88, 0, 0},
    {0x518a, 0x54, 0, 0}, {0x518b, 0xee, 0, 0}, {0x518c, 0xb2, 0, 0},
    {0x518d, 0x50, 0, 0}, {0x518e, 0x34, 0, 0}, {0x518f, 0x6b, 0, 0},
    {0x5190, 0x46, 0, 0}, {0x5191, 0xf8, 0, 0}, {0x5192, 0x04, 0, 0},
    {0x5193, 0x70, 0, 0}, {0x5194, 0xf0, 0, 0}, {0x5195, 0xf0, 0, 0},
    {0x5196, 0x03, 0, 0}, {0x5197, 0x01, 0, 0}, {0x5198, 0x04, 0, 0},
    {0x5199, 0x6c, 0, 0}, {0x519a, 0x04, 0, 0}, {0x519b, 0x00, 0, 0},
    {0x519c, 0x09, 0, 0}, {0x519d, 0x2b, 0, 0}, {0x519e, 0x38, 0, 0},
    {0x5381, 0x1e, 0, 0}, {0x5382, 0x5b, 0, 0}, {0x5383, 0x08, 0, 0},
    {0x5384, 0x0a, 0, 0}, {0x5385, 0x7e, 0, 0}, {0x5386, 0x88, 0, 0},
    {0x5387, 0x7c, 0, 0}, {0x5388, 0x6c, 0, 0}, {0x5389, 0x10, 0, 0},
    {0x538a, 0x01, 0, 0}, {0x538b, 0x98, 0, 0}, {0x5300, 0x08, 0, 0},
    {0x5301, 0x30, 0, 0}, {0x5302, 0x10, 0, 0}, {0x5303, 0x00, 0, 0},
    {0x5304, 0x08, 0, 0}, {0x5305, 0x30, 0, 0}, {0x5306, 0x08, 0, 0},
    {0x5307, 0x16, 0, 0}, {0x5309, 0x08, 0, 0}, {0x530a, 0x30, 0, 0},
    {0x530b, 0x04, 0, 0}, {0x530c, 0x06, 0, 0}, {0x5480, 0x01, 0, 0},
    {0x5481, 0x08, 0, 0}, {0x5482, 0x14, 0, 0}, {0x5483, 0x28, 0, 0},
    {0x5484, 0x51, 0, 0}, {0x5485, 0x65, 0, 0}, {0x5486, 0x71, 0, 0},
    {0x5487, 0x7d, 0, 0}, {0x5488, 0x87, 0, 0}, {0x5489, 0x91, 0, 0},
    {0x548a, 0x9a, 0, 0}, {0x548b, 0xaa, 0, 0}, {0x548c, 0xb8, 0, 0},
    {0x548d, 0xcd, 0, 0}, {0x548e, 0xdd, 0, 0}, {0x548f, 0xea, 0, 0},
    {0x5490, 0x1d, 0, 0}, {0x5580, 0x02, 0, 0}, {0x5583, 0x40, 0, 0},
    {0x5584, 0x10, 0, 0}, {0x5589, 0x10, 0, 0}, {0x558a, 0x00, 0, 0},
    {0x558b, 0xf8, 0, 0}, {0x5800, 0x23, 0, 0}, {0x5801, 0x14, 0, 0},
    {0x5802, 0x0f, 0, 0}, {0x5803, 0x0f, 0, 0}, {0x5804, 0x12, 0, 0},
    {0x5805, 0x26, 0, 0}, {0x5806, 0x0c, 0, 0}, {0x5807, 0x08, 0, 0},
    {0x5808, 0x05, 0, 0}, {0x5809, 0x05, 0, 0}, {0x580a, 0x08, 0, 0},
    {0x580b, 0x0d, 0, 0}, {0x580c, 0x08, 0, 0}, {0x580d, 0x03, 0, 0},
    {0x580e, 0x00, 0, 0}, {0x580f, 0x00, 0, 0}, {0x5810, 0x03, 0, 0},
    {0x5811, 0x09, 0, 0}, {0x5812, 0x07, 0, 0}, {0x5813, 0x03, 0, 0},
    {0x5814, 0x00, 0, 0}, {0x5815, 0x01, 0, 0}, {0x5816, 0x03, 0, 0},
    {0x5817, 0x08, 0, 0}, {0x5818, 0x0d, 0, 0}, {0x5819, 0x08, 0, 0},
    {0x581a, 0x05, 0, 0}, {0x581b, 0x06, 0, 0}, {0x581c, 0x08, 0, 0},
    {0x581d, 0x0e, 0, 0}, {0x581e, 0x29, 0, 0}, {0x581f, 0x17, 0, 0},
    {0x5820, 0x11, 0, 0}, {0x5821, 0x11, 0, 0}, {0x5822, 0x15, 0, 0},
    {0x5823, 0x28, 0, 0}, {0x5824, 0x46, 0, 0}, {0x5825, 0x26, 0, 0},
    {0x5826, 0x08, 0, 0}, {0x5827, 0x26, 0, 0}, {0x5828, 0x64, 0, 0},
    {0x5829, 0x26, 0, 0}, {0x582a, 0x24, 0, 0}, {0x582b, 0x22, 0, 0},
    {0x582c, 0x24, 0, 0}, {0x582d, 0x24, 0, 0}, {0x582e, 0x06, 0, 0},
    {0x582f, 0x22, 0, 0}, {0x5830, 0x40, 0, 0}, {0x5831, 0x42, 0, 0},
    {0x5832, 0x24, 0, 0}, {0x5833, 0x26, 0, 0}, {0x5834, 0x24, 0, 0},
    {0x5835, 0x22, 0, 0}, {0x5836, 0x22, 0, 0}, {0x5837, 0x26, 0, 0},
    {0x5838, 0x44, 0, 0}, {0x5839, 0x24, 0, 0}, {0x583a, 0x26, 0, 0},
    {0x583b, 0x28, 0, 0}, {0x583c, 0x42, 0, 0}, {0x583d, 0xce, 0, 0},
    {0x5025, 0x00, 0, 0}, {0x3a0f, 0x30, 0, 0}, {0x3a10, 0x28, 0, 0},
    {0x3a1b, 0x30, 0, 0}, {0x3a1e, 0x26, 0, 0}, {0x3a11, 0x60, 0, 0},
    {0x3a1f, 0x14, 0, 0}, {0x3008, 0x02, 0, 0}, {0x3c00, 0x04, 0, 300},
};

static const struct reg_value ov5640_setting_VGA_640_480[] = {
    {0x3c07, 0x08, 0, 0},
    {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
    {0x3814, 0x31, 0, 0},
    {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
    {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0}, {0x3804, 0x0a, 0, 0},
    {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0}, {0x3807, 0x9b, 0, 0},
    {0x3810, 0x00, 0, 0},
    {0x3811, 0x10, 0, 0}, {0x3812, 0x00, 0, 0}, {0x3813, 0x06, 0, 0},
    {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3708, 0x64, 0, 0},
    {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x03, 0, 0},
    {0x3a03, 0xd8, 0, 0}, {0x3a08, 0x01, 0, 0}, {0x3a09, 0x27, 0, 0},
    {0x3a0a, 0x00, 0, 0}, {0x3a0b, 0xf6, 0, 0}, {0x3a0e, 0x03, 0, 0},
    {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x03, 0, 0}, {0x3a15, 0xd8, 0, 0},
    {0x4001, 0x02, 0, 0}, {0x4004, 0x02, 0, 0},
    {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0}, {0x460c, 0x22, 0, 0},
    {0x3824, 0x02, 0, 0}, {0x5001, 0xa3, 0, 0},
};

static const struct reg_value ov5640_setting_XGA_1024_768[] = {
    {0x3c07, 0x08, 0, 0},
    {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
    {0x3814, 0x31, 0, 0},
    {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
    {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0}, {0x3804, 0x0a, 0, 0},
    {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0}, {0x3807, 0x9b, 0, 0},
    {0x3810, 0x00, 0, 0},
    {0x3811, 0x10, 0, 0}, {0x3812, 0x00, 0, 0}, {0x3813, 0x06, 0, 0},
    {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3708, 0x64, 0, 0},
    {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x03, 0, 0},
    {0x3a03, 0xd8, 0, 0}, {0x3a08, 0x01, 0, 0}, {0x3a09, 0x27, 0, 0},
    {0x3a0a, 0x00, 0, 0}, {0x3a0b, 0xf6, 0, 0}, {0x3a0e, 0x03, 0, 0},
    {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x03, 0, 0}, {0x3a15, 0xd8, 0, 0},
    {0x4001, 0x02, 0, 0}, {0x4004, 0x02, 0, 0},
    {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0}, {0x460c, 0x22, 0, 0},
    {0x3824, 0x02, 0, 0}, {0x5001, 0xa3, 0, 0},
};

static const struct reg_value ov5640_setting_QVGA_320_240[] = {
    {0x3c07, 0x08, 0, 0},
    {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
    {0x3814, 0x31, 0, 0},
    {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
    {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0}, {0x3804, 0x0a, 0, 0},
    {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0}, {0x3807, 0x9b, 0, 0},
    {0x3810, 0x00, 0, 0},
    {0x3811, 0x10, 0, 0}, {0x3812, 0x00, 0, 0}, {0x3813, 0x06, 0, 0},
    {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3708, 0x64, 0, 0},
    {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x03, 0, 0},
    {0x3a03, 0xd8, 0, 0}, {0x3a08, 0x01, 0, 0}, {0x3a09, 0x27, 0, 0},
    {0x3a0a, 0x00, 0, 0}, {0x3a0b, 0xf6, 0, 0}, {0x3a0e, 0x03, 0, 0},
    {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x03, 0, 0}, {0x3a15, 0xd8, 0, 0},
    {0x4001, 0x02, 0, 0}, {0x4004, 0x02, 0, 0},
    {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0}, {0x460c, 0x22, 0, 0},
    {0x3824, 0x02, 0, 0}, {0x5001, 0xa3, 0, 0},
};

static const struct reg_value ov5640_setting_QCIF_176_144[] = {
    {0x3c07, 0x08, 0, 0},
    {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
    {0x3814, 0x31, 0, 0},
    {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
    {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0}, {0x3804, 0x0a, 0, 0},
    {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0}, {0x3807, 0x9b, 0, 0},
    {0x3810, 0x00, 0, 0},
    {0x3811, 0x10, 0, 0}, {0x3812, 0x00, 0, 0}, {0x3813, 0x06, 0, 0},
    {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3708, 0x64, 0, 0},
    {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x03, 0, 0},
    {0x3a03, 0xd8, 0, 0}, {0x3a08, 0x01, 0, 0}, {0x3a09, 0x27, 0, 0},
    {0x3a0a, 0x00, 0, 0}, {0x3a0b, 0xf6, 0, 0}, {0x3a0e, 0x03, 0, 0},
    {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x03, 0, 0}, {0x3a15, 0xd8, 0, 0},
    {0x4001, 0x02, 0, 0}, {0x4004, 0x02, 0, 0},
    {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0}, {0x460c, 0x22, 0, 0},
    {0x3824, 0x02, 0, 0}, {0x5001, 0xa3, 0, 0},
};

static const struct reg_value ov5640_setting_NTSC_720_480[] = {
    {0x3c07, 0x08, 0, 0},
    {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
    {0x3814, 0x31, 0, 0},
    {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
    {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0}, {0x3804, 0x0a, 0, 0},
    {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0}, {0x3807, 0x9b, 0, 0},
    {0x3810, 0x00, 0, 0},
    {0x3811, 0x10, 0, 0}, {0x3812, 0x00, 0, 0}, {0x3813, 0x3c, 0, 0},
    {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3708, 0x64, 0, 0},
    {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x03, 0, 0},
    {0x3a03, 0xd8, 0, 0}, {0x3a08, 0x01, 0, 0}, {0x3a09, 0x27, 0, 0},
    {0x3a0a, 0x00, 0, 0}, {0x3a0b, 0xf6, 0, 0}, {0x3a0e, 0x03, 0, 0},
    {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x03, 0, 0}, {0x3a15, 0xd8, 0, 0},
    {0x4001, 0x02, 0, 0}, {0x4004, 0x02, 0, 0},
    {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0}, {0x460c, 0x22, 0, 0},
    {0x3824, 0x02, 0, 0}, {0x5001, 0xa3, 0, 0},
};

static const struct reg_value ov5640_setting_PAL_720_576[] = {
    {0x3c07, 0x08, 0, 0},
    {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
    {0x3814, 0x31, 0, 0},
    {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
    {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0}, {0x3804, 0x0a, 0, 0},
    {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0}, {0x3807, 0x9b, 0, 0},
    {0x3810, 0x00, 0, 0},
    {0x3811, 0x38, 0, 0}, {0x3812, 0x00, 0, 0}, {0x3813, 0x06, 0, 0},
    {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3708, 0x64, 0, 0},
    {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x03, 0, 0},
    {0x3a03, 0xd8, 0, 0}, {0x3a08, 0x01, 0, 0}, {0x3a09, 0x27, 0, 0},
    {0x3a0a, 0x00, 0, 0}, {0x3a0b, 0xf6, 0, 0}, {0x3a0e, 0x03, 0, 0},
    {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x03, 0, 0}, {0x3a15, 0xd8, 0, 0},
    {0x4001, 0x02, 0, 0}, {0x4004, 0x02, 0, 0},
    {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0}, {0x460c, 0x22, 0, 0},
    {0x3824, 0x02, 0, 0}, {0x5001, 0xa3, 0, 0},
};

static const struct reg_value ov5640_setting_720P_1280_720[] = {
    {0x3c07, 0x07, 0, 0},
    {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
    {0x3814, 0x31, 0, 0},
    {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
    {0x3802, 0x00, 0, 0}, {0x3803, 0xfa, 0, 0}, {0x3804, 0x0a, 0, 0},
    {0x3805, 0x3f, 0, 0}, {0x3806, 0x06, 0, 0}, {0x3807, 0xa9, 0, 0},
    {0x3810, 0x00, 0, 0},
    {0x3811, 0x10, 0, 0}, {0x3812, 0x00, 0, 0}, {0x3813, 0x04, 0, 0},
    {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3708, 0x64, 0, 0},
    {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x02, 0, 0},
    {0x3a03, 0xe4, 0, 0}, {0x3a08, 0x01, 0, 0}, {0x3a09, 0xbc, 0, 0},
    {0x3a0a, 0x01, 0, 0}, {0x3a0b, 0x72, 0, 0}, {0x3a0e, 0x01, 0, 0},
    {0x3a0d, 0x02, 0, 0}, {0x3a14, 0x02, 0, 0}, {0x3a15, 0xe4, 0, 0},
    {0x4001, 0x02, 0, 0}, {0x4004, 0x02, 0, 0},
    {0x4407, 0x04, 0, 0}, {0x460b, 0x37, 0, 0}, {0x460c, 0x20, 0, 0},
    {0x3824, 0x04, 0, 0}, {0x5001, 0x83, 0, 0},
};

static const struct reg_value ov5640_setting_1080P_1920_1080[] = {
    {0x3c07, 0x08, 0, 0},
    {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
    {0x3814, 0x11, 0, 0},
    {0x3815, 0x11, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
    {0x3802, 0x00, 0, 0}, {0x3803, 0x00, 0, 0}, {0x3804, 0x0a, 0, 0},
    {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0}, {0x3807, 0x9f, 0, 0},
    {0x3810, 0x00, 0, 0},
    {0x3811, 0x10, 0, 0}, {0x3812, 0x00, 0, 0}, {0x3813, 0x04, 0, 0},
    {0x3618, 0x04, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3708, 0x21, 0, 0},
    {0x3709, 0x12, 0, 0}, {0x370c, 0x00, 0, 0}, {0x3a02, 0x03, 0, 0},
    {0x3a03, 0xd8, 0, 0}, {0x3a08, 0x01, 0, 0}, {0x3a09, 0x27, 0, 0},
    {0x3a0a, 0x00, 0, 0}, {0x3a0b, 0xf6, 0, 0}, {0x3a0e, 0x03, 0, 0},
    {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x03, 0, 0}, {0x3a15, 0xd8, 0, 0},
    {0x4001, 0x02, 0, 0}, {0x4004, 0x06, 0, 0},
    {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0}, {0x460c, 0x22, 0, 0},
    {0x3824, 0x02, 0, 0}, {0x5001, 0x83, 0, 0},
    {0x3c07, 0x07, 0, 0}, {0x3c08, 0x00, 0, 0},
    {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
    {0x3800, 0x01, 0, 0}, {0x3801, 0x50, 0, 0}, {0x3802, 0x01, 0, 0},
    {0x3803, 0xb2, 0, 0}, {0x3804, 0x08, 0, 0}, {0x3805, 0xef, 0, 0},
    {0x3806, 0x05, 0, 0}, {0x3807, 0xf1, 0, 0},
    {0x3612, 0x2b, 0, 0}, {0x3708, 0x64, 0, 0},
    {0x3a02, 0x04, 0, 0}, {0x3a03, 0x60, 0, 0}, {0x3a08, 0x01, 0, 0},
    {0x3a09, 0x50, 0, 0}, {0x3a0a, 0x01, 0, 0}, {0x3a0b, 0x18, 0, 0},
    {0x3a0e, 0x03, 0, 0}, {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x04, 0, 0},
    {0x3a15, 0x60, 0, 0}, {0x4407, 0x04, 0, 0},
    {0x460b, 0x37, 0, 0}, {0x460c, 0x20, 0, 0}, {0x3824, 0x04, 0, 0},
    {0x4005, 0x1a, 0, 0},
};

static const struct reg_value ov5640_setting_QSXGA_2592_1944[] = {
    {0x3c07, 0x08, 0, 0},
    {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
    {0x3814, 0x11, 0, 0},
    {0x3815, 0x11, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
    {0x3802, 0x00, 0, 0}, {0x3803, 0x00, 0, 0}, {0x3804, 0x0a, 0, 0},
    {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0}, {0x3807, 0x9f, 0, 0},
    {0x3810, 0x00, 0, 0},
    {0x3811, 0x10, 0, 0}, {0x3812, 0x00, 0, 0}, {0x3813, 0x04, 0, 0},
    {0x3618, 0x04, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3708, 0x21, 0, 0},
    {0x3709, 0x12, 0, 0}, {0x370c, 0x00, 0, 0}, {0x3a02, 0x03, 0, 0},
    {0x3a03, 0xd8, 0, 0}, {0x3a08, 0x01, 0, 0}, {0x3a09, 0x27, 0, 0},
    {0x3a0a, 0x00, 0, 0}, {0x3a0b, 0xf6, 0, 0}, {0x3a0e, 0x03, 0, 0},
    {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x03, 0, 0}, {0x3a15, 0xd8, 0, 0},
    {0x4001, 0x02, 0, 0}, {0x4004, 0x06, 0, 0},
    {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0}, {0x460c, 0x22, 0, 0},
    {0x3824, 0x02, 0, 0}, {0x5001, 0x83, 0, 70},
};

/* power-on sensor init reg table */
static const struct ov5640_mode_info ov5640_mode_init_data = {
    0, SUBSAMPLING, 640, 1896, 480, 984,
    ov5640_init_setting_30fps_VGA,
    ARRAY_SIZE(ov5640_init_setting_30fps_VGA),
    OV5640_30_FPS,
};

static const struct ov5640_mode_info
    ov5640_mode_data[OV5640_NUM_MODES] = {
    {   OV5640_MODE_QCIF_176_144, SUBSAMPLING,
        176, 1896, 144, 984, ov5640_setting_QCIF_176_144,
        ARRAY_SIZE(ov5640_setting_QCIF_176_144), OV5640_30_FPS
    },
    {   OV5640_MODE_QVGA_320_240, SUBSAMPLING,
        320, 1896, 240, 984, ov5640_setting_QVGA_320_240,
        ARRAY_SIZE(ov5640_setting_QVGA_320_240), OV5640_30_FPS
    },
    {   OV5640_MODE_VGA_640_480, SUBSAMPLING,
        640, 1896, 480, 1080, ov5640_setting_VGA_640_480,
        ARRAY_SIZE(ov5640_setting_VGA_640_480), OV5640_60_FPS
    },
    {   OV5640_MODE_NTSC_720_480, SUBSAMPLING,
        720, 1896, 480, 984, ov5640_setting_NTSC_720_480,
        ARRAY_SIZE(ov5640_setting_NTSC_720_480), OV5640_30_FPS
    },
    {   OV5640_MODE_PAL_720_576, SUBSAMPLING,
        720, 1896, 576, 984, ov5640_setting_PAL_720_576,
        ARRAY_SIZE(ov5640_setting_PAL_720_576), OV5640_30_FPS
    },
    {   OV5640_MODE_XGA_1024_768, SUBSAMPLING,
        1024, 1896, 768, 1080, ov5640_setting_XGA_1024_768,
        ARRAY_SIZE(ov5640_setting_XGA_1024_768), OV5640_30_FPS
    },
    {   OV5640_MODE_720P_1280_720, SUBSAMPLING,
        1280, 1892, 720, 740, ov5640_setting_720P_1280_720,
        ARRAY_SIZE(ov5640_setting_720P_1280_720), OV5640_30_FPS
    },
    {   OV5640_MODE_1080P_1920_1080, SCALING,
        1920, 2500, 1080, 1120, ov5640_setting_1080P_1920_1080,
        ARRAY_SIZE(ov5640_setting_1080P_1920_1080), OV5640_30_FPS
    },
    {   OV5640_MODE_QSXGA_2592_1944, SCALING,
        2592, 2844, 1944, 1968, ov5640_setting_QSXGA_2592_1944,
        ARRAY_SIZE(ov5640_setting_QSXGA_2592_1944), OV5640_15_FPS
    },
};

static u32 ov5640_ilog2(u32 x)
{
    int r = 0;

    if (x >= (u32)65536) {
        x >>= 16;
        r += 16;
    }
    if (x >= 256) {
        x >>= 8;
        r += 8;
    }
    if (x >= 16) {
        x >>= 4;
        r += 4;
    }
    if (x >= 4) {
        x >>= 2;
        r += 2;
    }
    if (x >= 2) {
        r += 1;
    }
    return r;
}

static int ov5640_write_reg(struct ov5640_dev *sensor, u16 reg, u8 val)
{
    struct rt_i2c_bus_device *client = sensor->iic;
    u8 buf[3];
    struct rt_i2c_msg msgs;

    buf[0] = reg >> 8;
    buf[1] = reg & 0xff;
    buf[2] = val;

    msgs.addr = OV5640_DEFAULT_SLAVE_ID;
    msgs.flags = RT_I2C_WR;
    msgs.buf = buf;
    msgs.len = 3;

    if (rt_i2c_transfer(client, &msgs, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }

    // printf("TWI_write_byte_16add(0x%02x,0x%02x,0x%02x,0x%02x);\n",
    //        OV5640_DEFAULT_SLAVE_ID, buf[0], buf[1], val);
    return 0;
}

static int ov5640_read_reg(struct ov5640_dev *sensor, u16 reg, u8 *val)
{
    struct rt_i2c_bus_device *client = sensor->iic;
    struct rt_i2c_msg msg[2];
    u8 buf[2] = {0};

    buf[0] = reg >> 8;
    buf[1] = reg & 0xff;

    msg[0].addr  = OV5640_DEFAULT_SLAVE_ID;
    msg[0].flags = RT_I2C_WR;
    msg[0].buf   = buf;
    msg[0].len   = 2;

    msg[1].addr  = OV5640_DEFAULT_SLAVE_ID;
    msg[1].flags = RT_I2C_RD;
    msg[1].buf   = val;
    msg[1].len   = 1;

    if (rt_i2c_transfer(client, msg, 2) != 2) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, *val);
        return -1;
    }

    // printf("%s(0x%02x,0x%02x,0x%02x,0x%02x);\n", __func__,
    //        OV5640_DEFAULT_SLAVE_ID, buf[0], buf[1], *val);
    return 0;

}

static int ov5640_read_reg16(struct ov5640_dev *sensor, u16 reg, u16 *val)
{
    u8 hi = 0, lo = 0;
    int ret;

    ret = ov5640_read_reg(sensor, reg, &hi);
    if (ret)
        return ret;
    ret = ov5640_read_reg(sensor, reg + 1, &lo);
    if (ret)
        return ret;

    *val = ((u16)hi << 8) | (u16)lo;
    return 0;
}

static int ov5640_write_reg16(struct ov5640_dev *sensor, u16 reg, u16 val)
{
    int ret;

    ret = ov5640_write_reg(sensor, reg, val >> 8);
    if (ret)
        return ret;

    return ov5640_write_reg(sensor, reg + 1, val & 0xff);
}

static int ov5640_mod_reg(struct ov5640_dev *sensor, u16 reg, u8 mask, u8 val)
{
    u8 readval;
    int ret;

    ret = ov5640_read_reg(sensor, reg, &readval);
    if (ret)
        return ret;

    readval &= ~mask;
    val &= mask;
    val |= readval;
    return ov5640_write_reg(sensor, reg, val);
}
/*
 * After trying the various combinations, reading various
 * documentations spread around the net, and from the various
 * feedback, the clock tree is probably as follows:
 *
 *   +--------------+
 *   |  Ext. Clock  |
 *   +-+------------+
 *     |  +----------+
 *     +->|   PLL1   | - reg 0x3036, for the multiplier
 *        +-+--------+ - reg 0x3037, bits 0-3 for the pre-divider
 *          |  +--------------+
 *          +->| System Clock |  - reg 0x3035, bits 4-7
 *             +-+------------+
 *               |  +--------------+
 *               +->| MIPI Divider | - reg 0x3035, bits 0-3
 *               |  +-+------------+
 *               |    +----------------> MIPI SCLK
 *               |    +  +-----+
 *               |    +->| / 2 |-------> MIPI BIT CLK
 *               |       +-----+
 *               |  +--------------+
 *               +->| PLL Root Div | - reg 0x3037, bit 4
 *                  +-+------------+
 *                    |  +---------+
 *                    +->| Bit Div | - reg 0x3034, bits 0-3
 *                       +-+-------+
 *                         |  +-------------+
 *                         +->| SCLK Div    | - reg 0x3108, bits 0-1
 *                         |  +-+-----------+
 *                         |    +---------------> SCLK
 *                         |  +-------------+
 *                         +->| SCLK 2X Div | - reg 0x3108, bits 2-3
 *                         |  +-+-----------+
 *                         |    +---------------> SCLK 2X
 *                         |  +-------------+
 *                         +->| PCLK Div    | - reg 0x3108, bits 4-5
 *                            ++------------+
 *                             +  +-----------+
 *                             +->|   P_DIV   | - reg 0x3035, bits 0-3
 *                                +-----+-----+
 *                                       +------------> PCLK
 *
 * This is deviating from the datasheet at least for the register
 * 0x3108, since it's said here that the PCLK would be clocked from
 * the PLL.
 *
 * There seems to be also (unverified) constraints:
 *  - the PLL pre-divider output rate should be in the 4-27MHz range
 *  - the PLL multiplier output rate should be in the 500-1000MHz range
 *  - PCLK >= SCLK * 2 in YUV, >= SCLK in Raw or JPEG
 *
 * In the two latter cases, these constraints are met since our
 * factors are hardcoded. If we were to change that, we would need to
 * take this into account. The only varying parts are the PLL
 * multiplier and the system clock divider, which are shared between
 * all these clocks so won't cause any issue.
 */

/*
 * This is supposed to be ranging from 1 to 8, but the value is always
 * set to 3 in the vendor kernels.
 */
#define OV5640_PLL_PREDIV   3

#define OV5640_PLL_MULT_MIN 4
#define OV5640_PLL_MULT_MAX 252

/*
 * This is supposed to be ranging from 1 to 16, but the value is
 * always set to either 1 or 2 in the vendor kernels.
 */
#define OV5640_SYSDIV_MIN   1
#define OV5640_SYSDIV_MAX   16

/*
 * Hardcode these values for scaler and non-scaler modes.
 * FIXME: to be re-calcualted for 1 data lanes setups
 */
#define OV5640_MIPI_DIV_PCLK    2
#define OV5640_MIPI_DIV_SCLK    1

/*
 * This is supposed to be ranging from 1 to 2, but the value is always
 * set to 2 in the vendor kernels.
 */
#define OV5640_PLL_ROOT_DIV         2
#define OV5640_PLL_CTRL3_PLL_ROOT_DIV_2     BIT(4)

/*
 * We only supports 8-bit formats at the moment
 */
#define OV5640_BIT_DIV              2
#define OV5640_PLL_CTRL0_MIPI_MODE_8BIT     0x08

/*
 * This is supposed to be ranging from 1 to 8, but the value is always
 * set to 2 in the vendor kernels.
 */
#define OV5640_SCLK_ROOT_DIV    2

/*
 * This is hardcoded so that the consistency is maintained between SCLK and
 * SCLK 2x.
 */
#define OV5640_SCLK2X_ROOT_DIV (OV5640_SCLK_ROOT_DIV / 2)

/*
 * This is supposed to be ranging from 1 to 8, but the value is always
 * set to 1 in the vendor kernels.
 */
#define OV5640_PCLK_ROOT_DIV            1
#define OV5640_PLL_SYS_ROOT_DIVIDER_BYPASS  0x00

static unsigned long ov5640_compute_sys_clk(struct ov5640_dev *sensor,
        u8 pll_prediv, u8 pll_mult,
        u8 sysdiv)
{
    unsigned long sysclk = sensor->xclk_freq / pll_prediv * pll_mult;

    /* PLL1 output cannot exceed 1GHz. */
    if (sysclk / 1000000 > 1000)
        return 0;

    return sysclk / sysdiv;
}

static unsigned long ov5640_calc_sys_clk(struct ov5640_dev *sensor,
        unsigned long rate,
        u8 *pll_prediv, u8 *pll_mult,
        u8 *sysdiv)
{
    unsigned long best = ~0;
    u8 best_sysdiv = 1, best_mult = 1;
    u8 _sysdiv, _pll_mult;

    for (_sysdiv = OV5640_SYSDIV_MIN;
            _sysdiv <= OV5640_SYSDIV_MAX;
            _sysdiv++) {
        for (_pll_mult = OV5640_PLL_MULT_MIN;
                _pll_mult <= OV5640_PLL_MULT_MAX;
                _pll_mult++) {
            unsigned long _rate;

            /*
             * The PLL multiplier cannot be odd if above
             * 127.
             */
            if (_pll_mult > 127 && (_pll_mult % 2))
                continue;

            _rate = ov5640_compute_sys_clk(sensor,
                                           OV5640_PLL_PREDIV,
                                           _pll_mult, _sysdiv);

            /*
             * We have reached the maximum allowed PLL1 output,
             * increase sysdiv.
             */
            if (!_rate)
                break;

            /*
             * Prefer rates above the expected clock rate than
             * below, even if that means being less precise.
             */
            if (_rate < rate)
                continue;

            if (abs(rate - _rate) < abs(rate - best)) {
                best = _rate;
                best_sysdiv = _sysdiv;
                best_mult = _pll_mult;
            }

            if (_rate == rate)
                goto out;
        }
    }

out:
    *sysdiv = best_sysdiv;
    *pll_prediv = OV5640_PLL_PREDIV;
    *pll_mult = best_mult;

    return best;
}

/*
 * ov5640_set_mipi_pclk() - Calculate the clock tree configuration values
 *              for the MIPI CSI-2 output.
 *
 * @rate: The requested bandwidth per lane in bytes per second.
 *    'Bandwidth Per Lane' is calculated as:
 *    bpl = HTOT * VTOT * FPS * bpp / num_lanes;
 *
 * This function use the requested bandwidth to calculate:
 * - sample_rate = bpl / (bpp / num_lanes);
 *           = bpl / (PLL_RDIV * BIT_DIV * PCLK_DIV * MIPI_DIV / num_lanes);
 *
 * - mipi_sclk   = bpl / MIPI_DIV / 2; ( / 2 is for CSI-2 DDR)
 *
 * with these fixed parameters:
 *  PLL_RDIV    = 2;
 *  BIT_DIVIDER = 2; (MIPI_BIT_MODE == 8 ? 2 : 2,5);
 *  PCLK_DIV    = 1;
 *
 * The MIPI clock generation differs for modes that use the scaler and modes
 * that do not. In case the scaler is in use, the MIPI_SCLK generates the MIPI
 * BIT CLk, and thus:
 *
 * - mipi_sclk = bpl / MIPI_DIV / 2;
 *   MIPI_DIV = 1;
 *
 * For modes that do not go through the scaler, the MIPI BIT CLOCK is generated
 * from the pixel clock, and thus:
 *
 * - sample_rate = bpl / (bpp / num_lanes);
 *           = bpl / (2 * 2 * 1 * MIPI_DIV / num_lanes);
 *       = bpl / (4 * MIPI_DIV / num_lanes);
 * - MIPI_DIV    = bpp / (4 * num_lanes);
 *
 * FIXME: this have been tested with 16bpp and 2 lanes setup only.
 * MIPI_DIV is fixed to value 2, but it -might- be changed according to the
 * above formula for setups with 1 lane or image formats with different bpp.
 *
 * FIXME: this deviates from the sensor manual documentation which is quite
 * thin on the MIPI clock tree generation part.
 */
static int ov5640_set_mipi_pclk(struct ov5640_dev *sensor,
                                unsigned long rate)
{
    const struct ov5640_mode_info *mode = sensor->current_mode;
    u8 prediv, mult, sysdiv;
    u8 mipi_div;
    int ret;

    /*
     * 1280x720 is reported to use 'SUBSAMPLING' only,
     * but according to the sensor manual it goes through the
     * scaler before subsampling.
     */
    if (mode->dn_mode == SCALING ||
            (mode->id == OV5640_MODE_720P_1280_720))
        mipi_div = OV5640_MIPI_DIV_SCLK;
    else
        mipi_div = OV5640_MIPI_DIV_PCLK;

    ov5640_calc_sys_clk(sensor, rate, &prediv, &mult, &sysdiv);

    ret = ov5640_mod_reg(sensor, OV5640_REG_SC_PLL_CTRL0,
                         0x0f, OV5640_PLL_CTRL0_MIPI_MODE_8BIT);

    ret = ov5640_mod_reg(sensor, OV5640_REG_SC_PLL_CTRL1,
                         0xff, sysdiv << 4 | mipi_div);
    if (ret)
        return ret;

    ret = ov5640_mod_reg(sensor, OV5640_REG_SC_PLL_CTRL2, 0xff, mult);
    if (ret)
        return ret;

    ret = ov5640_mod_reg(sensor, OV5640_REG_SC_PLL_CTRL3,
                         0x1f, OV5640_PLL_CTRL3_PLL_ROOT_DIV_2 | prediv);
    if (ret)
        return ret;

    return ov5640_mod_reg(sensor, OV5640_REG_SYS_ROOT_DIVIDER,
                          0x30, OV5640_PLL_SYS_ROOT_DIVIDER_BYPASS);
}

static unsigned long ov5640_calc_pclk(struct ov5640_dev *sensor,
                                      unsigned long rate,
                                      u8 *pll_prediv, u8 *pll_mult, u8 *sysdiv,
                                      u8 *pll_rdiv, u8 *bit_div, u8 *pclk_div)
{
    unsigned long _rate = rate * OV5640_PLL_ROOT_DIV * OV5640_BIT_DIV *
                          OV5640_PCLK_ROOT_DIV;

    _rate = ov5640_calc_sys_clk(sensor, _rate, pll_prediv, pll_mult,
                                sysdiv);
    *pll_rdiv = OV5640_PLL_ROOT_DIV;
    *bit_div = OV5640_BIT_DIV;
    *pclk_div = OV5640_PCLK_ROOT_DIV;

    return _rate / *pll_rdiv / *bit_div / *pclk_div;
}

static int ov5640_set_dvp_pclk(struct ov5640_dev *sensor, unsigned long rate)
{
    u8 prediv, mult, sysdiv, pll_rdiv, bit_div, pclk_div;
    int ret;

    ov5640_calc_pclk(sensor, rate, &prediv, &mult, &sysdiv, &pll_rdiv,
                     &bit_div, &pclk_div);

    if (bit_div == 2)
        bit_div = 8;

    ret = ov5640_mod_reg(sensor, OV5640_REG_SC_PLL_CTRL0,
                         0x0f, bit_div);
    if (ret)
        return ret;

    /*
     * We need to set sysdiv according to the clock, and to clear
     * the MIPI divider.
     */
    ret = ov5640_mod_reg(sensor, OV5640_REG_SC_PLL_CTRL1,
                         0xff, sysdiv << 4);
    if (ret)
        return ret;

    ret = ov5640_mod_reg(sensor, OV5640_REG_SC_PLL_CTRL2,
                         0xff, mult);
    if (ret)
        return ret;

    ret = ov5640_mod_reg(sensor, OV5640_REG_SC_PLL_CTRL3,
                         0x1f, prediv | ((pll_rdiv - 1) << 4));
    if (ret)
        return ret;

    return ov5640_mod_reg(sensor, OV5640_REG_SYS_ROOT_DIVIDER, 0x30,
                  (ov5640_ilog2(pclk_div) << 4));
}

/* set JPEG framing sizes */
static int ov5640_set_jpeg_timings(struct ov5640_dev *sensor,
                                   const struct ov5640_mode_info *mode)
{
    int ret;

    /*
     * compression mode 3 timing
     *
     * Data is transmitted with programmable width (VFIFO_HSIZE).
     * No padding done. Last line may have less data. Varying
     * number of lines per frame, depending on amount of data.
     */
    ret = ov5640_mod_reg(sensor, OV5640_REG_JPG_MODE_SELECT, 0x7, 0x3);
    if (ret < 0)
        return ret;

    ret = ov5640_write_reg16(sensor, OV5640_REG_VFIFO_HSIZE, mode->hact);
    if (ret < 0)
        return ret;

    return ov5640_write_reg16(sensor, OV5640_REG_VFIFO_VSIZE, mode->vact);
}

/* download ov5640 settings to sensor through i2c */
static int ov5640_set_timings(struct ov5640_dev *sensor,
                              const struct ov5640_mode_info *mode)
{
    int ret;

    if (sensor->fmt.code == MEDIA_BUS_FMT_JPEG_1X8) {
        ret = ov5640_set_jpeg_timings(sensor, mode);
        if (ret < 0)
            return ret;
    }

    ret = ov5640_write_reg16(sensor, OV5640_REG_TIMING_DVPHO, mode->hact);
    if (ret < 0)
        return ret;

    ret = ov5640_write_reg16(sensor, OV5640_REG_TIMING_DVPVO, mode->vact);
    if (ret < 0)
        return ret;

    ret = ov5640_write_reg16(sensor, OV5640_REG_TIMING_HTS, mode->htot);
    if (ret < 0)
        return ret;

    return ov5640_write_reg16(sensor, OV5640_REG_TIMING_VTS, mode->vtot);
}

static int ov5640_load_regs(struct ov5640_dev *sensor,
                            const struct ov5640_mode_info *mode)
{
    const struct reg_value *regs = mode->reg_data;
    unsigned int i;
    u32 delay_ms;
    u16 reg_addr;
    u8 mask, val;
    int ret = 0;

    for (i = 0; i < mode->reg_data_size; ++i, ++regs) {
        delay_ms = regs->delay_ms;
        reg_addr = regs->reg_addr;
        val = regs->val;
        mask = regs->mask;

        /* remain in power down mode for DVP */
        if (regs->reg_addr == OV5640_REG_SYS_CTRL0 &&
                val == OV5640_REG_SYS_CTRL0_SW_PWUP &&
                sensor->fmt.bus_type != MEDIA_BUS_CSI2_DPHY)
            continue;

        if (mask)
            ret = ov5640_mod_reg(sensor, reg_addr, mask, val);
        else
            ret = ov5640_write_reg(sensor, reg_addr, val);
        if (ret)
            break;

        if (delay_ms)
            aicos_msleep(delay_ms);
    }

    LOG_D("Loaded mode %d", mode->id);
    return ov5640_set_timings(sensor, mode);
}

static int ov5640_set_autoexposure(struct ov5640_dev *sensor, bool on)
{
    return ov5640_mod_reg(sensor, OV5640_REG_AEC_PK_MANUAL,
                          BIT(0), on ? 0 : BIT(0));
}

/* read exposure, in number of line periods */
static int ov5640_get_exposure(struct ov5640_dev *sensor)
{
    int exp, ret;
    u8 temp;

    ret = ov5640_read_reg(sensor, OV5640_REG_AEC_PK_EXPOSURE_HI, &temp);
    if (ret)
        return ret;
    exp = ((int)temp & 0x0f) << 16;
    ret = ov5640_read_reg(sensor, OV5640_REG_AEC_PK_EXPOSURE_MED, &temp);
    if (ret)
        return ret;
    exp |= ((int)temp << 8);
    ret = ov5640_read_reg(sensor, OV5640_REG_AEC_PK_EXPOSURE_LO, &temp);
    if (ret)
        return ret;
    exp |= (int)temp;

    return exp >> 4;
}

/* write exposure, given number of line periods */
static int ov5640_set_exposure(struct ov5640_dev *sensor, u32 exposure)
{
    int ret;

    exposure <<= 4;

    ret = ov5640_write_reg(sensor,
                           OV5640_REG_AEC_PK_EXPOSURE_LO,
                           exposure & 0xff);
    if (ret)
        return ret;
    ret = ov5640_write_reg(sensor,
                           OV5640_REG_AEC_PK_EXPOSURE_MED,
                           (exposure >> 8) & 0xff);
    if (ret)
        return ret;
    return ov5640_write_reg(sensor,
                            OV5640_REG_AEC_PK_EXPOSURE_HI,
                            (exposure >> 16) & 0x0f);
}

static int ov5640_get_gain(struct ov5640_dev *sensor)
{
    u16 gain;
    int ret;

    ret = ov5640_read_reg16(sensor, OV5640_REG_AEC_PK_REAL_GAIN, &gain);
    if (ret)
        return ret;

    return gain & 0x3ff;
}

static int ov5640_set_gain(struct ov5640_dev *sensor, int gain)
{
    return ov5640_write_reg16(sensor, OV5640_REG_AEC_PK_REAL_GAIN,
                              (u16)gain & 0x3ff);
}

static int ov5640_set_autogain(struct ov5640_dev *sensor, bool on)
{
    return ov5640_mod_reg(sensor, OV5640_REG_AEC_PK_MANUAL,
                          BIT(1), on ? 0 : BIT(1));
}

static int ov5640_set_stream_dvp(struct ov5640_dev *sensor, bool on)
{
    return ov5640_write_reg(sensor, OV5640_REG_SYS_CTRL0, on ?
                            OV5640_REG_SYS_CTRL0_SW_PWUP :
                            OV5640_REG_SYS_CTRL0_SW_PWDN);
}

static int ov5640_set_stream_mipi(struct ov5640_dev *sensor, bool on)
{
    int ret;

    /*
     * Enable/disable the MIPI interface
     *
     * 0x300e = on ? 0x45 : 0x40
     *
     * FIXME: the sensor manual (version 2.03) reports
     * [7:5] = 000  : 1 data lane mode
     * [7:5] = 001  : 2 data lanes mode
     * But this settings do not work, while the following ones
     * have been validated for 2 data lanes mode.
     *
     * [7:5] = 010  : 2 data lanes mode
     * [4] = 0  : Power up MIPI HS Tx
     * [3] = 0  : Power up MIPI LS Rx
     * [2] = 1/0    : MIPI interface enable/disable
     * [1:0] = 01/00: FIXME: 'debug'
     */
    ret = ov5640_write_reg(sensor, OV5640_REG_IO_MIPI_CTRL00,
                           on ? 0x45 : 0x40);
    if (ret)
        return ret;

    return ov5640_write_reg(sensor, OV5640_REG_FRAME_CTRL01,
                            on ? 0x00 : 0x0f);
}

static int ov5640_get_sysclk(struct ov5640_dev *sensor)
{
    /* calculate sysclk */
    u32 xvclk = sensor->xclk_freq / 10000;
    u32 multiplier, prediv, VCO, sysdiv, pll_rdiv;
    u32 sclk_rdiv_map[] = {1, 2, 4, 8};
    u32 bit_div2x = 1, sclk_rdiv, sysclk;
    u8 temp1, temp2;
    int ret;

    ret = ov5640_read_reg(sensor, OV5640_REG_SC_PLL_CTRL0, &temp1);
    if (ret)
        return ret;
    temp2 = temp1 & 0x0f;
    if (temp2 == 8 || temp2 == 10)
        bit_div2x = temp2 / 2;

    ret = ov5640_read_reg(sensor, OV5640_REG_SC_PLL_CTRL1, &temp1);
    if (ret)
        return ret;
    sysdiv = temp1 >> 4;
    if (sysdiv == 0)
        sysdiv = 16;

    ret = ov5640_read_reg(sensor, OV5640_REG_SC_PLL_CTRL2, &temp1);
    if (ret)
        return ret;
    multiplier = temp1;

    ret = ov5640_read_reg(sensor, OV5640_REG_SC_PLL_CTRL3, &temp1);
    if (ret)
        return ret;
    prediv = temp1 & 0x0f;
    pll_rdiv = ((temp1 >> 4) & 0x01) + 1;

    ret = ov5640_read_reg(sensor, OV5640_REG_SYS_ROOT_DIVIDER, &temp1);
    if (ret)
        return ret;
    temp2 = temp1 & 0x03;
    sclk_rdiv = sclk_rdiv_map[temp2];

    if (!prediv || !sysdiv || !pll_rdiv || !bit_div2x)
        return -EINVAL;

    VCO = xvclk * multiplier / prediv;

    sysclk = VCO / sysdiv / pll_rdiv * 2 / bit_div2x / sclk_rdiv;
    LOG_D("sysclk %d, sysdiv %d", sysclk, sysdiv);

    return sysclk;
}

static int ov5640_set_night_mode(struct ov5640_dev *sensor)
{
    /* read HTS from register settings */
    u8 mode;
    int ret;

    ret = ov5640_read_reg(sensor, OV5640_REG_AEC_CTRL00, &mode);
    if (ret)
        return ret;
    mode &= 0xfb;
    return ov5640_write_reg(sensor, OV5640_REG_AEC_CTRL00, mode);
}

static int ov5640_get_hts(struct ov5640_dev *sensor)
{
    /* read HTS from register settings */
    u16 hts;
    int ret;

    ret = ov5640_read_reg16(sensor, OV5640_REG_TIMING_HTS, &hts);
    if (ret)
        return ret;
    return hts;
}

static int ov5640_get_vts(struct ov5640_dev *sensor)
{
    u16 vts;
    int ret;

    ret = ov5640_read_reg16(sensor, OV5640_REG_TIMING_VTS, &vts);
    if (ret)
        return ret;
    return vts;
}

static int ov5640_set_vts(struct ov5640_dev *sensor, int vts)
{
    return ov5640_write_reg16(sensor, OV5640_REG_TIMING_VTS, vts);
}

static int ov5640_get_light_freq(struct ov5640_dev *sensor)
{
    /* get banding filter value */
    int ret, light_freq = 0;
    u8 temp, temp1;

    ret = ov5640_read_reg(sensor, OV5640_REG_HZ5060_CTRL01, &temp);
    if (ret)
        return ret;

    if (temp & 0x80) {
        /* manual */
        ret = ov5640_read_reg(sensor, OV5640_REG_HZ5060_CTRL00,
                              &temp1);
        if (ret)
            return ret;
        if (temp1 & 0x04) {
            /* 50Hz */
            light_freq = 50;
        } else {
            /* 60Hz */
            light_freq = 60;
        }
    } else {
        /* auto */
        ret = ov5640_read_reg(sensor, OV5640_REG_SIGMADELTA_CTRL0C,
                              &temp1);
        if (ret)
            return ret;

        if (temp1 & 0x01) {
            /* 50Hz */
            light_freq = 50;
        } else {
            /* 60Hz */
        }
    }

    return light_freq;
}

static int ov5640_set_bandingfilter(struct ov5640_dev *sensor)
{
    u32 band_step60, max_band60, band_step50, max_band50, prev_vts;
    int ret;

    /* read preview PCLK */
    ret = ov5640_get_sysclk(sensor);
    if (ret < 0)
        return ret;
    if (ret == 0)
        return -EINVAL;
    sensor->prev_sysclk = ret;
    /* read preview HTS */
    ret = ov5640_get_hts(sensor);
    if (ret < 0)
        return ret;
    if (ret == 0)
        return -EINVAL;
    sensor->prev_hts = ret;

    /* read preview VTS */
    ret = ov5640_get_vts(sensor);
    if (ret < 0)
        return ret;
    prev_vts = ret;

    /* calculate banding filter */
    /* 60Hz */
    band_step60 = sensor->prev_sysclk * 100 / sensor->prev_hts * 100 / 120;
    ret = ov5640_write_reg16(sensor, OV5640_REG_AEC_B60_STEP, band_step60);
    if (ret)
        return ret;
    if (!band_step60)
        return -EINVAL;
    max_band60 = (int)((prev_vts - 4) / band_step60);
    ret = ov5640_write_reg(sensor, OV5640_REG_AEC_CTRL0D, max_band60);
    if (ret)
        return ret;

    /* 50Hz */
    band_step50 = sensor->prev_sysclk * 100 / sensor->prev_hts;
    ret = ov5640_write_reg16(sensor, OV5640_REG_AEC_B50_STEP, band_step50);
    if (ret)
        return ret;
    if (!band_step50)
        return -EINVAL;
    max_band50 = (int)((prev_vts - 4) / band_step50);
    return ov5640_write_reg(sensor, OV5640_REG_AEC_CTRL0E, max_band50);
}

static int ov5640_set_ae_target(struct ov5640_dev *sensor, int target)
{
    /* stable in high */
    u32 fast_high, fast_low;
    int ret;

    sensor->ae_low = target * 23 / 25;  /* 0.92 */
    sensor->ae_high = target * 27 / 25; /* 1.08 */

    fast_high = sensor->ae_high << 1;
    if (fast_high > 255)
        fast_high = 255;

    fast_low = sensor->ae_low >> 1;

    ret = ov5640_write_reg(sensor, OV5640_REG_AEC_CTRL0F, sensor->ae_high);
    if (ret)
        return ret;
    ret = ov5640_write_reg(sensor, OV5640_REG_AEC_CTRL10, sensor->ae_low);
    if (ret)
        return ret;
    ret = ov5640_write_reg(sensor, OV5640_REG_AEC_CTRL1B, sensor->ae_high);
    if (ret)
        return ret;
    ret = ov5640_write_reg(sensor, OV5640_REG_AEC_CTRL1E, sensor->ae_low);
    if (ret)
        return ret;
    ret = ov5640_write_reg(sensor, OV5640_REG_AEC_CTRL11, fast_high);
    if (ret)
        return ret;
    return ov5640_write_reg(sensor, OV5640_REG_AEC_CTRL1F, fast_low);
}

static int ov5640_get_binning(struct ov5640_dev *sensor)
{
    u8 temp;
    int ret;

    ret = ov5640_read_reg(sensor, OV5640_REG_TIMING_TC_REG21, &temp);
    if (ret)
        return ret;

    return temp & BIT(0);
}

static int ov5640_set_binning(struct ov5640_dev *sensor, bool enable)
{
    int ret;

    /*
     * TIMING TC REG21:
     * - [0]:   Horizontal binning enable
     */
    ret = ov5640_mod_reg(sensor, OV5640_REG_TIMING_TC_REG21,
                         BIT(0), enable ? BIT(0) : 0);
    if (ret)
        return ret;
    /*
     * TIMING TC REG20:
     * - [0]:   Undocumented, but hardcoded init sequences
     *      are always setting REG21/REG20 bit 0 to same value...
     */
    return ov5640_mod_reg(sensor, OV5640_REG_TIMING_TC_REG20,
                          BIT(0), enable ? BIT(0) : 0);
}

static int ov5640_set_virtual_channel(struct ov5640_dev *sensor)
{
    u8 temp, virt_ch = 0;
    int ret;

    if (virt_ch > 3) {
        LOG_E("invalid virtual channel: %d, expected [0..3]", virt_ch);
        return -EINVAL;
    }

    ret = ov5640_read_reg(sensor, OV5640_REG_DEBUG_MODE, &temp);
    if (ret)
        return ret;
    temp &= ~(3 << 6);
    temp |= (virt_ch << 6);
    return ov5640_write_reg(sensor, OV5640_REG_DEBUG_MODE, temp);
}

static u64 ov5640_calc_pixel_rate(struct ov5640_dev *sensor)
{
    u64 rate = 0;

    rate = sensor->current_mode->vtot * sensor->current_mode->htot;
    rate *= ov5640_framerates[sensor->current_fr];
    LOG_I("v %d, h %d, fps %d, rate: %d", sensor->current_mode->vtot,
        sensor->current_mode->htot, ov5640_framerates[sensor->current_fr], (u32)rate);

    return rate;
}

/*
 * sensor changes between scaling and subsampling, go through
 * exposure calculation
 */
static int ov5640_set_mode_exposure_calc(struct ov5640_dev *sensor,
        const struct ov5640_mode_info *mode)
{
    u32 prev_shutter, prev_gain16;
    u32 cap_shutter, cap_gain16;
    u32 cap_sysclk, cap_hts, cap_vts;
    u32 light_freq, cap_bandfilt, cap_maxband;
    u32 cap_gain16_shutter;
    u8 average;
    int ret;

    if (!mode->reg_data)
        return -EINVAL;

    /* read preview shutter */
    ret = ov5640_get_exposure(sensor);
    if (ret < 0)
        return ret;
    prev_shutter = ret;
    ret = ov5640_get_binning(sensor);
    if (ret < 0)
        return ret;
    if (ret && mode->id != OV5640_MODE_720P_1280_720 &&
            mode->id != OV5640_MODE_1080P_1920_1080)
        prev_shutter *= 2;

    /* read preview gain */
    ret = ov5640_get_gain(sensor);
    if (ret < 0)
        return ret;
    prev_gain16 = ret;

    /* get average */
    ret = ov5640_read_reg(sensor, OV5640_REG_AVG_READOUT, &average);
    if (ret)
        return ret;

    /* turn off night mode for capture */
    ret = ov5640_set_night_mode(sensor);
    if (ret < 0)
        return ret;

    /* Write capture setting */
    ret = ov5640_load_regs(sensor, mode);
    if (ret < 0)
        return ret;

    /* read capture VTS */
    ret = ov5640_get_vts(sensor);
    if (ret < 0)
        return ret;
    cap_vts = ret;
    ret = ov5640_get_hts(sensor);
    if (ret < 0)
        return ret;
    if (ret == 0)
        return -EINVAL;
    cap_hts = ret;

    ret = ov5640_get_sysclk(sensor);
    if (ret < 0)
        return ret;
    if (ret == 0)
        return -EINVAL;
    cap_sysclk = ret;

    /* calculate capture banding filter */
    ret = ov5640_get_light_freq(sensor);
    if (ret < 0)
        return ret;
    light_freq = ret;

    if (light_freq == 60) {
        /* 60Hz */
        cap_bandfilt = cap_sysclk * 100 / cap_hts * 100 / 120;
    } else {
        /* 50Hz */
        cap_bandfilt = cap_sysclk * 100 / cap_hts;
    }

    if (!sensor->prev_sysclk) {
        ret = ov5640_get_sysclk(sensor);
        if (ret < 0)
            return ret;
        if (ret == 0)
            return -EINVAL;
        sensor->prev_sysclk = ret;
    }

    if (!cap_bandfilt)
        return -EINVAL;

    cap_maxband = (int)((cap_vts - 4) / cap_bandfilt);

    /* calculate capture shutter/gain16 */
    if (average > sensor->ae_low && average < sensor->ae_high) {
        /* in stable range */
        cap_gain16_shutter =
            prev_gain16 * prev_shutter *
            cap_sysclk / sensor->prev_sysclk *
            sensor->prev_hts / cap_hts *
            sensor->ae_target / average;
    } else {
        cap_gain16_shutter =
            prev_gain16 * prev_shutter *
            cap_sysclk / sensor->prev_sysclk *
            sensor->prev_hts / cap_hts;
    }

    /* gain to shutter */
    if (cap_gain16_shutter < (cap_bandfilt * 16)) {
        /* shutter < 1/100 */
        cap_shutter = cap_gain16_shutter / 16;
        if (cap_shutter < 1)
            cap_shutter = 1;

        cap_gain16 = cap_gain16_shutter / cap_shutter;
        if (cap_gain16 < 16)
            cap_gain16 = 16;
    } else {
        if (cap_gain16_shutter > (cap_bandfilt * cap_maxband * 16)) {
            /* exposure reach max */
            cap_shutter = cap_bandfilt * cap_maxband;
            if (!cap_shutter)
                return -EINVAL;

            cap_gain16 = cap_gain16_shutter / cap_shutter;
        } else {
            /* 1/100 < (cap_shutter = n/100) =< max */
            cap_shutter =
                ((int)(cap_gain16_shutter / 16 / cap_bandfilt))
                * cap_bandfilt;
            if (!cap_shutter)
                return -EINVAL;

            cap_gain16 = cap_gain16_shutter / cap_shutter;
        }
    }

    /* set capture gain */
    ret = ov5640_set_gain(sensor, cap_gain16);
    if (ret)
        return ret;

    /* write capture shutter */
    if (cap_shutter > (cap_vts - 4)) {
        cap_vts = cap_shutter + 4;
        ret = ov5640_set_vts(sensor, cap_vts);
        if (ret < 0)
            return ret;
    }

    /* set exposure */
    return ov5640_set_exposure(sensor, cap_shutter);
}

/*
 * if sensor changes inside scaling or subsampling
 * change mode directly
 */
static int ov5640_set_mode_direct(struct ov5640_dev *sensor,
                                  const struct ov5640_mode_info *mode)
{
    if (!mode->reg_data)
        return -EINVAL;

    /* Write capture setting */
    return ov5640_load_regs(sensor, mode);
}

static int ov5640_set_mode(struct ov5640_dev *sensor)
{
    const struct ov5640_mode_info *mode = sensor->current_mode;
    const struct ov5640_mode_info *orig_mode = sensor->last_mode;
    enum ov5640_downsize_mode dn_mode, orig_dn_mode;
    bool auto_gain = OV5640_AUTO_EXPOSURE == 1;
    bool auto_exp = OV5640_AUTO_GAIN == 1;
    unsigned long rate;
    int ret;

    dn_mode = mode->dn_mode;
    orig_dn_mode = orig_mode->dn_mode;

    LOG_D("Set mode %d -> %d ...", orig_mode->id, mode->id);
    if (orig_mode->id == mode->id)
        return 0;

    /* auto gain and exposure must be turned off when changing modes */
    if (auto_gain) {
        ret = ov5640_set_autogain(sensor, false);
        if (ret)
            return ret;
    }

    if (auto_exp) {
        ret = ov5640_set_autoexposure(sensor, false);
        if (ret)
            goto restore_auto_gain;
    }

    /*
     * All the formats we support have 16 bits per pixel, seems to require
     * the same rate than YUV, so we can just use 16 bpp all the time.
     */
    rate = ov5640_calc_pixel_rate(sensor) * 16;
    if (sensor->fmt.bus_type == MEDIA_BUS_CSI2_DPHY) {
        rate = rate / OV5640_CSI2_DATA_LANES;
        ret = ov5640_set_mipi_pclk(sensor, rate);
    } else {
        rate = rate / OV5640_DVP_BUS_WIDTH;
        ret = ov5640_set_dvp_pclk(sensor, rate);
    }

    if (ret < 0)
        return 0;

    if ((dn_mode == SUBSAMPLING && orig_dn_mode == SCALING) ||
            (dn_mode == SCALING && orig_dn_mode == SUBSAMPLING)) {
        /*
         * change between subsampling and scaling
         * go through exposure calculation
         */
        ret = ov5640_set_mode_exposure_calc(sensor, mode);
    } else {
        /*
         * change inside subsampling or scaling
         * download firmware directly
         */
        ret = ov5640_set_mode_direct(sensor, mode);
    }
    if (ret < 0)
        goto restore_auto_exp_gain;

    /* restore auto gain and exposure */
    if (auto_gain)
        ov5640_set_autogain(sensor, true);
    if (auto_exp)
        ov5640_set_autoexposure(sensor, true);

    ret = ov5640_set_binning(sensor, dn_mode != SCALING);
    if (ret < 0)
        return ret;
    ret = ov5640_set_ae_target(sensor, sensor->ae_target);
    if (ret < 0)
        return ret;
    ret = ov5640_get_light_freq(sensor);
    if (ret < 0)
        return ret;
    ret = ov5640_set_bandingfilter(sensor);
    if (ret < 0)
        return ret;
    ret = ov5640_set_virtual_channel(sensor);
    if (ret < 0)
        return ret;

    sensor->last_mode = mode;

    return 0;

restore_auto_exp_gain:
    if (auto_exp)
        ov5640_set_autoexposure(sensor, true);
restore_auto_gain:
    if (auto_gain)
        ov5640_set_autogain(sensor, true);

    return ret;
}

static int ov5640_set_framefmt(struct ov5640_dev *sensor,
                               struct mpp_video_fmt *format)
{
    int ret = 0;
    bool is_jpeg = false;
    u8 fmt, mux;

    LOG_D("Set format %#x ...", format->code);
    switch (format->code) {
    case MEDIA_BUS_FMT_UYVY8_2X8:
        /* YUV422, UYVY */
        fmt = 0x3f;
        mux = OV5640_FMT_MUX_YUV422;
        break;
    case MEDIA_BUS_FMT_YUYV8_2X8:
        /* YUV422, YUYV */
        fmt = 0x30;
        mux = OV5640_FMT_MUX_YUV422;
        break;
    case MEDIA_BUS_FMT_RGB565_2X8_LE:
        /* RGB565 {g[2:0],b[4:0]},{r[4:0],g[5:3]} */
        fmt = 0x6F;
        mux = OV5640_FMT_MUX_RGB;
        break;
    case MEDIA_BUS_FMT_RGB565_2X8_BE:
        /* RGB565 {r[4:0],g[5:3]},{g[2:0],b[4:0]} */
        fmt = 0x61;
        mux = OV5640_FMT_MUX_RGB;
        break;
    case MEDIA_BUS_FMT_JPEG_1X8:
        /* YUV422, YUYV */
        fmt = 0x30;
        mux = OV5640_FMT_MUX_YUV422;
        is_jpeg = true;
        break;
    case MEDIA_BUS_FMT_SBGGR8_1X8:
        /* Raw, BGBG... / GRGR... */
        fmt = 0x00;
        mux = OV5640_FMT_MUX_RAW_DPC;
        break;
    case MEDIA_BUS_FMT_SGBRG8_1X8:
        /* Raw bayer, GBGB... / RGRG... */
        fmt = 0x01;
        mux = OV5640_FMT_MUX_RAW_DPC;
        break;
    case MEDIA_BUS_FMT_SGRBG8_1X8:
        /* Raw bayer, GRGR... / BGBG... */
        fmt = 0x02;
        mux = OV5640_FMT_MUX_RAW_DPC;
        break;
    case MEDIA_BUS_FMT_SRGGB8_1X8:
        /* Raw bayer, RGRG... / GBGB... */
        fmt = 0x03;
        mux = OV5640_FMT_MUX_RAW_DPC;
        break;
    default:
        return -EINVAL;
    }

    /* FORMAT CONTROL00: YUV and RGB formatting */
    ret = ov5640_write_reg(sensor, OV5640_REG_FORMAT_CONTROL00, fmt);
    if (ret)
        return ret;

    /* FORMAT MUX CONTROL: ISP YUV or RGB */
    ret = ov5640_write_reg(sensor, OV5640_REG_ISP_FORMAT_MUX_CTRL, mux);
    if (ret)
        return ret;

    /*
     * TIMING TC REG21:
     * - [5]:   JPEG enable
     */
    ret = ov5640_mod_reg(sensor, OV5640_REG_TIMING_TC_REG21,
                         BIT(5), is_jpeg ? BIT(5) : 0);
    if (ret)
        return ret;

    /*
     * SYSTEM RESET02:
     * - [4]:   Reset JFIFO
     * - [3]:   Reset SFIFO
     * - [2]:   Reset JPEG
     */
    ret = ov5640_mod_reg(sensor, OV5640_REG_SYS_RESET02,
                         BIT(4) | BIT(3) | BIT(2),
                         is_jpeg ? 0 : (BIT(4) | BIT(3) | BIT(2)));
    if (ret)
        return ret;

    /*
     * CLOCK ENABLE02:
     * - [5]:   Enable JPEG 2x clock
     * - [3]:   Enable JPEG clock
     */
    return ov5640_mod_reg(sensor, OV5640_REG_SYS_CLOCK_ENABLE02,
                          BIT(5) | BIT(3),
                          is_jpeg ? (BIT(5) | BIT(3)) : 0);
}

static int ov5640_set_ctrl_light_freq(struct ov5640_dev *sensor, int value)
{
    int ret;

    ret = ov5640_mod_reg(sensor, OV5640_REG_HZ5060_CTRL01, BIT(7),
                 (value == MEDIA_POWER_LINE_FREQUENCY_AUTO) ?
                 0 : BIT(7));
    if (ret)
        return ret;

    return ov5640_mod_reg(sensor, OV5640_REG_HZ5060_CTRL00, BIT(2),
                  (value == MEDIA_POWER_LINE_FREQUENCY_50HZ) ?
                  BIT(2) : 0);
}

/* restore the last set video mode after chip power-on */
static int ov5640_restore_mode(struct ov5640_dev *sensor)
{
    int ret;

    /* first load the initial register values */
    ret = ov5640_load_regs(sensor, &ov5640_mode_init_data);
    if (ret < 0)
        return ret;
    sensor->last_mode = &ov5640_mode_init_data;

    ret = ov5640_mod_reg(sensor, OV5640_REG_SYS_ROOT_DIVIDER, 0x3f,
                 (ov5640_ilog2(OV5640_SCLK2X_ROOT_DIV) << 2) |
                 ov5640_ilog2(OV5640_SCLK_ROOT_DIV));
    if (ret)
        return ret;

    /* now restore the last capture mode */
    ret = ov5640_set_mode(sensor);
    if (ret < 0)
        return ret;

    return ov5640_set_framefmt(sensor, &sensor->fmt);
}

static int ov5640_s_stream(struct ov5640_dev *sensor, int enable)
{
    int ret = 0;

    if (sensor->streaming == !enable) {
        if (enable && sensor->pending_mode_change) {
            ret = ov5640_set_mode(sensor);
            if (ret)
                goto out;
        }

        ret = ov5640_set_framefmt(sensor, &sensor->fmt);
        if (ret)
            goto out;

        if (sensor->fmt.bus_type == MEDIA_BUS_CSI2_DPHY)
            ret = ov5640_set_stream_mipi(sensor, enable);
        else
            ret = ov5640_set_stream_dvp(sensor, enable);

        // ov5640_set_binning(sensor, 1);
        ov5640_set_ctrl_light_freq(sensor, MEDIA_POWER_LINE_FREQUENCY_50HZ);
        if (!ret)
            sensor->streaming = enable;
    }
out:
    return ret;
}

static int ov5640_iic_init(struct ov5640_dev *sensor)
{
    char name[8] = "";

    snprintf(name, 8, "i2c%d", AIC_CAMERA_I2C_CHAN);
    sensor->iic = rt_i2c_bus_device_find(name);
    if (sensor->iic == RT_NULL) {
        LOG_E("Failed to open %s", name);
        return -ENODEV;
    }

    sensor->pending_mode_change = true;
    return 0;
}

static void ov5640_power(struct ov5640_dev *sensor, bool enable)
{
#ifndef FPGA_BOARD_ARTINCHIP
    rt_pin_write(sensor->pwdn_pin, enable ? 0 : 1);
#endif
}

static int ov5640_set_power_on(struct ov5640_dev *sensor)
{
#ifndef FPGA_BOARD_ARTINCHIP
    rt_pin_write(sensor->rst_pin, PIN_LOW);

    ov5640_power(sensor, 0);
    rt_thread_mdelay(10);
    ov5640_power(sensor, 1);
    rt_thread_mdelay(10);

    rt_pin_write(sensor->rst_pin, PIN_HIGH);
    rt_thread_mdelay(2);
    pr_info("power on\n");
#endif
    return 0;
}

static int ov5640_set_power_off(struct ov5640_dev *sensor)
{
    pr_info("power off\n");
    ov5640_power(sensor, 0);
    return 0;
}

static int ov5640_set_power_mipi(struct ov5640_dev *sensor, bool on)
{
    int ret;

    if (!on) {
        /* Reset MIPI bus settings to their default values. */
        ov5640_write_reg(sensor, OV5640_REG_IO_MIPI_CTRL00, 0x58);
        ov5640_write_reg(sensor, OV5640_REG_MIPI_CTRL00, 0x04);
        ov5640_write_reg(sensor, OV5640_REG_PAD_OUTPUT00, 0x00);
        return 0;
    }

    /*
     * Power up MIPI HS Tx and LS Rx; 2 data lanes mode
     *
     * 0x300e = 0x40
     * [7:5] = 010  : 2 data lanes mode (see FIXME note in
     *        "ov5640_set_stream_mipi()")
     * [4] = 0  : Power up MIPI HS Tx
     * [3] = 0  : Power up MIPI LS Rx
     * [2] = 0  : MIPI interface disabled
     */
    ret = ov5640_write_reg(sensor, OV5640_REG_IO_MIPI_CTRL00, 0x40);
    if (ret)
        return ret;

    /*
     * Gate clock and set LP11 in 'no packets mode' (idle)
     *
     * 0x4800 = 0x24
     * [5] = 1  : Gate clock when 'no packets'
     * [2] = 1  : MIPI bus in LP11 when 'no packets'
     */
    ret = ov5640_write_reg(sensor, OV5640_REG_MIPI_CTRL00, 0x24);
    if (ret)
        return ret;

    /*
     * Set data lanes and clock in LP11 when 'sleeping'
     *
     * 0x3019 = 0x70
     * [6] = 1  : MIPI data lane 2 in LP11 when 'sleeping'
     * [5] = 1  : MIPI data lane 1 in LP11 when 'sleeping'
     * [4] = 1  : MIPI clock lane in LP11 when 'sleeping'
     */
    ret = ov5640_write_reg(sensor, OV5640_REG_PAD_OUTPUT00, 0x70);
    if (ret)
        return ret;

    /* Give lanes some time to coax into LP11 state. */
    aicos_udelay(1000);

    return 0;
}

static int ov5640_set_power_dvp(struct ov5640_dev *sensor, bool on)
{
    unsigned int flags = sensor->fmt.flags;
    bool bt656 = sensor->fmt.bus_type == MEDIA_BUS_BT656;
    u8 polarities = 0;
    int ret;

    if (!on) {
        /* Reset settings to their default values. */
        ov5640_write_reg(sensor, OV5640_REG_CCIR656_CTRL00, 0x00);
        ov5640_write_reg(sensor, OV5640_REG_IO_MIPI_CTRL00, 0x58);
        ov5640_write_reg(sensor, OV5640_REG_POLARITY_CTRL00, 0x20);
        ov5640_write_reg(sensor, OV5640_REG_PAD_OUTPUT_ENABLE01, 0x00);
        ov5640_write_reg(sensor, OV5640_REG_PAD_OUTPUT_ENABLE02, 0x00);
#ifdef CONFIG_FPGA_BOARD_ARTINCHIP
        ov5640_write_reg(sensor, OV5640_REG_OUTPUT_DRV_CAP, 0x03);
#endif
        return 0;
    }

    /*
     * Note about parallel port configuration.
     *
     * When configured in parallel mode, the OV5640 will
     * output 10 bits data on DVP data lines [9:0].
     * If only 8 bits data are wanted, the 8 bits data lines
     * of the camera interface must be physically connected
     * on the DVP data lines [9:2].
     *
     * Control lines polarity can be configured through
     * devicetree endpoint control lines properties.
     * If no endpoint control lines properties are set,
     * polarity will be as below:
     * - VSYNC: active high
     * - HREF:  active low
     * - PCLK:  active low
     *
     * VSYNC & HREF are not configured if BT656 bus mode is selected
     */

    /*
     * BT656 embedded synchronization configuration
     *
     * CCIR656 CTRL00
     * - [7]:   SYNC code selection (0: auto generate sync code,
     *      1: sync code from regs 0x4732-0x4735)
     * - [6]:   f value in CCIR656 SYNC code when fixed f value
     * - [5]:   Fixed f value
     * - [4:3]: Blank toggle data options (00: data=1'h040/1'h200,
     *      01: data from regs 0x4736-0x4738, 10: always keep 0)
     * - [1]:   Clip data disable
     * - [0]:   CCIR656 mode enable
     *
     * Default CCIR656 SAV/EAV mode with default codes
     * SAV=0xff000080 & EAV=0xff00009d is enabled here with settings:
     * - CCIR656 mode enable
     * - auto generation of sync codes
     * - blank toggle data 1'h040/1'h200
     * - clip reserved data (0x00 & 0xff changed to 0x01 & 0xfe)
     */
    ret = ov5640_write_reg(sensor, OV5640_REG_CCIR656_CTRL00,
                   bt656 ? 0x01 : 0x00);
    if (ret)
        return ret;

    /*
     * configure parallel port control lines polarity
     *
     * POLARITY CTRL0
     * - [5]:   PCLK polarity (0: active low, 1: active high)
     * - [1]:   HREF polarity (0: active low, 1: active high)
     * - [0]:   VSYNC polarity (mismatch here between
     *      datasheet and hardware, 0 is active high
     *      and 1 is active low...)
     */
    if (!bt656) {
        if (flags & MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH)
            polarities |= BIT(1);
        if (flags & MEDIA_SIGNAL_VSYNC_ACTIVE_LOW)
            polarities |= BIT(0);
    }
    if (flags & MEDIA_SIGNAL_PCLK_SAMPLE_RISING)
        polarities |= BIT(5);

    ret = ov5640_write_reg(sensor, OV5640_REG_POLARITY_CTRL00, polarities);
    if (ret)
        return ret;

    /*
     * powerdown MIPI TX/RX PHY & enable DVP
     *
     * MIPI CONTROL 00
     * [4] = 1  : Power down MIPI HS Tx
     * [3] = 1  : Power down MIPI LS Rx
     * [2] = 0  : DVP enable (MIPI disable)
     */
    ret = ov5640_write_reg(sensor, OV5640_REG_IO_MIPI_CTRL00, 0x18);
    if (ret)
        return ret;

    /*
     * enable VSYNC/HREF/PCLK DVP control lines
     * & D[9:6] DVP data lines
     *
     * PAD OUTPUT ENABLE 01
     * - 6:     VSYNC output enable
     * - 5:     HREF output enable
     * - 4:     PCLK output enable
     * - [3:0]: D[9:6] output enable
     */
    ret = ov5640_write_reg(sensor, OV5640_REG_PAD_OUTPUT_ENABLE01,
                   bt656 ? 0x1f : 0x7f);
    if (ret)
        return ret;

    /*
     * enable D[5:0] DVP data lines
     *
     * PAD OUTPUT ENABLE 02
     * - [7:2]: D[5:0] output enable
     */
    return ov5640_write_reg(sensor, OV5640_REG_PAD_OUTPUT_ENABLE02, 0xfc);
}

static int ov5640_set_power(struct ov5640_dev *sensor, bool on)
{
    int ret = 0;

    if (on) {
        ret = ov5640_set_power_on(sensor);
        if (ret)
            return ret;

        ret = ov5640_restore_mode(sensor);
        if (ret)
            goto power_off;
    }

    if (sensor->fmt.bus_type == MEDIA_BUS_CSI2_DPHY)
        ret = ov5640_set_power_mipi(sensor, on);
    else
        ret = ov5640_set_power_dvp(sensor, on);
    if (ret)
        goto power_off;

    if (!on)
        ov5640_set_power_off(sensor);

    return 0;

power_off:
    ov5640_set_power_off(sensor);
    return ret;
}

static int ov5640_check_chip_id(struct ov5640_dev *sensor)
{
    int ret = 0;
    u16 chip_id = 0;

    ret = ov5640_set_power_on(sensor);
    if (ret)
        return ret;

    // printk("%s() - Delay 100us...\n", __func__);
    // aicos_udelay(100);

    ret = ov5640_read_reg16(sensor, OV5640_REG_CHIP_ID, &chip_id);
    if (ret) {
        LOG_E("%s: failed to read chip identifier", __func__);
        goto power_off;
    }

    if (chip_id != 0x5640) {
        LOG_E("%s: wrong chip identifier, expected 0x5640, got 0x%x", __func__, chip_id);
        ret = -ENXIO;
    }

power_off:
    ov5640_set_power_off(sensor);
    return ret;
}

static int ov5640_set_xclk(u32 freq)
{
    s32 ret = 0;

    if (freq < OV5640_XCLK_MIN || freq > OV5640_XCLK_MAX) {
        LOG_E("xclk freq out of range: %d Hz", freq);
        return -1;
    }

    ret = hal_clk_set_freq(OV5640_CLK_SRC, freq);
    if (ret < 0) {
        LOG_E("Failed to set OV5640_CLK_SRC %d", freq);
        return -1;
    }

    ret = hal_clk_enable(OV5640_CLK_SRC);
    if (ret < 0) {
        LOG_E("Failed to enable OV5640_CLK_SRC");
        return -1;
    }

    g_ov5640_dev.xclk_freq = freq;
    return 0;
}

static rt_err_t ov5640_init(rt_device_t dev)
{
    int ret = 0;
    struct ov5640_dev *sensor = &g_ov5640_dev;

    ret = ov5640_iic_init(sensor);
    if (ret != 0)
        return -RT_EINVAL;

    ov5640_set_xclk(24000000);

    sensor->current_mode = &ov5640_mode_data[OV5640_MODE];
    sensor->last_mode = sensor->current_mode;
    sensor->fmt.code   = OV5640_CODE;
    sensor->fmt.width  = sensor->current_mode->hact;
    sensor->fmt.height = sensor->current_mode->vact;
    sensor->fmt.bus_type = OV5640_BUS_TYPE;
    sensor->fmt.flags = MEDIA_SIGNAL_HSYNC_ACTIVE_LOW |
                        MEDIA_SIGNAL_VSYNC_ACTIVE_HIGH |
                        MEDIA_SIGNAL_PCLK_SAMPLE_RISING;
    sensor->current_fr = OV5640_FPS;
    sensor->ae_target = 52;

    sensor->rst_pin = rt_pin_get(AIC_CAMERA_RST_PIN);
    sensor->pwdn_pin = rt_pin_get(AIC_CAMERA_PWDN_PIN);
    rt_pin_mode(sensor->rst_pin, PIN_MODE_OUTPUT);
    rt_pin_mode(sensor->pwdn_pin, PIN_MODE_OUTPUT);

    ret = ov5640_check_chip_id(sensor);
    if (ret)
        return -RT_ERROR;

    LOG_I("OV5640 inited");
    return RT_EOK;
}

static rt_err_t ov5640_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct ov5640_dev *sensor = (struct ov5640_dev *)dev;

    ov5640_set_power(sensor, 1);
    LOG_I("OV5640 Open");
    return RT_EOK;
}

static rt_err_t ov5640_close(rt_device_t dev)
{
    struct ov5640_dev *sensor = (struct ov5640_dev *)dev;

    ov5640_set_power(sensor, 0);
    LOG_I("OV5640 Close");
    return RT_EOK;
}

static int ov5640_get_fmt(rt_device_t dev, struct mpp_video_fmt *cfg)
{
    struct ov5640_dev *sensor = (struct ov5640_dev *)dev;

    cfg->code   = sensor->fmt.code;
    cfg->width  = sensor->fmt.width;
    cfg->height = sensor->fmt.height;
    cfg->flags  = sensor->fmt.flags;
    cfg->bus_type = sensor->fmt.bus_type;
    return RT_EOK;
}

static int ov5640_start(rt_device_t dev)
{
    return ov5640_s_stream((struct ov5640_dev *)dev, 1);
}

static int ov5640_stop(rt_device_t dev)
{
    return ov5640_s_stream((struct ov5640_dev *)dev, 0);
}

static rt_err_t ov5640_control(rt_device_t dev, int cmd, void *args)
{
    switch (cmd) {
    case CAMERA_CMD_START:
        return ov5640_start(dev);
    case CAMERA_CMD_STOP:
        return ov5640_stop(dev);
    case CAMERA_CMD_GET_FMT:
        return ov5640_get_fmt(dev, (struct mpp_video_fmt *)args);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops ov5640_ops =
{
    .init = ov5640_init,
    .open = ov5640_open,
    .close = ov5640_close,
    .control = ov5640_control,
};
#endif

int rt_hw_ov5640_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_ov5640_dev.dev.ops = &ov5640_ops;
#else
    g_ov5640_dev.dev.init = ov5640_init;
    g_ov5640_dev.dev.open = ov5640_open;
    g_ov5640_dev.dev.close = ov5640_close;
    g_ov5640_dev.dev.control = ov5640_control;
#endif
    g_ov5640_dev.dev.type = RT_Device_Class_CAMERA;

    rt_device_register(&g_ov5640_dev.dev, CAMERA_NAME_OV, 0);
    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_ov5640_init);
