#include <finsh.h>
#include <drivers/rtc.h>
#include <drivers/alarm.h>

#include "aic_core.h"

static struct rt_alarm *g_alarm = RT_NULL;

static void test_alarm_callback(rt_alarm_t alarm, time_t timestamp)
{
    pr_info("Test alarm callback function.\n");
}

static void cmd_test_alarm(int argc, char **argv)
{
    struct rt_alarm_setup setup = {0};
    u32 timeout = 0;
    time_t now;
    struct tm p_tm;

    if (argc != 2) {
        pr_err("Invalid parameter\n");
        return;
    }
    timeout = atoi(argv[1]);

    if (g_alarm)
        rt_alarm_delete(g_alarm);

    now = time(NULL) + timeout;
    gmtime_r(&now, &p_tm);

    setup.wktime = p_tm;
    g_alarm = rt_alarm_create(test_alarm_callback, &setup);
    if (g_alarm) {
        g_alarm->flag = RT_ALARM_ONESHOT;
        rt_alarm_start(g_alarm);
    }
}
MSH_CMD_EXPORT_ALIAS(cmd_test_alarm, test_alarm, test RTC alarm);
