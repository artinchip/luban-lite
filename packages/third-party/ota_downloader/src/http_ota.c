/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-03-22     Murphy       the first version
 */
#include <rtconfig.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <finsh.h>
#include <string.h>
#include "aic_core.h"
#include "webclient.h"
#include <env.h>
#include <absystem.h>

#ifdef AIC_SPINOR_DRV
#include <fal.h>

/* the address offset of download partition */
#ifndef RT_USING_FAL
#error "Please enable and confirgure FAL part."
#endif /* RT_USING_FAL */

const struct fal_partition *dl_part = RT_NULL;
#endif

#ifdef AIC_SPINAND_DRV
struct rt_mtd_nand_device *nand_mtd;
rt_device_t nand_dev;
#endif

#define DBG_ENABLE
#define DBG_SECTION_NAME "http_ota"
#ifdef OTA_DOWNLOADER_DEBUG
#define DBG_LEVEL DBG_LOG
#else
#define DBG_LEVEL DBG_INFO
#endif
#define DBG_COLOR
#include <rtdbg.h>

#ifdef LPKG_USING_HTTP_OTA

#define ALIGN_xB_UP(x, y) (((x) + (y - 1)) & ~(y - 1))

#define MAX_CPIO_FILE_NAME 18

#define GET_HEADER_BUFSZ  512
#define GET_RESP_BUFSZ    512
#define HTTP_OTA_DL_DELAY (10 * RT_TICK_PER_SECOND)

#define HTTP_OTA_URL PKG_HTTP_OTA_URL

#define MAX_IMAGE_FNAME 32

/*
 * OTA upgrade has two caches
 * OTA unpacking requires one cache because the cpio header info may be split into two transfer,
 * determine by HTTP_OTA_HEAD_LEN
 * Burning requires a cache because burning requires address and length align,
 * determine by HTTP_OTA_BURN_BUFF_LEN
 */
#define HTTP_OTA_BURN_BUFF_LEN (2048 * 2)
#define HTTP_OTA_BURN_LEN      2048
#define HTTP_OTA_HEAD_LEN      (2048 * 2)
#define HTTP_OTA_BUFF_LEN      (4096 / 2)

enum flag_cpio {
    FLAG_CPIO_HEAD1,
    FLAG_CPIO_HEAD2,
    FLAG_CPIO_HEAD3,
    FLAG_CPIO_HEAD4,
    FLAG_CPIO_HEAD,
    FLAG_CPIO_FILE,
};

struct filehdr {
    unsigned int format;
    unsigned int size;
    unsigned int size_align;
    unsigned int begin_offset;
    unsigned int cpio_header_len;
    unsigned int namesize;
    unsigned int filename_size;
    unsigned int filename_align;
    unsigned int burn_len;
    unsigned int chksum;
    unsigned int sum;
    char filename[MAX_IMAGE_FNAME];
};

struct bufhdr {
    int buflen; //effective data size
    int head;   //start address of valid data
    int size;   //cache size
    char *buf;
};

enum cpio_fields {
    C_MAGIC,
    C_INO,
    C_MODE,
    C_UID,
    C_GID,
    C_NLINK,
    C_MTIME,
    C_FILESIZE,
    C_MAJ,
    C_MIN,
    C_RMAJ,
    C_RMIN,
    C_NAMESIZE,
    C_CHKSUM,
    C_NFIELDS
};

#ifdef OTA_DOWNLOADER_DEBUG
static unsigned int test_sum = 0;
static unsigned int test_leng = 0;
#endif

static int file_offset = 0; /*received data offset*/
static int file_size = 0;

static unsigned char flag_cpio = FLAG_CPIO_HEAD1; /*upgrade file name index*/
static unsigned char cpio_or_file = FLAG_CPIO_HEAD;
/*parse header info or file content*/

static struct bufhdr bhdr = { 0 };  /*burn buffer*/
static struct bufhdr shdr = { 0 };  /*head buffer*/
static struct filehdr fhdr = { 0 }; /*upgrade file info*/

unsigned int cpio_file_checksum(unsigned char *buffer, unsigned int length)
{
    unsigned int sum = 0;
    int i = 0;

    for (i = 0; i < length; i++)
        sum += buffer[i];

#ifdef OTA_DOWNLOADER_DEBUG
    test_sum += sum;
    test_leng += length;
    printf("%s sum = 0x%x length = %d\n", __func__, test_sum, test_leng);
#endif
    return sum;
}

