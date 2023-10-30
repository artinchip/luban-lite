/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef _FAT_FS_H
#define _FAT_FS_H

#include <aic_core.h>
#include <config_parse.h>

#define INVALID_VAL          0xFFFFFFFFU
#define CONFIG_SYS_TEXT_BASE 0x40000000
#define IMAGE_HEADER_SIZE    0x40
#define BOOTLOADER_BUF_ADDR  (CONFIG_SYS_TEXT_BASE - IMAGE_HEADER_SIZE)
#define SD_BOOT_HEAD_BLK_CNT (1)

/* Disk Partition Table */
#define DPT_START 0x1be

/* BIOS Parameter Block
 * BPB locate in DBR(DOS Boot Record), 0x0B~0x52
 */
#define BPB_START 0x0b

#define BS_FilSysType32 82		/* FAT32: Filesystem type string (8-byte) */
#define _TO_U16(x, s)   ((u16)(x[s+1]<<8|x[s]))
#define _TO_U32(x, s)   ((u32)(x[s+3]<<24|x[s+2]<<16|x[s+1]<<8|x[s]))

#define BPB_SECT_SIZE(x)     _TO_U16(x, 0x0b)
#define BPB_CLUS_SIZE(x)     ((u8)(x[0x0d]))
#define BPB_RESV_SECT_CNT(x) _TO_U16(x, 0x0e)
#define BPB_FAT_NUM(x)       ((u8)(x[0x10]))
/* FAT32: FAT table size(sector) */
#define BPB_FAT32_SIZE(x) _TO_U32(x, 0x24)
//((u32)(x[0x27]<<24|x[0x26]>>16|x[0x25]<<8|x[0x24]))
#define BPB_ROOT_ENTRY_CNT(x)    _TO_U16(x, 0x11)
#define BPB_ROOT_START_CLUS32(x) _TO_U32(x, 0x2c)

/*
 * FAT32 will create short name dirent for long file name, to make compabtible
 * for old application
 */
#define SHT_DIR_Name_idx        0
#define SHT_DIR_Name_Suffix_idx 8
#define SHT_DIR_Attr(d)         ((u8)d[0xB])
#define SHT_DIR_StartCluster(d) ((u32)(d[0x15]<<24|d[0x14]<<16|d[0x1B]<<8|d[0x1A]))
#define SHT_DIR_Filesize(d)     _TO_U32(d, 0x1C)

#define DIRENT_DEL 0xE5
#define DIRENT_END 0x0

#define DIR_ATTR_RW      0
#define DIR_ATTR_RO      1
#define DIR_ATTR_HIDDEN  2
#define DIR_ATTR_SYSTEM  4
#define DIR_ATTR_VOLUME  8
#define DIR_ATTR_SUBDIR  16
#define DIR_ATTR_ARCHIVE 32

/* 1: Last dirent for LFN */
#define LFN_DIR_Attr_Last(d)    ((d[0] & 0x40) >> 6)
#define LFN_DIR_Attr_Number(d)  ((d[0] & 0x1F))
#define LFN_DIR_Flag(d)         (d[0xB] == 0x0F)
#define LFN_DIR_StartCluster(d) _TO_U16(d, 0x1A)

#define DIR_ENTRY_SIZE 32
/*
 * One FAT item use 4 bytes
 */
#define MAX_FAT_ITEM_PER_SECT (LBA_SIZE >> 2)

#define FAT32_MINIMAL_CLUS 65525
#define FAT32_ENTRY_UNUSED 0x00000000U
#define FAT32_ENTRY_LAST   0x0FFFFFFFU
#define FAT32_ENTRY_BAD    0x0FFFFFF7U
#define EXFAT_ENTRY_UNUSED 0x00000000U
#define EXFAT_ENTRY_LAST   0xFFFFFFFFU
#define EXFAT_ENTRY_BAD    0xFFFFFFF7U

/*
 * Default Cluster size for FAT32:
 * Volume Size    | Cluster Size
 * < 16MB         | Not supported
 * 16MB  ~ 64MB   | 512B
 * 64MB  ~ 128MB  | 1KB
 * 128MB ~ 256MB  | 2KB
 * 256MB ~ 8GB    | 4KB
 * 8GB   ~ 16GB   | 8KB
 * 16GB  ~ 32GB   | 16KB
 * 32GB  ~ 2TB    | 32KB
 * > 2TB          | Not Supported
 *
 * Default Cluster size for exFAT:
 * Volume Size    | Cluster Size
 * 7MB   ~ 256MB  | 4KB
 * 256MB ~ 32GB   | 32KB
 * 32GB  ~ 256TB  | 128KB
 * > 256TB        | Not Supported
 *
 * FSBL image should not large than 256 KB, so here set MAX to 64
 */
#define IMG_MAX_CLUSTER 8192

#define BOOTCFG_FILENAME "bootcfg.txt"
#define BOOTCFG_NAME_LEN 11

#define F_PARSE_NONE          0
#define F_PARSE_LONG          1
#define F_PARSE_SHORT         2
#define TYPE_MBR_PARTITION    1
#define TYPE_GPT_PARTITION    2
#define TYPE_RAW_VOLUME_FAT32 3
#define TYPE_RAW_VOLUME_EXFAT 4

/* Macro for exFAT */
#define BS_FilSysTypeEXFAT 3  /* EXFAT string, 8 bytes */
#define BS_MUSTBEZERO      11 /* EXFAT, this field must be zero, 53 bytes */
#define BS_MUSTBEZERO_LEN  53

