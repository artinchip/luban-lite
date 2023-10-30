/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date         Author          Notes
 * 2018-08-15   misonyo         first implementation.
 * 2023-05-25   geo.dong        ArtInChip
 */

#include <string.h>
#include <rtthread.h>
#include <hwcrypto.h>
#include <hw_hash.h>
#include <hw_symmetric.h>
#include <aic_core.h>

#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')

static void cmd_test_ce_help(void)
{
    printf("test_ce command usage:\n");
    printf("  test_ce help : Get this help.\n");
    printf("  test_ce symm : Test symmetric algorithms.\n");
    printf("  test_ce hash : Test hash algorithms.\n");
    printf("  test_ce all  : Test all algorithms.\n");
    printf("  test_ce stability  : Test all algorithms stability.\n");
}

static void dump_hex(const rt_uint8_t *ptr, rt_size_t buflen)
{
    unsigned char *buf = (unsigned char *)ptr;
    int i, j;

    for (i = 0; i < buflen; i += 16) {
        rt_kprintf("%08X: ", i);

        for (j = 0; j < 16; j++) {
            if (i + j < buflen) {
                rt_kprintf("%02X ", buf[i + j]);
            } else {
                rt_kprintf("   ");
            }
        }
        rt_kprintf(" ");

        for (j = 0; j < 16; j++) {
            if (i + j < buflen) {
                rt_kprintf("%c", __is_print(buf[i + j]) ? buf[i + j] : '.');
            }
        }
        rt_kprintf("\n");
    }
}

