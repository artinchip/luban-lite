/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */
#include <rtconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <mtd.h>
#include <aic_core.h>
#include <libfdt.h>
#include "fitimage.h"

int fit_find_config_node(const void *fdt)
{
    const char *name;
    int conf, node, len;
    const char *dflt_conf_name;
    const char *dflt_conf_desc = NULL;
    int dflt_conf_node = -1;

    conf = fdt_path_offset(fdt, FIT_CONFS_PATH);
    if (conf < 0) {
        printf("%s: Cannot find /configurations node: %d\n", __func__,
              conf);
        return -1;
    }

    dflt_conf_name = fdt_getprop(fdt, conf, "default", &len);

    for (node = fdt_first_subnode(fdt, conf);
         node >= 0;
         node = fdt_next_subnode(fdt, node)) {
        name = fdt_getprop(fdt, node, "description", &len);
        if (!name) {
            printf("%s: Missing FDT description in DTB\n",
                   __func__);
            return -1;
        }

        if (dflt_conf_name) {
            const char *node_name = fdt_get_name(fdt, node, NULL);
            if (strcmp(dflt_conf_name, node_name) == 0) {
                dflt_conf_node = node;
                dflt_conf_desc = name;
            }
        }
    }

    if (dflt_conf_node != -1) {
        printf("Selecting default config '%s'\n", dflt_conf_desc);
        return dflt_conf_node;
    }

    return -1;
}

static int spl_simple_fit_parse(struct spl_fit_info *ctx)
{
    /* Find the correct subnode under "/configurations" */
    ctx->conf_node = fit_find_config_node(ctx->fit);
    if (ctx->conf_node < 0)
    {
        printf("%s: Cannot find /configurations node: %d\n", __func__,
              ctx->conf_node);
        return -1;
    }

    /* find the node holding the images information */
    ctx->images_node = fdt_path_offset(ctx->fit, FIT_IMAGES_PATH);
    if (ctx->images_node < 0) {
        printf("%s: Cannot find /images node: %d\n", __func__,
              ctx->images_node);
        return -1;
    }

    return 0;
}

int spl_fit_get_image_name(const struct spl_fit_info *ctx,
                  const char *type, int index,
                  const char **outname)
{
    const char *name, *str;
    int len, i;
    bool found = true;

    name = fdt_getprop(ctx->fit, ctx->conf_node, type, &len);
    if (!name) {
        printf("cannot find property '%s': %d\n", type, len);
        return -1;
    }

    str = name;
    for (i = 0; i < index; i++) {
        str = strchr(str, '\0') + 1;
        if (!str || (str - name >= len)) {
            found = false;
            break;
        }
    }

    if (!found) {
        printf("no string for index %d\n", index);
        return -1;
    }

    *outname = str;
    return 0;
}

int spl_fit_get_image_node(const struct spl_fit_info *ctx,
                  const char *type, int index)
{
    const char *str;
    int err;
    int node;

    err = spl_fit_get_image_name(ctx, type, index, &str);
    if (err)
        return err;

    node = fdt_subnode_offset(ctx->fit, ctx->images_node, str);
    if (node < 0) {
        printf("cannot find image node '%s': %d\n", str, node);
        return -1;
    }

    return node;
}

static void fit_get_debug(const void *fit, int noffset,
        char *prop_name, int err)
{
    printf("Can't get '%s' property from FIT 0x%08lx, node: offset %d, name %s (%s)\n",
          prop_name, (ulong)fit, noffset, fit_get_name(fit, noffset, NULL),
          fdt_strerror(err));
}

static int fit_image_get_address(const void *fit, int noffset, char *name,
              ulong *load)
{
    int len, cell_len;
    const fdt32_t *cell;
    uint64_t load64 = 0;

    cell = fdt_getprop(fit, noffset, name, &len);
    if (cell == NULL) {
        fit_get_debug(fit, noffset, name, len);
        return -1;
    }

    cell_len = len >> 2;
    /* Use load64 to avoid compiling warning for 32-bit target */
    while (cell_len--) {
        load64 = (load64 << 32) | uimage_to_cpu(*cell);
        cell++;
    }

    if (len > sizeof(ulong) && (uint32_t)(load64 >> 32)) {
        printf("Unsupported %s address size\n", name);
        return -1;
    }

    *load = (ulong)load64;

    return 0;
}

int fit_image_get_load(const void *fit, int noffset, ulong *load)
{
    return fit_image_get_address(fit, noffset, FIT_LOAD_PROP, load);
}

int fit_image_get_data(const void *fit, int noffset,
        const void **data, size_t *size)
{
    int len;

    *data = fdt_getprop(fit, noffset, FIT_DATA_PROP, &len);
    if (*data == NULL) {
        fit_get_debug(fit, noffset, FIT_DATA_PROP, len);
        *size = 0;
        return -1;
    }

    *size = len;
    return 0;
}

