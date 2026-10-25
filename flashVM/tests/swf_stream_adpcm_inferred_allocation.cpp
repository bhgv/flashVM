/* Inferred ADPCM preserves budget rollback; normalized reads allocate nothing after VM teardown. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "swf_stream_adpcm_inferred_fixture.hpp"


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
    Bits bits = partial_bits(5, 2, 17);
    Bytes swf = swf_block(bits.bytes, 2);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamSoundSnapshot *source = 0;
    if (!(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c, 0, 0, &source) == FVM_OUT_OF_MEMORY && !source))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (long n = first; n < last; ++n)
    {
        allowance = n;
        FVM_Result r = fvm_swf_stream_sound_snapshot_open_pcm_inferred(c, 0, (size_t)-1, &source);
        allowance = -1;
        if (n < 4)
        {
            if (!(r == FVM_OUT_OF_MEMORY && !source))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(r == FVM_OK && source))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_swf_stream_sound_snapshot_close(source);
            source = 0;
        }
    }
    if (!(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c, 0, (size_t)-1, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamSoundStorageInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_stream_sound_snapshot_storage_info(source, &info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(source);
    if (!(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c, 0, info.peak_bytes - 1, &source) == FVM_OUT_OF_MEMORY &&
          !source))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c, 0, info.peak_bytes, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    allowance = 0;
    verify_partial(source, 17, 2, 5);
    fvm_swf_stream_sound_snapshot_close(source);
    if (!(!allowance))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    return 0;
}
