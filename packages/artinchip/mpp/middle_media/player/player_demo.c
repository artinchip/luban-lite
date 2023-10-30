/*
* Copyright (C) 2020-2023 ArtInChip Technology Co. Ltd
*
*  author: <jun.ma@artinchip.com>
*  Desc: OMX_VdecComponent tunneld  OMX_VideoRenderComponent demo
*/

#include <string.h>
#include <malloc.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <dirent.h>
#include <inttypes.h>
#include <getopt.h>

#include "mpp_dec_type.h"
#include "mpp_list.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "aic_message.h"
#include "aic_player.h"

#include <rthw.h>
#include <rtthread.h>
#include <shell.h>

#ifdef LPKG_USING_CPU_USAGE
#include "cpu_usage.h"
#endif

#define PLAYER_DEMO_FILE_MAX_NUM 128
#define PLAYER_DEMO_FILE_PATH_MAX_LEN 256
#define BUFFER_LEN 16

static int g_player_end = 0;
static int g_demuxer_detected_flag = 0;
static int g_sync_flag = AIC_PLAYER_PREPARE_SYNC;
static struct av_media_info g_media_info;
struct file_list {
    char *file_path[PLAYER_DEMO_FILE_MAX_NUM];
    int file_num;
};

static void print_help(const char* prog)
{
    printf("name: %s\n", prog);
    printf("Compile time: %s\n", __TIME__);
    printf("Usage: player_demo [options]:\n"
        "\t-i                             input stream file name\n"
        "\t-t                             directory of test files\n"
        "\t-l                             loop time\n"
        "\t-c                             save capture file path,default /sdcard/video/capture.jpg \n"
        "\t-W                             capture widht\n"
        "\t-H                             capture height\n"
        "\t-q                             capture quality\n"
        "\t-h                             help\n\n"
        "Example1(test single file for 1 time): player_demo -i /mnt/video/test.mp4 \n"
        "Example2(test single file for 3 times): player_demo -i /mnt/video/test.mp4 -l 3 \n"
        "Example3(test some files for 1 time ) : player_demo -t /mnt/video \n"
        "Example4(test some files for 3 times ): player_demo -t /mnt/video -l 3 \n"
        "---------------------------------------------------------------------------------------\n"
        "-------------------------------control key while playing-------------------------------\n"
        "---------------------------------------------------------------------------------------\n"
        "('d'): play next \n"
        "('u'): play previous \n"
        "('p'): pause/play \n"
        "('+'): volum+5 \n"
        "('-'): volum-5 \n"
        "('f'): forward seek +8s \n"
        "('b'): back seek -8s \n"
        "('z'): seek to begin pos \n"
        "('m':  enter/eixt mute \n"
        "('e'): eixt app \n"
        "('c'): capture pic,firstly,please pause and then capture \n");
}

static int read_dir(char* path, struct file_list *files)
{
    char* ptr = NULL;
    int file_path_len = 0;
    struct dirent* dir_file;
    DIR* dir = opendir(path);
    if (dir == NULL) {
        loge("read dir failed");
        return -1;
    }

    while((dir_file = readdir(dir))) {
        if (strcmp(dir_file->d_name, ".") == 0 || strcmp(dir_file->d_name, "..") == 0)
            continue;

        ptr = strrchr(dir_file->d_name, '.');
        if (ptr == NULL)
            continue;

        if (strcmp(ptr, ".h264") && strcmp(ptr, ".264") && strcmp(ptr, ".mp4"))
            continue;

        logd("name: %s", dir_file->d_name);

        file_path_len = 0;
        file_path_len += strlen(path);
        file_path_len += 1; // '/'
        file_path_len += strlen(dir_file->d_name);
        printf("file_path_len:%d\n",file_path_len);
        if (file_path_len > PLAYER_DEMO_FILE_PATH_MAX_LEN-1) {
            loge("%s too long \n",dir_file->d_name);
            continue;
        }
        files->file_path[files->file_num] = (char *)mpp_alloc(file_path_len+1);
        files->file_path[files->file_num][file_path_len] = '\0';
        strcpy(files->file_path[files->file_num], path);
        strcat(files->file_path[files->file_num], "/");
        strcat(files->file_path[files->file_num], dir_file->d_name);
        logd("i: %d, filename: %s", files->file_num, files->file_path[files->file_num]);
        files->file_num ++;
        if (files->file_num >= PLAYER_DEMO_FILE_MAX_NUM)
            break;
    }
    return 0;
}

