/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 * Authors:  dwj <weijie.ding@artinchip.com>
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#define BUFSZ   1024
static rt_device_t snd_dev;

struct RIFF_HEADER_DEF
{
    char riff_id[4];     // 'R','I','F','F'
    uint32_t riff_size;
    char riff_format[4]; // 'W','A','V','E'
};

struct WAVE_FORMAT_DEF
{
    uint16_t FormatTag;
    uint16_t Channels;
    uint32_t SamplesPerSec;
    uint32_t AvgBytesPerSec;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
};

struct FMT_BLOCK_DEF
{
    char fmt_id[4];    // 'f','m','t',' '
    uint32_t fmt_size;
    struct WAVE_FORMAT_DEF wav_format;
};

struct DATA_BLOCK_DEF
{
    char data_id[4];     // 'R','I','F','F'
    uint32_t data_size;
};

struct wav_info
{
    struct RIFF_HEADER_DEF header;
    struct FMT_BLOCK_DEF   fmt_block;
    struct DATA_BLOCK_DEF  data_block;
};

int test_wavplay(int argc, char **argv)
{
    int fd = -1, len, stream = 0, ret = RT_EOK;
    uint8_t *buffer = NULL;
    struct wav_info *info = NULL;
    struct rt_audio_caps caps = {0};

    if (argc != 3)
    {
        rt_kprintf("Usage:\n");
        rt_kprintf("\taplay soundCard song.wav\n\n");
        rt_kprintf("\tFor example:\n");
        rt_kprintf("\t\taplay sound0 test.wav\n");
        rt_kprintf("\t\taplay i2s0_sound test.wav\n");
        rt_kprintf("\t\taplay i2s1_sound test.wav\n");
        return 0;
    }

    fd = open(argv[2], O_RDONLY);
    if (fd < 0)
    {
        rt_kprintf("open file failed!\n");
        ret = -RT_ERROR;
        goto __exit;
    }

    buffer = rt_malloc(BUFSZ);
    if (buffer == RT_NULL) {
        ret = -RT_ENOMEM;
        goto __exit;
    }

    rt_memset(buffer, 0, BUFSZ);

    info = (struct wav_info *) rt_malloc(sizeof * info);
    if (info == RT_NULL) {
        ret = -RT_ENOMEM;
        goto __exit;
    }

    len = read(fd, &(info->header), sizeof(struct RIFF_HEADER_DEF));
    if (len < sizeof(struct RIFF_HEADER_DEF) ||
        strncmp(info->header.riff_id, "RIFF", 4))
    {
        rt_kprintf("Get header chunk failed!\n");
        ret = -RT_EIO;
        goto __exit;
    }

    len = read(fd, &(info->fmt_block),  sizeof(struct FMT_BLOCK_DEF));
    if (len < sizeof(struct FMT_BLOCK_DEF) ||
        strncmp(info->fmt_block.fmt_id, "fmt", 3))
    {
        rt_kprintf("Get format chunk failed!\n");
        ret = -RT_EIO;
        goto __exit;
    }

    /*
     * Some wav file has LIST chunk, we should skip LIST chunk and
     * eventually find data chunk
     */
    do {
        len = read(fd, &(info->data_block), sizeof(struct DATA_BLOCK_DEF));
        if (len < sizeof(struct DATA_BLOCK_DEF))
        {
            rt_kprintf("Get data chunk failed!\n");
            ret = -RT_EIO;
            goto __exit;
        }
        else if (strncmp(info->data_block.data_id, "data", 4))
            lseek(fd, info->data_block.data_size, SEEK_CUR);
        else
            break;
    } while(1);

    rt_kprintf("wav information:\n");
    rt_kprintf("samplerate %d\n", info->fmt_block.wav_format.SamplesPerSec);
    rt_kprintf("channel %d\n", info->fmt_block.wav_format.Channels);
    rt_kprintf("samplebits %d\n", info->fmt_block.wav_format.BitsPerSample);
    rt_kprintf("data_size 0x%08x\n", info->data_block.data_size);

    snd_dev = rt_device_find(argv[1]);
    if (!snd_dev) {
        rt_kprintf("%s not found!\n", argv[1]);
        ret = -RT_ERROR;
        goto __exit;
    }

    rt_device_open(snd_dev, RT_DEVICE_OFLAG_WRONLY);

    caps.main_type               = AUDIO_TYPE_OUTPUT;
    caps.sub_type                = AUDIO_DSP_PARAM;
    caps.udata.config.samplerate = info->fmt_block.wav_format.SamplesPerSec;
    caps.udata.config.channels   = info->fmt_block.wav_format.Channels;
    caps.udata.config.samplebits = info->fmt_block.wav_format.BitsPerSample;
    rt_device_control(snd_dev, AUDIO_CTL_CONFIGURE, &caps);

    /* Start to playback. This step will enable Power Amplifier */
    stream = AUDIO_STREAM_REPLAY;
    rt_device_control(snd_dev, AUDIO_CTL_START, (void *)&stream);

    /* Wait Power Amplifier stable */
    rt_thread_mdelay(200);

    while (1)
    {
        int length;

        length = read(fd, buffer, BUFSZ);

        if (length <= 0)
            break;

        rt_device_write(snd_dev, 0, buffer, length);
    }

    rt_device_close(snd_dev);

__exit:

    if (fd >= 0)
        close(fd);

    if (buffer)
        rt_free(buffer);

    if (info)
        rt_free(info);

    return ret;
}
MSH_CMD_EXPORT_ALIAS(test_wavplay, aplay, play wav file);
