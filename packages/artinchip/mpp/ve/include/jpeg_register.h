/*
* Copyright (C) 2020-2022 Artinchip Technology Co. Ltd
*
*  author: author: <qi.xu@artinchip.com>
*  Desc: jpeg register define
*
*/

#ifndef JPEG_REGISTER_H
#define JPEG_REGISTER_H

#include "ve_top_register.h"

typedef struct
{
    unsigned start : 1;	// [0]: start
    unsigned r : 31;
} reg_jpg_start;

typedef struct
{
    unsigned finish : 1;
    unsigned error : 1;
    unsigned bit_req : 1;
    unsigned overflow : 1;
    unsigned r : 28;
} reg_jpg_status;

typedef struct
{
    unsigned start_pos_y : 12;	//start pos x
    unsigned r0 : 4;
    unsigned start_pos_x : 12;	//start pos y
    unsigned r1 : 4;
} reg_jpg_start_pos;

typedef struct
{
    unsigned cr_v_size : 2;	// cr: [3:2]:DU number in x, [1:0]: DU number in y
    unsigned cr_h_size : 2;
    unsigned cb_v_size : 2;	// cb: [7:6]:DU number in x, [5:4]: DU number in y
    unsigned cb_h_size : 2;
    unsigned y_v_size : 2; 	// y: [11:10]:DU number in x, [9:8]: DU number in y
    unsigned y_h_size : 2;
    unsigned comp_num : 3;
    unsigned r0 : 1;
    unsigned blk_num : 4;	// [19:16], total DU number in one MCU
    unsigned r : 12;
} reg_jpg_mcu_info;

typedef struct
{
    unsigned jpeg_vsize : 16; // vsize of picture, align with mcu width
    unsigned jpeg_hsize : 16; // hsize of picture, align with mcu height
} reg_jpg_size;

typedef struct
{
    unsigned finish : 1;  	// [0]: 0-finish interrupt enable
    unsigned error : 1; 	// [1]: 0-error interrupt enable
    unsigned bit_req : 1;   // [2]: 0: bit request interrupt enable
    unsigned r : 29;
} reg_jpg_intr_en;

typedef struct
{
    unsigned qmat_enable : 1;	// 1-enable this quant matrix
    unsigned qmat_auto : 1; 	// 1-auto address
    unsigned r0 : 4;
    unsigned qmat_idx : 2; 		// [7:6]: 0-Y; 1-Cb; 2-Cr
    unsigned r1 : 24;
} reg_jpg_qmat_info;

typedef struct
{
    unsigned qmat_addr : 6; // address, the order is zigzag scan, it is also the order parser from DQT
    unsigned qmat_idx : 2;  // [7:6]: 0-Y; 1-Cb; 2-Cr
    unsigned r1 : 24;
} reg_jpg_qmat_addr;

typedef struct
{
    unsigned enable : 1; 	// 1-enable this huffman table
    unsigned huff_auto : 1; // 1-auto address
    unsigned r0 : 8;
    unsigned huff_idx : 2; 	// [11:10]: 0-huff_min; 1-huff_max; 2- huff_ptr; 3-huff_val
    unsigned r1 : 20;
} reg_jpg_huff_info;

typedef struct
{
    unsigned rotate : 2;    // [1:0]: 1-L90; 2-L180; 3-L270
    unsigned mirror : 2;    // [3:2]: 1-VM; 2-HM; 11-VM&HM
    unsigned rotmir_en : 1; // [4] enable
    unsigned r : 27;
} reg_jpg_rotmir;

typedef struct
{
    unsigned v_ratio : 2;  // [1:0] vertical scale
    unsigned h_ratio : 2;  // [3:2] horizontal scale
    unsigned scale_en : 1; // [4] enable
    unsigned r : 27;
} reg_jpg_scale;

typedef struct
{
    unsigned enc_clip_en : 1;
    unsigned dec_clip_en : 1;
    unsigned r : 30;
} reg_jpg_clip;

typedef struct
{
    /* start address of write huff table,
    1) if huff_idx is 0/1/2, start address is
        AC_LUMA: 0
        AC_CHROMA:16;
        DC_LUMA:32;
        DC_CHROMA:48
    2) if huff_idx is 3
        DC_LUMA:0;
        DC_CHROMA:12
        AC_LUMA: 24
        AC_CHROMA:186;
    */
    unsigned huff_addr : 10;
    unsigned huff_idx : 2; 	// [11:10]: huffman table idx(0,1,2,3)
    unsigned r1 : 20;
} reg_jpg_huff_addr;

