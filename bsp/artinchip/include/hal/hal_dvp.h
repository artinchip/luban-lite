/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_DVP_H_
#define _ARTINCHIP_HAL_DVP_H_

#define DVP_PLANE_NUM           2
#define DVP_MAX_HEIGHT          4096U
#define DVP_MAX_WIDTH           4096U

#define DVP_CH_BASE                 0x100
#define DVP_CTL                     0x0
#define DVP_IRQ_EN                  (DVP_CH_BASE + 0x00)
#define DVP_IRQ_STA                 (DVP_CH_BASE + 0x04)
#define DVP_IRQ_CFG                 (DVP_CH_BASE + 0x08)
#define DVP_IN_CFG                  (DVP_CH_BASE + 0x0C)
#define DVP_IN_HOR_SIZE             (DVP_CH_BASE + 0x10)
#define DVP_IN_VER_SIZE             (DVP_CH_BASE + 0x14)
#define DVP_OUT_HOR_SIZE            (DVP_CH_BASE + 0x20)
#define DVP_OUT_VER_SIZE            (DVP_CH_BASE + 0x28)
#define DVP_OUT_FRA_NUM             (DVP_CH_BASE + 0x30)
#define DVP_OUT_CUR_FRA             (DVP_CH_BASE + 0x34)
#define DVP_OUT_CTL                 (DVP_CH_BASE + 0x38)
#define DVP_OUT_UPDATE_CTL          (DVP_CH_BASE + 0x3C)
#define DVP_OUT_ADDR_BUF0           (DVP_CH_BASE + 0x40)
#define DVP_OUT_ADDR_BUF1           (DVP_CH_BASE + 0x44)
#define DVP_OUT_READ_ADDR0          (DVP_CH_BASE + 0x48)
#define DVP_OUT_READ_ADDR1          (DVP_CH_BASE + 0x4C)
#define DVP_OUT_LINE_STRIDE0        (DVP_CH_BASE + 0x50)
#define DVP_OUT_LINE_STRIDE1        (DVP_CH_BASE + 0x54)
#define DVP_OUT_ADDR_BUF0_SHA       (DVP_CH_BASE + 0x58)
#define DVP_OUT_ADDR_BUF1_SHA       (DVP_CH_BASE + 0x5C)
#define DVP_OUT_LINE_STRIDE_SHA     (DVP_CH_BASE + 0x60)
#define DVP_QOS_CFG                 (DVP_CH_BASE + 0x2C)
#define DVP_VER                     0xFFC

#define DVP_CTL_OUT_FMT(v)          ((v) << 12)
#define DVP_CTL_OUT_FMT_MASK        GENMASK(14, 12)
#define DVP_CTL_IN_SEQ(v)           ((v) << 8)
#define DVP_CTL_IN_SEQ_MASK         GENMASK(9, 8)
#define DVP_CTL_IN_FMT(v)           ((v) << 4)
#define DVP_CTL_IN_FMT_MASK         GENMASK(6, 4)
#define DVP_CTL_DROP_FRAME_EN       BIT(2)
#define DVP_CTL_CLR                 BIT(1)
#define DVP_CTL_EN                  BIT(0)

#define DVP_IRQ_EN_UPDATE_DONE      BIT(7)
#define DVP_IRQ_EN_FRAME_DONE       BIT(1)

#define DVP_IRQ_STA_UPDATE_DONE     BIT(7)
#define DVP_IRQ_STA_XY_CODE_ERR     BIT(6)
#define DVP_IRQ_STA_FIFO_FULL       BIT(3)
#define DVP_IRQ_STA_FRAME_DONE      BIT(1)

#define DVP_IN_CFG_FILED_POL_NORMAL             BIT(3)
#define DVP_IN_CFG_VSYNC_POL_ACTIVE_HIGH        BIT(2)
#define DVP_IN_CFG_HREF_POL_ACTIVE_HIGH         BIT(1)
#define DVP_IN_CFG_PCLK_POL_RISING_EDGE         BIT(0)

#define DVP_OUT_HOR_NUM(w)              (((w) * 2 - 1) << 16)
#define DVP_OUT_VER_NUM(h)              (((h) - 1) << 16)

#define DVP_OUT_ADDR_BUF(plane) (plane ? DVP_OUT_ADDR_BUF1 : DVP_OUT_ADDR_BUF0)

#define DVP_OUT_CTL_CAP_OFF_IMMEDIATELY BIT(1)
#define DVP_OUT_CTL_CAP_ON              BIT(0)

#define DVP_QOS_CUSTOM              BIT(26)
#define DVP_QOS_INC_THR_MASK        GENMASK(25, 17)
#define DVP_QOS_INC_THR_SHIFT       17
#define DVP_QOS_DEC_THR_MASK        GENMASK(16, 8)
#define DVP_QOS_DEC_THR_SHIFT       8
#define DVP_QOS_HIGH_MASK           GENMASK(7, 4)
#define DVP_QOS_HIGH_SHIFT          4
#define DVP_QOS_LOW_MASK            GENMASK(3, 0)

enum dvp_input {
    DVP_IN_RAW      = 0,
    DVP_IN_YUV422   = 1,
    DVP_IN_BT656    = 2,
};

enum dvp_output {
    DVP_OUT_RAW_PASSTHROUGH         = 0,
    DVP_OUT_YUV422_COMBINED_NV16    = 1,
    DVP_OUT_YUV420_COMBINED_NV12    = 2,
};

enum dvp_input_yuv_seq {
    DVP_YUV_DATA_SEQ_YUYV   = 0,
    DVP_YUV_DATA_SEQ_YVYU   = 1,
    DVP_YUV_DATA_SEQ_UYVY   = 2,
    DVP_YUV_DATA_SEQ_VYUY   = 3,
};

enum dvp_capture_mode {
    DVP_CAPTURE_PICTURE = 0,
    DVP_CAPTURE_VIDEO = 1
};

enum dvp_subdev_pads {
    DVP_SUBDEV_SINK = 0,
    DVP_SUBDEV_SOURCE,
    DVP_SUBDEV_PAD_NUM,
};

/* Save the configuration information for DVP controller. */
struct aic_dvp_config {
    /* Input format */
    enum dvp_input          input;
    enum dvp_input_yuv_seq  input_seq;
    u32                     flags;

    /* Output format */
    enum dvp_output output;
    u32             width;
    u32             height;
    u32             stride[DVP_PLANE_NUM];
    u32             sizeimage[DVP_PLANE_NUM];
};

/* Some API of register, Defined in hal_dvp.c */
void aich_dvp_enable(int enable);
void aich_dvp_capture_start(void);
void aich_dvp_capture_stop(void);
void aich_dvp_clr_fifo(void);
int aich_dvp_clr_int(void);
void aich_dvp_enable_int(int enable);
void aich_dvp_set_pol(u32 flags);
void aich_dvp_set_cfg(struct aic_dvp_config *cfg);
void aich_dvp_update_buf_addr(dma_addr_t y, dma_addr_t uv);
void aich_dvp_update_ctl(void);
void aich_dvp_record_mode(void);
void aich_dvp_qos_cfg(u32 high, u32 low, u32 inc_thd, u32 dec_thd);

#endif /* _ARTINCHIP_HAL_DVP_H_ */
