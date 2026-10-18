/* Conversion config/voice OOM preserves old playback; read/info/stop/close allocate nothing. */
#define main allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "swf_sound_conversion_fixture.hpp"
using namespace conversion_fixture;
int main()
{
    FVM_SWFSoundMixerConfig config = {sizeof(config), 192000, 2, 2, FVM_SWF_SOUND_MIXER_CONVERT};
    bool failed = false, passed = false;
    for (long boundary = 0; boundary < 16 && !passed; ++boundary)
    {
        FVM_SWFSoundMixer *mixer = (FVM_SWFSoundMixer *)1;
        allowance = boundary;
        FVM_Result result = fvm_swf_sound_mixer_create_configured(&config, &mixer);
        allowance = -1;
        if (result == FVM_OUT_OF_MEMORY)
        {
            failed = true;
            assert(!mixer);
        }
        else
        {
            assert(result == FVM_OK && mixer);
            passed = true;
            fvm_swf_sound_mixer_close(mixer);
        }
    }
    assert(failed && passed);
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    Samples samples;
    samples.push_back(-32768);
    samples.push_back(30000);
    samples.push_back(-12345);
    Bytes swf = movie(samples, 0, 1, 3);
    assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
    failed = passed = false;
    for (long boundary = 0; boundary < 24 && !passed; ++boundary)
    {
        FVM_SWFSoundMixer *mixer = create(192000, 2);
        assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
        allowance = boundary;
        FVM_Result result = fvm_swf_sound_mixer_submit(mixer, context, 0);
        allowance = -1;
        if (result == FVM_OUT_OF_MEMORY)
        {
            failed = true;
            assert(active(mixer) == 1);
        }
        else
        {
            assert(result == FVM_OK && active(mixer) == 2);
            passed = true;
        }
        short output[514];
        size_t written = 0;
        allowance = 0;
        FVM_SWFSoundMixerConfig queried;
        queried.struct_size = sizeof(queried);
        assert(fvm_swf_sound_mixer_get_config(mixer, &queried) == FVM_OK &&
               queried.flags == config.flags);
        assert(fvm_swf_sound_mixer_read(mixer, 257, output, 514, &written) == FVM_OK &&
               written == 514);
        for (size_t frame = 0; frame < 257; ++frame)
        {
            long expected = oracle(samples, 5512, 1, 192000, 2, frame, 0, 3);
            if (result == FVM_OK)
                expected *= 2;
            if (expected < -32768)
                expected = -32768;
            if (expected > 32767)
                expected = 32767;
            assert(output[frame * 2] == expected && output[frame * 2 + 1] == expected);
        }
        assert(fvm_swf_sound_mixer_stop_all(mixer) == FVM_OK);
        fvm_swf_sound_mixer_close(mixer);
        allowance = -1;
    }
    assert(failed && passed);
    FVM_SWFSoundMixer *mixer = create(192000, 2);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    allowance = 0;
    short output[1024];
    size_t written = 0;
    assert(fvm_swf_sound_mixer_read(mixer, 512, output, 1024, &written) == FVM_OK &&
           written == 1024 && !active(mixer));
    assert(fvm_swf_sound_mixer_stop_all(mixer) == FVM_OK);
    fvm_swf_sound_mixer_close(mixer);
    allowance = -1;
    return 0;
}