#ifdef AIC_VE_DRV_V10

#define JPG_START_REG 		(JPG_REG_OFFSET_ADDR + 0x00)
#define JPG_STATUS_REG 		(JPG_REG_OFFSET_ADDR + 0x04)
#define JPG_START_POS_REG 	(JPG_REG_OFFSET_ADDR + 0x0c)

typedef struct
{
	unsigned r : 3;
	unsigned encode : 1; 		// [3]: 1-encode; 0-decode
	unsigned dir : 1; 		// [4]: 1-used for encode mode; 0-used for decode mode
	unsigned r0 : 1;
	unsigned use_huff_en : 1; 	// [6]: 0:use default huffman table
	unsigned r1 : 1;
	unsigned dc_huff_idx : 3; 	// [8:10]: ac coeff huffman table index
	unsigned r2 : 1;
	unsigned ac_huff_idx : 3; 	// [14: 12]: dc coeff huffman table index
	unsigned r3 : 17;
} reg_jpg_ctrl;
#define JPG_CTRL_REG 		(JPG_REG_OFFSET_ADDR + 0x10)

#define JPG_SIZE_REG 		(JPG_REG_OFFSET_ADDR + 0x14)
#define JPG_MCU_INFO_REG 	(JPG_REG_OFFSET_ADDR + 0x18)
#define JPG_ROTMIR_REG 		(JPG_REG_OFFSET_ADDR + 0x1C)
#define JPG_SCALE_REG 		(JPG_REG_OFFSET_ADDR + 0x20)
#define JPG_CLIP_REG 		(JPG_REG_OFFSET_ADDR + 0x28)

typedef struct
{
	unsigned handle_mcu_num : 3; // handle mcu number
	unsigned r : 29;
} reg_buf_num;
#define JPG_HANDLE_NUM_REG 	(JPG_REG_OFFSET_ADDR + 0x2C)

// uv interleave
#define JPG_UV_REG 		(JPG_REG_OFFSET_ADDR + 0x30)

// frame idx in picture info
#define JPG_FRAME_IDX_REG 	(JPG_REG_OFFSET_ADDR + 0x40)
#define JPG_HUFF_INFO_REG 	(JPG_REG_OFFSET_ADDR + 0x80)
#define JPG_HUFF_ADDR_REG 	(JPG_REG_OFFSET_ADDR + 0x84)
#define JPG_HUFF_VAL_REG 	(JPG_REG_OFFSET_ADDR + 0x88)

#define JPG_QMAT_INFO_REG 	(JPG_REG_OFFSET_ADDR + 0x90)
#define JPG_QMAT_ADDR_REG 	(JPG_REG_OFFSET_ADDR + 0x94)
#define JPG_QMAT_VAL_REG 	(JPG_REG_OFFSET_ADDR + 0x98)

#define JPG_RST_INTERVAL_REG 	(JPG_REG_OFFSET_ADDR + 0xb0)
#define JPG_INTRRUPT_EN_REG 	(JPG_REG_OFFSET_ADDR + 0xc0)
#define JPG_CYCLES_REG 		(JPG_REG_OFFSET_ADDR + 0xc8)

#define JPG_SUB_CTRL_REG 	(JPG_REG_OFFSET_ADDR + 0x100)
#define JPG_WD_PTR 		(JPG_REG_OFFSET_ADDR + 0x114)
// start address of internal memory, defalut 0
#define JPG_MEM_SA_REG 		(JPG_REG_OFFSET_ADDR + 0x140)

// end address of internal memory, defalut 0x7f
#define JPG_MEM_EA_REG 		(JPG_REG_OFFSET_ADDR + 0x144)

// default 0
#define JPG_MEM_IA_REG 		(JPG_REG_OFFSET_ADDR + 0x148)
#define JPG_MEM_HA_REG 		(JPG_REG_OFFSET_ADDR + 0x14C)
#define JPG_RBIT_OFFSET_REG 	(JPG_REG_OFFSET_ADDR + 0x160)
#define JPG_WBIT_OFFSET_REG 	(JPG_REG_OFFSET_ADDR + 0x164)
#define JPG_REQ_REG 		(JPG_REG_OFFSET_ADDR + 0x200)

// end address in DRAM of bitstream, 256 byte align
#define JPG_STREAM_END_ADDR_REG (JPG_REG_OFFSET_ADDR + 0x208)

