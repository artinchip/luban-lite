#ifndef __CONSOLE_H_
#define __CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

#define CONSOLE_OK        (0)
#define CONSOLE_ERROR     (-1)
#define CONSOLE_QUIT      (-2)
#define CONSOLE_ERROR_ARG (-3)

#define __console_init \
    __attribute__((used)) __attribute__((section(".tinyspl.console.cmd")))

struct console_init_cmd {
    const char *cmdname;
    int (*proc)(int argc, char **argv);
    uint8_t mode;
    bool initialized;
    const char *help;
};

#define CONSOLE_CMD(_cmd, _proc, _help)                                      \
    __console_init struct console_init_cmd console_init_cmd_##_cmd = {       \
        .cmdname = #_cmd, .proc = _proc, .initialized = false, .help = _help \
    };

struct console_cmd {
    char *cmdname;
    int (*proc)(int argc, char **argv);
    char *help;
    struct console_cmd *next;
};

void console_init(void);
void console_loop(void);
void console_set_usrname(const char *usrname);
int console_run_cmd(const char *cmdstr);
struct console_cmd *console_find_cmd_by_name(const char *name);
struct console_cmd *console_register_cmd(const char *cmd,
                                         int (*proc)(int, char **),
                                         const char *help);
int console_unregister_cmd(const char *cmd);
int console_set_bootcmd(const char *cmdstr);
int console_get_ctrlc(void);

#ifdef __cplusplus
}
#endif

#endif
