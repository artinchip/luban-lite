/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <driver.h>
#include <aic_core.h>

#include <image.h>
#include <fatfs.h>
#include <hexdump.h>
#include <mmc.h>
#include <usbhost.h>

#define TAG "FATF"
#define LBA_SIZE 512

static struct blkdev blk_dev;
s32 aic_fat_set_blk_dev(int id, int type)
{
    if (type == BLK_DEV_TYPE_MMC) {
#ifdef AIC_BOOTLOADER_MMC_SUPPORT
        struct aic_sdmc *host;

        host = find_mmc_dev_by_index(id);
        if (host == NULL) {
            pr_err("find mmc dev failed.\n");
            return -1;
        }
        blk_dev.priv = host;
#endif
    } else if (type == BLK_DEV_TYPE_MSC) {
#ifdef AIC_BOOTLOADER_UDISK_SUPPORT
        blk_dev.priv = NULL;
#endif
    }
    blk_dev.type = type;

    return 0;
}

static u32 blkdev_bread(struct blkdev *dev, u32 start_blk, u32 blkcnt, u8 *buf)
{
    if (dev->type == BLK_DEV_TYPE_MMC) {
#ifdef AIC_BOOTLOADER_MMC_SUPPORT
        return mmc_bread(dev->priv, start_blk, blkcnt, buf);
#endif
    } else {
#ifdef AIC_BOOTLOADER_UDISK_SUPPORT
        return usbh_msc_read(start_blk, blkcnt, buf);
#endif
    }

    return 0;
}

static u32 fat32_get_cluster_sec_cnt(u8 *head)
{
    u32 fatsz, fatnum, totsec, resvcnt, sec_per_clus;
    u32 data_sec_cnt, clus_cnt;

    sec_per_clus = (u32)head[13];
    resvcnt = _TO_U16(head, 14);
    fatnum = (u32)head[16];
    totsec = _TO_U32(head, 32);
    fatsz = _TO_U32(head, 36);

    data_sec_cnt = totsec - (resvcnt + fatnum * fatsz);
    if (sec_per_clus)
        clus_cnt = data_sec_cnt / sec_per_clus;
    else
        return 0;

    return clus_cnt;
}

static int fat32_confirm(u8 *head)
{
    u32 root_dir_sec, clus_cnt;

    root_dir_sec = _TO_U16(head, 17);
    /* FAT32 this field should be 0 */
    if (root_dir_sec != 0) {
        pr_err("FAT32 check: Root Directory Sector Count is not 0.");
        return false;
    }

    clus_cnt = fat32_get_cluster_sec_cnt(head);
    if (clus_cnt < FAT32_MINIMAL_CLUS) {
        pr_err("FAT32 check: Cluster Count is not enough: %d\n", clus_cnt);
        return false;
    }
    return true;
}

/*
 * Should verify boot region(11 sectors), but we just verify it in a simple
 * way, because we only have 1 sector data now: Just verify some key fields.
 */
static int exfat_confirm(u8 *head)
{
    int i;
    u8 *p;

    p = head + BS_MUSTBEZERO;
    for (i = 0; i < BS_MUSTBEZERO_LEN; i++) {
        if (*p != 0)
            return false;
        p++;
    }
    return true;
}

static s32 check_identifier(u8 *head)
{
    struct mbr_dpt_entry part;

    /* Last two bytes, MBR identifier */
    if ((head[510] != 0x55) || (head[511] != 0xAA))
        return -1;

    /* Check FAT32 string, 8 characters */
    if (strncmp((char *)&head[BS_FilSysType32], "FAT32   ", 8) == 0) {
        if (fat32_confirm(head))
            return TYPE_RAW_VOLUME_FAT32;
    }

    if (strncmp((char *)&head[BS_FilSysTypeEXFAT], "EXFAT   ", 8) == 0) {
        if (exfat_confirm(head))
            return TYPE_RAW_VOLUME_EXFAT;
    }

    /* Check partition type flag */
    memcpy(&part, head + DPT_START, sizeof(struct mbr_dpt_entry));

    if (part.lba_start == 0 || part.lba_cnt == 0)
        return -1;

    switch (part.part_type) {
        case 0x07: /* exFAT */
        case 0x0B: /* Win95+FAT32 */
        case 0x0C: /* Win95+FAT32(using LBA-mode INT 13 extensions) */
        case 0x1B: /* Hidden Win95+FAT32 */
        case 0x1C: /* Hidden Win95+FAT32(using LBA-mode INT 13 extensions) */
            return TYPE_MBR_PARTITION;
        case 0xEE: /* GPT */
            return TYPE_GPT_PARTITION;
    }

    return -1;
}

