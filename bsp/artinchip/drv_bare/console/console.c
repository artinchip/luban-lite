#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <console.h>
#include <rtconfig.h>
#include <aic_osal.h>

#ifdef __GNUC__
#define MAYBE_UNUSED(d) d __attribute__((unused))
#else
#define MAYBE_UNUSED(d) d
#endif

#ifndef CTRL
#define CTRL(c) (c - '@')
#endif

#define CONSOLE_PROMPT             '#'
#ifdef AIC_CONSOLE_SYSNAME
#define CONSOLE_SYSNAME            AIC_CONSOLE_SYSNAME
#else
#define CONSOLE_SYSNAME            "tinySPL"
#endif
#define MAX_NAME_LENGTH            255
#define CONSOLE_MAX_CMD_BUFFER_LEN (512)
#define CONSOLE_MAX_HISTORY        (32)

extern unsigned long __console_init_start;
extern unsigned long __console_init_end;

struct tiny_console {
    struct console_cmd *cmds_head;
    char *history[CONSOLE_MAX_HISTORY];
    unsigned char display_prompt;
    unsigned char usrname_len;
    unsigned char sysname_len;
    char prompt_char;
    char usrname[MAX_NAME_LENGTH + 1];
    char sysname[MAX_NAME_LENGTH + 1];
    char *bootcmd;
};

static struct tiny_console *g_console = NULL;

static int putnchar(const char *str, int n)
{
    int i;

    for (i = 0; i < n; i++)
        putchar(str[i]);

    return n;
}

static void console_clear_line(char *cmdstr, int l, int cursor)
{
    int i;

    if (cursor < l) {
        for (i = 0; i < (l - cursor); i++)
            putnchar(" ", 1);
    }
    for (i = 0; i < l; i++)
        cmdstr[i] = '\b';
    for (; i < l * 2; i++)
        cmdstr[i] = ' ';
    for (; i < l * 3; i++)
        cmdstr[i] = '\b';
    putnchar(cmdstr, i);
    memset(cmdstr, 0, i);
    l = cursor = 0;
}

static void console_set_sysname(struct tiny_console *cons, char *name)
{
    if (name && *name) {
        strncpy(cons->sysname, name, MAX_NAME_LENGTH);
        cons->sysname_len = strlen(name);
    }
}

static void console_set_prompt(struct tiny_console *cons, char prompt)
{
    cons->prompt_char = prompt;
}

void console_set_usrname(const char *name)
{
    struct tiny_console *cons = g_console;

    if (name && *name) {
        strncpy(cons->usrname, name, MAX_NAME_LENGTH);
        cons->usrname_len = strlen(name);
    }
}

static int show_prompt(struct tiny_console *cons)
{
    int len = 0;

    if (cons->usrname_len) {
        len += putnchar(cons->usrname, cons->usrname_len);
        len += putnchar("@", 1);
    }

    if (cons->sysname_len) {
        len += putnchar(cons->sysname, cons->sysname_len);
        len += putnchar(" ", 1);
    }

    len += putnchar(&cons->prompt_char, 1);
    len += putnchar(" ", 1);
    return len;
}

static char *console_cmd_get_name(struct tiny_console *cons,
                                  struct console_cmd *cmd)
{
    return cmd->cmdname;
}

static int console_show_help(struct tiny_console *cons, struct console_cmd *cmd)
{
    struct console_cmd *p;

    for (p = cmd; p; p = p->next)
        printf("  %-20s %s\n", console_cmd_get_name(cons, p), p->help ?: "");

    return CONSOLE_OK;
}

static int console_help(MAYBE_UNUSED(int argc), MAYBE_UNUSED(char *argv[]))
{
    struct tiny_console *cons = g_console;

    printf("Command list:\n");
    console_show_help(cons, cons->cmds_head);

    return CONSOLE_OK;
}

static int console_history(MAYBE_UNUSED(int argc), MAYBE_UNUSED(char *argv[]))
{
    int i;
    struct tiny_console *cons = g_console;

    for (i = 0; i < CONSOLE_MAX_HISTORY; i++) {
        if (cons->history[i])
            printf("%3d. %s\n", i, cons->history[i]);
    }

    return CONSOLE_OK;
}

