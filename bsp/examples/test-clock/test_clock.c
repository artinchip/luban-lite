/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0*
 *
 * Authors: Senye.Liang <senye.liang@artinchip.com>
 */
#include <string.h>
#include <ctype.h>
#include "aic_core.h"
#include "aic_clk_id.h"
#include "aic_hal_clk.h"

enum cmu_cmd {
    GET_INFO = 0,
    GET_PARENT =1,
};

static int __get_info_byid(uint32_t id)
{
    const char *name = NULL;
    unsigned long rate;
    int status = 0;

    name = hal_clk_get_name(id);
    if (name == NULL)
        return -1;

    rate = hal_clk_get_freq(id);
    status = hal_clk_is_enabled(id);

     printf("    %-6ld      %-14s      %-10lu      %10d\n",
           (long int)id, name, rate, status);

    return 0;
}

static int clk_dump(char *argv)
{
    uint32_t id, start_id;
    int ret;

    start_id = strtoul(argv, NULL, 10);
    if (start_id < 0 || start_id > AIC_CLK_END)
        start_id = 0;

    for (id = start_id; id < AIC_CLK_END; id++) {
        ret = __get_info_byid(id);
        if (ret < 0)
            continue;
    }

    return 0;
}

/* RT-THREAD */
#if defined(RT_USING_FINSH)
#include <finsh.h>
#include <getopt.h>
static int __get_parent_byid(uint32_t id)
{
    int parent_id;
    parent_id = hal_clk_get_parent(id);
    if (parent_id < 0) {
        __get_info_byid(id);
        return -1;
    }

    __get_parent_byid(parent_id);
    __get_info_byid(id);

    return 0;
}

static int __get_byname(char *name, int cmu_cmd)
{
    const char *clk_name;
	int i, ret;
    bool flag = false;

	for (i = 0; i < AIC_CLK_END; i++) {
		clk_name = hal_clk_get_name(i);
        if (clk_name == NULL)
            continue;
		ret = strncmp(clk_name, name, strlen(name));
		if (ret == 0) {
            switch (cmu_cmd) {
            case GET_INFO:
                __get_info_byid(i);
                break;
            case GET_PARENT:
                __get_parent_byid(i);
                printf("\n");
                break;
            default:
                break;
            }
            flag = true;
        }
	}

    if (flag != true)
        return -1;

    return 0;
}

static int clk_set_rate(char *str_id, char *str_rate)
{
    int ret;
    unsigned long id, rate;

    id = strtoul(str_id, NULL, 10);
    rate = strtoul(str_rate, NULL, 10);

    ret = hal_clk_set_freq(id, rate);
    if (ret < 0)
        return -1;

    __get_info_byid(id);

    return 0;
}

static int clk_get_info(char *argv, int cmu_cmd)
{
    int ret = 0;
	uint32_t id = 0;

    id = strtoul(argv, NULL, 10);

    switch (cmu_cmd) {
    case GET_INFO:
        ret = (isalpha((unsigned char)*argv) == 0) ?
               __get_info_byid(id):__get_byname(argv, GET_INFO);
        if (ret < 0)
            goto __err;
        break;
    case GET_PARENT:
        ret = (isalpha((unsigned char)*argv) == 0) ?
               __get_parent_byid(id):__get_byname(argv, GET_PARENT);
        if (ret < 0)
            goto __err;
        break;
    default:
        break;
    }

	return 0;

__err:
    printf("failed: no clk: %s!\n", argv);
    return -1;
}

static void cmd_cmu_usage(char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\t -a, \tall clk info \n");
    printf("\t -f [id] or [name],\tget clk info by id or name\n");
    printf("\t -p [id] or [name],\tget clk parent by id or name\n");
    printf("\t -s [id],\t\tset clk rate by id\n");
    printf("\t -h ,\tusage\n");
}

static char sopts[] = "af:p:s:h";
static struct option lopts[] = {
    {"-a all clk info ",    no_argument, NULL, 'a'},
    {"-f clk info ",        required_argument, NULL, 'f'},
    {"-p clk parent",       required_argument, NULL, 'p'},
    {"-s set rate",         required_argument, NULL, 's'},
    {"-h help",             no_argument, NULL, 'h'},
    {0, 0, 0, 0}
    };

static int cmd_test_cmu(int argc, char **argv)
{
    int opt;

    printf("--------------------------------------------------------------------------\n");
	printf("Clk-ID    |        NAME        |        Hz        |        enable        |\n");

    if (argc < 2) {
        clk_dump("0");
        goto __out;
    }

    optind = 0;
    while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (opt) {
        case 'a':
            clk_dump("0");
            break;
        case 'f':
            clk_get_info(optarg, GET_INFO);
            break;
        case 'p':
            clk_get_info(optarg, GET_PARENT);
            break;
        case 's':
            clk_set_rate(argv[2], argv[3]);
        case'h':
        default:
            cmd_cmu_usage(argv[0]);
            break;
        }
    }

__out:
    printf("\n---------------------------------------------------------------------------\n");

    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_cmu, test_clock, Test CMU CLK);
/* BAREMETAL */
#elif defined(AIC_CONSOLE_BARE_DRV)
#include <console.h>
static int cmd_test_cmu_bare(int argc, char *argv[])
{
    clk_dump("0");
    return 0;
}
CONSOLE_CMD(test_clock, cmd_test_cmu_bare, "Test CMU CLK.");
#endif

