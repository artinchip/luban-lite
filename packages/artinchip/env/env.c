/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <rtconfig.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <aic_core.h>
#include <env.h>

#ifndef KERNEL_BAREMETAL

#include <rtthread.h>
#include <rtdevice.h>

#ifdef AIC_SPINOR_DRV
#include <fal.h>
#endif

#else

#include <console.h>

#if (defined (AIC_SPINOR_DRV) || defined (AIC_SPINAND_DRV))
#include <mtd.h>
#endif

#endif

#define O_RDONLY (00000000)
#define O_WRONLY (00000001)
#define O_RDWR   (00000002)

#define CONFIG_EXTRA_ENV_SETTINGS \
    "upgrade_available=0\0"       \
    "bootlimit=5\0"               \
    "bootcount=0\0"               \
    "bootdelay=0\0"               \
    "osAB_next=A\0"               \
    "osAB_now=A\0"

static char default_environment[] = {
#ifdef CONFIG_EXTRA_ENV_SETTINGS
    CONFIG_EXTRA_ENV_SETTINGS
#endif
    "\0"
};

#ifdef AIC_SYS_REDUNDAND_ENVIRONMENT
static int have_redund_env = 1;
#else
static int have_redund_env = 0;
#endif

static unsigned char active_flag = 1;
/* obsolete_flag must be 0 to efficiently set it on NOR flash without erasing */
static unsigned char obsolete_flag = 0;
static unsigned long usable_envsize;

static int dev_current;

static int bd = 0; //boot device

struct env_image_single {
    uint32_t crc; /* CRC32 over data bytes    */
    char data[];
};

struct env_image_redundant {
    uint32_t crc;        /* CRC32 over data bytes    */
    unsigned char flags; /* active or obsolete */
    char data[];
};

enum flag_scheme {
    FLAG_NONE,
    FLAG_BOOLEAN,
    FLAG_INCREMENTAL,
};

struct environment {
    void *image;
    uint32_t *crc;
    unsigned char *flags;
    char *data;
    enum flag_scheme flag_scheme;
};

static struct environment environment = {
    .flag_scheme = FLAG_NONE,
};

static int flash_io(int mode);

#define SET_ENV_HELP                                   \
    "Usage: fw_setenv name value\n"                    \
    "Modify variables in U-Boot environment\n"         \
    "Need to run cmd fw_setbd in advance\n"            \
    "Examples:\n"                                      \
    "fw_setenv foo bar   set variable foo equal bar\n" \
    "fw_setenv foo       clear variable foo\n"

#ifndef KERNEL_BAREMETAL
static aicos_mutex_t lock_env = NULL;

static int env_lock(void)
{
    if (aicos_mutex_take(lock_env, AICOS_WAIT_FOREVER) == 0)
        return 0;
    else
        return -1;
}

static int env_unlock(void)
{
    if (lock_env != NULL) {
        aicos_mutex_give(lock_env);
        return 0;
    } else {
        return -1;
    }
}

static int env_lock_init(void)
{
    if (lock_env == NULL) {
        lock_env = aicos_mutex_create();
        if (lock_env == NULL)
            return -1;
    }

    return 0;
}
#endif

static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

static uint32_t crc32_0(const uint8_t *data, uint32_t length)
{
    register uint32_t crc = 0xFFFFFFFF;

    while (length--) {
        crc = ((crc >> 8) & 0x00FFFFFF) ^
              crc32_table[(crc ^ *data++) & 0x000000FF];
    }

    return (crc ^ 0xFFFFFFFF);
}

static uint32_t crc32(uint32_t crc, const uint8_t *buf, uint32_t len)
{
    return crc32_0(buf, len);
}

/*
 * s1 is either a simple 'name', or a 'name=value' pair.
 * s2 is a 'name=value' pair.
 * If the names match, return the value of s2, else NULL.
 */