static int dummy_proc(MAYBE_UNUSED(int argc), MAYBE_UNUSED(char *argv[]))
{
    return CONSOLE_OK;
}

static int console_add_history(struct tiny_console *cons, char *cmdstr)
{
    int i;

    for (i = 0; i < CONSOLE_MAX_HISTORY; i++) {
        if (!cons->history[i]) {
            if (i == 0 || strcasecmp(cons->history[i - 1], cmdstr))
                if (!(cons->history[i] = strdup(cmdstr)))
                    return CONSOLE_ERROR;
            return CONSOLE_OK;
        }
    }

    if (CONSOLE_MAX_HISTORY > 0) {
        free(cons->history[0]);
        for (i = 0; i < CONSOLE_MAX_HISTORY - 1; i++)
            cons->history[i] = cons->history[i + 1];

        if (!(cons->history[CONSOLE_MAX_HISTORY - 1] = strdup(cmdstr)))
            return CONSOLE_ERROR;
    }

    return CONSOLE_OK;
}

static void console_free_history(struct tiny_console *cons)
{
    int i;
    for (i = 0; i < CONSOLE_MAX_HISTORY; i++) {
        if (cons->history[i]) {
            free(cons->history[i]);
            cons->history[i] = NULL;
        }
    }
}

static struct console_cmd *
console_register_cmd_internal(struct tiny_console *cons, const char *cmdname,
                              int (*proc)(int, char **), const char *help)
{
    struct console_cmd *cmd, *p, *insert;

    if (!cmdname)
        return NULL;

    if (!(cmd = malloc(sizeof(struct console_cmd))))
        return NULL;

    memset(cmd, 0, sizeof(struct console_cmd));

    if (proc)
        cmd->proc = proc;
    else
        cmd->proc = dummy_proc;

    cmd->next = NULL;
    if (!(cmd->cmdname = strdup(cmdname))) {
        free(cmd);
        return NULL;
    }

    if (help) {
        if (!(cmd->help = strdup(help))) {
            free(cmd->cmdname);
            free(cmd);
            return NULL;
        }
    }

    if (cons->cmds_head == NULL) {
        cons->cmds_head = cmd;
    } else {
        insert = NULL;
        /* Insert and sort in alphabet */
        for (p = cons->cmds_head; p; p = p->next) {
            if (strncmp(cmd->cmdname, p->cmdname, strlen(cmd->cmdname)) > 0)
                insert = p;
        }
        if (insert == NULL) {
            p = cons->cmds_head;
            cons->cmds_head = cmd;
            cmd->next = p;
        } else {
            p = insert->next;
            insert->next = cmd;
            cmd->next = p;
        }
    }

    return cmd;
}

static int console_parse_opts(const char *line, char *args[], int max_argc)
{
    int argc = 0;
    char *p = (char *)line;
    char *word_start = 0;
    int inquote = 0;

    while (*p) {
        if (!isspace((int)*p)) {
            word_start = p;
            break;
        }
        p++;
    }

    while (argc < max_argc - 1) {
        if (!*p || *p == inquote ||
            (word_start && !inquote && (isspace((int)*p) || *p == '|'))) {
            if (word_start) {
                int len = p - word_start;

                memcpy(args[argc] = malloc(len + 1), word_start, len);
                args[argc++][len] = 0;
            }

            if (!*p)
                break;

            if (inquote)
                p++; /* skip over trailing quote */

            inquote = 0;
            word_start = 0;
        } else if (*p == '"' || *p == '\'') {
            inquote = *p++;
            word_start = p;
        } else {
            if (!word_start) {
                if (*p == '|') {
                    if (!(args[argc++] = strdup("|")))
                        return 0;
                } else if (!isspace((int)*p))
                    word_start = p;
            }

            p++;
        }
    }

    return argc;
}

