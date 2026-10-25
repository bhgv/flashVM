/* Four OS workers share immutable post-VM MP3 metadata with independent conversion grids. */
#define main original_decode_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_mp3_decode.cpp"

#undef main
#include "flashvm/swf_stream_convert.h"
#include "../ports/thread_port.h"


struct SeekJob
{
    const FVM_SWFStreamSoundSnapshot *source;
    unsigned rate, channels, done;
};


static void seek_worker(void *raw)
{
    SeekJob *job = (SeekJob *)raw;
    FVM_SWFStreamConverter converter;
    if (!(fvm_swf_stream_converter_init(&converter, job->source, job->rate, job->channels) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned epoch = 0; epoch < 64; ++epoch)
    {
        FVM_SWFStreamSoundSeekPoint point;
        point.struct_size = sizeof(point);
        if (!(fvm_swf_stream_sound_snapshot_seek_point(job->source, epoch % 2 ? 3 : 1, &point) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_converter_seek(&converter, point.block_index, point.first) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        short actual[8], expected[8];
        size_t written = 0, count = 0;
        FVM_SWFStreamConvertedChunk chunk;
        chunk.struct_size = sizeof(chunk);
        if (!(fvm_swf_stream_converter_next(&converter, 4, actual, 8, &chunk, &written) == FVM_OK &&
              written == 4 * job->channels))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_sound_snapshot_read_pcm(
                  job->source, point.block_index, point.first, 4, expected, 8, &count) == FVM_OK &&
              count == written))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (size_t i = 0; i < count; ++i)
        {
            if (!(actual[i] == expected[i]))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        ++job->done;
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const mp3_fixture::Fixture &fixture = mp3_fixture::fixtures[0];
    Bytes swf = stream_movie(fixture, 1105);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamSoundSnapshot *source = 0;
    if (!(fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    SeekJob jobs[4];
    FVM_PortThread *threads[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        jobs[i].source = source;
        jobs[i].rate = fixture.rate;
        jobs[i].channels = fixture.channels;
        jobs[i].done = 0;
        threads[i] = fvm_port_thread_start(seek_worker, jobs + i);
        if (!(threads[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_port_thread_join(threads[i]);
        if (!(jobs[i].done == 64))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_swf_stream_sound_snapshot_close(source);
    return 0;
}