static char *envmatch(char *s1, char *s2)
{
    if (s1 == NULL || s2 == NULL)
        return NULL;

    while (*s1 == *s2++)
        if (*s1++ == '=')
            return s2;
    if (*s1 == '\0' && *(s2 - 1) == '=')
        return s2;
    return NULL;
}

/**
 * Search the environment for a variable.
 * Return the value, if found, or NULL, if not found.
 */
char *fw_getenv(char *name)
{
    char *env, *nxt;

    for (env = environment.data; *env; env = nxt + 1) {
        char *val;

        for (nxt = env; *nxt; ++nxt) {
            if (nxt >= &environment.data[usable_envsize]) {
                pr_err("Environment not terminated\n");
                return NULL;
            }
        }
        val = envmatch(name, env);
        if (!val)
            continue;
        return val;
    }
    return NULL;
}

/*
 * Set/Clear a single variable in the environment.
 * This is called in sequence to update the environment
 * in RAM without updating the copy in flash after each set
 */
int fw_env_write(char *name, char *value)
{
    int len;
    char *env, *nxt;
    char *oldval = NULL;
    int deleting, creating, overwriting;

    /*
	 * search if variable with this name already exists
	 */
    for (nxt = env = environment.data; *env; env = nxt + 1) {
        for (nxt = env; *nxt; ++nxt) {
            if (nxt >= &environment.data[usable_envsize]) {
                pr_err("Environment not terminated\n");
                /* errno = EINVAL; */
                return -1;
            }
        }
        oldval = envmatch(name, env);
        if (oldval)
            break;
    }

    deleting = (oldval && !(value && strlen(value)));
    creating = (!oldval && (value && strlen(value)));
    overwriting = (oldval && (value && strlen(value)));

    if (deleting) {
        printf("Env: delting\n");
    } else if (overwriting) {
        pr_debug("Env: overwriting\n");
    } else if (creating) {
        printf("Env: creating\n");
    } else {
        printf("Env: nothing\n");
        return 0;
    }

    if (deleting || overwriting) {
        if (*++nxt == '\0') {
            *env = '\0';
        } else {
            for (;;) {
                *env = *nxt++;
                if ((*env == '\0') && (*nxt == '\0'))
                    break;
                ++env;
            }
        }
        *++env = '\0';
    }

    /* Delete only ? */
    if (!value || !strlen(value))
        return 0;

    /*
	 * Append new definition at the end
	 */
    for (env = environment.data; *env || *(env + 1); ++env)
        continue;

    if (env > environment.data)
        ++env;
    /*
	 * Overflow when:
	 * "name" + "=" + "val" +"\0\0"  > AIC_ENV_SIZE - (env-environment)
	 */
    len = strlen(name) + 2;
    /* add '=' for first arg, ' ' for all others */
    len += strlen(value) + 1;

    if (len > (&environment.data[usable_envsize] - env)) {
        pr_err("Environment overflow, \"%s\" deleted\n", name);
        return -1;
    }

    while ((*env = *name++) != '\0')
        env++;
    *env = '=';
    while ((*++env = *value++) != '\0')
        continue;

    /* end is marked with double '\0' */
    *++env = '\0';

    return 0;
}

int fw_env_flush(void)
{
    /*
	 * Update CRC
	 */
    *environment.crc = crc32(0, (uint8_t *)environment.data, usable_envsize);

    /* write environment back to flash */
    if (flash_io(O_RDWR)) {
        pr_err("Can't write fw_env to flash\n");
        return -1;
    }

    return 0;
}

