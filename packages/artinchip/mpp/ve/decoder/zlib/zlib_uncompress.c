/*
* Copyright (C) 2020-2022 Artinchip Technology Co. Ltd
*
*  author: <jun.ma@artinchip.com>
*  Desc: zlib_uncompress interface
*/

#include <stddef.h>
#include "ve.h"
#include "ve_top_register.h"
#include "../png/png_hal.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "aic_core.h"

#ifdef AIC_VE_DRV_V10
#define USE_IRQ (0)
#else
#define USE_IRQ (1)
#endif

static int get_gzip_header(unsigned char* src)
{
    return 2;
}

static void config_ve_top(unsigned long reg_base)
{
    write_reg_u32(reg_base + VE_CLK_REG, 1);
    write_reg_u32(reg_base + VE_RST_REG, 0);

    while ((read_reg_u32(reg_base + VE_RST_REG) >> 16)) {
    }

    write_reg_u32(reg_base + VE_INIT_REG, 1);
    write_reg_u32(reg_base + VE_IRQ_REG, USE_IRQ);
    write_reg_u32(reg_base + VE_PNG_EN_REG, 1);
}

 int mpp_zlib_uncompressed(
        unsigned char *compressed_data,
        unsigned int compressed_len,
        unsigned char *uncompressed_data,
        unsigned int uncompressed_len)
{
    int ret = 0;
    int real_uncompress_len = -1;
    int offset = 0;
    unsigned int status;
    unsigned long reg_base;
    unsigned long lz77_buf;
    unsigned long lz77_buf_align_8;

    if (compressed_data  == NULL
        || compressed_len ==0
        || uncompressed_data == NULL
        || uncompressed_len == 0 ) {
            loge("param error!!!\n");
            return -1;
    }
    ret = ve_open_device();
    if (ret != 0) {
        loge("ve_open_device error!!!\n");
        return -1;
    }

    lz77_buf = (unsigned long)aicos_malloc(MEM_CMA, (32*1024+7));

    if (lz77_buf == 0) {
        loge("mpp_alloc error!!!\n");
        ve_close_device();
        return -1;
    }

    lz77_buf_align_8 = ((lz77_buf+7)&(~7));
    logd("lz77_buf:0x%lx,lz77_buf_align_8:0x%lx\n", lz77_buf, lz77_buf_align_8);

    offset = get_gzip_header(compressed_data);
    logd("offset:%d\n",offset);
    reg_base = ve_get_reg_base();
    ve_get_client();
    config_ve_top(reg_base);
    write_reg_u32(reg_base+PNG_CTRL_REG, 0);
    write_reg_u32(reg_base+OUTPUT_BUFFER_ADDR_REG, (unsigned long)uncompressed_data);
    write_reg_u32(reg_base+OUTPUT_BUFFER_LENGTH_REG, (unsigned long)uncompressed_data + uncompressed_len -1);
    write_reg_u32(reg_base+INFLATE_WINDOW_BUFFER_ADDR_REG, lz77_buf_align_8);
    write_reg_u32(reg_base+INFLATE_INTERRUPT_REG, 15);
    write_reg_u32(reg_base+INFLATE_STATUS_REG, 15);
    write_reg_u32(reg_base+INFLATE_START_REG, 1);
    write_reg_u32(reg_base+INPUT_BS_START_ADDR_REG, (unsigned long)compressed_data);
    write_reg_u32(reg_base+INPUT_BS_END_ADDR_REG, (unsigned long)compressed_data+compressed_len-1);
    write_reg_u32(reg_base+INPUT_BS_OFFSET_REG, offset*8);
    write_reg_u32(reg_base+INPUT_BS_LENGTH_REG, (compressed_len-offset)*8);
    write_reg_u32(reg_base+INPUT_BS_DATA_VALID_REG, 0x80000003);

#if USE_IRQ
    if (ve_wait(&status) < 0) {
        loge("timeout");
        goto _exit;
    }
    if (status & PNG_ERROR) {
        loge("decode error, status: %08x", status);
        goto _exit;
    } else if (status & PNG_FINISH) {
        logd("decode ok, status: %08x", status);
    } else {
        loge("decode error, status: %08x", status);
        goto _exit;
    }
#else
    while ((read_reg_u32(reg_base + OUTPUT_COUNT_REG) != uncompressed_len)
        && ((read_reg_u32(reg_base + INFLATE_STATE_REG) & 0x7800) != 0x800)) {
            status = read_reg_u32(reg_base + INFLATE_STATUS_REG);
            if (status & PNG_ERROR) {
                goto _exit;
            }

            usleep(1000);
    }
#endif

    real_uncompress_len = read_reg_u32(reg_base+OUTPUT_COUNT_REG);
    logd("real_uncompress_len:%d\n",real_uncompress_len);

_exit:
    ve_put_client();
    ve_close_device();
    if (lz77_buf) {
        aicos_free(MEM_CMA, (void *)lz77_buf);
    }
    return real_uncompress_len;
}
