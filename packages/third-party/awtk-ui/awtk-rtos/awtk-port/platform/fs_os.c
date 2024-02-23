/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 *
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "dirent.h"
#include "stdint.h"
#include "unistd.h"
#include <pwd.h>
#include "tkc/types_def.h"
#include "tkc/utf8.h"
#include "tkc/mem.h"
#include "tkc/fs.h"
#include "tkc/mem.h"
#include "tkc/utils.h"

#define DT_LNK 16

static ret_t fs_stat_info_from_stat(fs_stat_info_t* fst, struct stat* st) {
  return_value_if_fail(fst != NULL && st != NULL, RET_BAD_PARAMS);
  memset(fst, 0x00, sizeof(fs_stat_info_t));
  fst->dev = st->st_dev;
  fst->ino = st->st_ino;
  fst->mode = st->st_mode;
  fst->nlink = st->st_nlink;
  fst->uid = st->st_uid;
  fst->gid = st->st_gid;
  fst->rdev = st->st_rdev;
  fst->size = st->st_size;
  fst->atime = st->st_atime;
  fst->mtime = st->st_mtime;
  fst->ctime = st->st_ctime;
  fst->is_dir = (st->st_mode & S_IFDIR) != 0;
#ifdef S_IFLNK
  fst->is_link = (st->st_mode & S_IFLNK) != 0;
#else
  fst->is_link = FALSE;
#endif /*S_IFLNK*/
  fst->is_reg_file = (st->st_mode & S_IFREG) != 0;

  return RET_OK;
}

static int32_t fs_os_file_read(fs_file_t* file, void* buffer, uint32_t size) {
  FILE* fp = (FILE*)(file->data);

  return (int32_t)fread(buffer, 1, size, fp);
}

static int32_t fs_os_file_write(fs_file_t* file, const void* buffer, uint32_t size) {
  FILE* fp = (FILE*)(file->data);

  return fwrite(buffer, 1, size, fp);
}

static int32_t fs_os_file_printf(fs_file_t* file, const char* const format_str, va_list vl) {
  FILE* fp = (FILE*)(file->data);

  return vfprintf(fp, format_str, vl);
}

static ret_t fs_os_file_seek(fs_file_t* file, int32_t offset) {
  FILE* fp = (FILE*)(file->data);

  return fseek(fp, offset, SEEK_SET) == 0 ? RET_OK : RET_FAIL;
}

static int64_t fs_os_file_tell(fs_file_t* file) {
  FILE* fp = (FILE*)(file->data);

  return ftell(fp);
}

static int64_t fs_os_file_size(fs_file_t* file) {
  fs_stat_info_t st;

  if (fs_file_stat(file, &st) == RET_OK && st.is_reg_file) {
    return st.size;
  } else {
    return -1;
  }
}

static ret_t fs_os_file_stat(fs_file_t* file, fs_stat_info_t* fst) {
  int rc = 0;
  FILE* fp = (FILE*)(file->data);

  struct stat st;
  rc = fstat(fileno(fp), &st);

  if (rc == 0) {
    return fs_stat_info_from_stat(fst, &st);
  } else {
    memset(fst, 0x00, sizeof(fs_stat_info_t));

    return RET_FAIL;
  }
}

static ret_t fs_os_file_sync(fs_file_t* file) {
  FILE* fp = (FILE*)(file->data);

  fflush(fp);
  return fsync(fileno(fp)) == 0 ? RET_OK : RET_FAIL;
}

static ret_t fs_os_file_truncate(fs_file_t* file, int32_t size) {
  FILE* fp = (FILE*)(file->data);

  if (fseek(fp, size, SEEK_END) != 0) {
    return RET_FAIL;
  }

  if (ftruncate(fileno(fp), ftell(fp)) != 0) {
    return RET_FAIL;
  }

  return RET_OK;
}

static bool_t fs_os_file_eof(fs_file_t* file) {
  FILE* fp = (FILE*)(file->data);

  return feof(fp) != 0;
}

static ret_t fs_os_file_close(fs_file_t* file) {
  FILE* fp = (FILE*)(file->data);
  fclose(fp);
  TKMEM_FREE(file);

  return RET_OK;
}

