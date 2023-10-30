
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <mtd.h>

static struct mtd_partition *_part_parse(char *parts, u32 start)
{
    struct mtd_partition *part;
    int cnt = 0;
    char *p;

    part = malloc(sizeof(struct mtd_partition));
    if (!part)
        return NULL;

    memset(part, 0, sizeof(struct mtd_partition));

    part->start = start;
    p = parts;
    if (*p == '-') {
        /* All remain space */
        part->size = 0;
        p++;
    } else {
        part->size = strtoul(p, &p, 0);
        if ((*p == 'k') || (*p == 'K')) {
            part->size *= 1024;
            p++;
        } else if ((*p == 'm') || (*p == 'M')) {
            part->size *= (1024 * 1024);
            p++;
        } else if ((*p == 'g') || (*p == 'G')) {
            part->size *= (1024 * 1024 * 1024);
            p++;
        }
    }
    if (*p == '@') {
        p++;
        /* Don't care offset here, just skip it */
        part->start = strtoul(p, &p, 0);
    }
    if (*p != '(') {
        printf("Partition name should be next of size.\n");
        printf("%s\n", p);
        goto err;
    }
    p++;

    cnt = 0;
    while (*p != ')') {
        if (cnt >= MAX_MTD_NAME)
            break;
        part->name[cnt++] = *p++;
    }
    p++;

    /* Skip characters until '\0', ',', ';' */
    while ((*p != '\0') && (*p != ';') && (*p != ','))
        p++;

    if (*p == ',') {
        p++;
        part->next = _part_parse(p, part->start + part->size);
    }

    return part;
err:
    if (part)
        free(part);
    return NULL;
}

struct mtd_partition *mtd_parts_parse(char *parts)
{
    char *p;

    p = parts;

    while ((*p != '\0') && (*p != ':'))
        p++;
    if (*p != ':') {
        printf("mtdparts is invalid: %s\n", parts);
        return NULL;
    }
    p++;
    return _part_parse(p, 0);
}

void mtd_parts_free(struct mtd_partition *head)
{
    struct mtd_partition *p, *n;

    p = head;
    n = p;
    while (p) {
        n = p->next;
        free(p);
        p = n;
    }
}