static u8 *get_next_dirent(struct dirent_cluster *d_clus, u8 *start, u8 *cur)
{
    u8 *next = (u8 *)((uintptr_t)cur + DIR_ENTRY_SIZE);
    u32 blkcnt, start_sec;

    if (next >= start && next < (start + LBA_SIZE)) {
        /* Still search in current sector */
        return next;
    }

    /* Need to read new sector */
    start_sec = d_clus->vol->clus.start +
                (d_clus->clus_id - 2) * d_clus->vol->clus.size;
    if (d_clus->sec_idx >= d_clus->vol->clus.size) {
        pr_err("Only search in first cluster.\n");
        return NULL;
    }
    start_sec += d_clus->sec_idx;
    blkcnt = blkdev_bread(d_clus->vol->dev, start_sec, 1, start);
    if (blkcnt != 1) {
        pr_err("Read dirent sector failed.\n");
        return NULL;
    }

    d_clus->sec_idx++;
    next = start;

    return next;
}

static s32 fat32_short_name_cmp(u8 *shtent, u8 *fname, u32 len)
{
    int i, j, check_suffix = 0;
    u8 ch;

    for (i = 0; i < 8; i++) {
        if (fname[i] >= 'a' && fname[i] <= 'z')
            ch = fname[i] - 32;
        else
            ch = fname[i];

        if (shtent[i] == ch)
            continue;
        if (ch == '.') {
            check_suffix = 1;
            break;
        }
        break;
    }

    if (i < 8 && check_suffix == 0) {
        return -1;
    }
    i++;
    for (j = 8; j < 11; j++, i++) {
        if (fname[i] >= 'a' && fname[i] <= 'z')
            ch = fname[i] - 32;
        else
            ch = fname[i];
        if (shtent[j] != ch)
            return -1;
    }

    return 0;
}

/*
 * Search file with short file name in root directory.
 *
 * Output:
 *  file data's start cluster id
 */
static s32 fat32_search_file_short(struct fat_volume *vol,
                                   struct fat_file *file, u8 *wbuf)
{
    struct dirent_cluster dclus;
    u8 *pdirent = NULL;
    u32 status;

    if (vol->clus.root_dirent_cluster != 2) {
        pr_err("Root directory entry should use cluster 2.\n");
        return -1;
    }

    memset(&dclus, 0, sizeof(dclus));
    dclus.vol = vol;
    dclus.clus_id = vol->clus.root_dirent_cluster;

    /* Step1: Search File in dirent sectors  */
    while (1) {
        pdirent = get_next_dirent(&dclus, wbuf, pdirent);
        if (!pdirent || pdirent[0] == DIRENT_END) { /* End of dirent list */
            pr_debug("No dirent\n");
            break;
        }
        if (pdirent[0] == DIRENT_DEL) { /* Deleted item */
            pr_debug("Deleted item.\n");
            continue;
        }
        if (LFN_DIR_Flag(pdirent)) { /* Long DIRENT */
            pr_debug("Long DIRENT.\n");
            continue;
        }
        if (SHT_DIR_Filesize(pdirent) == 0) {
            pr_debug("Filesize is 0.\n");
            continue;
        }

        status =
            fat32_short_name_cmp(pdirent, (u8 *)file->name, file->name_len);
        for (int i = 0; i < 11; i++)
            pr_debug("%c %c", pdirent[i], pdirent[i]);
        pr_debug("\n");
        for (int i = 0; i < 11; i++)
            pr_debug("%02x %02x", pdirent[i], pdirent[i]);
        pr_debug("\n");
        if (status != 0)
            continue; /* Not match, next */

        /* Get bootcfg.txt's start cluster */
        file->start_clus = SHT_DIR_StartCluster(pdirent);
        pr_debug("Start cluster: 0x%X\n", file->start_clus);
        file->size = SHT_DIR_Filesize(pdirent);
        pr_debug("bootcfg size: 0x%X\n", file->size);
        return 0;
    }

