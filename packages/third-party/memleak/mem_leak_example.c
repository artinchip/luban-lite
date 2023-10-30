#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <avn_mem.h>


// memory leaks (and other bugs) spotter.
// keeps track of malloc(), free(), calloc() and realloc() calls count and of what
// happens to the allocated memory.


int memleak_check_example(int argc, char const *argv[])
{
    leak_info li;

    aic_mem_leak_init(&li);

    void *mem = malloc(10);
    if (mem == RT_NULL) {
        return -1;
    }
    aic_mem_leak_peek();

    // free(mem);
    aic_mem_leak_get_info(&li);

    return 0;
}

MSH_CMD_EXPORT(memleak_check_example, memleak example);