static void ota_buf_init(struct bufhdr *hdr, char *buf, int size)
{
    hdr->buf = buf;
    hdr->size = size;
    hdr->buflen = 0;
    hdr->head = 0;
}

static int ota_buf_push(struct bufhdr *hdr, char *data, int len)
{
    if ((hdr->buflen + len) > hdr->size) {
        LOG_E("Queue overflow,please increase buffer size\n");
        return -RT_ERROR;
    }

    rt_memcpy(hdr->buf + hdr->buflen, data, len);
    hdr->buflen += len;

#ifdef OTA_DOWNLOADER_DEBUG
    int i;
    printf("%s:\n", __func__);
    for (i = 0; i < 20; i++)
        printf("0x%x ", data[i]);
    printf("\n");

    printf("%s hdr->buflen = %d\n", __func__, hdr->buflen);
#endif

    return RT_EOK;
}

static void print_progress(size_t cur_size, size_t total_size)
{
    static unsigned char progress_sign[100 + 1];
    uint8_t i, per = cur_size * 100 / total_size;
    static unsigned char per_size = 0;

    if (per > 100) {
        per = 100;
    }

    if (per_size == per)
        return;

    for (i = 0; i < 100; i++) {
        if (i < per) {
            progress_sign[i] = '=';
        } else if (per == i) {
            progress_sign[i] = '>';
        } else {
            progress_sign[i] = ' ';
        }
    }

    progress_sign[sizeof(progress_sign) - 1] = '\0';

    LOG_I("Download: [%s] %03d%%\033[1A", progress_sign, per);

    per_size = per;
}

static int aic_ota_find_part(char *partname)
{
    switch (aic_get_boot_device()) {
#ifdef AIC_SPINOR_DRV
        case BD_SPINOR:
            /* Get download partition information and erase download partition data */
            if ((dl_part = fal_partition_find(partname)) == RT_NULL) {
                LOG_E("Firmware download failed! Partition (%s) find error!",
                      partname);
                return -RT_ERROR;
            }
            break;
#endif
#ifdef AIC_SPINAND_DRV
        case BD_SPINAND:
            nand_dev = rt_device_find(partname);
            if (nand_dev == RT_NULL) {
                LOG_E("Firmware download failed! Partition (%s) find error!",
                      partname);
                return -RT_ERROR;
            }

            nand_mtd = (struct rt_mtd_nand_device *)nand_dev;
            break;
#endif
        default:
            return -RT_ERROR;
            break;
    }

    LOG_I("Partition (%s) find success!", partname);
    return 0;
}

#ifdef AIC_SPINOR_DRV
static int aic_ota_nor_erase_part(void)
{
    LOG_I("Start erase flash (%s) partition!", dl_part->name);
    if (fal_partition_erase(dl_part, 0, dl_part->len) < 0) {
        LOG_E("Firmware download failed! Partition (%s) erase error! len = %d",
              dl_part->name, dl_part->len);
        return -RT_ERROR;
    }
    LOG_I("Erase flash (%s) partition success! len = %d", dl_part->name,
          dl_part->len);
    return 0;
}
#endif

#ifdef AIC_SPINAND_DRV
static int aic_ota_nand_erase_part(void)
{
    unsigned long blk_offset = 0;

    LOG_I("Start erase nand flash partition!");

    while (nand_mtd->block_total > blk_offset) {
        if (rt_mtd_nand_check_block(nand_mtd, blk_offset) != RT_EOK) {
            LOG_W("Erase block is bad, skip it.\n");
            blk_offset++;
            continue;
        }

        rt_mtd_nand_erase_block(nand_mtd, blk_offset);
        blk_offset++;
    }

    LOG_I("Erase nand flash partition success! len = %d",
          nand_mtd->block_total);

    return 0;
}