// write ptr, default 0
#define JPG_STREAM_WRITE_PTR_REG (JPG_REG_OFFSET_ADDR + 0x20C)
// read ptr, default 0
#define JPG_STREAM_READ_PTR_REG (JPG_REG_OFFSET_ADDR + 0x210)

// start address in DRAM of bitstream, 256 byte align
#define JPG_STREAM_START_ADDR_REG (JPG_REG_OFFSET_ADDR + 0x214)

// default 0
#define JPG_STREAM_INT_ADDR_REG	(JPG_REG_OFFSET_ADDR + 0x218)

// read data number, default 64
#define JPG_DATA_CNT_REG 	(JPG_REG_OFFSET_ADDR + 0x21c)

#define JPG_COMMAND_REG 	(JPG_REG_OFFSET_ADDR + 0x220)

// read data busy status (Read Only)
#define JPG_BUSY_REG 		(JPG_REG_OFFSET_ADDR + 0x224)

// not request enable
#define JPG_BITREQ_EN_REG 	(JPG_REG_OFFSET_ADDR + 0x228)

// current stream pos ( one stream is 256 byte data)
#define JPG_CUR_POS_REG 	(JPG_REG_OFFSET_ADDR + 0x22c)

// bas address
#define JPG_BAS_ADDR_REG 	(JPG_REG_OFFSET_ADDR + 0x230)

// total stream number ( one stream is 256 byte data)
#define JPG_STREAM_NUM_REG 	(JPG_REG_OFFSET_ADDR + 0x234)

#else  // ifndef AIC_VE_DRV_V10

/************* V3.0 later register define *************/
#define JPG_START_REG 		(JPG_REG_OFFSET_ADDR + 0x00)
#define JPG_STATUS_REG 		(JPG_REG_OFFSET_ADDR + 0x04)
#define JPG_INTRRUPT_EN_REG 	(JPG_REG_OFFSET_ADDR + 0x08)
#define JPG_START_POS_REG 	(JPG_REG_OFFSET_ADDR + 0x10)
#define JPG_SIZE_REG 		(JPG_REG_OFFSET_ADDR + 0x14)

// it is not same with v1.0
typedef struct
{
	unsigned r : 3;
	unsigned encode : 1; 		// [3]: 1-encode; 0-decode
	unsigned dir : 1; 		    // [4]: 1-used for encode mode; 0-used for decode mode
	unsigned use_huff_en : 1; 	// [5]: 0:use default huffman table
    unsigned r0 : 1;
	unsigned r1 : 1;
	unsigned dc_huff_idx : 3; 	// [8:10]: ac coeff huffman table index
	unsigned r2 : 1;
	unsigned ac_huff_idx : 3; 	// [14: 12]: dc coeff huffman table index
	unsigned r3 : 17;
} reg_jpg_ctrl;
#define JPG_CTRL_REG 		(JPG_REG_OFFSET_ADDR + 0x18)

#define JPG_MCU_INFO_REG 	(JPG_REG_OFFSET_ADDR + 0x1C)

typedef struct
{
	unsigned handle_mcu_num : 2; // handle mcu number minus 1
	unsigned r : 30;
} reg_buf_num;
#define JPG_HANDLE_NUM_REG 	(JPG_REG_OFFSET_ADDR + 0x20)
#define JPG_QMAT_INFO_REG 	(JPG_REG_OFFSET_ADDR + 0x24)
#define JPG_QMAT_ADDR_REG 	(JPG_REG_OFFSET_ADDR + 0x28)
#define JPG_QMAT_VAL_REG 	(JPG_REG_OFFSET_ADDR + 0x2C)

#define JPG_HUFF_INFO_REG 	(JPG_REG_OFFSET_ADDR + 0x30)
#define JPG_HUFF_ADDR_REG 	(JPG_REG_OFFSET_ADDR + 0x34)
#define JPG_HUFF_VAL_REG 	(JPG_REG_OFFSET_ADDR + 0x38)

#define JPG_CLIP_REG 		(JPG_REG_OFFSET_ADDR + 0x3C)

typedef struct
{
	unsigned clip_ybase : 16;
	unsigned clip_xbase : 16;
} reg_jpg_clip_base;
#define JPG_CLIP_BASE_REG 	(JPG_REG_OFFSET_ADDR + 0x40)

