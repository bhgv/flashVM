/* Exact preparation peak budget, acquisition failures and allocation-free post-VM seeks. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define main original_decode_main
#include "swf_stream_mp3_decode.cpp"

#undef main
#include "flashvm/swf_stream_convert.h"


int main(int argc, char **)
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
    if (argc == 1)
    {
        for (long boundary = 0; boundary < 4; ++boundary)
        {
            allowance = boundary;
            if (!(fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &source) == FVM_OUT_OF_MEMORY && !source))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            allowance = -1;
        }
    }
    if (!(fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamSoundStorageInfo storage;
    storage.struct_size = sizeof(storage);
    if (!(fvm_swf_stream_sound_snapshot_storage_info(source, &storage) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamSoundSnapshot *exact = 0;
    if (!(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, storage.peak_bytes, &exact) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(exact);
    if (!(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, storage.peak_bytes - 1, &exact) == FVM_OUT_OF_MEMORY &&
          !exact))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    allowance = 0;
    FVM_SWFStreamConverter converter;
    if (!(fvm_swf_stream_converter_init(&converter, source, fixture.rate, fixture.channels) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned epoch = 0; epoch < 128; ++epoch)
    {
        FVM_SWFStreamSoundSeekPoint point;
        point.struct_size = sizeof(point);
        if (!(fvm_swf_stream_sound_snapshot_seek_point(source, epoch % 2 ? 3 : 1, &point) == FVM_OK &&
              fvm_swf_stream_converter_seek(&converter, point.block_index, point.first) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        short pcm[8];
        size_t n = 0;
        FVM_SWFStreamConvertedChunk chunk;
        chunk.struct_size = sizeof(chunk);
        if (!(fvm_swf_stream_converter_next(&converter, 4, pcm, 8, &chunk, &n) == FVM_OK && n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_swf_stream_sound_snapshot_close(source);
    allowance = -1;
    return 0;
}
