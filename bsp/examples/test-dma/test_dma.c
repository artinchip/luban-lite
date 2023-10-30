#include <string.h>
#if defined(RT_USING_FINSH)
#include <finsh.h>
#endif
#include "aic_core.h"
#include "drv_dma.h"

#ifdef RT_USING_POSIX_CLOCK
#include <sys/time.h>

static float time_diff(struct timespec *start, struct timespec *end)
{
    float diff;
#define NS_PER_SEC      1000000000

    if (end->tv_nsec < start->tv_nsec) {
        diff = (float)(NS_PER_SEC + end->tv_nsec - start->tv_nsec)/NS_PER_SEC;
        diff += end->tv_sec - 1 - start->tv_sec;
    } else {
        diff = (float)(end->tv_nsec - start->tv_nsec)/NS_PER_SEC;
        diff += end->tv_sec - start->tv_sec;
    }

    return diff;
}
#endif

static void dma_test_cb(void *param)
{
	printf("DMA complete, callback....\n");
}

static void cmd_test_dma_memcpy(int argc, char **argv)
{
    struct dma_chan *chan = NULL;
    u32 test_len = 0, align_len = 0;
    char *src = NULL, *dest = NULL;
    int ret = 0, i;
    u32 size = 0;
#ifdef RT_USING_POSIX_CLOCK
    struct timespec start = {0}, end = {0};
    float speed = 0.0;
#endif

    if (argc != 2) {
        pr_err("Invalid parameter\n");
        return;
    }
    test_len = atoi(argv[1]);

    align_len = roundup(test_len, CACHE_LINE_SIZE);

    src = aicos_malloc_align(0, align_len, CACHE_LINE_SIZE);
    dest = aicos_malloc_align(0, align_len, CACHE_LINE_SIZE);
    if ((src == NULL) || (dest == NULL)){
        pr_err("Alloc %d mem fail!\n ", align_len);
        goto free_mem;
    }

    printf("DMA memcpy test: from 0x%lx to 0x%lx, len %d/%d\n",
           (unsigned long)src, (unsigned long)dest, test_len, align_len);

    for (i = 0;i < test_len; i++) {
        src[i] = i & 0xff;
        dest[i] = 0x55;
    }

#ifdef RT_USING_POSIX_CLOCK
    clock_gettime(CLOCK_REALTIME, &start);
#endif

    chan = dma_request_channel();
    if (chan == NULL){
        pr_err("Alloc dma chan fail!\n ");
        goto free_mem;
    }

    ret = dmaengine_prep_dma_memcpy(chan, (unsigned long)dest, (unsigned long)src, align_len);
    if (ret){
        pr_err("dmaengine_prep_dma_memcpy fail! ret = %d\n ", ret);
        goto free_chan;
    }

    ret = dmaengine_submit(chan, dma_test_cb, chan);
    if (ret){
        pr_err("dmaengine_submit fail! ret = %d\n ", ret);
        goto free_chan;
    }

    dma_async_issue_pending(chan);

    while (dmaengine_tx_status(chan, &size) != DMA_COMPLETE);
    aicos_dcache_invalid_range(dest, align_len);

#ifdef RT_USING_POSIX_CLOCK
    clock_gettime(CLOCK_REALTIME, &end);
#endif

    for (i = 0;i < test_len; i++){
        if (dest[i] != src[i]){
            printf("Error 0x%lx -> 0x%lx: expect 0x%x, actual 0x%x\n",
                   i + (ptr_t)src, i + (ptr_t)dest, src[i], dest[i]);
            ret = -1;
        }
    }

    if (ret)
        printf("DMA test fail!\n");
    else
        printf("DMA test succeed!\n");

#ifdef RT_USING_POSIX_CLOCK
    speed = (float)align_len / 1024 / 1024 / time_diff(&start, &end);
    printf("DMA memcpy %u bytes, speed %d.%02d MB/s\n",
           align_len, (u32)speed, (u32)(speed * 100) % 100);
#endif

free_chan:
    if (chan)
        dma_release_channel(chan);
free_mem:
    if (src)
        aicos_free_align(0, src);
    if (dest)
        aicos_free_align(0, dest);
}