s32 event_handle(void* app_data,s32 event,s32 data1,s32 data2)
{
    int ret = 0;
    switch(event) {
        case AIC_PLAYER_EVENT_PLAY_END:
            g_player_end = 1;
            logd("g_player_end\n");
            break;
        case AIC_PLAYER_EVENT_PLAY_TIME:
            break;
        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_DETECTED:
            if (AIC_PLAYER_PREPARE_ASYNC == g_sync_flag) {
                g_demuxer_detected_flag = 1;
                logd("AIC_PLAYER_EVENT_DEMUXER_FORMAT_DETECTED\n");
            }
            break;

        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_NOT_DETECTED:
            if (AIC_PLAYER_PREPARE_ASYNC == g_sync_flag) {
                logd("AIC_PLAYER_EVENT_DEMUXER_FORMAT_NOT_DETECTED\n");
                logd("cur file format not detected,play next file!!!!!!\n");
                g_player_end = 1;
            }

            break;
        default:
            break;
    }
    return ret;
}

static int set_volume(struct aic_player *player,int volume)
{
    if (volume < 0) {
        volume = 0;
    } else if (volume < 101) {

    } else {
        volume = 100;
    }
    logd("volume:%d\n",volume);
    return aic_player_set_volum(player,volume);
}

static int do_seek(struct aic_player *player,int forward)
{
    s64 pos;
    pos = aic_player_get_play_time(player);
    if (pos == -1) {
        loge("aic_player_get_play_time error!!!!\n");
        return -1;
    }
    if (forward == 1) {
        pos += 8*1000*1000;//+8s
    } else {
        pos -= 8*1000*1000;//-8s
    }

    if (pos < 0) {
        pos = 0;
    } else if (pos < g_media_info.duration) {

    } else {
        pos = g_media_info.duration;
    }

    if (aic_player_seek(player,pos) != 0) {
        loge("aic_player_seek error!!!!\n");
        return -1;
    }
    logd("aic_player_seek ok\n");
    return 0;
}

static int do_rotation(struct aic_player *player)
{
    static int index = 0;
    int rotation = MPP_ROTATION_0;

    if (index % 4 == 0) {
        rotation = MPP_ROTATION_90;
        logd("*********MPP_ROTATION_90***************\n");
    } else if(index % 4 == 1) {
        rotation = MPP_ROTATION_180;
        logd("*********MPP_ROTATION_180***************\n");
    } else if(index % 4 == 2) {
        rotation = MPP_ROTATION_270;
        logd("*********MPP_ROTATION_270***************\n");
    } else if(index % 4 == 3) {
        rotation = MPP_ROTATION_0;
        logd("*********MPP_ROTATION_0***************\n");
    }
    aic_player_set_rotation(player,rotation);
    index++;
    return 0;
}

static int start_play(struct aic_player *player,int volume)
{
    int ret = -1;
    static struct av_media_info media_info;
    struct mpp_size screen_size;
    struct mpp_rect disp_rect;

    ret = aic_player_start(player);
    if (ret != 0) {
        loge("aic_player_start error!!!!\n");
        return -1;
    }
    printf("[%s:%d]aic_player_start ok\n",__FUNCTION__,__LINE__);

    ret =  aic_player_get_media_info(player,&media_info);
    if (ret != 0) {
        loge("aic_player_get_media_info error!!!!\n");
        return -1;
    }
    g_media_info = media_info;
    logd("aic_player_get_media_info duration:"FMT_x64",file_size:"FMT_x64"\n",media_info.duration,media_info.file_size);

    logd("has_audio:%d,has_video:%d,"
        "width:%d,height:%d,\n"
        "bits_per_sample:%d,nb_channel:%d,sample_rate:%d\n"
        ,media_info.has_audio
        ,media_info.has_video
        ,media_info.video_stream.width
        ,media_info.video_stream.height
        ,media_info.audio_stream.bits_per_sample
        ,media_info.audio_stream.nb_channel
        ,media_info.audio_stream.sample_rate);

    if (media_info.has_video) {
        ret = aic_player_get_screen_size(player, &screen_size);
        if (ret != 0) {
            loge("aic_player_get_screen_size error!!!!\n");
            return -1;
        }
        logd("screen_width:%d,screen_height:%d\n",screen_size.width,screen_size.height);
        disp_rect.x = 324;
        disp_rect.y = 50;
        disp_rect.width = 600;
        disp_rect.height = 500;
        ret = aic_player_set_disp_rect(player, &disp_rect);//attention:disp not exceed screen_size
        if (ret != 0) {
            loge("aic_player_set_disp_rect error\n");
            return -1;
        }
        logd("aic_player_set_disp_rect  ok\n");
    }

    if (media_info.has_audio) {
        ret = set_volume(player,volume);
        if (ret != 0) {
            loge("set_volume error!!!!\n");
            return -1;
        }
    }

    ret = aic_player_play(player);
    if (ret != 0) {
        loge("aic_player_play error!!!!\n");
        return -1;
    }
    printf("[%s:%d]aic_player_play ok\n",__FUNCTION__,__LINE__);
    return 0;
}