static int aic_ota_nand_write(uint32_t addr, const uint8_t *buf, size_t size)
{
    unsigned long blk = 0, offset = 0, page = 0;
    static unsigned long bad_block_off = 0;
    unsigned long blk_size = nand_mtd->pages_per_block * nand_mtd->page_size;
    rt_err_t ret = 0;

    if (size > 2048) {
        LOG_E("HTTP_OTA_BURN_LEN need set 2048! size = %d", size);
        return -RT_ERROR;
    }

    ret = rt_device_open(nand_dev, RT_DEVICE_OFLAG_RDWR);
    if (ret) {
        LOG_E("Open MTD device failed.!\n");
        return ret;
    }

    offset = addr + bad_block_off;

    /* Search for the first good block after the given offset */
    if (offset % blk_size == 0) {
        blk = offset / blk_size;
        while (rt_mtd_nand_check_block(nand_mtd, blk) != RT_EOK) {
            LOG_W("find a bad block, off adjust to the next block\n");
            bad_block_off += nand_mtd->pages_per_block;
            offset = addr + bad_block_off;
            blk = offset / blk_size;
        }
    }

    page = offset / nand_mtd->page_size;
    ret = rt_mtd_nand_write(nand_mtd, page, buf, size, RT_NULL, 0);
    if (ret) {
        LOG_E("Failed to write data to NAND.\n");
        ret = -RT_ERROR;
        goto aic_ota_nand_write_exit;
    }

aic_ota_nand_write_exit:
    rt_device_close(nand_dev);

    return 0;
}
#endif

static int aic_ota_erase_part(void)
{
    int ret = 0;

    switch (aic_get_boot_device()) {
#ifdef AIC_SPINOR_DRV
        case BD_SPINOR:
            ret = aic_ota_nor_erase_part();
            break;
#endif
#ifdef AIC_SPINAND_DRV
        case BD_SPINAND:
            ret = aic_ota_nand_erase_part();
            break;
#endif
        default:
            break;
    }

    return ret;
}

static int aic_ota_part_write(uint32_t addr, const uint8_t *buf, size_t size)
{
    int ret = 0;

    switch (aic_get_boot_device()) {
#ifdef AIC_SPINOR_DRV
        case BD_SPINOR:
            ret = fal_partition_write(dl_part, addr, buf, size);
            if (ret < 0) {
                LOG_E(
                    "Firmware download failed! Partition (%s) write data error!",
                    dl_part->name);
                return -RT_ERROR;
            }
            break;
#endif
#ifdef AIC_SPINAND_DRV
        case BD_SPINAND:
            ret = aic_ota_nand_write(addr, buf, size);
            if (ret < 0) {
                LOG_E(
                    "Firmware download failed! nand partition write data error!");
                return -RT_ERROR;
            }
            break;
#endif
        default:
            return -RT_ERROR;
            break;
    }

    return ret;
}

/*
 * if the valid data in the buffer exceeds HTTP_OTA_BURN_LEN,
 * start burn data
 */
static int download_buf_pop(struct bufhdr *bhdr, struct filehdr *fhdr)
{
    int ret = RT_EOK;
    int burn_len = 0;
    int burn_last = 0;

download_buf_pop_last:
    /*Last upgrade data*/
    if (fhdr->size - fhdr->begin_offset < bhdr->buflen) {
        /*upgrade in two stags*/
        if (fhdr->size - fhdr->begin_offset > HTTP_OTA_BURN_LEN) {
            burn_len = HTTP_OTA_BURN_LEN;
            burn_last = 1;
        } else if (fhdr->size - fhdr->begin_offset == HTTP_OTA_BURN_LEN) {
            burn_len = HTTP_OTA_BURN_LEN;
#ifdef OTA_DOWNLOADER_DEBUG
            LOG_I("Burn the last data size = %d!", HTTP_OTA_BURN_LEN);
#endif
        } else {
            burn_len = fhdr->size - fhdr->begin_offset;
#ifdef OTA_DOWNLOADER_DEBUG
            LOG_I("Burn the last data size = %d!", burn_len);
#endif
        }
    } else {
        /*when the buffer len is insufficient, process after receive data next time*/
        if (bhdr->buflen < HTTP_OTA_BURN_LEN)
            return 0;
        else
            burn_len = HTTP_OTA_BURN_LEN;
    }

    /* Write the data to the corresponding partition address */
    ret = aic_ota_part_write(fhdr->begin_offset, (const uint8_t *)bhdr->buf,
                             burn_len);
    if (ret)
        return -RT_ERROR;

    fhdr->sum += cpio_file_checksum((unsigned char *)bhdr->buf, burn_len);

    fhdr->begin_offset += burn_len;
    print_progress(fhdr->begin_offset, fhdr->size);

    if (burn_len == HTTP_OTA_BURN_LEN) {
        rt_memcpy(bhdr->buf, bhdr->buf + HTTP_OTA_BURN_LEN,
                  bhdr->buflen - HTTP_OTA_BURN_LEN);
        bhdr->buflen -= HTTP_OTA_BURN_LEN;
        if (fhdr->size - fhdr->begin_offset <= 0) {
            bhdr->head += fhdr->size_align;
            bhdr->buflen -= fhdr->size_align;
            /*transfer bhdr all data to shdr*/
            ret = ota_buf_push(&shdr, bhdr->buf + bhdr->head, bhdr->buflen);
            if (ret < 0) {
                return ret;
            } else {
                /*bhdr all data has been passed to shdr*/
                bhdr->buflen = 0;
                bhdr->head = 0;
            }
        }
    } else {
        bhdr->head += (burn_len + fhdr->size_align);
        bhdr->buflen -= (burn_len + fhdr->size_align);

        /*transfer bhdr all data to shdr*/
        ret = ota_buf_push(&shdr, bhdr->buf + bhdr->head, bhdr->buflen);
        if (ret < 0) {
            return ret;
        } else {
            /*bhdr all data has been passed to shdr*/
            bhdr->buflen = 0;
            bhdr->head = 0;
        }
    }

    /*Start upgrade of remain data*/
    if (burn_last == 1) {
        burn_last = 0;
        goto download_buf_pop_last;
    }

    return 0;
}

