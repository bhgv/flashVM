/* Fail every ADPCM preparation allocation; normalized reads need no allocator after VM teardown. */
#define main allocation_reference_main
#include "native_instance_allocation.cpp"
#undef main
#define main adpcm_reference_main
#include "swf_stream_adpcm.cpp"
#undef main
int main(int argc, char **argv)
{
    long begin = 0, end = 32;
    bool isolated = argc == 3;
    if (isolated)
    {
        assert(!std::strcmp(argv[1], "--allocation-budget"));
        char *tail = 0;
        begin = std::strtol(argv[2], &tail, 10);
        assert(tail && !*tail && begin >= 0 && begin < 32);
        end = begin + 1;
    }
    else
        assert(argc == 1);
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
           fvm_context_create(rt, &c) == FVM_OK);
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
            bits.put(0, 4);
    }
    Bytes swf = swf_block(bits.bytes, 1);
    assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK);
    FVM_SWFStreamSoundSnapshot *snapshot = 0;
    bool success = false;
    unsigned failed = 0;
    for (long budget = begin; budget < end && !success; ++budget)
    {
        allowance = budget;
        FVM_Result result =
            fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, (size_t)-1, &snapshot);
        allowance = -1;
        if (result == FVM_OK)
            success = true;
        else
        {
            assert(result == FVM_OUT_OF_MEMORY && !snapshot);
            ++failed;
        }
    }
    if (!isolated)
        assert(success && failed == 4);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    if (success)
    {
        allowance = 0;
        FVM_SWFStreamSoundStorageInfo storage;
        storage.struct_size = sizeof(storage);
        assert(fvm_swf_stream_sound_snapshot_storage_info(snapshot, &storage) == FVM_OK &&
               storage.peak_bytes >= storage.retained_bytes);

        verify(snapshot, 1, 4);
        fvm_swf_stream_sound_snapshot_close(snapshot);
        allowance = -1;
    }
    return isolated && !success ? 77 : 0;
}
