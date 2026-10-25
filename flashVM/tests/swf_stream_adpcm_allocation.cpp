/* Fail every ADPCM preparation allocation; normalized reads need no allocator after VM teardown. */
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
    long begin = 0, end = 32;
    bool isolated = argc == 3;
    bool encoded_only = isolated && !std::strcmp(argv[1], "--encoded-allocation-budget");
    if (isolated)
    {
        if (!(encoded_only || !std::strcmp(argv[1], "--allocation-budget")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        char *tail = 0;
        begin = std::strtol(argv[2], &tail, 10);
        if (!(tail && !*tail && begin >= 0 && begin < 32))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        end = begin + 1;
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
    for (unsigned f = 0; f < 8192; ++f)
    {
        if (!(f % 4096))
        {
            bits.put(100, 16);
            bits.put(0, 6);
        }
        else
        {
            bits.put(0, 4);
        }
    }
    Bytes swf = swf_block(bits.bytes, 1);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* A rejected ownership ceiling must not consume even the first allocation attempt. */
    FVM_SWFStreamSoundSnapshot *encoded = reinterpret_cast<FVM_SWFStreamSoundSnapshot *>(1);
    allowance = 1;
    if (!(fvm_swf_stream_sound_snapshot_open_bounded(c, 0, 0, &encoded) == FVM_OUT_OF_MEMORY && !encoded &&
          allowance == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    bool raw_success = false;
    for (long raw_budget = encoded_only ? begin : (isolated ? 3 : 0); raw_budget < (encoded_only ? end : 4);
         ++raw_budget)
    {
        allowance = raw_budget;
        FVM_Result result = fvm_swf_stream_sound_snapshot_open_bounded(c, 0, (size_t)-1, &encoded);
        allowance = -1;
        if (raw_budget < 3)
        {
            if (!(result == FVM_OUT_OF_MEMORY && !encoded))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(result == FVM_OK && encoded))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            raw_success = true;
        }
    }
    if (encoded_only)
    {
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
        if (raw_success)
        {
            allowance = 0;
            FVM_SWFStreamSoundBlock block;
            block.struct_size = sizeof(block);
            if (!(fvm_swf_stream_sound_snapshot_block_at(encoded, 0, &block) == FVM_OK &&
                  block.data_size == bits.bytes.size() &&
                  !std::memcmp(block.data, bits.bytes.begin(), bits.bytes.size())))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_swf_stream_sound_snapshot_close(encoded);
            allowance = -1;
        }
        return raw_success ? 0 : 77;
    }
    if (!(raw_success))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamSoundSnapshot *snapshot = 0;
    bool success = false;
    unsigned failed = 0;
    for (long budget = begin; budget < end && !success; ++budget)
    {
        allowance = budget;
        FVM_Result result = fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, (size_t)-1, &snapshot);
        allowance = -1;
        if (result == FVM_OK)
        {
            success = true;
        }
        else
        {
            if (!(result == FVM_OUT_OF_MEMORY && !snapshot))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ++failed;
        }
    }
    if (!isolated)
    {
        if (!(success && failed == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    allowance = 0;
    FVM_SWFStreamSoundBlock raw_block;
    raw_block.struct_size = sizeof(raw_block);
    if (!(fvm_swf_stream_sound_snapshot_block_at(encoded, 0, &raw_block) == FVM_OK &&
          raw_block.data_size == bits.bytes.size() &&
          !std::memcmp(raw_block.data, bits.bytes.begin(), bits.bytes.size())))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(encoded);
    allowance = -1;
    if (success)
    {
        allowance = 0;
        FVM_SWFStreamSoundStorageInfo storage;
        storage.struct_size = sizeof(storage);
        if (!(fvm_swf_stream_sound_snapshot_storage_info(snapshot, &storage) == FVM_OK &&
              storage.peak_bytes >= storage.retained_bytes))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }

        verify(snapshot, 1, 4);
        fvm_swf_stream_sound_snapshot_close(snapshot);
        allowance = -1;
    }
    return isolated && !success ? 77 : 0;
}
