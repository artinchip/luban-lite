/*
* Copyright (C) 2020-2023 ArtInChip Technology Co. Ltd
*
*  author: <jun.ma@artinchip.com>
*  Desc: mpp zlip
*/

#ifndef __MPP_ZLIB_H__
#define __MPP_ZLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * mpp_zlib_uncompressed - uncompressed data
 * @param[in] compressed_data,compressed_data addr
 * @param[in] compressed_len
 * @param[in] uncompressed_data,uncompressed_data addr
 * @param[in] uncompressed_len,it must larger than or eq  real_uncompressed_len
 *
 * @return:return real_uncompressed_len if uncompressed ok,or other return -1;
 *
 * @note
 * The compressed_data addr should be aligned with 2^n bytes.  n = 4,5,6.....
 * compressed_len should be valid data len.
 * call aicos_dcache_clean_range to flush cache after fill data into compressed_data
 *
 * uncompressed_data addr should be aligned with 2^n bytes .n=3,4,5.....
 * uncompressed_len should be larger than or eq real_uncompressed_len and should be aligned with  2^n bytes. n= 3,4,5.....
 * call aicos_dcache_invalid_range to invalid cache before get data from uncompressed_data
 */
 int mpp_zlib_uncompressed(
        unsigned char *compressed_data,
        unsigned int compressed_len,
        unsigned char *uncompressed_data,
        unsigned int uncompressed_len);

#ifdef __cplusplus
}
#endif

#endif