static int console_get_completions(struct tiny_console *cons, char *cmdname,
                                   char **completions, int max_completions)
{
    struct console_cmd *c;
    struct console_cmd *n;
    int argc = 0, i, k = 0;
    int cnt_keep = 0;
    char *args[128] = { 0 };

    if (!cmdname)
        return 0;
    while (isspace((int)*cmdname))
        cmdname++;

    argc = console_parse_opts(cmdname, args, sizeof(args) / sizeof(args[0]));

    cnt_keep = argc;
    if (!cmdname[0] || cmdname[strlen(cmdname) - 1] == ' ')
        argc++;

    if (!argc)
        return 0;

    for (c = cons->cmds_head, i = 0; c && i < argc && k < max_completions;
         c = n) {
        n = c->next;

        if (args[i] && strncasecmp(c->cmdname, args[i], strlen(args[i])))
            continue;

        completions[k++] = c->cmdname;
    }

    for (i = 0; i < cnt_keep; i++)
        free(args[i]);

    return k;
}

static int console_run_cmd_internal(struct tiny_console *cons,
                                    struct console_cmd *root, int argc,
                                    char *args[], int starg_arg,
                                    int show_result)
{
    struct console_cmd *c;
    char *p;
    int len;

    if (!args[starg_arg])
        return CONSOLE_ERROR;

    p = args[starg_arg];
    len = strlen(p);
    if (p[len - 1] == '?') {
        for (c = root; c; c = c->next) {
            if (strncasecmp(c->cmdname, p, len - 1) == 0 && (c->proc))
                printf("  %-20s %s\n", c->cmdname, c->help ?: "");
        }

        return CONSOLE_OK;
    }

    for (c = root; c; c = c->next) {
        int rc = CONSOLE_OK;

        if (strncasecmp(c->cmdname, args[starg_arg], strlen(args[starg_arg])))
            continue;

        /* name is matched */
        if (!c->proc) {
            printf("  No proc for \"%s\"\n", console_cmd_get_name(cons, c));
            return CONSOLE_ERROR;
        }

        rc = c->proc(argc - starg_arg, args + starg_arg);

        return rc;
    }

    if (starg_arg == 0) {
        if (show_result) {
            printf("  Invalid command: \"%s\"\n", args[starg_arg]);
        }
    }

    return CONSOLE_ERROR_ARG;
}

struct console_cmd *console_find_cmd_by_name(const char *name)
{
    struct console_cmd *p;

    for (p = g_console->cmds_head; p; p = p->next) {
        if (strcmp(p->cmdname, name) == 0)
            return p;
    }

    return NULL;
}

int console_set_bootcmd(const char *cmdstr)
{
    if (g_console->bootcmd) {
        free(g_console->bootcmd);
        g_console->bootcmd = NULL;
    }
    g_console->bootcmd = strdup(cmdstr);
    return 0;
}

int console_run_cmd(const char *cmdstr)
{
    struct tiny_console *cons;
    unsigned int argc, i;
    char *args[128] = { 0 };
    int r;

    cons = g_console;

    if (!cmdstr)
        return CONSOLE_ERROR;
    while (isspace((int)*cmdstr))
        cmdstr++;

    if (!*cmdstr)
        return CONSOLE_OK;

    argc = console_parse_opts(cmdstr, args, sizeof(args) / sizeof(args[0]));

    if (argc) {
        r = console_run_cmd_internal(cons, cons->cmds_head, argc, args, 0, 1);
    } else {
        r = CONSOLE_ERROR;
    }

    for (i = 0; i < argc; i++)
        free(args[i]);

    return r;
}

static int console_delete_cmd(struct tiny_console *cons,
                              struct console_cmd *cmd)
{
    struct console_cmd *start, *t;

    start = cons->cmds_head;

    for (t = start; t; t = t->next) {
        if (t == cmd && t == start)
            cons->cmds_head = t->next;

        if (t->next == cmd)
            t->next = cmd->next;
    }

    free(cmd->cmdname);
    if (cmd->help)
        free(cmd->help);
    free(cmd);

    return 0;
}