#ifdef AIC_SPINOR_DRV
#ifndef KERNEL_BAREMETAL
static int rtt_spinor_load_env_simple(void *buf, size_t size)
{
    const struct fal_partition *env_current;

    if (dev_current == 0) {
        env_current = fal_partition_find(AIC_ENV_PART_NAME);
    } else if (dev_current == 1) {
        env_current = fal_partition_find(AIC_ENV_REDUNDAND_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (env_current == RT_NULL) {
        pr_err("Not found dev_current:%d\n", dev_current);
        return -1;
    }

    if (fal_partition_read(env_current, 0, buf, size) != size) {
        pr_err("Fal read env fail\n");
        return -1;
    }

    return 0;
}

static int rtt_spinor_save_env_simple(void *buf, size_t size)
{
    const struct fal_partition *env_current;

    if (dev_current == 0) {
        env_current = fal_partition_find(AIC_ENV_REDUNDAND_PART_NAME);
    } else if (dev_current == 1) {
        env_current = fal_partition_find(AIC_ENV_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (env_current == RT_NULL) {
        pr_err("Not found dev_current:%d\n", dev_current);
        return -1;
    }

    if (fal_partition_erase(env_current, 0, AIC_ENV_SIZE) < 0) {
        pr_err("Fal erase env fail\n");
        return -1;
    }

    if (fal_partition_write(env_current, 0, buf, size) != size) {
        pr_err("Fal write env fail\n");
        return -1;
    }

    return 0;
}
#else
static int bar_spinor_load_env_simple(void *buf, size_t size)
{
    struct mtd_dev *env_current;

    if (dev_current == 0) {
        env_current = mtd_get_device(AIC_ENV_PART_NAME);
    } else if (dev_current == 1) {
        env_current = mtd_get_device(AIC_ENV_REDUNDAND_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (env_current == NULL) {
        pr_err("Not found dev_current:%d\n", dev_current);
        return -1;
    }

    if (mtd_read(env_current, 0, buf, size) < 0) {
        pr_err("Read env fail\n");
        return -1;
    }

    return 0;
}

static int bar_spinor_save_env_simple(void *buf, size_t size)
{
    struct mtd_dev *env_current;

    if (dev_current == 0) {
        env_current = mtd_get_device(AIC_ENV_REDUNDAND_PART_NAME);
    } else if (dev_current == 1) {
        env_current = mtd_get_device(AIC_ENV_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (env_current == NULL) {
        pr_err("Not found dev_current:%d\n", dev_current);
        return -1;
    }

    if (mtd_erase(env_current, 0, AIC_ENV_SIZE)) {
        pr_err("Mtd erase env fail\n");
        return -1;
    }

    if (mtd_write(env_current, 0, buf, size)) {
        pr_err("Mtd write env fail\n");
        return -1;
    }

    return 0;
}
#endif
#endif

#ifdef AIC_SPINAND_DRV
#ifndef KERNEL_BAREMETAL
static int mtd_is_block_aligned(rt_off_t page, rt_uint32_t ppb)
{
    if (page & (ppb - 1))
        return 0;

    return 1;
}

static int rtt_spinand_load_env_simple(void *buf, size_t size)
{
    struct rt_mtd_nand_device *mtd;
    rt_device_t dev;
    rt_err_t ret;
    rt_off_t offset = 0;
    rt_uint32_t page_id = 0, blk = 0, remain = 0;

    if (dev_current == 0) {
        dev = rt_device_find(AIC_ENV_PART_NAME);
    } else if (dev_current == 1) {
        dev = rt_device_find(AIC_ENV_REDUNDAND_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -RT_ERROR;
    }

    if (dev == RT_NULL) {
        pr_err("Not found dev_current:%d\n", dev_current);
        return -RT_ERROR;
    }

    ret = rt_device_open(dev, RT_DEVICE_OFLAG_RDWR);
    if (ret) {
            pr_err("Open MTD env failed.!\n");
            return ret;
    }

    mtd = (struct rt_mtd_nand_device *)dev;
    remain = size;

    while (remain) {
        page_id = offset / mtd->page_size;

        if (page_id > (mtd->block_total * mtd->pages_per_block)) {
            pr_err("All blocks in the part is bad!\n");
            ret = -RT_ERROR;
            goto rtt_spinand_load_env_simple_exit;
        }

        blk = page_id / mtd->pages_per_block;

        if (mtd_is_block_aligned(page_id, mtd->pages_per_block) &&
            rt_mtd_nand_check_block(mtd, blk) != RT_EOK) {
            pr_err("Block is bad, skip it.\n");
            offset += mtd->pages_per_block * mtd->page_size;
            continue;
        }

        ret = rt_mtd_nand_read(mtd, page_id, buf, mtd->page_size, RT_NULL, 0);
        if (ret) {
            pr_err("Failed to read page data from NAND.\n");
            ret = -RT_ERROR;
            goto rtt_spinand_load_env_simple_exit;
        }

        buf += mtd->page_size;
        offset += mtd->page_size;
        if (remain >= mtd->page_size)
            remain -= mtd->page_size;
        else
            remain = 0;
    }

rtt_spinand_load_env_simple_exit:
    rt_device_close(dev);

    return ret;
}

static int rtt_spinand_save_env_simple(void *buf, size_t size)
{
    struct rt_mtd_nand_device *mtd;
    rt_device_t dev;
    rt_err_t ret = 0;
    rt_off_t offset = 0;
    rt_uint32_t page_id = 0, blk = 0, remain = 0, goodblk = 0;

    if (dev_current == 0) {
        dev = rt_device_find(AIC_ENV_REDUNDAND_PART_NAME);
    } else if (dev_current == 1) {
        dev = rt_device_find(AIC_ENV_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -RT_ERROR;
    }

    if (dev == RT_NULL) {
        pr_err("Not found dev_current:%d\n", dev_current);
        return -RT_ERROR;
    }

    ret = rt_device_open(dev, RT_DEVICE_OFLAG_RDWR);
    if (ret) {
            pr_err("Open MTD env failed.!\n");
            return ret;
    }

    mtd = (struct rt_mtd_nand_device *)dev;
    remain = size;

    while (!goodblk) {
        page_id = offset / mtd->page_size;

        if (page_id > (mtd->block_total * mtd->pages_per_block)) {
            pr_err("All blocks in the part is bad!\n");
            ret = -RT_ERROR;
            goto rtt_spinand_save_env_simple;
        }

        blk = page_id / mtd->pages_per_block;

        if (rt_mtd_nand_check_block(mtd, blk) != RT_EOK) {
            pr_err("Block is bad, skip it.\n");
            offset += mtd->pages_per_block * mtd->page_size;
            continue;
        } else {
            rt_mtd_nand_erase_block(mtd, blk);
            goodblk = true;
        }
    }

    while (remain) {
        page_id = offset / mtd->page_size;

        ret = rt_mtd_nand_write(mtd, page_id, buf, mtd->page_size, RT_NULL, 0);
        if (ret) {
            pr_err("Failed to write page data to NAND.\n");
            ret = -RT_ERROR;
            goto rtt_spinand_save_env_simple;
        }

        buf += mtd->page_size;
        offset += mtd->page_size;
        if (remain >= mtd->page_size)
            remain -= mtd->page_size;
        else
            remain = 0;
    }

rtt_spinand_save_env_simple:
    rt_device_close(dev);
    return ret;
}
#else
static int bar_spinand_load_env_simple(void *buf, size_t size)
{
    struct mtd_dev *env_current;
    unsigned long offset = 0;

    if (dev_current == 0) {
        env_current = mtd_get_device(AIC_ENV_PART_NAME);
    } else if (dev_current == 1) {
        env_current = mtd_get_device(AIC_ENV_REDUNDAND_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (mtd_block_isbad(env_current, offset)) {
        pr_err("Block is bad, skip it.\n");
        offset += env_current->erasesize;
        if (offset >= env_current->size) {
            pr_err("All blocks in the part is bad!\n");
            return -1;
        }
    }

    if (mtd_read(env_current, offset, buf, size) < 0) {
        pr_err("Read env fail\n");
        return -1;
    }

    return 0;
}

static int bar_spinand_save_env_simple(void *buf, size_t size)
{
    struct mtd_dev *env_current;
    unsigned long offset = 0;

    if (dev_current == 0) {
        env_current = mtd_get_device(AIC_ENV_REDUNDAND_PART_NAME);
    } else if (dev_current == 1) {
        env_current = mtd_get_device(AIC_ENV_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (mtd_block_isbad(env_current, offset)) {
        pr_err("Block is bad, skip it.\n");
        offset += env_current->erasesize;
        if (offset >= env_current->size) {
            pr_err("All blocks in the part is bad!\n");
            return -1;
        }
    }

    if (mtd_erase(env_current, offset, AIC_ENV_SIZE)) {
        pr_err("Mtd erase env fail\n");
        return -1;
    }

    if (mtd_write(env_current, offset, buf, size)) {
        pr_err("Mtd write env fail\n");
        return -1;
    }

    return 0;
}
#endif
#endif

static int flash_env_read(void *buf, size_t size)
{
    int ret = 0;

    switch (bd) {
#ifdef AIC_SPINOR_DRV
        case BD_SPINOR:
#ifndef KERNEL_BAREMETAL
            ret = rtt_spinor_load_env_simple(buf, size);
#else
            ret = bar_spinor_load_env_simple(buf, size);
#endif
            break;
#endif
#ifdef AIC_SPINAND_DRV
        case BD_SPINAND:
#ifndef KERNEL_BAREMETAL
            ret = rtt_spinand_load_env_simple(buf, size);
#else
            ret = bar_spinand_load_env_simple(buf, size);
#endif
            break;
#endif
        default:
            break;
    }

    if (ret)
        return ret;

#ifdef AIC_ENV_DEBUG
    int i = 0, j = 0;
    char *str;

    str = buf;

    for (i = 0; i < size; i++) {
        printf("0x%x ", str[i]);
        j++;
        if (j >= 16) {
            printf("\n");
            j = 0;
        }
    }
#endif

    return 0;
}

static int flash_env_write(void *buf, size_t size)
{
    int ret = 0;

    switch (environment.flag_scheme) {
        case FLAG_NONE:
            break;
        case FLAG_INCREMENTAL:
            (*environment.flags)++;
            break;
        case FLAG_BOOLEAN:
            *environment.flags = active_flag;
            break;
        default:
            pr_info("Unimplemented flash scheme %d\n", environment.flag_scheme);
            return -1;
    }

    switch (bd) {
#ifdef AIC_SPINOR_DRV
        case BD_SPINOR:
#ifndef KERNEL_BAREMETAL
            ret = rtt_spinor_save_env_simple(buf, size);
#else
            ret = bar_spinor_save_env_simple(buf, size);
#endif
            break;
#endif
#ifdef AIC_SPINAND_DRV
        case BD_SPINAND:
#ifndef KERNEL_BAREMETAL
            ret = rtt_spinand_save_env_simple(buf, size);
#else
            ret = bar_spinand_save_env_simple(buf, size);
#endif
            break;
#endif
        default:
            break;
    }

    if (ret)
        return ret;

    return 0;
}

static int flash_io(int mode)
{
    void *buf;
    size_t size;
    int ret = 0;

#ifndef KERNEL_BAREMETAL
    ret = env_lock();
    if (ret) {
        pr_err("Lock env failed\n");
        goto exit_flash_io;
    }
#endif

    if (mode == O_RDWR) {
        buf = environment.image;
        size = AIC_ENV_SIZE;
        ret = flash_env_write(buf, size);
    } else {
        buf = environment.image;
        size = AIC_ENV_SIZE;
        ret = flash_env_read(buf, size);
    }

#ifndef KERNEL_BAREMETAL
    ret = env_unlock();
    if (ret) {
        ret = -1;
        pr_err("Unlock env failed\n");
    }

exit_flash_io:
#endif

    return ret;
}

/*
 * Prevent confusion if running from erased flash memory
 */
int fw_env_open(void)
{
    uint32_t crc0, crc0_ok;
    unsigned char flag0;
    void *addr0 = NULL;

    int crc1, crc1_ok;
    unsigned char flag1;
    void *addr1 = NULL;

    int ret;

    struct env_image_single *single;
    struct env_image_redundant *redundant;

    if (!bd) {
        bd = aic_get_boot_device();

        if ((bd <= BD_NONE) || (bd >= BD_SDFAT32)) {
            pr_err("Parameter bd is not right\n");
            return -1;
        }
    }

#ifndef KERNEL_BAREMETAL
    ret = env_lock_init();
    if (ret) {
        return -1;
    }
#endif

    addr0 = aicos_malloc_align(0, AIC_ENV_SIZE, CACHE_LINE_SIZE);
    if (addr0 == NULL) {
        pr_err("Not enough memory for environment (%d bytes)\n", AIC_ENV_SIZE);
        ret = -1;
        goto open_cleanup;
    }

    /* read environment from FLASH to local buffer */
    environment.image = addr0;

    if (have_redund_env) {
        redundant = addr0;
        environment.crc = &redundant->crc;
        environment.flags = &redundant->flags;
        environment.data = redundant->data;
    } else {
        single = addr0;
        environment.crc = &single->crc;
        environment.flags = NULL;
        environment.data = single->data;
    }

    usable_envsize = AIC_ENV_SIZE - sizeof(uint32_t);
    if (have_redund_env)
        usable_envsize -= sizeof(char);

    dev_current = 0;
    if (flash_io(O_RDONLY)) {
        ret = -EIO;
        goto open_cleanup;
    }
    crc0 = crc32(0, (uint8_t *)environment.data, usable_envsize);

#ifdef AIC_ENV_DEBUG
    printf("crc0 = 0x%x,environment.crc = 0x%x\n", (unsigned int)crc0,
           (unsigned int)*environment.crc);
#endif
    crc0_ok = (crc0 == *environment.crc);

    if (!have_redund_env) {
        if (!crc0_ok) {
            pr_err("Bad CRC, using default environment\n");
            memcpy(environment.data, default_environment,
                   sizeof(default_environment));
        }
    } else {
        flag0 = *environment.flags;

        dev_current = 1;
        addr1 = aicos_malloc_align(0, AIC_ENV_SIZE, CACHE_LINE_SIZE);
        if (addr1 == NULL) {
            pr_err("Not enough memory for environment (%d bytes)\n",
                   AIC_ENV_SIZE);
            ret = -ENOMEM;
            goto open_cleanup;
        }
        redundant = addr1;

        /*
		 * have to set environment.image for flash_read(), careful -
		 * other pointers in environment still point inside addr0
		 */
        environment.image = addr1;
        if (flash_io(O_RDONLY)) {
            ret = -EIO;
            goto open_cleanup;
        }

        /* Check flag scheme compatibility */
        /* uboot default is FLAG_INCREMENTAL */
        environment.flag_scheme = FLAG_INCREMENTAL,

        crc1 = crc32(0, (uint8_t *)redundant->data, usable_envsize);
#ifdef AIC_ENV_DEBUG
        printf("crc1 = 0x%x,redundant->crc = 0x%x\n", (unsigned int)crc1,
               (unsigned int)redundant->crc);
#endif
        crc1_ok = (crc1 == redundant->crc);
        flag1 = redundant->flags;

        if (crc0_ok && !crc1_ok) {
            dev_current = 0;
        } else if (!crc0_ok && crc1_ok) {
            dev_current = 1;
        } else if (!crc0_ok && !crc1_ok) {
            pr_err("Warning: Bad CRC, using default environment\n");
            memcpy(environment.data, default_environment,
                   sizeof(default_environment));
            dev_current = 0;
        } else {
            switch (environment.flag_scheme) {
                case FLAG_BOOLEAN:
                    if (flag0 == active_flag && flag1 == obsolete_flag) {
                        dev_current = 0;
                    } else if (flag0 == obsolete_flag && flag1 == active_flag) {
                        dev_current = 1;
                    } else if (flag0 == flag1) {
                        dev_current = 0;
                    } else if (flag0 == 0xFF) {
                        dev_current = 0;
                    } else if (flag1 == 0xFF) {
                        dev_current = 1;
                    } else {
                        dev_current = 0;
                    }
                    break;
                case FLAG_INCREMENTAL:
                    if (flag0 == 255 && flag1 == 0)
                        dev_current = 1;
                    else if ((flag1 == 255 && flag0 == 0) || flag0 >= flag1)
                        dev_current = 0;
                    else /* flag1 > flag0 */
                        dev_current = 1;
                    break;
                default:
                    pr_info("Unknown flag scheme %d\n",
                            environment.flag_scheme);
                    ret = -1;
                    goto open_cleanup;
            }
        }

        /*
		 * If we are reading, we don't need the flag and the CRC any
		 * more, if we are writing, we will re-calculate CRC and update
		 * flags before writing out
		 */
        if (dev_current) {
            environment.image = addr1;
            environment.crc = &redundant->crc;
            environment.flags = &redundant->flags;
            environment.data = redundant->data;
            aicos_free_align(0, addr0);
        } else {
            environment.image = addr0;
            /* Other pointers are already set */
            aicos_free_align(0, addr1);
        }
    }
    return 0;

open_cleanup:
    if (addr0) {
        aicos_free_align(0, addr0);
    }

    if (addr1) {
        aicos_free_align(0, addr1);
    }

    return ret;
}

/*
 * Simply free allocated buffer with environment
 */
int fw_env_close(void)
{
    int ret = 0;

    if (environment.image) {
        aicos_free_align(0, environment.image);
    }

    environment.image = NULL;
    return ret;
}

static int cmd_fw_printenv(int argc, char **argv)
{
    int i, ret = 0;

    if (fw_env_open()) {
        pr_err("Open env failed\n");
        return -1;
    }

    if (argc == 1) { /* Print all env variables  */
        char *env, *nxt;
        for (env = environment.data; *env; env = nxt + 1) {
            for (nxt = env; *nxt; ++nxt) {
                if (nxt >= &environment.data[usable_envsize]) {
                    pr_err("Environment not terminated\n");
                    return -1;
                }
            }

            printf("%s\n", env);
        }
        fw_env_close();
        return 0;
    }

    for (i = 1; i < argc; ++i) { /* print a subset of env variables */
        char *name = argv[i];
        char *val = NULL;

        val = fw_getenv(name);
        if (!val) {
            pr_err("\"%s\" not defined\n", name);
            ret = -1;
            continue;
        }

        pr_err("%s=%s\n", name, val);
    }

    fw_env_close();
    return ret;
}

#ifndef KERNEL_BAREMETAL
MSH_CMD_EXPORT_ALIAS(cmd_fw_printenv, fw_printenv, Print env);
#else
CONSOLE_CMD(fw_printenv, cmd_fw_printenv, "Print env");
#endif

static int cmd_fw_setenv(int argc, char **argv)
{
    int ret = 0;

    if ((argc != 3) && (argc != 2)) {
        printf(SET_ENV_HELP);
        return -1;
    }

    if (fw_env_open()) {
        ret = -1;
        goto fw_setenv_err;
    }

    ret = fw_env_write(argv[1], argv[2]);
    if (ret) {
        pr_err("Env write fail\n");
        goto fw_setenv_err;
    }

    fw_env_flush();

fw_setenv_err:
    fw_env_close();

    return ret;
}

#ifndef KERNEL_BAREMETAL
MSH_CMD_EXPORT_ALIAS(cmd_fw_setenv, fw_setenv, Set env);
#else
CONSOLE_CMD(fw_setenv, cmd_fw_setenv, "Set env");
#endif