int fit_image_get_data_position(const void *fit, int noffset,
                int *data_position)
{
    const fdt32_t *val;

    val = fdt_getprop(fit, noffset, FIT_DATA_POSITION_PROP, NULL);
    if (!val)
        return -ENOENT;

    *data_position = fdt32_to_cpu(*val);

    return 0;
}

int fit_image_get_data_offset(const void *fit, int noffset, int *data_offset)
{
    const fdt32_t *val;

    val = fdt_getprop(fit, noffset, FIT_DATA_OFFSET_PROP, NULL);
    if (!val)
        return -ENOENT;

    *data_offset = fdt32_to_cpu(*val);

    return 0;
}

int fit_image_get_data_size(const void *fit, int noffset, unsigned int *data_size)
{
    const fdt32_t *val;

    val = fdt_getprop(fit, noffset, FIT_DATA_SIZE_PROP, NULL);
    if (!val)
        return -ENOENT;

    *data_size = fdt32_to_cpu(*val);

    return 0;
}

int fit_image_get_entry(const void *fit, int noffset, ulong *entry)
{
    return fit_image_get_address(fit, noffset, FIT_ENTRY_PROP, entry);
}

int spl_load_fit_image(struct spl_load_info *info, struct spl_fit_info *ctx, int node, ulong *entry_point)
{
    ulong load_addr = 0;
    unsigned int length;
    int ret, offset = 0;
    int start_offset, overhead, read_length;
    const void *fit = ctx->fit;
    bool external_data = false;
    struct mtd_dev *mtd = (struct mtd_dev *)info->dev;

    if (fit_image_get_load(fit, node, &load_addr))
    {
        printf("Can't load %s: No load address\n", fit_get_name(fit, node, NULL));
        return -1;
    }

    if (!fit_image_get_data_position(fit, node, &offset))
    {
        external_data = true;
    }
    else if (!fit_image_get_data_offset(fit, node, &offset))
    {
        offset += ctx->ext_data_offset;
        external_data = true;
    }

    if (external_data)
    {
        if (fit_image_get_data_size(fit, node, &length))
            return -1;

        start_offset = ALIGN_DOWN(offset, info->bl_len);
        overhead = offset - start_offset;
        read_length = ALIGN_UP(offset + length, info->bl_len) - start_offset;

        ret = mtd_read(mtd, start_offset, (u8 *)(load_addr - overhead), read_length);
        if (ret < 0)
        {
            printf("mtd read external_data error\n");
            return -1;
        }
    }
    else
    {
        printf("External_data not found in ITB, cann't load image\n");
        return -1;
    }

    if (entry_point)
    {
        if (fit_image_get_entry(fit, node, entry_point))
        {
            printf("Can't load %s: No entry_point address\n", fit_get_name(fit, node, NULL));
            return -1;
        }
    }

    return 0;
}

int spl_load_simple_fit(struct spl_load_info *info, ulong *entry_point)
{
    struct fdt_header header;
    struct spl_fit_info ctx;
    void *buf = NULL;
    int size, ret = 0;
    int index = 0, node = -1;
    struct mtd_dev *mtd = (struct mtd_dev *)info->dev;

    ret = mtd_read(mtd, 0, (void *)&header, sizeof(header));
    if (ret < 0)
    {
        printf("mtd read header error\n");
        return -1;
    }

    if (fdt_magic(&header) != FDT_MAGIC)
    {
        printf("Not found FIT\n");
        return -1;
    }

    size = FIT_ALIGN(fdt_totalsize(&header), 4);
    ctx.ext_data_offset = size;

    buf = aicos_malloc(MEM_DEFAULT, size);
    if (!buf)
    {
        printf("No space to malloc for itb\n");
        return -1;
    }

    ret = mtd_read(mtd, 0, buf, size);
    if (ret < 0)
    {
        printf("mtd read itb error\n");
        goto __exit;
    }

    ctx.fit = (void *)buf;

    ret = spl_simple_fit_parse(&ctx);
    if (ret < 0)
    {
        printf("fit parse error\n");
        goto __exit;
    }

    for (; ; index++)
    {
        node = spl_fit_get_image_node(&ctx, FIT_FIRMWARE_PROP, index);
        if (node < 0)
        {
            if (!index)
            {
                printf("FIT get image node error\n");
                ret = -1;
                goto __exit;
            }
            else
            {
                break;
            }
        }

        /* Only seg0 node has entry attribute,  */
        if (index)
            entry_point = NULL;

        ret = spl_load_fit_image(info, &ctx, node, entry_point);
        if (ret)
        {
            printf("FIT load image error\n");
            goto __exit;
        }
    }

__exit:
    aicos_free(MEM_DEFAULT, buf);
    return ret;
}