static int hw_hash_sample()
{
    int i = 0;
    struct rt_hwcrypto_ctx *ctx = RT_NULL;
    const uint8_t hash_input[] = "RT-Thread was born in 2006, it is an open source, neutral, and community-based real-time operating system (RTOS).";

    static uint8_t sha1_output[20];
    static uint8_t sha1_except[20] = { 0xff, 0x3c, 0x95, 0x54, 0x95, 0xf0, 0xad,
                                       0x02, 0x1b, 0xa8, 0xbc, 0xa2, 0x2e, 0xa5,
                                       0xb0, 0x62, 0x1b, 0xdf, 0x7f, 0xec };

    static uint8_t md5_output[16];
    static uint8_t md5_except[16] = { 0x40, 0x86, 0x03, 0x80, 0x0d, 0x8c,
                                      0xb9, 0x4c, 0xd6, 0x7d, 0x28, 0xfc,
                                      0xf6, 0xc3, 0xac, 0x8b };

    static uint8_t sha224_output[28];
    static uint8_t sha224_except[28] = { 0x6f, 0x62, 0x52, 0x7d, 0x80, 0xe6,
                                         0x9f, 0x82, 0x78, 0x7a, 0x46, 0x91,
                                         0xb0, 0xe9, 0x64, 0x89, 0xe6, 0xc3,
                                         0x6b, 0x7e, 0xcf, 0xca, 0x11, 0x42,
                                         0xc8, 0x77, 0x13, 0x79 };
    static uint8_t sha256_output[32];
    static uint8_t sha256_except[32] = {
        0x74, 0x19, 0xb9, 0x0e, 0xd1, 0x46, 0x37, 0x0a, 0x55, 0x18, 0x26,
        0x6c, 0x50, 0xd8, 0x71, 0x34, 0xfa, 0x1f, 0x5f, 0x5f, 0xe4, 0x9a,
        0xe9, 0x40, 0x0a, 0x7d, 0xa0, 0x26, 0x1b, 0x86, 0x67, 0x45
    };

    static uint8_t sha384_output[48];
    static uint8_t sha384_except[48] = {
        0x3a, 0xfd, 0xae, 0xa2, 0x44, 0xa6, 0xbe, 0x11, 0x45, 0x14, 0xf3, 0x2f,
        0x5e, 0x6b, 0xc8, 0x31, 0x11, 0x4e, 0xab, 0x22, 0x09, 0xdf, 0xb6, 0x49,
        0x87, 0x94, 0xc1, 0x2e, 0xec, 0xb9, 0x74, 0xae, 0x35, 0xad, 0xd3, 0x25,
        0x38, 0x24, 0x8a, 0x18, 0xc6, 0x63, 0x50, 0x29, 0xa1, 0x09, 0x52, 0x71
    };

    static uint8_t sha512_output[64];
    static uint8_t sha512_except[64] = {
        0x44, 0xac, 0x32, 0x26, 0x1c, 0x21, 0x1d, 0x2b, 0x15, 0x62, 0x7e,
        0xe2, 0x25, 0x16, 0x11, 0xf8, 0x69, 0xa8, 0x4e, 0x3d, 0xd6, 0x14,
        0x26, 0x71, 0x52, 0x6d, 0xaf, 0x2c, 0x41, 0xe4, 0x90, 0x26, 0xab,
        0x29, 0x44, 0x97, 0x98, 0xce, 0xa5, 0x47, 0x7e, 0x7f, 0xb9, 0xb9,
        0x19, 0xc5, 0x93, 0x8b, 0xc6, 0xca, 0x8a, 0x94, 0x58, 0x46, 0x81,
        0x97, 0x48, 0xea, 0x06, 0x6c, 0xf7, 0x92, 0xc4, 0x06
    };

    rt_kprintf("Hash Test start: \n");
    rt_kprintf("Hash Test string: \n");
    for (i = 0; i < sizeof(hash_input); i++) {
        rt_kprintf("%c", hash_input[i]);
    }
    rt_kprintf("\n");

    /* sha1 test*/
    ctx = rt_hwcrypto_hash_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_SHA1);
    if (ctx == RT_NULL) {
        rt_kprintf("create hash[%08x] context err!\n", HWCRYPTO_TYPE_SHA1);
        return -1;
    }

    rt_kprintf("Create sha1 type success!\n");
    rt_kprintf("Except sha1 result: \n");
    for (i = 0; i < sizeof(sha1_except); i++) {
        rt_kprintf("%x ", sha1_except[i]);
    }
    rt_kprintf("\n");

    /* start sha1 */
    rt_hwcrypto_hash_update(ctx, hash_input, rt_strlen((char const *)hash_input));
    /* get sha1 result */
    rt_hwcrypto_hash_finish(ctx, sha1_output, rt_strlen((char const *)sha1_output));

    rt_kprintf("Actual sha1 result: \n");
    for (i = 0; i < sizeof(sha1_output); i++) {
        rt_kprintf("%x ", sha1_output[i]);
    }
    rt_kprintf("\n");

    if (rt_memcmp(sha1_output, sha1_except,
                  sizeof(sha1_except) / sizeof(sha1_except[0])) != 0) {
        rt_kprintf("Hash type sha1 Test error, The actual result is not equal to the except result\n");
        return -1;
    } else {
        rt_kprintf("Hash type sha1 Test success, The actual result is equal to the except result\n");
    }

    /* deinit hash*/
    rt_hwcrypto_hash_destroy(ctx);

    /* md5 test*/
    ctx = rt_hwcrypto_hash_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_MD5);
    if (ctx == RT_NULL) {
        rt_kprintf("create hash[%08x] context err!\n", HWCRYPTO_TYPE_MD5);
        return -1;
    }
    rt_kprintf("Create md5 type success!\n");
    rt_kprintf("Except md5 result: \n");
    for (i = 0; i < sizeof(md5_except); i++) {
        rt_kprintf("%x ", md5_except[i]);
    }
    rt_kprintf("\n");

    /* start md5 */
    rt_hwcrypto_hash_update(ctx, hash_input, rt_strlen((char const *)hash_input));
    /* get md5 result */
    rt_hwcrypto_hash_finish(ctx, md5_output, rt_strlen((char const *)md5_output));

    rt_kprintf("Actual md5 result: \n");
    for (i = 0; i < sizeof(md5_output); i++) {
        rt_kprintf("%x ", md5_output[i]);
    }
    rt_kprintf("\n");

    if (rt_memcmp(md5_output, md5_except,
                  sizeof(md5_except) / sizeof(md5_except[0])) != 0) {
        rt_kprintf("Hash type md5 Test error, The actual result is not equal to the except result\n");
        return -1;
    } else {
        rt_kprintf(
            "Hash type md5 Test success, The actual result is equal to the except result\n");
    }

    /* deinit hash*/
    rt_hwcrypto_hash_destroy(ctx);

    /* sha224 test */
    ctx = rt_hwcrypto_hash_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_SHA224);
    if (ctx == RT_NULL) {
        rt_kprintf("create hash[%08x] context err!\n", HWCRYPTO_TYPE_SHA224);
        return -1;
    }
    rt_kprintf("Create sha224 type success!\n");
    rt_kprintf("Except sha224 result: \n");
    for (i = 0; i < sizeof(sha224_except); i++) {
        rt_kprintf("%x ", sha224_except[i]);
    }
    rt_kprintf("\n");

    /* start sha224 */
    rt_hwcrypto_hash_update(ctx, hash_input, rt_strlen((char const *)hash_input));
    /* get sha224 result */
    rt_hwcrypto_hash_finish(ctx, sha224_output, rt_strlen((char const *)sha224_output));

    rt_kprintf("Actual sha224 result: \n");
    for (i = 0; i < sizeof(sha224_output); i++) {
        rt_kprintf("%x ", sha224_output[i]);
    }
    rt_kprintf("\n");

    if (rt_memcmp(sha224_output, sha224_except,
                  sizeof(sha224_except) / sizeof(sha224_except[0])) != 0) {
        rt_kprintf("Hash type sha224 Test error, The actual result is not equal to the except result\n");
        return -1;
    } else {
        rt_kprintf("Hash type sha224 Test success, The actual result is equal to the except result\n");
    }
    rt_hwcrypto_hash_destroy(ctx);

    /* sha256 test*/
    ctx = rt_hwcrypto_hash_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_SHA256);
    if (ctx == RT_NULL) {
        rt_kprintf("create hash[%08x] context err!\n", HWCRYPTO_TYPE_SHA256);
        return -1;
    }

    rt_kprintf("Create sha256 type success!\n");
    rt_kprintf("Except sha256 result: \n");
    for (i = 0; i < sizeof(sha256_except); i++) {
        rt_kprintf("%x ", sha256_except[i]);
    }
    rt_kprintf("\n");

    /* start sha256 */
    rt_hwcrypto_hash_update(ctx, hash_input, rt_strlen((char const *)hash_input));
    /* get sha256 result */
    rt_hwcrypto_hash_finish(ctx, sha256_output, rt_strlen((char const *)sha256_output));

    rt_kprintf("Actual sha256 result: \n");
    for (i = 0; i < sizeof(sha256_output); i++) {
        rt_kprintf("%x ", sha256_output[i]);
    }
    rt_kprintf("\n");

    if (rt_memcmp(sha256_output, sha256_except,
                  sizeof(sha256_except) / sizeof(sha256_except[0])) != 0) {
        rt_kprintf("Hash type sha256 Test error, The actual result is not equal to the except result\n");
        return -1;
    } else {
        rt_kprintf("Hash type sha256 Test success, The actual result is equal to the except result\n");
    }
    rt_hwcrypto_hash_destroy(ctx);

    /* sha384 test*/
    ctx = rt_hwcrypto_hash_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_SHA384);
    if (ctx == RT_NULL) {
        rt_kprintf("create hash[%08x] context err!\n", HWCRYPTO_TYPE_SHA384);
        return -1;
    }

    rt_kprintf("Create sha384 type success!\n");
    rt_kprintf("Except sha384 result: \n");
    for (i = 0; i < sizeof(sha384_except); i++) {
        rt_kprintf("%x ", sha384_except[i]);
    }
    rt_kprintf("\n");

    /* start sha384 */
    rt_hwcrypto_hash_update(ctx, hash_input, rt_strlen((char const *)hash_input));
    /* get sha384 result */
    rt_hwcrypto_hash_finish(ctx, sha384_output, rt_strlen((char const *)sha384_output));

    rt_kprintf("Actual sha384 result: \n");
    for (i = 0; i < sizeof(sha384_output); i++) {
        rt_kprintf("%x ", sha384_output[i]);
    }
    rt_kprintf("\n");

    if (rt_memcmp(sha384_output, sha384_except,
                  sizeof(sha384_except) / sizeof(sha384_except[0])) != 0) {
        rt_kprintf("Hash type sha384 Test error, The actual result is not equal to the except result\n\n");
        return -1;
    } else {
        rt_kprintf("Hash type sha384 Test success, The actual result is equal to the except result\n\n");
    }
    rt_hwcrypto_hash_destroy(ctx);

    /* sha512 test*/
    ctx = rt_hwcrypto_hash_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_SHA512);
    if (ctx == RT_NULL) {
        rt_kprintf("create hash[%08x] context err!\n", HWCRYPTO_TYPE_SHA512);
        return -1;
    }

    rt_kprintf("Create sha512 type success!\n");
    rt_kprintf("Except sha512 result: \n");
    for (i = 0; i < sizeof(sha512_except); i++) {
        rt_kprintf("%x ", sha512_except[i]);
    }
    rt_kprintf("\n");

    /* start sha512 */
    rt_hwcrypto_hash_update(ctx, hash_input, rt_strlen((char const *)hash_input));
    /* get sha512 result */
    rt_hwcrypto_hash_finish(ctx, sha512_output, rt_strlen((char const *)sha512_output));

    rt_kprintf("Actual sha512 result: \n");
    for (i = 0; i < sizeof(sha512_output); i++) {
        rt_kprintf("%x ", sha512_output[i]);
    }
    rt_kprintf("\n");

    if (rt_memcmp(sha512_output, sha512_except,
                  sizeof(sha512_except) / sizeof(sha512_except[0])) != 0) {
        rt_kprintf("Hash type sha512 Test error, The actual result is not equal to the except result\n\n");
        return -1;
    } else {
        rt_kprintf("Hash type sha512 Test success, The actual result is equal to the except result\n\n");
    }
    rt_hwcrypto_hash_destroy(ctx);

    rt_kprintf("Hash Test over!\n");

    return 0;
}

