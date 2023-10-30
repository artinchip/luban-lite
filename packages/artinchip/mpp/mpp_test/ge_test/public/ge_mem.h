#ifndef GE_MEM_CAL_H
#define GE_MEM_CAL_H

#include "mpp_types.h"

#define BYTE_ALIGN(x, byte) (((x) + ((byte) - 1))&(~((byte) - 1)))

struct ge_buf {
    int use_buf_size[3];
    void *ori_buf[3]; /* unaligned memory */
    struct mpp_buf buf;
};

struct ge_buf * ge_buf_malloc(int width, int height, enum mpp_pixel_format fmt);
void ge_buf_free(struct ge_buf * buffer);
void ge_buf_clean_dcache(struct ge_buf * buffer);

#endif