/*
 * find_cpio_data - Search for files in an uncompressed cpio
 * @fhdr:     struct filehdr containing the address, length and
 *              filename (with the directory path cut off) of the found file.
 * @data:       Pointer to the cpio archive or a header inside
 * @len:        Remaining length of the cpio based on data pointer
 * @return:     0 is success,others is failed
 */
int find_cpio_data(struct filehdr *fhdr, void *data, size_t len)
{
    const char *p;
    unsigned int *chp, v;
    unsigned char c, x;
    int i, j;
    unsigned int ch[C_NFIELDS];
    int file_end_offset = 0;
    int cpio_header_len = 0;

    fhdr->cpio_header_len = 8 * C_NFIELDS - 2;

    p = data;

    if (!*p) {
        /* All cpio headers need to be 4-byte aligned */
        LOG_I("*p = 0x%x\n", *p);
        p += 4;
        len -= 4;
    }

    j = 6; /* The magic field is only 6 characters */
    chp = ch;
    for (i = C_NFIELDS; i; i--) {
        v = 0;
        while (j--) {
            v <<= 4;
            c = *p++;

            x = c - '0';
            if (x < 10) {
                v += x;
                continue;
            }

            x = (c | 0x20) - 'a';
            if (x < 6) {
                v += x + 10;
                continue;
            }

            LOG_E("error 1 Invalid hexadecimal\n");
            goto quit; /* Invalid hexadecimal */
        }
        *chp++ = v;
        j = 8; /* All other fields are 8 characters */
    }

    if ((ch[C_MAGIC] - 0x070701) > 1) {
        LOG_E("error 2 Invalid magic\n");
        goto quit; /* Invalid magic */
    }

#ifdef OTA_DOWNLOADER_DEBUG
    for (i = 0; i < 14; i++)
        printf("ch[%d] = 0x%x\n", i, ch[i]);
#endif

    if ((ch[C_MODE] & 0170000) == 0100000) {
        if (ch[C_NAMESIZE] >= MAX_CPIO_FILE_NAME) {
            LOG_E("File %s exceeding MAX_CPIO_FILE_NAME [%d]\n", p,
                  MAX_CPIO_FILE_NAME);
        }
        strncpy(fhdr->filename, p, ch[C_NAMESIZE]);

        fhdr->format = ch[C_MAGIC];
        fhdr->size = ch[C_FILESIZE]; /*upgrade file size*/
        fhdr->begin_offset = 0;
        fhdr->filename_size = ch[C_NAMESIZE];
        fhdr->chksum = ch[C_CHKSUM];
        fhdr->sum = 0;

        /*filename align offset*/
        file_end_offset = fhdr->cpio_header_len + fhdr->filename_size;
        fhdr->filename_align =
            ALIGN_xB_UP(file_end_offset, 4) - file_end_offset;
#ifdef OTA_DOWNLOADER_DEBUG
        printf("fhdr->filename_align = %d %d %d\n", fhdr->filename_align,
               file_end_offset, ALIGN_xB_UP(file_end_offset, 4));
#endif

        /*Parse of cpio header info,filename and align data,next parse if Incomplete*/
        cpio_header_len =
            fhdr->cpio_header_len + fhdr->filename_size + fhdr->filename_align;
        if (len < cpio_header_len) {
            LOG_E("Incomplete filename and align data!\n");
            return -1;
        }

        /*file align offset*/
        file_end_offset = fhdr->size;
        fhdr->size_align = ALIGN_xB_UP(file_end_offset, 4) - file_end_offset;

#ifdef OTA_DOWNLOADER_DEBUG
        printf("fhdr->size_align = %d %d %d\n", fhdr->size_align,
               file_end_offset, ALIGN_xB_UP(file_end_offset, 4));
        test_sum = 0;
        test_leng = 0;
#endif

        LOG_I("find file %s cpio data success\n", fhdr->filename);
        return 0; /* Found it! */
    } else {
        strncpy(fhdr->filename, p, ch[C_NAMESIZE]);
        fhdr->filename_size = ch[C_NAMESIZE];
        LOG_I("find file %s cpio data success\n", fhdr->filename);
        return 0; /* Found it! */
    }

quit:
    LOG_E("find file in cpio data failed\n");
    return -1;
}