    return -1;
}

/*
 * Search file with long file name in root directory.
 *
 * Output:
 *  file data's start cluster id
 */
static s32 fat32_search_file_long(struct fat_volume *vol, struct fat_file *file,
                                  u8 *wbuf)
{
    struct dirent_cluster dclus;
    u32 j, chidx, do_flag = 0;
    u8 *pdirent = NULL;
    char lngname[IMG_NAME_MAX_SIZ];

    if (vol->clus.root_dirent_cluster != 2) {
        pr_err("Root directory entry should use cluster 2.\n");
        return -1;
    }
    memset(&dclus, 0, sizeof(dclus));
    dclus.vol = vol;
    dclus.clus_id = vol->clus.root_dirent_cluster;

    /* Step1: Search File in dirent sectors  */
    while (1) {
        /* Search in Long DIRENT only
		 *
		 * Step:
		 * 1. Get parse long DIRENT to get long name first
		 * 2. Compare long name
		 * 3. If long name is match, then parse Short DIRENT
		 *    to get start cluster and file size
		 *
		 * Note:
		 * Long DIRENTs is in Reverse Order.
		 */
        pdirent = get_next_dirent(&dclus, wbuf, pdirent);
        if (!pdirent || pdirent[0] == DIRENT_END) { /* End of dirent list */
            pr_err("No dirent\n");
            break;
        }
        if (pdirent[0] == DIRENT_DEL) /* Deleted item */
            continue;

        /* Long DIRENT and the last one. OK begin parse it */
        if (LFN_DIR_Flag(pdirent) && LFN_DIR_Attr_Last(pdirent)) {
            do_flag = F_PARSE_LONG;
            memset(lngname, 0, IMG_NAME_MAX_SIZ);
            // HEXDUMP("Long dirent", pdirent, 32);
        }

        if ((do_flag == F_PARSE_LONG) && LFN_DIR_Flag(pdirent)) {
            /* Get file name. Only support ASCII */
            chidx = (LFN_DIR_Attr_Number(pdirent) - 1) * 13;

            /* Part 1 */
            for (j = 0x1; j < 0xB; j += 2) {
                if (pdirent[j] != 0xFF)
                    lngname[chidx++] = pdirent[j];
            }
            /* Part 2 */
            for (j = 0xE; j < 0x1A; j += 2) {
                if (pdirent[j] != 0xFF)
                    lngname[chidx++] = pdirent[j];
            }
            /* Part 3 */
            for (j = 0x1C; j < 0x20; j += 2) {
                if (pdirent[j] != 0xFF)
                    lngname[chidx++] = pdirent[j];
            }
        }

        if ((do_flag == F_PARSE_LONG) && LFN_DIR_Flag(pdirent) &&
            (LFN_DIR_Attr_Number(pdirent) == 1)) {
            pr_debug("Find name:%s\n", file->name);
            pr_debug("Got  name:%s\n", lngname);
            /*
             * The last Long DIRENT is parsed, need to
			 * compare file name
			 */
            if (!strncmp(file->name, lngname, file->name_len))
                do_flag = F_PARSE_SHORT;
            else
                do_flag = F_PARSE_NONE;
        }

        if ((do_flag == F_PARSE_SHORT) && (LFN_DIR_Flag(pdirent) == 0) &&
            (SHT_DIR_Filesize(pdirent) != 0)) {
            /*
             * Usually Short DIRENT following after Long
			 * DIRENT for the same file, so here don't
			 * verify it.
			 */
            // HEXDUMP("Short dirent", pdirent, 32);
            file->start_clus = SHT_DIR_StartCluster(pdirent);
            file->size = SHT_DIR_Filesize(pdirent);
            pr_debug("Start cluster: 0x%X\n", file->start_clus);
            pr_debug("Image size: 0x%X\n", file->size);

            return 0;
        }
    }
    return -1;
}