//#define _THREAD_TRACE_INFO_

#ifdef _THREAD_TRACE_INFO_
struct thread_trace_info {
    uint32_t enter_run_tick;
    uint32_t total_run_tick;
    char thread_name[8];
};

static struct  thread_trace_info thread_trace_infos[6];


// count the cpu usage time of each thread
static void hook_of_scheduler(struct rt_thread *from,struct rt_thread *to) {
    static int show = 0;
    static uint32_t sys_tick = 0;
    int i = 0;
    for(i=0;i<6;i++) {
        if (!strcmp(thread_trace_infos[i].thread_name,from->name)) {
            uint32_t run_tick;
            run_tick = rt_tick_get() -  thread_trace_infos[i].enter_run_tick;
            thread_trace_infos[i].total_run_tick += run_tick;
            break;
        }
    }

    for(i=0;i<6;i++) {
        if (!strcmp(thread_trace_infos[i].thread_name,to->name)) {
                thread_trace_infos[i].enter_run_tick = rt_tick_get();
            break;
        }
    }
    show++;
    if (show > 10*1000) {
        rt_kprintf("[%u:%u:%u:%u:%u:%u:%u]:%u\n"
            ,thread_trace_infos[0].total_run_tick
            ,thread_trace_infos[1].total_run_tick
            ,thread_trace_infos[2].total_run_tick
            ,thread_trace_infos[3].total_run_tick
            ,thread_trace_infos[4].total_run_tick
            ,thread_trace_infos[5].total_run_tick
            ,thread_trace_infos[5].total_run_tick+thread_trace_infos[4].total_run_tick+thread_trace_infos[3].total_run_tick+thread_trace_infos[2].total_run_tick+thread_trace_infos[1].total_run_tick+thread_trace_infos[0].total_run_tick
            ,rt_tick_get() - sys_tick);

         for(i=0;i<6;i++) {
             thread_trace_infos[i].total_run_tick = 0;
         }
         show = 0;
         sys_tick = rt_tick_get();
    }
}
#endif

