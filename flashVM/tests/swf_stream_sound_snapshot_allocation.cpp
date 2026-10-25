/* Every snapshot allocation failure is recoverable; prepared reads survive VM teardown without allocation. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define main stream_reference_main
#include "swf_stream_sound.cpp"

#undef main
#include "../src/swf/sound_stream_internal.hpp"
#include "flashvm/swf_stream_pcm.h"
#include "flashvm/swf_stream_convert.h"


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
    Bytes tags, pcm;
    pcm.push_back(255);
    tag(tags, 45, header(12));
    tag(tags, 19, pcm);
    load(c, movie(tags));
    FVM_SWFStreamSoundSnapshot *s = 0;
    for (long boundary = 0; boundary < 3; ++boundary)
    {
        allowance = boundary;
        if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &s) == FVM_OUT_OF_MEMORY && !s))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        allowance = -1;
    }
    rt->config.memory_limit_bytes = (unsigned long)(sizeof(FVM_SWFStreamSoundEntry));
    if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &s) == FVM_OUT_OF_MEMORY && !s))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = (unsigned long)(sizeof(FVM_SWFStreamSoundEntry) + pcm.size());
    if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &s) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    allowance = 0;
    FVM_SWFStreamSoundInfo info;
    info.struct_size = sizeof(info);
    FVM_SWFStreamSoundBlock block;
    block.struct_size = sizeof(block);
    size_t n = 0;
    short out = 0;
    if (!(fvm_swf_stream_sound_snapshot_info(s, &info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_block_count(s, &n) == FVM_OK && n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_block_at(s, 0, &block) == FVM_OK && block.data[0] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_read_pcm(s, 0, 0, 1, &out, 1, &n) == FVM_OK && n == 1 && out == 32512))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamPCMReader reader;
    if (!(fvm_swf_stream_pcm_reader_init(&reader, s) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamPCMChunk chunk;
    chunk.struct_size = sizeof(chunk);
    if (!(fvm_swf_stream_pcm_reader_next(&reader, 1, &out, 1, &chunk, &n) == FVM_OK && n == 1 && out == 32512))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_pcm_reader_seek(&reader, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_pcm_reader_next(&reader, 1, &out, 1, &chunk, &n) == FVM_OK && n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_pcm_reader_next(&reader, 1, &out, 1, &chunk, &n) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamConverter converter;
    FVM_SWFStreamConvertedChunk converted;
    converted.struct_size = sizeof(converted);
    short stereo[2];
    if (!(fvm_swf_stream_converter_init(&converter, s, 48000, 2) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_converter_next(&converter, 1, stereo, 2, &converted, &n) == FVM_OK && n == 2 &&
          stereo[0] == 32512 && stereo[1] == 32512))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_converter_next(&converter, 1, stereo, 2, &converted, &n) == FVM_OK && n == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_converter_next(&converter, 1, stereo, 2, &converted, &n) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_converter_seek(&converter, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(s);
    allowance = -1;
    return 0;
}
