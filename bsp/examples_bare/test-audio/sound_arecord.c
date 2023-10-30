#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <hal_audio.h>
#include <unistd.h>
#include "ringbuffer.h"
#include "sound.h"

#ifdef LPKG_USING_DFS
#include <dfs.h>
#include <dfs_fs.h>
#ifdef LPKG_USING_DFS_ELMFAT
#include <dfs_elm.h>
#endif
#endif

#define RECORD_TIME_S       5
#define RECORD_SAMPLERATE   32000
#define RECORD_CHANNEL      2
#define RX_DMIC_FIFO_SIZE   (40960)
#define RECORD_CHUNK_SZ     (RX_DMIC_FIFO_SIZE / 2)

aic_audio_ctrl audio_ctrl_r;
ringbuf_t *ring_buf_r;
rt_uint8_t dmic_rx_fifo[RX_DMIC_FIFO_SIZE] __attribute__((aligned(64)));

static void wavheader_init(struct wav_header *header, int sample_rate,
                           int channels, int datasize)
{
    memcpy(header->riff_id, "RIFF", 4);
    header->riff_datasize = datasize + 44 - 8;
    memcpy(header->riff_type, "WAVE", 4);
    memcpy(header->fmt_id, "fmt ", 4);
    header->fmt_datasize = 16;
    header->fmt_compression_code = 1;
    header->fmt_channels = channels;
    header->fmt_sample_rate = sample_rate;
    header->fmt_bit_per_sample = 16;
    header->fmt_avg_bytes_per_sec = header->fmt_sample_rate *
                                    header->fmt_channels *
                                    header->fmt_bit_per_sample / 8;
    header->fmt_block_align = header->fmt_bit_per_sample *
                              header->fmt_channels / 8;
    memcpy(header->data_id, "data", 4);
    header->data_datasize = datasize;
}

static void drv_dmic_callback(aic_audio_ctrl *pcodec, void *arg)
{
    unsigned long event = (unsigned long)arg;

    switch (event)
    {
    case AUDIO_RX_DMIC_PERIOD_INT:
        ring_buf_r->write = (ring_buf_r->write + pcodec->dmic_info.buf_info.period_len) % RX_DMIC_FIFO_SIZE;
        ring_buf_r->data_len += pcodec->dmic_info.buf_info.period_len;
        break;
    default:
        hal_log_err("%s(%d)\n", __func__, __LINE__);
        break;
    }

}

static int sound_arecord(int argc, char *argv[])
{
    int fd;
    unsigned int *buffer = NULL;
    struct wav_header header;
    int length, total_len = 0;

    if (argc != 2)
    {
        hal_log_err("Usage:\n");
        hal_log_err("arecord song.wav\n");
        return 0;
    }

    fd = open(argv[1], O_WRONLY | O_CREAT);
    if (fd < 0)
    {
        hal_log_err("open file failed!\n");
        goto __exit;
    }

    write(fd, &header, sizeof(struct wav_header));

    buffer = aicos_malloc(MEM_CMA, RECORD_CHUNK_SZ);
    if (!buffer)
    {
        hal_log_err("buffer malloc error!\n");
        goto __exit;
    }

    ring_buf_r = aicos_malloc(MEM_CMA, sizeof(ringbuf_t));
    if (!ring_buf_r)
    {
        hal_log_err("ring_buf_r malloc error!\n");
        goto __exit;
    }

    ring_buf_r->buffer = dmic_rx_fifo;
    ring_buf_r->size = RX_DMIC_FIFO_SIZE;
    ring_buf_r->write = 0;
    ring_buf_r->read = 0;
    ring_buf_r->data_len = 0;

    audio_ctrl_r.dmic_info.buf_info.buf = (void *)dmic_rx_fifo;
    audio_ctrl_r.dmic_info.buf_info.buf_len = RX_DMIC_FIFO_SIZE;
    audio_ctrl_r.dmic_info.buf_info.period_len = RX_DMIC_FIFO_SIZE / 2;

    hal_audio_init(&audio_ctrl_r);
    hal_audio_attach_callback(&audio_ctrl_r, drv_dmic_callback, NULL);

    hal_dma_init();
    aicos_request_irq(DMA_IRQn, hal_dma_irq, 0, NULL, NULL);

    hal_audio_set_dmic_channel(&audio_ctrl_r, RECORD_CHANNEL);
    hal_audio_set_samplerate(&audio_ctrl_r, RECORD_SAMPLERATE);
    audio_ctrl_r.config.samplerate = RECORD_SAMPLERATE;
    audio_ctrl_r.config.channel = RECORD_CHANNEL;
    audio_ctrl_r.config.samplebits = 16;

    hal_audio_dmic_start(&audio_ctrl_r);

    while (1)
    {
        int wr_size = 0;
        length = ringbuf_out(ring_buf_r, buffer, RECORD_CHUNK_SZ);

        if (length) {
            wr_size = write(fd, buffer, length);
            total_len += wr_size;
        }

        if (total_len >= (RECORD_SAMPLERATE * RECORD_CHANNEL * RECORD_TIME_S) * 2)
            break;
    }

    wavheader_init(&header, RECORD_SAMPLERATE, RECORD_CHANNEL, total_len);
    lseek(fd, 0, SEEK_SET);
    write(fd, &header, sizeof(struct wav_header));

    hal_audio_dmic_stop(&audio_ctrl_r);

__exit:
    if (fd >= 0)
        close(fd);
    if (buffer != NULL)
        aicos_free(MEM_CMA, buffer);
    if (ring_buf_r != NULL)
        aicos_free(MEM_CMA, ring_buf_r);

    return 0;
}

CONSOLE_CMD(arecord, sound_arecord, "arecord song.wav");