#define BS_FAT_OFFSET(x)     _TO_U32(x, 80)
#define BS_FAT_LEN(x)        _TO_U32(x, 84)
#define BS_CLUSTER_OFFSET(x) _TO_U32(x, 88)
#define BS_CLUSTER_CNT(x)    _TO_U32(x, 92)
#define BS_ROOT_DIR_CLUS(x)  _TO_U32(x, 96)
#define BS_SECT_SHIFT(x)     ((u8)(x[108]))
#define BS_CLUS_SHIFT(x)     ((u8)(x[109]))
#define BS_FAT_NUM(x)        ((u8)(x[110]))

#define ENT_TYPE_IMPORTANCE_CRITICAL (0)
#define ENT_TYPE_IMPORTANCE_BENIGN   (1 << 5)
#define ENT_TYPE_MSK_IMPORTANCE      (1 << 5)
#define ENT_TYPE_CATEGORY_PRIMARY    (0)
#define ENT_TYPE_CATEGORY_SECONDARY  (1 << 6)
#define ENT_TYPE_MSK_CATEGORY        (1 << 6)
#define ENT_TYPE_MSK_IN_USE          (1 << 7)

/* dentry types */
#define TYPE_EXFAT_UNUSED   0x00 /* end of directory */
#define TYPE_EXFAT_DELETE   (~0x80)
#define IS_EXFAT_DELETED(x) ((x) < 0x80) /* deleted file (0x01~0x7F) */
#define TYPE_EXFAT_INVAL    0x80         /* invalid value */
#define TYPE_EXFAT_BITMAP   0x81         /* allocation bitmap */
#define TYPE_EXFAT_UPCASE   0x82         /* upcase table */
#define TYPE_EXFAT_VOLUME   0x83         /* volume label */
#define TYPE_EXFAT_FILE     0x85         /* file or dir */
#define TYPE_EXFAT_GUID     0xA0
#define TYPE_EXFAT_PADDING  0xA1
#define TYPE_EXFAT_ACLTAB   0xA2
#define TYPE_EXFAT_STREAM   0xC0 /* stream entry */
#define TYPE_EXFAT_NAME     0xC1 /* file name entry */
#define TYPE_EXFAT_ACL      0xC2 /* stream entry */

#define FS_TYPE_FAT32 (0)
#define FS_TYPE_EXFAT (1)

/* Disk Partition Table Entry
 *
 * MBR first 446 bytes(Byte[0:445]) are reserved for boot code.
 * DPT 64 bytes, locate at Byte[446:509], 4 entries, 16 bytes per partition
 * MBR Identification Codes 2 bytes, Byte[510:511], value 0x55, 0xAA
 *
 * Partition Type:
 *     0x00: Empty partition table entry
 *     0x01: DOS FAT12
 *     0x04: DOS FAT16
 *     0x05: DOS3.3+extended partition
 *     0x06: DOS3.31+FAT16
 *     0x07: Maybe exFAT
 *     0x0B: Win95+FAT32
 *     0x0C: Win95+FAT32(using LBA-mode INT 13 extensions)
 *     0x0E: DOS FAT16(over 32MB, using INT 13 extensions)
 *     0x1B: Hidden Win95+FAT32
 *     0x1C: Hidden Win95+FAT32(using LBA-mode INT 13 extensions)
 */
struct mbr_dpt_entry {
    /* 0x80: This partition is activing; 0x00: Partition is not activing. */
    u8 boot_indicator;
    u8 start_head;
    u16 start_sector   : 6;
    u16 start_cylinder : 10;
    /* Partition type: somewhere call it Operating system indicator. */
    u8 part_type;
    u8 end_head;
    u16 end_sector   : 6;
    u16 end_cylinder : 10;
    /* Partition start LBA */
    u32 lba_start;
    /* Partition length: count of LBA */
    u32 lba_cnt;
};

struct gpt_header {
    u8 signature[8];
    u32 revision;
    u32 header_size;
    u32 header_crc32;
    u32 reserved1;
    u64 my_lba;
    u64 alternate_lba;
    u64 first_usable_lba;
    u64 last_usable_lba;
    u8 disk_guid[16];
    u64 partition_entry_lba;
    u32 num_partition_entries;
    u32 sizeof_partition_entry;
    u32 partition_entry_array_crc32;
};

struct gpt_entry {
    u8 partition_type_guid[16];
    u8 unique_partition_guid[16];
    u64 starting_lba;
    u64 ending_lba;
    u8 other[80]; // attr and part name
};

struct fat_file {
    char name[IMG_NAME_MAX_SIZ];
    u32 name_len;
    u32 size;
    u32 start_clus;
    u32 clus[IMG_MAX_CLUSTER];
};

struct fat_region {
    u32 fs_type;
    u32 start; /* Start sector offset */
    u32 size;  /* Sector count */
};

struct clus_region {
    u32 start;               /* Start sector offset */
    u32 size;                /* Sector count per cluster */
    u32 root_dirent_cluster; /* Root directory cluster id */
};

#define BLK_DEV_TYPE_MMC 0
#define BLK_DEV_TYPE_MSC 1

struct blkdev {
    int type;
    void *priv;
};

struct fat_volume {
    struct blkdev *dev;
    struct fat_region fat;
    struct clus_region clus;
    u32 total_clus_sec;
};

struct dirent_cluster {
    struct fat_volume *vol;
    u32 clus_id;
    u32 sec_idx;
};

s32 aic_fat_read_file(char *filename, void *buf, ulong offset, ulong maxsize,
                      ulong *actread);
s32 aic_fat_set_blk_dev(int id, int type);

#endif /* _FAT_FS_H */