static ret_t fs_os_dir_rewind(fs_dir_t* dir) {
  DIR *d = (DIR *)(dir->data);

  rewinddir(d);

  return RET_OK;
}

static ret_t fs_os_dir_read(fs_dir_t* dir, fs_item_t* item) {
  DIR* d = (DIR*)(dir->data);
  struct dirent* ent = readdir(d);

  memset(item, 0x00, sizeof(fs_item_t));
  if (ent != NULL) {
    uint8_t type = ent->d_type;
    item->is_dir = (type & DT_DIR) != 0;
    item->is_link = (type & DT_LNK) != 0;
    item->is_reg_file = (type & DT_REG) != 0;
    tk_strncpy(item->name, ent->d_name, MAX_PATH);
    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

static ret_t fs_os_dir_close(fs_dir_t* dir) {
  DIR* d = (DIR*)dir->data;
  closedir(d);
  TKMEM_FREE(dir);

  return RET_OK;
}

static const fs_file_vtable_t s_file_vtable = {.read = fs_os_file_read,
                                               .write = fs_os_file_write,
                                               .printf = fs_os_file_printf,
                                               .seek = fs_os_file_seek,
                                               .tell = fs_os_file_tell,
                                               .size = fs_os_file_size,
                                               .stat = fs_os_file_stat,
                                               .sync = fs_os_file_sync,
                                               .truncate = fs_os_file_truncate,
                                               .eof = fs_os_file_eof,
                                               .close = fs_os_file_close};

static fs_file_t* fs_file_create(FILE* fp) {
  fs_file_t* f = NULL;
  return_value_if_fail(fp != NULL, NULL);

  f = TKMEM_ZALLOC(fs_file_t);
  if (f != NULL) {
    f->vt = &s_file_vtable;
    f->data = fp;
  } else {
    fclose(fp);
  }

  return f;
}

static fs_file_t* fs_os_open_file(fs_t* fs, const char* name, const char* mode) {
  (void)fs;
  return_value_if_fail(name != NULL && mode != NULL, NULL);

  return fs_file_create(fopen(name, mode));
}

static ret_t fs_os_remove_file(fs_t* fs, const char* name) {
  (void)fs;
  return_value_if_fail(name != NULL, RET_FAIL);

  unlink(name);

  return RET_OK;
}

static bool_t fs_os_file_exist(fs_t* fs, const char* name) {
  fs_stat_info_t st;
  return_value_if_fail(name != NULL, FALSE);

  if (fs_stat(fs, name, &st) == RET_OK) {
    return st.is_reg_file;
  } else {
    return FALSE;
  }
}

static ret_t fs_os_file_rename(fs_t* fs, const char* name, const char* new_name) {
  (void)fs;
  return_value_if_fail(name != NULL && new_name != NULL, RET_BAD_PARAMS);

  return rename(name, new_name) == 0 ? RET_OK : RET_FAIL;
}

static const fs_dir_vtable_t s_dir_vtable = {
    .read = fs_os_dir_read, .rewind = fs_os_dir_rewind, .close = fs_os_dir_close};

static fs_dir_t* fs_dir_create(DIR* dir) {
  fs_dir_t* d = NULL;
  return_value_if_fail(dir != NULL, NULL);

  d = TKMEM_ZALLOC(fs_dir_t);
  if (d != NULL) {
    d->vt = &s_dir_vtable;
    d->data = dir;
  } else {
    closedir(dir);
  }

  return d;
}

static fs_dir_t* fs_os_open_dir(fs_t* fs, const char* name) {
  (void)fs;
  return_value_if_fail(name != NULL, NULL);

  return fs_dir_create(opendir(name));
}

static ret_t fs_os_remove_dir(fs_t* fs, const char* name) {
  (void)fs;
  return_value_if_fail(name != NULL, RET_FAIL);

  if (rmdir(name) == 0) {
    return RET_OK;
  } else {
    perror(name);
    return RET_FAIL;
  }
}

static ret_t fs_os_change_dir(fs_t* fs, const char* name) {
  (void)fs;
  return_value_if_fail(name != NULL, RET_FAIL);

  if (chdir(name) == 0) {
    return RET_OK;
  } else {
    perror(name);
    return RET_FAIL;
  }
}

static ret_t fs_os_create_dir(fs_t* fs, const char* name) {
  (void)fs;
  return_value_if_fail(name != NULL, RET_FAIL);

  if (mkdir(name, 0755) == 0) {
    return RET_OK;
  } else {
    perror(name);
    return RET_FAIL;
  }
}

static bool_t fs_os_dir_exist(fs_t* fs, const char* name) {
  fs_stat_info_t st;
  return_value_if_fail(name != NULL, FALSE);

  if (fs_stat(fs, name, &st) == RET_OK) {
    return st.is_dir;
  } else {
    return FALSE;
  }
}

static ret_t fs_os_dir_rename(fs_t* fs, const char* name, const char* new_name) {
  return fs_os_file_rename(fs, name, new_name);
}

static int32_t fs_os_get_file_size(fs_t* fs, const char* name) {
  fs_stat_info_t st;
  return_value_if_fail(name != NULL, FALSE);

  if (fs_stat(fs, name, &st) == RET_OK && st.is_reg_file) {
    return st.size;
  } else {
    return -1;
  }
}

static ret_t fs_os_get_disk_info(fs_t* fs, const char* volume, int32_t* free_kb,
                                 int32_t* total_kb) {
  /*TODO*/
  *free_kb = 0;
  *total_kb = 0;
  (void)fs;
  assert(!"fs_os_get_disk_info not supported yet");

  return RET_FAIL;
}

static ret_t fs_os_get_exe(fs_t* fs, char path[MAX_PATH + 1]) {
  (void)fs;

  return RET_FAIL;
}

static ret_t fs_os_get_temp_path(fs_t* fs, char path[MAX_PATH + 1]) {
  const char* tempdir = NULL;
  memset(path, 0x00, MAX_PATH + 1);

  if ((tempdir = getenv("TMPDIR")) == NULL) {
    tempdir = "/tmp";
  }

  return_value_if_fail(tempdir != NULL, RET_FAIL);
  tk_strncpy(path, tempdir, MAX_PATH);

  return RET_OK;
}

static ret_t fs_os_get_user_storage_path(fs_t* fs, char path[MAX_PATH + 1]) {
  return RET_FAIL;
}

static ret_t fs_os_get_cwd(fs_t* fs, char path[MAX_PATH + 1]) {
  return_value_if_fail(fs != NULL && path != NULL, RET_BAD_PARAMS);

  memset(path, 0x00, MAX_PATH + 1);
  getcwd(path, MAX_PATH);

  return RET_OK;
}

static ret_t fs_os_stat(fs_t* fs, const char* name, fs_stat_info_t* fst) {
  (void)fs;
  return_value_if_fail(name != NULL && fst != NULL, RET_BAD_PARAMS);

  int stat_ret = 0;
  struct stat st;
  stat_ret = stat(name, &st);

  if (stat_ret == -1) {
    return RET_FAIL;
  } else {
    return fs_stat_info_from_stat(fst, &st);
  }
}

static const fs_t s_os_fs = {.open_file = fs_os_open_file,
                             .remove_file = fs_os_remove_file,
                             .file_exist = fs_os_file_exist,
                             .file_rename = fs_os_file_rename,

                             .open_dir = fs_os_open_dir,
                             .remove_dir = fs_os_remove_dir,
                             .create_dir = fs_os_create_dir,
                             .change_dir = fs_os_change_dir,
                             .dir_exist = fs_os_dir_exist,
                             .dir_rename = fs_os_dir_rename,

                             .get_file_size = fs_os_get_file_size,
                             .get_disk_info = fs_os_get_disk_info,
                             .get_cwd = fs_os_get_cwd,
                             .get_exe = fs_os_get_exe,
                             .get_user_storage_path = fs_os_get_user_storage_path,
                             .get_temp_path = fs_os_get_temp_path,
                             .stat = fs_os_stat};

fs_t* os_fs(void) {
  return (fs_t*)&s_os_fs;
}
