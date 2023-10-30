#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>

static void hex_dump_1(unsigned char *buf, unsigned long len)
{
    unsigned long i;

    for (i = 0; i < len; i++) {
        if (i && (i % 16) == 0)
            printf("\n");
        if ((i % 16) == 0)
            printf("0x%08lx : ", (unsigned long)&buf[i]);
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

static void hex_dump_2(unsigned char *buf, unsigned long len)
{
    unsigned long i;
    unsigned short data;

    i = 0;
    while (i < len) {
        if (i && (i % 16) == 0)
            printf("\n");
        if ((i % 16) == 0)
            printf("0x%08lx : ", (unsigned long)&buf[i]);
        data = 0;
        if ((i + 2) <= len) {
            memcpy(&data, &buf[i], 2);
            i += 2;
        } else {
            memcpy(&data, &buf[i], 1);
            i += 1;
        }
        printf("%04x ", data);
    }
    printf("\n");
}

static void hex_dump_4(unsigned char *buf, unsigned long len)
{
    unsigned long i;
    unsigned long data;

    i = 0;
    while (i < len) {
        if (i && (i % 16) == 0)
            printf("\n");
        if ((i % 16) == 0)
            printf("0x%08lx : ", (unsigned long)&buf[i]);
        data = 0;
        if ((i + 4) <= len) {
            memcpy(&data, &buf[i], 4);
            i += 4;
        } else {
            memcpy(&data, &buf[i], len - i);
            i += (len - i);
        }
        printf("%08lx ", data);
    }
    printf("\n");
}

static void hex_dump_8(unsigned char *buf, unsigned long len)
{
    unsigned long i;
    unsigned long long data;

    i = 0;
    while (i < len) {
        if (i && (i % 16) == 0)
            printf("\n");
        if ((i % 16) == 0)
            printf("0x%08lx : ", (unsigned long)&buf[i]);
        data = 0;
        if ((i + 8) <= len) {
            memcpy(&data, &buf[i], 8);
            i += 8;
        } else {
            memcpy(&data, &buf[i], len - i);
            i += (len - i);
        }
        printf("%16llx ", data);
    }
    printf("\n");
}

void hexdump(unsigned char *buf, unsigned long len, int groupsize)
{
    if (groupsize <= 1) {
        hex_dump_1(buf, len);
    } else if (groupsize <= 2) {
        hex_dump_2(buf, len);
    } else if (groupsize <= 4) {
        hex_dump_4(buf, len);
    } else if (groupsize <= 8) {
        hex_dump_8(buf, len);
    } else {
        hex_dump_1(buf, len);
    }
}

void show_speed(char *msg, u32 len, u32 us)
{
    u32 tmp, speed;

    /* Split to serval step to avoid overflow */
    tmp = 1000 * len;
    tmp = tmp / us;
    tmp = 1000 * tmp;
    speed = tmp / 1024;

    printf("%s: %d byte, %d us -> %d KB/s\n", msg, len, us, speed);
}