/*
 * Remove cpio header info, filename and align data
 * Then transfer shdr remain data to bhdr
 */
static int head_buf_pop(struct bufhdr *bhdr, struct bufhdr *shdr,
                        struct filehdr *fhdr)
{
    int ret = RT_EOK;
    int len =
        fhdr->cpio_header_len + fhdr->filename_size + fhdr->filename_align;

    /*Remove cpio header info*/
    shdr->head += len;
    shdr->buflen -= len;

    if (shdr->buflen > 0) {
#ifdef OTA_DOWNLOADER_DEBUG
        int i;
        printf("%s shdr->buflen = %d, len = %d\n", __func__, shdr->buflen, len);
        printf("%s:\n", __func__);
        for (i = 0; i < 20; i++)
            printf("0x%x ", (shdr->buf + shdr->head)[i]);
#endif
        ret = ota_buf_push(bhdr, shdr->buf + shdr->head, shdr->buflen);
        if (ret)
            return ret;

        shdr->buflen = 0;
        shdr->head = 0;
    } else {
        shdr->head = 0;
    }

    return ret;
}

/* handle function, you can store data and so on */
static int http_ota_shard_download_handle(char *buffer, int length)
{
    int ret = RT_EOK;
    int len = 8 * C_NFIELDS - 2;
    char *partname = NULL;

    file_offset += length;

    /*
     * Start default parsing of cpio header info,filename and align data
     * Afterwards, parse the file content
     */
    if (cpio_or_file != FLAG_CPIO_FILE) {
        ret = ota_buf_push(&shdr, buffer, length);
        if (ret)
            goto __download_exit;

    http_ota_shard_download_handle_last:

        if (shdr.buflen < len) {
            LOG_I("Incomplete header info! shdr.buflen = %d", shdr.buflen);
            goto __download_exit;
        }

        /*
         * Find complete header info, if not, direct return
         */
        ret = find_cpio_data(&fhdr, shdr.buf, shdr.buflen);
        if (ret < 0) {
            LOG_E("Not find file info\n");
            goto __download_exit;
        }

        /*TRAILER!!! is the last file, unpack is over*/
        ret = rt_strncmp(fhdr.filename, "TRAILER!!!", fhdr.filename_size);
        if (ret == 0) {
            goto __download_exit;
        }

        ret = head_buf_pop(&bhdr, &shdr, &fhdr);
        if (ret < 0) {
            LOG_E("head_buf_pop error!\n");
            goto __download_exit;
        }

        partname = aic_upgrade_get_partname(flag_cpio);

        ret = aic_ota_find_part(partname);
        if (ret)
            goto __download_exit;

        ret = aic_ota_erase_part();
        if (ret)
            goto __download_exit;

        LOG_I("Start upgrade %s!", fhdr.filename);

        ret = download_buf_pop(&bhdr, &fhdr);
        if (ret < 0) {
            LOG_E("download_buf_pop error! len = %d\n", len);
            goto __download_exit;
        }

        cpio_or_file = FLAG_CPIO_FILE;
    } else { /*Parse file content*/
        ret = ota_buf_push(&bhdr, buffer, length);
        if (ret)
            goto __download_exit;

        ret = download_buf_pop(&bhdr, &fhdr);
        if (ret < 0) {
            LOG_E("download_buf_pop error! len = %d\n", len);
            goto __download_exit;
        }

        if (fhdr.size <= fhdr.begin_offset) {
#ifdef OTA_DOWNLOADER_DEBUG
            LOG_I("fhdr.size = %d fhdr.begin_offset = %d\n", fhdr.size,
                  fhdr.begin_offset);
#endif
            if (fhdr.sum == fhdr.chksum) {
                LOG_I("Sum check success!");
                LOG_I("download %s success!\n", fhdr.filename);
            } else {
                LOG_E(
                    "Sum check failed, fhdr->sum = 0x%x,fhdr->chksum = 0x%x\n",
                    fhdr.sum, fhdr.chksum);
                goto __download_exit;
            }

            cpio_or_file = FLAG_CPIO_HEAD;
            flag_cpio++;

            goto http_ota_shard_download_handle_last;
        }
    }

__download_exit:
    rt_free(buffer);
    return ret;
}

