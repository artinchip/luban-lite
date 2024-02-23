#include <stdio.h>
#include <dlfcn.h>
#include <rtthread.h>
#include <dlmodule.h>
#if defined(RT_USING_FINSH)
#include <finsh.h>
#endif

#define DM_LIB_PATH "/sdcard/hello.so"
#define DM_LIB_FUNC "my_thread_init"
#define DEAMON_THREAD

static void cmd_test_dm_lib(int argc, char **argv)
{
    struct rt_dlmodule *module = NULL;
    int (*func)(void) = NULL;

    module = dlopen(DM_LIB_PATH, 0);
    if (!module) {
        printf("dlopen %s fail!\n", DM_LIB_PATH);
        return;
    }

    func = dlsym(module, DM_LIB_FUNC);
    if (!func) {
        printf("dlsym %s fail!\n", DM_LIB_FUNC);
        return;
    }

    func();

#ifndef DEAMON_THREAD
    dlclose(module);
#endif
}

#if defined(RT_USING_FINSH)
MSH_CMD_EXPORT_ALIAS(cmd_test_dm_lib, test_dm_lib,
                     Test dynamic load DM-lib. );
#endif
