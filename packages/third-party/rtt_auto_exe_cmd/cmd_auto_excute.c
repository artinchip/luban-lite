/*
 * Copyright (c) 2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023/1/29      supperthomas Add first version
 */

#include "rtthread.h"
#include "msh.h"
#ifdef LPKG_USING_RTT_AUTO_EXE_CMD
#else
#define RTT_AUTO_CMD_THREAD_STACK_SIZE 20480
#define RTT_AUTO_CMD_THREAD_PRIORITY   10
#define RTT_AUTO_INIT_TIME_MS  5000
#define RTT_CMD_1_STR         "version"
#define RTT_CMD_1_DELAY       1000
#define RTT_CMD_2_STR         "free"
#define RTT_CMD_2_DELAY       1000
#define RTT_CMD_LOOP_1_STR    "ps"
#define RTT_CMD_LOOP_1_DELAY_TIME 2000
#define RTT_CMD_LOOP_2_STR    "help"
#define RTT_CMD_LOOP_2_DELAY_TIME 2000

#define RTT_CMD_LOOP_TIME     3
#endif

typedef struct
{
    char *cmd_string;
    rt_uint32_t delay_time;
} rtt_cmd_array;

rtt_cmd_array rtt_cmd_init_array[]=
{
    {RTT_CMD_1_STR,RTT_CMD_1_DELAY},
    {RTT_CMD_2_STR,RTT_CMD_2_DELAY}
};

#ifdef RTT_CMD_LOOP_FLAG
rtt_cmd_array rtt_cmd_loop_array[]=
{
    {RTT_CMD_LOOP_1_STR,RTT_CMD_LOOP_1_DELAY_TIME},
    {RTT_CMD_LOOP_2_STR,RTT_CMD_LOOP_2_DELAY_TIME}
};
#endif

static void auto_cmd_thread_entry(void *parameter)
{

    rt_thread_mdelay(RTT_AUTO_INIT_TIME_MS);
    for(int i = 0;i < sizeof(rtt_cmd_init_array)/sizeof(rtt_cmd_array);i++)
    {
        if(rtt_cmd_init_array[i].cmd_string != RT_NULL)
        {
            msh_exec(rtt_cmd_init_array[i].cmd_string,rt_strlen(rtt_cmd_init_array[i].cmd_string));
            rt_thread_mdelay(rtt_cmd_init_array[i].delay_time);
        }
    }
#ifdef RTT_CMD_LOOP_FLAG
    rt_uint32_t loop_time = RTT_CMD_LOOP_TIME;
    while(loop_time--)
    {
        for(int i = 0;i < sizeof(rtt_cmd_loop_array)/sizeof(rtt_cmd_array);i++)
        {
            if(rtt_cmd_loop_array[i].cmd_string != RT_NULL)
            {
                msh_exec(rtt_cmd_loop_array[i].cmd_string,rt_strlen(rtt_cmd_loop_array[i].cmd_string));
                rt_thread_mdelay(rtt_cmd_loop_array[i].delay_time);
            }
        }
    }
#endif
}

static int cmd_auto_exe(void)
{
    rt_thread_t tid;
    rt_kprintf("\r\n Hello RTT_AUTO_EXE thread CREATE!\r\n");
    tid = rt_thread_create("RTT_AUTO_EXE", auto_cmd_thread_entry, RT_NULL,
                           RTT_AUTO_CMD_THREAD_STACK_SIZE, RTT_AUTO_CMD_THREAD_PRIORITY, 20);
    RT_ASSERT(tid != RT_NULL);
    rt_thread_startup(tid);
    return 0;
}
INIT_APP_EXPORT(cmd_auto_exe);