static s32 fat32_search_file(struct fat_volume *vol, struct fat_file *file,
                             u8 *wbuf)
{
    if (file->name_len <= 11)
        return fat32_search_file_short(vol, file, wbuf);

    return fat32_search_file_long(vol, file, wbuf);
}

static s32 exfat_search_file(struct fat_volume *vol, struct fat_file *file,
                             u8 *wbuf)
{
    u32 i, ni, status, secondary_cnt, name_len, data_len, first_cluster;
    u8 *pdirent = NULL;
    char filename[IMG_NAME_MAX_SIZ];
    struct dirent_cluster dclus;

    if (vol->clus.root_dirent_cluster < 2) {
        pr_err("Root directory entry cluster at least 2.\n");
        return -1;
    }

    memset(&dclus, 0, sizeof(dclus));
    dclus.vol = vol;
    dclus.clus_id = vol->clus.root_dirent_cluster;

    /*
	 * 1. Find File Directory Entry
	 * 2. Then get Stream Extension Directory Entry, get start cluster first
	 * 3. Finally get File Name Directory Entry
	 */
    while (1) {
        pdirent = get_next_dirent(&dclus, wbuf, pdirent);
        if (!pdirent || pdirent[0] == TYPE_EXFAT_UNUSED) {
            /* End of Directory / End of search */
            break;
        }
        /* Should start with FILE DIRENT */
        if (pdirent[0] != TYPE_EXFAT_FILE)
            continue;

        /* OK, got one FILE DIRENT */
        pr_debug("Got one FILE DIRENT\n");
        secondary_cnt = (u32)pdirent[1];

        /* Next should be Stream Extension DIRENT, find it */
        while (secondary_cnt) {
            pdirent = get_next_dirent(&dclus, wbuf, pdirent);
            if (!pdirent) {
                pr_err("Read dirent end.\n");
                return -1;
            }
            if (pdirent[0] == TYPE_EXFAT_STREAM)
                break;
            secondary_cnt--;
        }
        if (secondary_cnt == 0)
            continue;
        pr_debug("Got STREAM EXTENSION DIRENT\n");
        name_len = pdirent[3];
        first_cluster = _TO_U32(pdirent, 20);
        /* Get first 32bit only */
        data_len = _TO_U32(pdirent, 24);
        secondary_cnt--;

        if (name_len != file->name_len) {
            pr_err("Filename length is %d\n", name_len);
            continue;
        }

        /* Next should be FILE NAME DIRENT, find it */
        /* get and check the file name, only support ascii name */
        memset(filename, 0, IMG_NAME_MAX_SIZ);
        ni = 0;
        while (secondary_cnt) {
            pdirent = get_next_dirent(&dclus, wbuf, pdirent);
            if (!pdirent) {
                pr_err("Read dirent end.\n");
                return -1;
            }
            secondary_cnt--;
            if (pdirent[0] != TYPE_EXFAT_NAME)
                continue;
            for (i = 0; i < 15; i++) {
                /* Only support ascii filename */
                filename[ni++] = pdirent[2 + (i * 2)];
            }
            if (ni >= name_len)
                break;
        }
        pr_debug("Got filename: %s\n", filename);
        status = memcmp(filename, file->name, file->name_len);
        if (!status) {
            /* Bingo */
            file->start_clus = first_cluster;
            file->size = data_len;
            pr_err("Start cluster: 0x%X\n", file->start_clus);
            pr_err("bootcfg file size: 0x%X\n", file->size);
            return 0;
        }
    }

    return -1;
}