/* key */
static const rt_uint8_t cryp_key[16] = {
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
    0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF
};
static const rt_uint8_t cryp_iv[16] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                                        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

static int hw_aes_ecb(const rt_uint8_t in[32], rt_uint8_t out[32],
                       hwcrypto_mode mode)
{
    struct rt_hwcrypto_ctx *ctx;

    ctx = rt_hwcrypto_symmetric_create(rt_hwcrypto_dev_default(),
                                       HWCRYPTO_TYPE_AES_ECB);
    if (ctx == RT_NULL) {
        rt_kprintf("create AES-ECB context err!");
        return -1;
    }
    rt_hwcrypto_symmetric_setkey(ctx, cryp_key, 128);
    rt_hwcrypto_symmetric_crypt(ctx, mode, 32, in, out);
    rt_hwcrypto_symmetric_destroy(ctx);

    return 0;
}

static int hw_aes_cbc(const rt_uint8_t in[32], rt_uint8_t out[32],
                       hwcrypto_mode mode)
{
    struct rt_hwcrypto_ctx *ctx;

    ctx = rt_hwcrypto_symmetric_create(rt_hwcrypto_dev_default(),
                                       HWCRYPTO_TYPE_AES_CBC);
    if (ctx == RT_NULL) {
        rt_kprintf("create AES-CBC context err!");
        return -1;
    }
    rt_hwcrypto_symmetric_setkey(ctx, cryp_key, 128);
    rt_hwcrypto_symmetric_setiv(ctx, cryp_iv, 16);
    rt_hwcrypto_symmetric_crypt(ctx, mode, 32, in, out);
    rt_hwcrypto_symmetric_destroy(ctx);

    return 0;
}