static int _console_loop(struct tiny_console *cons)
{
    int c, l, oldl = 0, skip = 0, esc = 0;
    int cursor = 0, insertmode = 1;
    char *cmd = NULL, *oldcmd = 0;

    console_free_history(cons);

    if ((cmd = malloc(CONSOLE_MAX_CMD_BUFFER_LEN)) == NULL)
        return CONSOLE_ERROR;

    memset(cmd, 0, CONSOLE_MAX_CMD_BUFFER_LEN);
    while (1) {
        signed int in_history = 0;
        int lastchar = 0;

        cons->display_prompt = 1;
        if (oldcmd) {
            l = cursor = oldl;
            oldcmd[l] = 0;
            cons->display_prompt = 1;
            oldcmd = NULL;
            oldl = 0;
        } else {
            memset(cmd, 0, CONSOLE_MAX_CMD_BUFFER_LEN);
            l = 0;
            cursor = 0;
        }

        while (1) {
            if (cons->display_prompt) {
                show_prompt(cons);
                putnchar(cmd, l);
                if (cursor < l) {
                    int n = l - cursor;
                    while (n--)
                        putnchar("\b", 1);
                }

                cons->display_prompt = 0;
            }

#ifdef KERNEL_FREERTOS
            aicos_msleep(10);
#endif

            c = getchar();

            if (c <= 0)
                continue;

            if (skip) {
                skip--;
                continue;
            }

            if (esc) {
                if (esc == '[') {
                    /* remap to readline control codes */
                    switch (c) {
                        case 'A': /* Up */
                            c = CTRL('P');
                            break;

                        case 'B': /* Down */
                            c = CTRL('N');
                            break;

                        case 'C': /* Right */
                            c = CTRL('F');
                            break;

                        case 'D': /* Left */
                            c = CTRL('B');
                            break;

                        default:
                            c = 0;
                    }

                    esc = 0;
                } else {
                    esc = (c == '[') ? c : 0;
                    continue;
                }
            }

            if (c == 0)
                continue;

            if (c == '\r' || c == '\n') {
                putnchar("\n", 1);
                break;
            }

            if (c == 27) {
                esc = 1;
                continue;
            }

            if (c == CTRL('C')) {
                putnchar("\n", 1);
                strncpy(cmd, "\n", CONSOLE_MAX_CMD_BUFFER_LEN);
                lastchar = c;
                break;
            }

            /* back word, backspace/delete */
            if (c == CTRL('W') || c == CTRL('H') || c == 0x7f) {
                int back = 0;

                if (c == CTRL('W')) { /* word */
                    int nc = cursor;

                    if (l == 0 || cursor == 0)
                        continue;

                    while (nc && cmd[nc - 1] == ' ') {
                        nc--;
                        back++;
                    }

                    while (nc && cmd[nc - 1] != ' ') {
                        nc--;
                        back++;
                    }
                } else { /* char */
                    if (l == 0 || cursor == 0) {
                        putnchar("\a", 1);
                        continue;
                    }

                    back = 1;
                }

                if (back) {
                    while (back--) {
                        if (l == cursor) {
                            cmd[--cursor] = 0;
                            putnchar("\b \b", 3);
                        } else {
                            int i;
                            cursor--;
                            for (i = cursor; i <= l; i++)
                                cmd[i] = cmd[i + 1];

                            putnchar("\b", 1);
                            putnchar(cmd + cursor, strlen(cmd + cursor));
                            putnchar(" ", 1);

                            for (i = 0; i <= (int)strlen(cmd + cursor); i++)
                                putnchar("\b", 1);
                        }
                        l--;
                    }

                    continue;
                }
            }

            /* redraw */
            if (c == CTRL('L')) {
                int i;
                int cursorback = l - cursor;

                putnchar("\n", 1);
                show_prompt(cons);
                putnchar(cmd, l);

                for (i = 0; i < cursorback; i++)
                    putnchar("\b", 1);

                continue;
            }

            /* clear line */
            if (c == CTRL('U')) {
                console_clear_line(cmd, l, cursor);
                l = cursor = 0;
                continue;
            }

            /* kill to EOL */
            if (c == CTRL('K')) {
                int c;

                if (cursor == l)
                    continue;

                for (c = cursor; c < l; c++)
                    putnchar(" ", 1);

                for (c = cursor; c < l; c++)
                    putnchar("\b", 1);

                memset(cmd + cursor, 0, l - cursor);
                l = cursor;
                continue;
            }

            /* EOT */
            if (c == CTRL('D')) {
                if (l)
                    continue;

                strcpy(cmd, "quit");
                l = cursor = strlen(cmd);
                putnchar("quit\n", l + 1);
                break;
            }

            /* disable */
            if (c == CTRL('Z')) {
                console_clear_line(cmd, l, cursor);
                cons->display_prompt = 1;
                continue;
            }

            /* TAB completion */
            if (c == CTRL('I')) {
                char *completions[128];
                int num_completions = 0;

                if (cursor != l)
                    continue;

                num_completions =
                    console_get_completions(cons, cmd, completions, 128);
                if (num_completions == 0) {
                    putnchar("\a", 1);
                } else if (num_completions == 1) {
                    /* Single completion */

                    /* Only completion the part of command name, do nothing
                     * after command name is completed.
                     */
                    if (!strncmp(cmd, completions[0], strlen(completions[0])))
                        continue;

                    for (; l > 0; l--, cursor--) {
                        if (cmd[l - 1] == ' ' || cmd[l - 1] == '|')
                            break;
                        putnchar("\b", 1);
                    }
                    strcpy((cmd + l), completions[0]);
                    l += strlen(completions[0]);
                    cmd[l++] = ' ';
                    cursor = l;
                    putnchar(completions[0], strlen(completions[0]));
                    putnchar(" ", 1);
                } else if (lastchar == CTRL('I')) {
                    /* double tab */
                    int i, spcnt, mxlen;

                    mxlen = 0;
                    for (i = 0; i < num_completions; i++) {
                        if (mxlen < strlen(completions[i]))
                            mxlen = strlen(completions[i]);
                    }

                    /* 2 space between names */
                    mxlen += 2;
                    for (i = 0; i < num_completions; i++) {
                        if ((i % 4) == 0)
                            putnchar("\n", 1);
                        putnchar(completions[i], strlen(completions[i]));
                        spcnt = mxlen - strlen(completions[i]);
                        while (spcnt) {
                            putnchar(" ", 1);
                            spcnt--;
                        }
                    }
                    putnchar("\n", 1);
                    cons->display_prompt = 1;
                } else {
                    /* More than one completion */
                    lastchar = c;
                    putnchar("\a", 1);
                }
                continue;
            }

            /* history */
            if (c == CTRL('P') || c == CTRL('N')) {
                int history_found = 0;
                if (c == CTRL('P')) { /* Up */
                    in_history--;
                    if (in_history < 0) {
                        for (in_history = CONSOLE_MAX_HISTORY - 1;
                             in_history >= 0; in_history--) {
                            if (cons->history[in_history]) {
                                history_found = 1;
                                break;
                            }
                        }
                    } else {
                        if (cons->history[in_history])
                            history_found = 1;
                    }
                } else { /* Down */
                    in_history++;
                    if (in_history >= CONSOLE_MAX_HISTORY ||
                        !cons->history[in_history]) {
                        int i = 0;
                        for (i = 0; i < CONSOLE_MAX_HISTORY; i++) {
                            if (cons->history[i]) {
                                in_history = i;
                                history_found = 1;
                                break;
                            }
                        }
                    } else {
                        if (cons->history[in_history])
                            history_found = 1;
                    }
                }
                if (history_found && cons->history[in_history]) {
                    /* Show history item */
                    console_clear_line(cmd, l, cursor);
                    memset(cmd, 0, CONSOLE_MAX_CMD_BUFFER_LEN);
                    strncpy(cmd, cons->history[in_history],
                            CONSOLE_MAX_CMD_BUFFER_LEN - 1);
                    l = cursor = strlen(cmd);
                    putnchar(cmd, l);
                }

                continue;
            }

            /* left/right cursor motion */
            if (c == CTRL('B') || c == CTRL('F')) {
                if (c == CTRL('B')) { /* Left */
                    if (cursor) {
                        putnchar("\b", 1);
                        cursor--;
                    }
                } else { /* Right */
                    if (cursor < l) {
                        putnchar(&cmd[cursor], 1);
                        cursor++;
                    }
                }

                continue;
            }

            /* start of line */
            if (c == CTRL('A')) {
                if (cursor) {
                    putnchar("\r", 1);
                    show_prompt(cons);

                    cursor = 0;
                }

                continue;
            }

            /* end of line */
            if (c == CTRL('E')) {
                if (cursor < l) {
                    putnchar(&cmd[cursor], l - cursor);
                    cursor = l;
                }

                continue;
            }

            /* normal character typed */
            if (cursor == l) {
                /* append to end of line */
                cmd[cursor] = c;
                if (l < CONSOLE_MAX_CMD_BUFFER_LEN - 1) {
                    l++;
                    cursor++;
                } else {
                    putnchar("\a", 1);
                    continue;
                }
            } else {
                /* Middle of text */
                if (insertmode) {
                    int i;
                    /* Move everything one character to the right */
                    if (l >= CONSOLE_MAX_CMD_BUFFER_LEN - 2)
                        l--;
                    for (i = l; i >= cursor; i--)
                        cmd[i + 1] = cmd[i];
                    /* Write what we've just added */
                    cmd[cursor] = c;

                    putnchar(&cmd[cursor], l - cursor + 1);
                    for (i = 0; i < (l - cursor + 1); i++)
                        putnchar("\b", 1);
                    l++;
                } else {
                    cmd[cursor] = c;
                }
                cursor++;
            }

            putnchar((const char *)&c, 1);

            oldcmd = 0;
            oldl = 0;
            lastchar = c;
        }

        if (l < 0)
            break;

        if (l == 0)
            continue;
        if (lastchar != CTRL('C') && cmd[l - 1] != '?' &&
            strcasecmp(cmd, "history") != 0)
            console_add_history(cons, cmd);

        if (console_run_cmd(cmd) == CONSOLE_QUIT)
            break;
    }

    if (g_console->bootcmd) {
        free(g_console->bootcmd);
        g_console->bootcmd = NULL;
    }

    console_free_history(cons);
    free(cmd);

    return CONSOLE_OK;
}

