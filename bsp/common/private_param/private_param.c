/*
 * Copyright (c) 2020-2021 Artinchip Technology Co., Ltd
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <private_param.h>

static u32 *find_section(void *start, u32 type)
{
    u32 *p = start, data_type, data_len;

    while (1) {
        data_type = *p;
        if (data_type == type) {
            return p;
        }
        if (data_type == DATA_SECT_TYPE_END)
            return NULL;
        p++;

        data_len = *p;
        p++;

        p += (data_len / 4);
    }
    return NULL;
}

struct private_dram_param *private_get_ddr_init_param(void *res_addr, u32 type)
{
    struct private_dram_param *ddr;
    u32 *p, entry_cnt, len, data_len;

    if (res_addr == NULL) {
        puts("res not exist\n");
        return NULL;
    }

    /* Should be 4 byte aligned */
    if (((unsigned long)res_addr) & 0x3) {
        puts("res addr not alignment\n");
        return NULL;
    }

    p = find_section(res_addr, DATA_SECT_TYPE_DRAM);
    if (p == NULL) {
        puts("ddr %d init param not exist\n");
        return NULL;
    }
    p++;
    data_len = *p;
    p++;
    entry_cnt = *p;
    p++;

    len = data_len - 4; /* Entry count */
    if (len < (sizeof(struct private_dram_param) * entry_cnt)) {
        puts("length is invalid.\n");
        return NULL;
    }

    ddr = (void *)p;
    while (entry_cnt) {
        if (ddr->type == type)
            return ddr;
        ddr++;
        entry_cnt--;
    }
    puts("DDR parameter is not found.\n");
    return NULL;
}

struct private_system_upgmode_pin_param *
private_get_system_upgmode_pin_param(void *res_addr)
{
    struct private_system_upgmode_pin_param *sys;
    u32 *p, data_len;

    if (res_addr == NULL) {
        return NULL;
    }

    /* Should be 4 byte aligned */
    if (((unsigned long)res_addr) & 0x3) {
        return NULL;
    }

    p = find_section(res_addr, DATA_SECT_TYPE_SYS_UPGMODE);
    if (p == NULL) {
        return NULL;
    }
    p++;
    data_len = *p;
    p++;
    if (data_len < (sizeof(struct private_system_upgmode_pin_param)))
        return NULL;

    sys = (void *)p;
    return sys;
}

struct private_system_uart_param *private_get_system_uart_param(void *res_addr)
{
    struct private_system_uart_param *sys;
    u32 *p, data_len;

    if (res_addr == NULL) {
        return NULL;
    }

    /* Should be 4 byte aligned */
    if (((unsigned long)res_addr) & 0x3) {
        return NULL;
    }

    p = find_section(res_addr, DATA_SECT_TYPE_SYS_UART);
    if (p == NULL) {
        return NULL;
    }
    p++;
    data_len = *p;
    p++;
    if (data_len < (sizeof(struct private_system_uart_param)))
        return NULL;

    sys = (void *)p;
    return sys;
}

struct private_system_jtag_head *private_get_system_jtag_param(void *res_addr)
{
    struct private_system_jtag_head *jtag;
    u32 *p, data_len;

    if (res_addr == NULL) {
        return NULL;
    }

    /* Should be 4 byte aligned */
    if (((unsigned long)res_addr) & 0x3) {
        return NULL;
    }

    p = find_section(res_addr, DATA_SECT_TYPE_SYS_JTAG);
    if (p == NULL) {
        return NULL;
    }
    p++;
    data_len = *p;
    p++;
    if (data_len < (sizeof(struct private_system_jtag_param)))
        return NULL;

    jtag = (void *)p;
    return jtag;
}

char *private_get_partition_string(void *res_addr)
{
    u32 *p, data_len;

    if (res_addr == NULL) {
        return NULL;
    }

    /* Should be 4 byte aligned */
    if (((unsigned long)res_addr) & 0x3) {
        return NULL;
    }

    p = find_section(res_addr, DATA_SECT_TYPE_PARTITION);
    if (p == NULL) {
        return NULL;
    }
    p++;
    data_len = *p;
    p++;

    if (data_len == 0)
        return NULL;
    return (char *)p;
}