typedef struct
{
	unsigned clip_ysize : 16;
	unsigned clip_xsize : 16;
} reg_jpg_clip_size;
#define JPG_CLIP_SIZE_REG 	(JPG_REG_OFFSET_ADDR + 0x44)

typedef struct
{
	unsigned clip_ext_ybase : 16;
	unsigned clip_ext_xbase : 16;
} reg_jpg_clip_out_base;
#define JPG_CLIP_OUT_BASE_REG (JPG_REG_OFFSET_ADDR + 0x48)

#define JPG_ROTMIR_REG 		(JPG_REG_OFFSET_ADDR + 0x4C)
#define JPG_SCALE_REG 		(JPG_REG_OFFSET_ADDR + 0x50)

typedef struct
{
	unsigned enable : 1;        //[0]: enable yuv2rgb
	unsigned rgb_format : 3;    // rgb format
	unsigned r : 4;
	unsigned alpha : 8;         // alpha value
	unsigned r1 : 16;
}reg_jpg_rgb;
#define JPG_RGB_REG         (JPG_REG_OFFSET_ADDR + 0x54)

#define JPG_RST_INTERVAL_REG (JPG_REG_OFFSET_ADDR + 0x5C)

// frame idx in picture info
#define JPG_FRAME_IDX_REG 	(JPG_REG_OFFSET_ADDR + 0x64)

// uv interleave
#define JPG_UV_REG 		    (JPG_REG_OFFSET_ADDR + 0x68)

#define JPG_CYCLES_REG 		(JPG_REG_OFFSET_ADDR + 0x6C)

typedef struct
{
	unsigned left_weight_minus1 : 3;    // default 6:
	unsigned up_weight_minus1 : 3;      // default 5:
	unsigned leftup_weight_minus1 : 3;  // default 2:
	unsigned r : 23;
}reg_dither_param;
#define JPG_DITHER_PARAM_REG  (JPG_REG_OFFSET_ADDR+0x8c)

#define JPG_SUB_CTRL_REG 	(JPG_REG_OFFSET_ADDR + 0x100)

// default 0
#define JPG_MEM_IA_REG 		(JPG_REG_OFFSET_ADDR + 0x104)
#define JPG_MEM_HA_REG 		(JPG_REG_OFFSET_ADDR + 0x108)
// bit read offset
#define JPG_RBIT_OFFSET_REG (JPG_REG_OFFSET_ADDR + 0x11C)
#define JPG_WBIT_OFFSET_PTR (JPG_REG_OFFSET_ADDR + 0x120) // not used
// start address of internal memory, defalut 0
#define JPG_MEM_SA_REG 		(JPG_REG_OFFSET_ADDR + 0x124)
// end address of internal memory, defalut 0x7f
#define JPG_MEM_EA_REG 		(JPG_REG_OFFSET_ADDR + 0x128)

// not request enable
#define JPG_BITREQ_EN_REG 	(JPG_REG_OFFSET_ADDR + 0x180)
// total stream number ( one stream is 256 byte data)
#define JPG_STREAM_NUM_REG 	(JPG_REG_OFFSET_ADDR + 0x184)

// read data number, default 64
#define JPG_DATA_CNT_REG 	(JPG_REG_OFFSET_ADDR + 0x18C)
// read data busy status (Read Only)
#define JPG_BUSY_REG 		(JPG_REG_OFFSET_ADDR + 0x190)

#define JPG_REQ_REG 		(JPG_REG_OFFSET_ADDR + 0x194)

// end address in DRAM of bitstream, 256 byte align
#define JPG_STREAM_END_ADDR_REG (JPG_REG_OFFSET_ADDR + 0x198)
// start address in DRAM of bitstream, 8 byte align
#define JPG_STREAM_START_ADDR_REG (JPG_REG_OFFSET_ADDR + 0x19C)
// bas address
#define JPG_BAS_ADDR_REG 	    (JPG_REG_OFFSET_ADDR + 0x1A0)

#define JPG_STREAM_WRITE_PTR_REG (JPG_REG_OFFSET_ADDR+0x1A4)
// read ptr, default 0
#define JPG_STREAM_READ_PTR_REG (JPG_REG_OFFSET_ADDR + 0x1A8)
// current stream pos ( one stream is 256 byte data)
#define JPG_CUR_POS_REG 	    (JPG_REG_OFFSET_ADDR + 0x1B0)
// default 0
#define JPG_STREAM_INT_ADDR_REG	(JPG_REG_OFFSET_ADDR + 0x1B4)

#endif

#endif