static s32 read_file_data(struct fat_volume *vol, struct fat_file *file,
                          u32 skip_cnt, u32 skip_byte, u32 want_size,
                          u8 *databuf)
{
    u32 i, cnt, clus_id, rdsect_cnt, rdbyte_cnt, got_len, sect_addr, byte_len;
    u8 *p, temp[LBA_SIZE];

    /* First, reuse databuf to seek to data's cluster, and get cluster list */

    p = databuf;
    got_len = 0;
    for (i = 0; i < IMG_MAX_CLUSTER; i++) {
        clus_id = file->clus[i];
        pr_debug("Read cluster id 0x%X, cluster size %d blk\n", clus_id, vol->clus.size);
        if (clus_id == 0)
            break;
        /* Get cluster's sector address */
        sect_addr = vol->clus.start + (clus_id - 2) * vol->clus.size;
        rdsect_cnt = vol->clus.size;

        /* Skip sectors if specified */
        if (skip_cnt) {
            if (vol->clus.size > skip_cnt) {
                /* Skip sectors */
                sect_addr += skip_cnt;
                rdsect_cnt -= skip_cnt;
                skip_cnt = 0;
            } else {
                /* Skip this cluster */
                skip_cnt -= vol->clus.size;
                rdsect_cnt = 0;
                continue;
            }
        }

        if (skip_byte) {
            cnt = blkdev_bread(vol->dev, sect_addr, 1, temp);
            if (cnt != 1) {
                pr_err("Read file failed.\n");
                return -1;
            }
            byte_len = LBA_SIZE - skip_byte;
            memcpy(p, temp + skip_byte, byte_len);
            p += byte_len;
            got_len += byte_len;
            sect_addr += 1;
            rdsect_cnt -= 1;
            skip_byte = 0;
            if (got_len >= want_size)
                return 0;
        }
        rdbyte_cnt = rdsect_cnt * LBA_SIZE;

        if ((got_len + rdbyte_cnt) <= want_size) {
            cnt = blkdev_bread(vol->dev, sect_addr, rdsect_cnt, p);
            if (cnt != rdsect_cnt) {
                pr_err("Read file failed1. rdsect_cnt %d cnt %d\n", rdsect_cnt,
                       cnt);
                return -1;
            }
            p += rdbyte_cnt;
            got_len += rdbyte_cnt;
            if (got_len >= want_size) {
                return 0;
            }
        } else {
            /* The last data and less than one cluster */
            rdbyte_cnt = want_size - got_len;
            rdsect_cnt = (rdbyte_cnt + LBA_SIZE - 1) / LBA_SIZE;
            cnt = blkdev_bread(vol->dev, sect_addr, rdsect_cnt, p);
            if (cnt != rdsect_cnt) {
                pr_err("Read file failed2. rdsect_cnt %d cnt %d\n", rdsect_cnt,
                       cnt);
                return -1;
            }
            p += rdsect_cnt * LBA_SIZE;
            got_len += rdsect_cnt * LBA_SIZE;
            if (got_len >= want_size) {
                return 0;
            }
        }
    }

    return 0;
}

static u32 get_fat_ent_sector_num(struct fat_region *fat, u32 clus_id)
{
    u32 ent_offset = (clus_id * 4) / LBA_SIZE;
    return fat->start + ent_offset;
}

/*
 * Get all cluster ids which file data is using.
 */
static s32 get_file_cluster_list(struct fat_volume *vol, struct fat_file *file,
                                 u8 *wbuf)
{
    u32 cnt = 0, idx = 1, sectnum = 0, clus_id, next_id = 0, cur_sec;
    u8 *pfat, *pnext;

    /* Reuse BOOTLOADER_BUF_ADDR to read image */
    pfat = (u8 *)wbuf;

    clus_id = file->start_clus;
    file->clus[0] = clus_id;
    cur_sec = INVALID_VAL;
    while (1) {
        /* Read FAT sector which current cluster id located, to find out
		 * next cluster id.
		 */
        sectnum = get_fat_ent_sector_num(&vol->fat, clus_id);
        if (sectnum != cur_sec) {
            cur_sec = sectnum;
            cnt = blkdev_bread(vol->dev, cur_sec, 1, pfat);
            if (cnt != 1) {
                pr_err("Read FAT sector failed.\n");
                return -1;
            }
        }

        /* Get next cluster id */
        pnext = pfat + (clus_id * 4) % LBA_SIZE;
        next_id = _TO_U32(pnext, 0);

        if ((vol->fat.fs_type == FS_TYPE_EXFAT) &&
            (next_id == FAT32_ENTRY_UNUSED)) {
            next_id = clus_id + 1;
        }
        if (next_id < 2 || next_id > vol->total_clus_sec)
            break;
        if ((vol->fat.fs_type == FS_TYPE_FAT32) &&
            (next_id == FAT32_ENTRY_LAST))
            break;
        if ((vol->fat.fs_type == FS_TYPE_EXFAT) &&
            (next_id == EXFAT_ENTRY_LAST))
            break;
        file->clus[idx++] = next_id;
        clus_id = next_id;
        if (idx >= IMG_MAX_CLUSTER)
            break;
    }

    return idx;
}