struct console_cmd *console_register_cmd(const char *cmd,
                                         int (*proc)(int, char **),
                                         const char *help)
{
    return console_register_cmd_internal(g_console, cmd, proc, help);
}

int console_unregister_cmd(const char *cmd)
{
    struct tiny_console *cons = g_console;
    struct console_cmd *c;

    if (!cmd)
        return -1;
    if (!cons->cmds_head)
        return CONSOLE_OK;

    c = console_find_cmd_by_name(cmd);
    if (c == NULL)
        return -1;

    console_delete_cmd(cons, c);

    return CONSOLE_OK;
}

static void console_register_init_cmds(const char *name)
{
    struct console_init_cmd *cmd_start =
        (struct console_init_cmd *)&__console_init_start;
    struct console_init_cmd *cmd_end =
        (struct console_init_cmd *)&__console_init_end;
    struct console_init_cmd *p;

    for (p = cmd_start; p < cmd_end; p++) {
        if (strcmp(p->cmdname, name))
            continue;

        if (p->initialized)
            continue;

        console_register_cmd(p->cmdname, p->proc, p->help);
        p->initialized = true;
        break;
    }
}

void console_init(void)
{
	struct console_init_cmd *cmd_start =
		(struct console_init_cmd *)&__console_init_start;
	struct console_init_cmd *cmd_end =
		(struct console_init_cmd *)&__console_init_end;
	struct console_init_cmd *p;

    if (!(g_console = malloc(sizeof(struct tiny_console))))
        return;

    memset(g_console, 0, sizeof(struct tiny_console));

    console_set_prompt(g_console, CONSOLE_PROMPT);
    console_set_sysname(g_console, CONSOLE_SYSNAME);

    console_register_cmd("help", console_help, "Show all commands.");
    console_register_cmd("history", console_history, "Show history.");

	for (p = cmd_start; p < cmd_end; p++) {
		if (p->initialized)
			continue;
		console_register_init_cmds(p->cmdname);
	}

    return;
}

void console_loop(void)
{
    if (g_console->bootcmd)
        console_run_cmd(g_console->bootcmd);

    _console_loop(g_console);
}

int console_get_ctrlc(void)
{
    int c;

    c = getchar();
    if (c == CTRL('C'))
        return c;

    return -1;
}
