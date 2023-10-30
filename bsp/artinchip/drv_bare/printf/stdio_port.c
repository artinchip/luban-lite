#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <uart.h>

#define PORT console_uart_id
static int console_uart_id;

void stdio_set_uart(int id)
{
    console_uart_id = id;
}

int putchar_port(int c)
{
    if (c == '\n')
        uart_putchar(PORT, '\r');

    if (uart_putchar(PORT, c) < 0)
        return -1;
    return c;
}

int puts_port(const char *s)
{
    int cnt, c;

    cnt = 0;
    for (;;) {
        c = *s;
        if (c == 0)
            break;
        if (putchar_port(c) < 0)
            break;
        cnt++;
        s++;
    }
    putchar_port('\n');

    return cnt;
}

int getchar_port(void)
{
    return uart_getchar(PORT);
}

int getc(FILE *stream) __attribute__((alias("getchar_port")));
int getchar(void) __attribute__ ((alias("getchar_port")));
int puts(const char *s) __attribute__ ((alias("puts_port")));
int putc(int c, FILE *stream) __attribute__((alias("putchar_port")));
int putchar(int c) __attribute__ ((alias("putchar_port")));