static s32 fat_read_from_fatfs(struct fat_volume *vol, char *filename,
                               u8 *databuf, ulong offset, ulong maxsize,
                               ulong *actread)
{
    struct fat_file *file;
    u32 skip_cnt, skip_byte;
    s32 ret;

    /* Reuse one global var to reduce memory use */
    file = malloc(sizeof(struct fat_file));
    if (!file) {
        pr_err("malloc fat file failed.\n");
        return -1;
    }
    memset(file, 0, sizeof(struct fat_file));

    /* Search filename in root directory cluster data area */
    file->name_len = strlen(filename);
    memcpy(file->name, filename, strlen(filename));
    if (vol->fat.fs_type == FS_TYPE_FAT32) {
        ret = fat32_search_file(vol, file, databuf);
    } else {
        ret = exfat_search_file(vol, file, databuf);
    }

    if (ret != 0) {
        pr_err("Cannot find %s file.\n", filename);
        goto out;
    }

    /* Get all clusters for filename in FAT table */
    ret = get_file_cluster_list(vol, file, databuf);
    if (ret <= 0) {
        pr_err("Get cluster list for bootcfg failed.\n");
        goto out;
    }

    if (offset % LBA_SIZE) {
        pr_err("start offset 0x%lx not alignment with 0x%x\n", offset,
               LBA_SIZE);
    }
    skip_cnt = offset / LBA_SIZE;
    skip_byte = offset % LBA_SIZE;

    /* Read data to buffer: fw_base */
    ret = read_file_data(vol, file, skip_cnt, skip_byte, maxsize, databuf);
    if (ret != 0) {
        pr_err("Read %s file failed.\n", filename);
        goto out;
    }

    *actread = min((u32)file->size, (u32)maxsize);

    free(file);
    return 0;

out:
    if (file)
        free(file);

    return ret;
}

static s32 fat_read_file_from_fatfs(struct fat_volume *vol, char *filename,
                                    u8 *wbuf, u8 *buf, ulong offset,
                                    ulong maxsize, ulong *actread)
{
    s32 ret = 0;

    pr_debug("\n%s\n", __func__);
    ret = fat_read_from_fatfs(vol, filename, buf, offset, maxsize, actread);
    if (ret) {
        pr_err("read from fatfs failed.\n");
        return ret;
    }

    return ret;
}

static s32 fat_read_file_from_raw_volume_fat32(struct blkdev *pdev,
                                               char *filename, u32 vol_start,
                                               u8 *wbuf, u8 *buf, ulong offset,
                                               ulong maxsize, ulong *actread)
{
    struct fat_volume vol;
    u8 *fw_base = wbuf;
    u8 *head = wbuf;
    u32 sect_sz;
    s32 ret;

    memset(&vol, 0, sizeof(vol));
    sect_sz = BPB_SECT_SIZE(head);
    vol.clus.size = BPB_CLUS_SIZE(head);
    vol.fat.start = vol_start + BPB_RESV_SECT_CNT(head);
    vol.fat.size = BPB_FAT32_SIZE(head);
    vol.clus.start = vol.fat.start + vol.fat.size * BPB_FAT_NUM(head);
    vol.clus.root_dirent_cluster = BPB_ROOT_START_CLUS32(head);
    vol.fat.fs_type = FS_TYPE_FAT32;
    vol.dev = pdev;
    vol.total_clus_sec = fat32_get_cluster_sec_cnt(head);

    pr_debug("FAT32 volume start from LBA 0x%x\n", vol_start);
    pr_debug("FAT start %d, size %d, num %d\n", vol.fat.start, vol.fat.size,
             BPB_FAT_NUM(head));
    pr_debug("cluster start %d\n", vol.clus.start);

    if (sect_sz != LBA_SIZE) {
        pr_err("Unsupportted LBA size: 0x%X\n", sect_sz);
        return -1;
    }

    ret = fat_read_file_from_fatfs(&vol, filename, fw_base, buf, offset,
                                   maxsize, actread);
    if (ret) {
        pr_err("Read file from FAT32 failed.\n");
    }

    return ret;
}