static void player_demo_test(int argc, char **argv)
{
    int ret = 0;
    int i = 0;
    int j = 0;
    char ch;
    int file_path_len;
    int opt;
    int loop_time = 1;
    struct file_list  files;
    struct aic_player *player = NULL;
    int volume = 50;
    struct aic_capture_info   capture_info;
    char file_path[255] = {"/sdcard/video/capture.jpg"};
    rt_device_t dev = RT_NULL;
    // get serial dev
    dev = rt_device_find("uart0");
    //default capture_info
    capture_info.file_path = (s8 *)file_path;
    capture_info.width = 1024;
    capture_info.height = 600;
    capture_info.quality = 90;
    memset(&files,0x00,sizeof(struct file_list));

#ifdef _THREAD_TRACE_INFO_
    memset(&thread_trace_infos,0x00,sizeof(struct thread_trace_info));
    for (i = 0; i < 6 ;i++) {
        snprintf(thread_trace_infos[i].thread_name,sizeof(thread_trace_infos[i].thread_name),"%s%02d","pth",i);
        printf("%s\n",thread_trace_infos[i].thread_name);
    }
    rt_scheduler_sethook(hook_of_scheduler);
#endif

    optind = 0;
    while (1) {
        opt = getopt(argc, argv, "i:t:l:c:W:H:q:h");
        if (opt == -1) {
            break;
        }
        switch (opt) {
        case 'i':
            file_path_len = strlen(optarg);
            printf("file_path_len:%d\n",file_path_len);
            if (file_path_len > PLAYER_DEMO_FILE_PATH_MAX_LEN-1) {
                loge("file_path_len too long \n");
                goto _EXIT0_;
            }
            files.file_path[0] = (char *)mpp_alloc(file_path_len+1);
            files.file_path[0][file_path_len] = '\0';
            strcpy(files.file_path[0], optarg);
            files.file_num = 1;
            logd("file path: %s", files.file_path[0]);
            break;
        case 'l':
            loop_time = atoi(optarg);
            break;
        case 't':
            read_dir(optarg, &files);
            break;
        case 'c':
            memset(file_path,0x00,sizeof(file_path));
            strncpy(file_path, optarg,sizeof(file_path)-1);
            logd("file path: %s", file_path);
            break;
        case 'W':
            capture_info.width = atoi(optarg);
            break;
        case 'H':
            capture_info.height = atoi(optarg);
            break;
        case 'q':
            capture_info.quality = atoi(optarg);
            break;
        case 'h':
            print_help(argv[0]);
        default:
            goto _EXIT0_;
            break;
        }
    }

    if (files.file_num == 0) {
        print_help(argv[0]);
        loge("files.file_num ==0 !!!\n");
        goto _EXIT0_;
    }

    player = aic_player_create(NULL);
    if (player == NULL) {
        loge("aic_player_create fail!!!\n");
        goto _EXIT0_;
    }

    aic_player_set_event_callback(player,player,event_handle);
    g_sync_flag = AIC_PLAYER_PREPARE_SYNC;

    for(i = 0;i < loop_time; i++) {
        for(j = 0; j < files.file_num; j++) {
            aic_player_set_uri(player,files.file_path[j]);
            if (g_sync_flag == AIC_PLAYER_PREPARE_ASYNC) {
                ret = aic_player_prepare_async(player);
            } else {
                ret = aic_player_prepare_sync(player);
            }
            if (ret) {
                loge("aic_player_prepare error!!!!\n");
                g_player_end = 1;
                goto _NEXT_FILE_;
            }

            if (g_sync_flag == AIC_PLAYER_PREPARE_SYNC) {
                if (start_play(player,volume) != 0) {
                    g_player_end = 1;
                    goto _NEXT_FILE_;
                }
            }

            while(1)
            {
    _NEXT_FILE_:
                if (g_player_end == 1) {
                    logd("play file:%s end!!!!\n",files.file_path[j]);
                    ret = aic_player_stop(player);
                    g_player_end = 0;
                    break;
                }
                if (g_sync_flag == AIC_PLAYER_PREPARE_ASYNC && g_demuxer_detected_flag == 1) {
                    g_demuxer_detected_flag = 0;
                    if (start_play(player,volume) != 0) {
                        g_player_end = 1;
                        goto _NEXT_FILE_;
                    }
                }
                if (rt_device_read(dev, -1, &ch, 1) == 1) {
                    if (ch == 0x20) {// pause
                        logd("*********enter pause ***************\n");
                        aic_player_pause(player);
                    } else if (ch == 'd') {//stop cur, star next
                        logd("*********enter down ***************\n");
                        aic_player_stop(player);
                        break;
                    } else if (ch == 'u') {//stop cur, star pre
                        logd("*********enter up j:%d***************\n",j);
                        aic_player_stop(player);
                        j -= 2;
                        j = (j < -1)?(-1):(j);
                        break;
                    } else if (ch == '-') {
                        logd("*********enter volume--**************\n");
                        volume -= 5;
                        set_volume(player,volume);
                    } else if (ch == '+') {
                        logd("*********enter volume++***************\n");
                        volume += 5;
                        set_volume(player,volume);
                    } else if (ch == 'm') {
                        logd("*********enter/exit mute***************\n");
                            aic_player_set_mute(player);
                    } else if (ch == 'c') {
                        logd("*********capture***************\n");
                        if (aic_player_capture(player,&capture_info) == 0) {
                            logd("*********aic_player_capture ok***************\n");
                        } else {
                            loge("*********aic_player_capture fail ***************\n");
                        }
                    } else if (ch == 'f') {
                        logd("*********forward***************\n");
                        do_seek(player,1);//+8s
                    } else if (ch == 'b') {
                        logd("*********back***************\n");
                        do_seek(player,0);//-8s
                    } else if (ch == 'z') {//seek to start
                        if (aic_player_seek(player,0) != 0) {
                            loge("aic_player_seek error!!!!\n");
                        } else {
                            logd("aic_player_seek ok\n");
                        }
                    } else if (ch == 'r') {
                        do_rotation(player);
                    } else if (ch == 'e') {
                        aic_player_stop(player);
                        goto _EXIT0_;
                    }
                } else {
                #ifdef LPKG_USING_CPU_USAGE
                {
                    static int index = 0;
                    char data_str[64];
                    float value = 0.0;
                    if (index++ % 10 == 0) {
                        value = cpu_load_average();
                        snprintf(data_str,sizeof(data_str),"%.2f%%\n", value);
                        printf("cpu_loading:%s\n",data_str);
                    }

                }
                #endif
                    usleep(1000*1000);
                }
            }
        }
    }

_EXIT0_:
    if (player)
        aic_player_destroy(player);

    for(i = 0; i <files.file_num ;i++) {
        if (files.file_path[i]) {
            mpp_free(files.file_path[i]);
        }
    }
    logd("player_demo exit\n");
    return;
}

MSH_CMD_EXPORT_ALIAS(player_demo_test,player_demo, player demo);
