/* Fail each adapter acquisition; then forbid allocation across rejected and accepted native writes.
 */
#define main allocator_reference_main
#include "native_instance_allocation.cpp"
#undef main
#include "swf_sound_conversion_fixture.hpp"
#include "flashvm/swf_sound_mix_queue.h"
using namespace conversion_fixture;
static int fail_write = 1;
static FVM_Result write_native(void *, const short *pcm, size_t frames)
{
    for (size_t i = 0; i < frames; ++i)
        assert(pcm[i] == 100);
    return fail_write ? FVM_ERROR : FVM_OK;
}
static FVM_Result queued_native(void *, size_t *out)
{
    *out = 0;
    return FVM_OK;
}
static FVM_Result pause_native(void *, int)
{
    return FVM_OK;
}
static FVM_Result clear_native(void *)
{
    return FVM_OK;
}
int main(int argc, char **argv)
{
    long begin = 0, end = 4;
    bool isolated = argc == 3;
    if (isolated)
    {
        assert(!std::strcmp(argv[1], "--allocation-budget"));
        char *tail = 0;
        begin = std::strtol(argv[2], &tail, 10);
        assert(tail && !*tail && begin >= 0 && begin < 4);
        end = begin + 1;
    }
    else
        assert(argc == 1);
    FVM_Config config;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &rt) == FVM_OK &&
           fvm_context_create(rt, &c) == FVM_OK);
    Samples samples;
    for (size_t i = 0; i < 32; ++i)
        samples.push_back(100);
    Bytes swf = movie(samples, 3, 1);
    assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK);
    FVM_SWFSoundMixer *mixer = create(44100, 1);
    assert(fvm_swf_sound_mixer_submit(mixer, c, 0) == FVM_OK);
    FVM_AudioQueueBackend real;
    std::memset(&real, 0, sizeof(real));
    real.struct_size = sizeof(real);
    real.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;
    real.sample_rate = 44100;
    real.channels = 1;
    real.write = write_native;
    real.queued = queued_native;
    real.pause = pause_native;
    real.clear = clear_native;
    FVM_SWFSoundMixQueue *queue = 0;
    FVM_AudioQueueBackend table = real;
    for (long budget = begin; budget < end; ++budget)
    {
        allowance = budget;
        FVM_Result result = fvm_swf_sound_mix_queue_create(mixer, &real, 16, &queue, &table);
        allowance = -1;
        if (budget < 3)
            assert(result == FVM_OUT_OF_MEMORY && !queue &&
                   !std::memcmp(&real, &table, sizeof(real)));
        else
            assert(result == FVM_OK && queue);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    if (queue)
    {
        short base[16] = {0};
        allowance = 0;
        assert(table.write(table.userdata, base, 16) == FVM_ERROR);
        fail_write = 0;
        assert(table.write(table.userdata, base, 16) == FVM_OK);
        FVM_SWFSoundMixQueueInfo info;
        info.struct_size = sizeof(info);
        assert(fvm_swf_sound_mix_queue_info(queue, &info) == FVM_OK && !info.pending_frames);
        assert(table.clear(table.userdata) == FVM_OK);
        fvm_swf_sound_mix_queue_close(queue);
        fvm_swf_sound_mixer_close(mixer);
        allowance = -1;
    }
    else
        fvm_swf_sound_mixer_close(mixer);
    return isolated && !queue ? 77 : 0;
}