static s32 fat_read_file_from_raw_volume_exfat(struct blkdev *pdev,
                                               char *filename, u32 start_lba,
                                               u8 *wbuf, u8 *buf, ulong offset,
                                               ulong maxsize, ulong *actread)
{
    struct fat_volume vol;
    u8 *head = wbuf;
    u8 *fw_base = wbuf;
    u32 sect_sz;
    s32 ret;

    /* Get key information from boot sector */
    memset(&vol, 0, sizeof(vol));
    sect_sz = (1 << BS_SECT_SHIFT(head));
    vol.dev = pdev;
    vol.fat.start = start_lba + BS_FAT_OFFSET(head);
    vol.fat.size = BS_FAT_LEN(head);
    vol.fat.fs_type = FS_TYPE_EXFAT;
    vol.clus.start = start_lba + BS_CLUSTER_OFFSET(head);
    vol.clus.size = (1 << BS_CLUS_SHIFT(head));
    vol.clus.root_dirent_cluster = BS_ROOT_DIR_CLUS(head);
    vol.total_clus_sec = BS_CLUSTER_CNT(head);

    pr_debug("exFAT volume start from LBA 0x%x\n", start_lba);
    pr_debug("FAT start %d, size %d\n", vol.fat.start, vol.fat.size);
    pr_debug("cluster start %d\n", vol.clus.start);
    if (sect_sz != LBA_SIZE) {
        pr_err("Unsupportted LBA size: 0x%X\n", sect_sz);
        return -1;
    }

    ret = fat_read_file_from_fatfs(&vol, filename, fw_base, buf, offset,
                                   maxsize, actread);
    if (ret) {
        pr_err("Load image file from exFAT failed.\n");
    }

    return ret;
}

static s32 fat_read_file_from_mbr_partition(struct blkdev *pdev, char *filename,
                                            struct mbr_dpt_entry *part,
                                            u8 *wbuf, u8 *buf, ulong offset,
                                            ulong maxsize, ulong *actread)
{
    u32 blkcnt = 0;
    s32 type;

    blkcnt = blkdev_bread(pdev, part->lba_start, 1, wbuf);
    if (blkcnt != 1) {
        pr_err("Read partition first sector failed.\n");
        return -1;
    }

    type = check_identifier(wbuf);
    if (type == TYPE_RAW_VOLUME_FAT32) {
        return fat_read_file_from_raw_volume_fat32(pdev, filename,
                                                   part->lba_start, wbuf, buf,
                                                   offset, maxsize, actread);
    }
    if (type == TYPE_RAW_VOLUME_EXFAT) {
        return fat_read_file_from_raw_volume_exfat(pdev, filename,
                                                   part->lba_start, wbuf, buf,
                                                   offset, maxsize, actread);
    }
    pr_debug("Partition is not FATFS.\n");

    return -1;
}

