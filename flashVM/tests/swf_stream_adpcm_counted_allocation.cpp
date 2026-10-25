/* Exhaust counted-ADPCM allocation failures; immutable native reads allocate nothing. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define main adpcm_reference_main
#include "swf_stream_adpcm.cpp"

#undef main


int main(int argc, char **argv)
{
    long first = 0, last = 5;
    if (argc == 2)
    {
        char *end = 0;
        first = std::strtol(argv[1], &end, 10);
        if (!(end && !*end && first >= 0 && first <= 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        last = first + 1;
    }
    else
    {
        if (!(argc == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bits bits;
    bits.put(2, 2);
    bits.put(100, 16);
    bits.put(0, 6);
    for (unsigned i = 1; i < 17; ++i)
    {
        bits.put(0, 4);
    }
    Bytes swf = swf_block(bits.bytes, 1);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamSoundSnapshot *encoded = 0;
    if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &encoded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const size_t frames = 17;
    FVM_SWFStreamSoundSnapshot *source = 0;
    allowance = 1;
    if (!(fvm_swf_stream_sound_snapshot_open_adpcm_counted(c, 0, &frames, 1, 0, &source) == FVM_OUT_OF_MEMORY &&
          !source && allowance == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_open_adpcm_counted(c, 0, 0, 1, (size_t)-1, &source) == FVM_INVALID_ARGUMENT &&
          !source && allowance == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    /* Object, block table, encoded bytes and decoded S16 are the four native acquisitions. */
    for (long budget = first; budget < last; ++budget)
    {
        allowance = budget;
        FVM_Result result = fvm_swf_stream_sound_snapshot_open_adpcm_counted(c, 0, &frames, 1, (size_t)-1, &source);
        allowance = -1;
        if (!(budget < 4 ? result == FVM_OUT_OF_MEMORY && !source : result == FVM_OK && source))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Runtime payload limit still applies independently of the explicit native peak ceiling. */
    size_t old_limit = rt->config.memory_limit_bytes;
    rt->config.memory_limit_bytes = bits.bytes.size() + 128;
    FVM_SWFStreamSoundSnapshot *other = 0;
    if (!(fvm_swf_stream_sound_snapshot_open_adpcm_counted(c, 0, &frames, 1, (size_t)-1, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(other);
    rt->config.memory_limit_bytes = bits.bytes.size();
    if (!(fvm_swf_stream_sound_snapshot_open_adpcm_counted(c, 0, &frames, 1, (size_t)-1, &other) == FVM_OUT_OF_MEMORY &&
          !other))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = old_limit;
    allowance = 0;
    short raw_pcm[17];
    size_t raw_written = 91;
    if (!(fvm_swf_stream_sound_read_adpcm(c, 0, 0, frames, 0, frames, raw_pcm, 17, &raw_written) == FVM_OK &&
          raw_written == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    allowance = 0;
    if (!(fvm_swf_stream_sound_snapshot_read_adpcm(encoded, 0, frames, 0, frames, raw_pcm, 17, &raw_written) ==
              FVM_OK &&
          raw_written == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < 17; ++i)
    {
        if (!(raw_pcm[i] == 100))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_swf_stream_sound_snapshot_close(encoded);
    if (!(allowance == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    if (source)
    {
        allowance = 0;
        FVM_SWFStreamPCMReader reader;
        FVM_SWFStreamPCMChunk chunk;
        FVM_SWFStreamSoundStorageInfo storage;
        storage.struct_size = sizeof(storage);
        if (!(fvm_swf_stream_sound_snapshot_storage_info(source, &storage) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_pcm_reader_init(&reader, source) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        short pcm[17];
        size_t written;
        chunk.struct_size = sizeof(chunk);
        if (!(fvm_swf_stream_pcm_reader_next(&reader, 17, pcm, 17, &chunk, &written) == FVM_OK && written == 17))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (size_t i = 0; i < 17; ++i)
        {
            if (!(pcm[i] == 100))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_swf_stream_pcm_reader_next(&reader, 17, pcm, 17, &chunk, &written) == FVM_NOT_FOUND))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_swf_stream_sound_snapshot_close(source);
        if (!(allowance == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        allowance = -1;
    }
    return 0;
}