static int hw_symm_sample()
{
    int i;
    rt_uint8_t buf[32];
    rt_uint8_t buf_in[32];
    rt_uint8_t buf_out[32];
    const uint8_t aes_ecb_except[32] = {
        0x0a, 0x94, 0x0b, 0xb5, 0x41, 0x6e, 0xf0, 0x45, 0xf1, 0xc3, 0x94,
        0x58, 0xc6, 0x53, 0xea, 0x5a, 0x07, 0xfe, 0xef, 0x74, 0xe1, 0xd5,
        0x03, 0x6e, 0x90, 0x0e, 0xee, 0x11, 0x8e, 0x94, 0x92, 0x93
    };
    const uint8_t aes_cbc_except[32] = {
        0x0a, 0x94, 0x0b, 0xb5, 0x41, 0x6e, 0xf0, 0x45, 0xf1, 0xc3, 0x94,
        0x58, 0xc6, 0x53, 0xea, 0x5a, 0x3c, 0xf4, 0x56, 0xb4, 0xca, 0x48,
        0x8a, 0xa3, 0x83, 0xc7, 0x9c, 0x98, 0xb3, 0x47, 0x97, 0xcb
    };

    /* Populating test data */
    for (i = 0; i < sizeof(buf); i++) {
        buf[i] = i;
    }
    rt_memcpy(buf_in, buf, sizeof(buf));

    /* dump primitive data */
    rt_kprintf("key : \n");
    dump_hex(cryp_key, sizeof(cryp_key));
    rt_kprintf("primitive data : \n");
    dump_hex(buf_in, sizeof(buf_in));

    /* AES CBC test*/
    rt_memset(buf_out, 0, sizeof(buf_out));

    /* encrypt */
    hw_aes_cbc(buf_in, buf_out, HWCRYPTO_MODE_ENCRYPT);
    /* dump encrypt data */
    rt_kprintf("AES-CBC-enc : \n");
    dump_hex(buf_out, sizeof(buf_out));

    if (rt_memcmp(buf_out, aes_cbc_except,
                  sizeof(aes_cbc_except) / sizeof(aes_cbc_except[0])) != 0) {
        rt_kprintf("AES-CBC-enc Test Failed!\n\n");
        return -1;
    } else {
        rt_kprintf("AES-CBC-enc Test Success!\n\n");
    }

    rt_memset(buf_in, 0, sizeof(buf_in));

    /* decrypt */
    hw_aes_cbc(buf_out, buf_in, HWCRYPTO_MODE_DECRYPT);

    /* dump decrypt data */
    rt_kprintf("AES-CBC-dec : \n");
    dump_hex(buf_in, sizeof(buf_in));

    if (rt_memcmp(buf_in, buf, sizeof(buf) / sizeof(buf[0])) != 0) {
        rt_kprintf("AES-CBC-dec Test Failed!\n\n");
        return -1;
    } else {
        rt_kprintf("AES-CBC-dec Test Success!\n\n");
    }

    /* AES ECB test*/
    rt_memset(buf_out, 0, sizeof(buf_out));

    /* encrypt */
    hw_aes_ecb(buf_in, buf_out, HWCRYPTO_MODE_ENCRYPT);
    /* dump encrypt data */
    rt_kprintf("AES-ECB-enc : \n");
    dump_hex(buf_out, sizeof(buf_out));

    if (rt_memcmp(buf_out, aes_ecb_except,
                  sizeof(aes_ecb_except) / sizeof(aes_ecb_except[0])) != 0) {
        rt_kprintf("AES-ECB-enc Test Failed!\n\n");
        return -1;
    } else {
        rt_kprintf("AES-ECB-enc Test Success!\n\n");
    }

    rt_memset(buf_in, 0, sizeof(buf_in));

    /* decrypt */
    hw_aes_ecb(buf_out, buf_in, HWCRYPTO_MODE_DECRYPT);

    /* dump decrypt data */
    rt_kprintf("AES-ECB-dec : \n");
    dump_hex(buf_in, sizeof(buf_in));

    if (rt_memcmp(buf_in, buf, sizeof(buf) / sizeof(buf[0])) != 0) {
        rt_kprintf("AES-ECB-dec Test Failed!\n\n");
        return -1;
    } else {
        rt_kprintf("AES-ECB-dec Test Success!\n\n");
    }

    return 0;
}

extern char rt_hw_console_getchar(void);
static void test_ce_do(int argc, char **argv)
{
    rt_device_t serial;
    char ch = 0;

    if (argc < 2) {
        return;
    }
    if (!strcmp(argv[1], "hash")) {
        hw_hash_sample();
        return;
    }
    if (!strcmp(argv[1], "symm")) {
        hw_symm_sample();
        return;
    }
    if (!strcmp(argv[1], "all")) {
        hw_hash_sample();
        hw_symm_sample();
        return;
    }
    if (!strcmp(argv[1], "stability")) {
        serial = rt_device_find(RT_CONSOLE_DEVICE_NAME);
        while(1) {
            if (hw_hash_sample())
                break;
            if (hw_symm_sample())
                break;
            if(rt_device_read(serial, -1, &ch, 1)){
                if (ch == ('C' - '@'))
                    break;
            }
        }
        return;
    }

    cmd_test_ce_help();
}

MSH_CMD_EXPORT_ALIAS(test_ce_do, test_ce, Test ce command);
