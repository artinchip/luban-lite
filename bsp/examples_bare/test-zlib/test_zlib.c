
#include "dfs.h"
#include "unistd.h"
#include "mpp_zlib.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include <console.h>
#include "aic_core.h"

static void print_help(char *program)
{
    printf("Compile time: %s\n", __TIME__);
    printf("usage:%s input_file out_file out_put_buffer_len \n", program);
    printf("note:out_put_buffer_len >= out_file_size \n");
    printf("exsample:%s readme.zlib  readme.txt 204800\n",program);
}

int zlib_test(int argc,char **argv)
{
    int ret = 0;
    int fd_in = 0;
    int fd_out = 0;
    int file_len;
    int in_len_align;
    int out_len;
    int out_len_align;
    int r_len=0,w_len=0;
    int uncompress_len;
    unsigned long in_buff = 0;;
    unsigned long in_buff_align;
    unsigned long out_buff = 0;
    unsigned long out_buff_align;
    int align;
    unsigned int before;
    unsigned int after;

    if (argc != 4) {
        print_help(argv[0]);
       return -1;
    }

    fd_in = open(argv[1], O_RDONLY);
    if (fd_in < 0) {
        loge("open %s fail\n",argv[1]);
        return -1;
    }

    file_len = lseek(fd_in, 0, SEEK_END);
    lseek(fd_in, 0, SEEK_SET);

    #define INPUT_BUFFER_ALIGN 16
    if (CACHE_LINE_SIZE > INPUT_BUFFER_ALIGN) {
        align = CACHE_LINE_SIZE;
    } else {
        align = INPUT_BUFFER_ALIGN;
    }

    //input buffer len align max of {CACHE_LINE_SIZE,INPUT_BUFFER_ALIGN}
    in_len_align = (file_len+align-1)/align*align;
    in_buff = (unsigned long)aicos_malloc(MEM_CMA, in_len_align+align-1);
    if (in_buff == 0) {
        loge("mpp_alloc fail\n");
        ret = -1;
        goto _exit;
    }
    //input buffer addr align max of {CACHE_LINE_SIZE,INPUT_BUFFER_ALIGN}
    in_buff_align = ((in_buff+align-1)&(~(align-1)));
    r_len = read(fd_in,(void *)in_buff_align, file_len);
    logd("r_len:%d,in_len:%d\n",r_len,file_len);
    //flush cache*
    aicos_dcache_clean_range((unsigned long *)in_buff_align, (int64_t)in_len_align);

    out_len = atoi(argv[3]);
    if (out_len < file_len) {
        loge("param error :%d\n",out_len);
        ret = -1;
        goto _exit;
    }
    //  out buffer len align  CACHE_LINE_SIZE
    out_len_align = (out_len + CACHE_LINE_SIZE -1)/CACHE_LINE_SIZE*CACHE_LINE_SIZE;
    out_buff = (unsigned long)aicos_malloc(MEM_CMA, out_len_align+(CACHE_LINE_SIZE -1));
    if (out_buff == 0) {
        loge("mpp_alloc fail\n");
        ret = -1;
        goto _exit;
    }
    //out buffer addr align CACHE_LINE_SIZE
    out_buff_align = ((out_buff+CACHE_LINE_SIZE -1)&(~(CACHE_LINE_SIZE-1)));
    // uncompressed
    before = aic_get_time_us();
    uncompress_len =  mpp_zlib_uncompressed((unsigned char*)in_buff_align, file_len, (unsigned char*)out_buff_align, out_len_align);
    after =  aic_get_time_us();

    logd("diff:%u\n",after-before);

    if (uncompress_len < 0) {
        loge("mpp_zlib_uncompressed fail\n");
        ret = -1;
        goto _exit;
    }

    //save uncompressed data
    fd_out = open(argv[2], O_RDWR|O_CREAT);
    if (fd_out < 0) {
        loge("open %s fail\n",argv[2]);
        ret = -1;
        goto _exit;
    }

    // invalid cache
    aicos_dcache_invalid_range((unsigned long *)out_buff_align, (int64_t)out_len_align);

    w_len = write(fd_out,(void *)out_buff_align, uncompress_len);

    logd("w_len:%d,uncompress_len:%d\n", w_len,uncompress_len);

    close(fd_out);

_exit:
    if(out_buff)
        aicos_free(MEM_CMA, (void *)out_buff);
    if(in_buff)
        aicos_free(MEM_CMA, (void *)in_buff);
    if(fd_in)
        close(fd_in);
    return ret;
}


CONSOLE_CMD(zlib_test, zlib_test,"zlib test");
