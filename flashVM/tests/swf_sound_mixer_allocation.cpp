/* Inject preparation failures; existing voices and allocation-free PCM output remain usable. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
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
            if (!(!mixer))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(result == FVM_OK && mixer))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            passed = true;
            fvm_swf_sound_mixer_close(mixer);
        }
    }
    if (!(failed && passed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes swf = fixture(1, 3);
    if (!(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    failed = passed = false;
    for (long boundary = 0; boundary < 24 && !passed; ++boundary)
    {
        FVM_SWFSoundMixer *mixer = 0;
        if (!(fvm_swf_sound_mixer_create(44100, 1, 2, &mixer) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK && active(mixer) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        allowance = boundary;
        FVM_Result result = fvm_swf_sound_mixer_submit(mixer, context, 1);
        allowance = -1;
        if (result == FVM_OUT_OF_MEMORY)
        {
            failed = true;
            if (!(active(mixer) == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(result == FVM_OK && active(mixer) == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            passed = true;
        }
        short out[300];
        size_t written = 0;
        allowance = 0;
        if (!(fvm_swf_sound_mixer_read(mixer, 300, out, 300, &written) == FVM_OK && written == 300))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 300; ++i)
        {
            if (!(out[i] == (i < 3 ? result == FVM_OK ? 32767 : 30000 : 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(!active(mixer) && fvm_swf_sound_mixer_stop_all(mixer) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_swf_sound_mixer_close(mixer);
        allowance = -1;
    }
    if (!(failed && passed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixer *mixer = 0;
    if (!(fvm_swf_sound_mixer_create(44100, 1, 2, &mixer) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = 0;
    /* These synchronization controls require no prepared new voice, even under OOM. */
    if (!(fvm_swf_sound_mixer_submit(mixer, context, 3) == FVM_OK && active(mixer) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit(mixer, context, 4) == FVM_OK && !active(mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    if (!(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    runtime->config.memory_limit_bytes = 1;
    if (!(fvm_swf_sound_mixer_submit(mixer, context, 1) == FVM_OUT_OF_MEMORY && active(mixer) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    allowance = 0;
    if (!(fvm_swf_sound_mixer_stop_all(mixer) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mixer_close(mixer);
    allowance = -1;
    return 0;
}