static s32 fat_read_file_from_gpt_partition(struct blkdev *pdev, char *filename,
                                            u8 *wbuf, u8 *buf, ulong offset,
                                            ulong maxsize, ulong *actread)
{
    struct gpt_header gpthead;
    struct gpt_entry gptent;
    u32 blkcnt = 0;
    s32 type;

    /* Read LBA1 for GPT header */
    blkcnt = blkdev_bread(pdev, 1, 1, wbuf);
    if (blkcnt != 1) {
        pr_err("Read LBA1 failed.\n");
        return -1;
    }

    memcpy(&gpthead, wbuf, sizeof(struct gpt_header));
    if (strncmp((char *)gpthead.signature, "EFI PART", 8)) {
        pr_err("LBA1 is not GPT header.\n");
        return -1;
    }

    /* Read the first partition entry, only check the first partition */
    blkcnt = blkdev_bread(pdev, gpthead.partition_entry_lba, 1, wbuf);
    if (blkcnt != 1) {
        pr_err("Read the first partition entry failed.\n");
        return -1;
    }
    memcpy(&gptent, wbuf, sizeof(struct gpt_entry));

    /* Read first LBA of the first GPT partition */
    blkcnt = blkdev_bread(pdev, gptent.starting_lba, 1, wbuf);
    if (blkcnt != 1) {
        pr_err("Read the first partition entry failed.\n");
        return -1;
    }

    type = check_identifier(wbuf);
    if (type == TYPE_RAW_VOLUME_FAT32) {
        return fat_read_file_from_raw_volume_fat32(pdev, filename,
                                                   gptent.starting_lba, wbuf,
                                                   buf, offset, maxsize,
                                                   actread);
    }

    if (type == TYPE_RAW_VOLUME_EXFAT) {
        return fat_read_file_from_raw_volume_exfat(pdev, filename,
                                                   gptent.starting_lba, wbuf,
                                                   buf, offset, maxsize,
                                                   actread);
    }
    pr_debug("Partition is not FAT.\n");

    return -1;
}

/*
 * SD Card data layout(3 cases):
 * 1. No partition table, FAT32 FS volume only
 *    Sector 0 is BPB(BIOS Parameter Block)
 *
 * 2. With MBR partition
 *    Sector 0 is MBR with partition table
 *
 * 3. With GPT partition
 *    Sector 0 is Reserved MBR, Sector 1 is GPT header
 *
 * If SD card with partition table, only search in the first table.
 */
s32 aic_fat_read_file(char *filename, void *buf, ulong offset, ulong maxsize,
                      ulong *actread)
{
    struct mbr_dpt_entry part;
    s32 ret = 0, type;
    u32 blkcnt = 0;
    u8 *wbuf = NULL;

    wbuf = (u8 *)aicos_malloc_align(0, 1024, CACHE_LINE_SIZE);
    if (!wbuf) {
        pr_err("malloc wbuf failed.\n");
        return -1;
    }

    blkcnt = blkdev_bread(&blk_dev, 0, SD_BOOT_HEAD_BLK_CNT, wbuf);
    if (blkcnt != SD_BOOT_HEAD_BLK_CNT) {
        pr_err("Failed to read data from SD, IO error.\n");
        return -1;
    }

    type = check_identifier(wbuf);
    switch (type) {
        case TYPE_MBR_PARTITION:
            pr_debug("TYPE_MBR_PARTITION\n");
            /* MBR partition exist, try to boot from the first partition */
            memcpy(&part, wbuf + DPT_START, sizeof(part));
            ret = fat_read_file_from_mbr_partition(
                &blk_dev, filename, &part, wbuf, buf, offset, maxsize, actread);
            break;
        case TYPE_GPT_PARTITION:
            pr_debug("TYPE_GPT_PARTITION\n");
            /* GPT partition exist, try to boot from the first partition */
            ret = fat_read_file_from_gpt_partition(&blk_dev, filename, wbuf, buf,
                                                   offset, maxsize, actread);
            break;
        case TYPE_RAW_VOLUME_FAT32:
            pr_debug("TYPE_RAW_VOLUME_FAT32\n");
            /* There is no partition table in SD card, just FAT32 FS */
            ret = fat_read_file_from_raw_volume_fat32(
                &blk_dev, filename, 0, wbuf, buf, offset, maxsize, actread);
            break;
        case TYPE_RAW_VOLUME_EXFAT:
            pr_debug("TYPE_RAW_VOLUME_EXFAT\n");
            /* There is no partition table in SD card, just exFAT FS */
            ret = fat_read_file_from_raw_volume_exfat(
                &blk_dev, filename, 0, wbuf, buf, offset, maxsize, actread);
            break;
        default:
            pr_err("No FATFS.\n");
            ret = -1;
    }

    aicos_free_align(0, wbuf);
    return ret;
}
