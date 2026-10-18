/* Inject preparation failures; existing voices and allocation-free PCM output remain usable. */
#define main allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define main mixer_fixture_main
#include "swf_sound_mixer.cpp"
#undef main

int main()
{
    bool failed = false, passed = false;
    for (long boundary = 0; boundary < 12 && !passed; ++boundary)
    {
        FVM_SWFSoundMixer *mixer = (FVM_SWFSoundMixer *)1;
        allowance = boundary;
        FVM_Result result = fvm_swf_sound_mixer_create(44100, 1, 2, &mixer);
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
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    Bytes swf = fixture(1, 3);
    assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
    failed = passed = false;
    for (long boundary = 0; boundary < 24 && !passed; ++boundary)
    {
        FVM_SWFSoundMixer *mixer = 0;
        assert(fvm_swf_sound_mixer_create(44100, 1, 2, &mixer) == FVM_OK);
        assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK && active(mixer) == 1);
        allowance = boundary;
        FVM_Result result = fvm_swf_sound_mixer_submit(mixer, context, 1);
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
        short out[300];
        size_t written = 0;
        allowance = 0;
        assert(fvm_swf_sound_mixer_read(mixer, 300, out, 300, &written) == FVM_OK &&
               written == 300);
        for (unsigned i = 0; i < 300; ++i)
            assert(out[i] == (i < 3 ? result == FVM_OK ? 32767 : 30000 : 0));
        assert(!active(mixer) && fvm_swf_sound_mixer_stop_all(mixer) == FVM_OK);
        fvm_swf_sound_mixer_close(mixer);
        allowance = -1;
    }
    assert(failed && passed);
    FVM_SWFSoundMixer *mixer = 0;
    assert(fvm_swf_sound_mixer_create(44100, 1, 2, &mixer) == FVM_OK);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    allowance = 0;
    /* These synchronization controls require no prepared new voice, even under OOM. */
    assert(fvm_swf_sound_mixer_submit(mixer, context, 3) == FVM_OK && active(mixer) == 1);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 4) == FVM_OK && !active(mixer));
    allowance = -1;
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    runtime->config.memory_limit_bytes = 1;
    assert(fvm_swf_sound_mixer_submit(mixer, context, 1) == FVM_OUT_OF_MEMORY &&
           active(mixer) == 1);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    allowance = 0;
    assert(fvm_swf_sound_mixer_stop_all(mixer) == FVM_OK);
    fvm_swf_sound_mixer_close(mixer);
    allowance = -1;
    return 0;
}