#if defined(RT_USING_FINSH)
MSH_CMD_EXPORT_ALIAS(cmd_test_dma_memcpy, test_dma_memcpy,
                     Test DMA memcpy. Argument: length);
#elif defined(AIC_CONSOLE_BARE_DRV)
#include <console.h>

static int test_dma_memcpy(int argc, char *argv[])
{
    cmd_test_dma_memcpy(argc, argv);
    return 0;
}

CONSOLE_CMD(test_dma_memcpy, test_dma_memcpy, "Test DMA memcpy. Argument: length.");
#endif


#ifndef AIC_DMA_DRV_V10

static void cmd_test_dma_memset(int argc, char **argv)
{
    struct dma_chan * chan = NULL;
    u32 test_len = 0, align_len = 0;
    u32 test_val = 0;
    char *dest = NULL;
    char *val = NULL;
    int ret, i;
    u32 size = 0;
#ifdef RT_USING_POSIX_CLOCK
    struct timespec start = {0}, end = {0};
    float speed = 0.0;
#endif

    if (argc != 3) {
        pr_err("Invalid parameter\n");
        return;
    }
    test_val = atoi(argv[1]);
    test_len = atoi(argv[2]);

    align_len = roundup(test_len, CACHE_LINE_SIZE);

    dest = aicos_malloc_align(0, align_len, CACHE_LINE_SIZE);
    if ((dest == NULL)){
        pr_err("Alloc %d mem fail!\n ", align_len);
        goto free_mem;
    }
    memset(dest, 0, align_len);

    printf("DMA memset test: addr 0x%lx, val 0x%x, len %d(%d),\n",
           (unsigned long)dest, test_val, test_len, align_len);

#ifdef RT_USING_POSIX_CLOCK
    clock_gettime(CLOCK_REALTIME, &start);
#endif

    chan = dma_request_channel();
    if (chan == NULL){
        printf("Alloc dma chan fail!\n ");
        goto free_mem;
    }

    ret = dmaengine_prep_dma_memset(chan, (unsigned long)dest, test_val, align_len);
    if (ret){
        pr_err("hal_dma_chan_prep_memset fail! ret = %d\n ", ret);
        goto free_chan;
    }

    ret = dmaengine_submit(chan, dma_test_cb, chan);
    if (ret){
        pr_err("dmaengine_submit fail! ret = %d\n ", ret);
        goto free_chan;
    }

    dma_async_issue_pending(chan);

    while (dmaengine_tx_status(chan, &size) != DMA_COMPLETE);
    aicos_dcache_invalid_range(dest, align_len);
#ifdef RT_USING_POSIX_CLOCK
    clock_gettime(CLOCK_REALTIME, &end);
#endif

    val = (char *)&test_val;
    for (i = 0;i < test_len; i++){
        if (dest[i] != val[i%4]){
            printf("Error 0x%lx: expect 0x%x, actual 0x%x\n",
                   i + (ptr_t)dest, val[i%4], dest[i]);
            ret = -1;
        }
    }

    if (ret)
        printf("DMA test fail!\n");
    else
        printf("DMA test succeed!\n");

#ifdef RT_USING_POSIX_CLOCK
    speed = (float)align_len / 1024 / 1024 / time_diff(&start, &end);
    printf("DMA memset %u bytes, speed %d.%02d MB/s\n",
           align_len, (u32)speed, (u32)(speed * 100) % 100);
#endif

free_chan:
    if (chan)
        dma_release_channel(chan);
free_mem:
    if (dest)
        aicos_free_align(0, dest);
}

#if defined(RT_USING_FINSH)
MSH_CMD_EXPORT_ALIAS(cmd_test_dma_memset, test_dma_memset,
                     Test DMA memset. Argument: Value Length);
#elif defined(AIC_CONSOLE_BARE_DRV)
#include <console.h>

static int test_dma_memset(int argc, char *argv[])
{
    cmd_test_dma_memset(argc, argv);
    return 0;
}

CONSOLE_CMD(test_dma_memset, test_dma_memset, "Test DMA memset. Argument: Value Length.");
#endif


#endif