static int http_ota_fw_download(const char *uri)
{
    int ret = RT_EOK;
    char *buf = NULL, *buffer = NULL;
    struct webclient_session *session = RT_NULL;

    ret = aic_upgrade_start();
    if (ret) {
        LOG_E("Aic get os to upgrade");
        return ret;
    }

    buf = aicos_malloc_align(0, HTTP_OTA_BURN_BUFF_LEN, CACHE_LINE_SIZE);
    if (!buf) {
        LOG_E("malloc buf failed\n");
        ret = -RT_ERROR;
        goto __exit;
    }

    buffer = rt_malloc(HTTP_OTA_HEAD_LEN);
    if (!buffer) {
        LOG_E("malloc buffer failed\n");
        ret = -RT_ERROR;
        goto __exit;
    }

    ota_buf_init(&bhdr, buf, HTTP_OTA_BURN_BUFF_LEN);
    ota_buf_init(&shdr, buffer, HTTP_OTA_HEAD_LEN);

    /* create webclient session and set header response size */
    session = webclient_session_create(GET_HEADER_BUFSZ);
    if (!session) {
        LOG_E("open uri failed.");
        ret = -RT_ERROR;
        goto __exit;
    }

    /* get the real data length */
    webclient_shard_head_function(session, uri, &file_size);

    if (file_size == 0) {
        LOG_E("Request file size is 0!");
        LOG_I("Sample: http_ota http://192.168.31.22/ota.cpio");
        ret = -RT_ERROR;
        goto __exit;
    } else if (file_size < 0) {
        LOG_E("webclient GET request type is chunked.");
        ret = -RT_ERROR;
        goto __exit;
    }
    LOG_I("OTA file size is (%d)", file_size);
    LOG_I("\033[1A");

    /* register the handle function, you can handle data in the function */
    webclient_register_shard_position_function(session,
                                               http_ota_shard_download_handle);

    /* the "memory size" that you can provide in the project and uri */
    ret = webclient_shard_position_function(session, uri, file_offset,
                                            file_size, HTTP_OTA_BUFF_LEN);

    /* clear the handle function */
    webclient_register_shard_position_function(session, RT_NULL);

    if (ret == RT_EOK) {
        if (session != RT_NULL) {
            webclient_close(session);
            session = RT_NULL;
        }

        LOG_I("\033[0B");
        LOG_I("Download firmware to flash success.");
        LOG_I("System now will restart...");

        rt_thread_delay(rt_tick_from_millisecond(5));

        ret = aic_upgrade_end();
        if (ret) {
            LOG_E("Aic upgrade end");
        }

        /* Reset the device, Start new firmware */
        //extern void rt_hw_cpu_reset(void);
        //rt_hw_cpu_reset();
    } else {
        LOG_E("Download firmware failed.");
    }

__exit:
    if (session != RT_NULL)
        webclient_close(session);
    file_offset = 0;

    if (buf)
        aicos_free_align(0, buf);

    if (buffer)
        rt_free(buffer);

    return ret;
}

void http_ota(uint8_t argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("using uri: " HTTP_OTA_URL "\n");
        http_ota_fw_download(HTTP_OTA_URL);
    } else {
        http_ota_fw_download(argv[1]);
    }
}
/**
 * msh />http_ota [url]
*/
MSH_CMD_EXPORT(http_ota, Use HTTP to download the firmware);

#endif /* LPKG_USING_HTTP_OTA */
