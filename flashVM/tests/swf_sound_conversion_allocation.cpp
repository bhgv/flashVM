/* Conversion config/voice OOM preserves old playback; read/info/stop/close allocate nothing. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
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
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Samples samples;
    samples.push_back(-32768);
    samples.push_back(30000);
    samples.push_back(-12345);
    Bytes swf = movie(samples, 0, 1, 3);
    if (!(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    failed = passed = false;
    for (long boundary = 0; boundary < 24 && !passed; ++boundary)
    {
        FVM_SWFSoundMixer *mixer = create(192000, 2);
        if (!(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        allowance = boundary;
        FVM_Result result = fvm_swf_sound_mixer_submit(mixer, context, 0);
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
        short output[514];
        size_t written = 0;
        allowance = 0;
        FVM_SWFSoundMixerConfig queried;
        queried.struct_size = sizeof(queried);
        if (!(fvm_swf_sound_mixer_get_config(mixer, &queried) == FVM_OK && queried.flags == config.flags))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mixer_read(mixer, 257, output, 514, &written) == FVM_OK && written == 514))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (size_t frame = 0; frame < 257; ++frame)
        {
            long expected = oracle(samples, 5512, 1, 192000, 2, frame, 0, 3);
            if (result == FVM_OK)
            {
                expected *= 2;
            }
            if (expected < -32768)
            {
                expected = -32768;
            }
            if (expected > 32767)
            {
                expected = 32767;
            }
            if (!(output[frame * 2] == expected && output[frame * 2 + 1] == expected))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_swf_sound_mixer_stop_all(mixer) == FVM_OK))
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
    FVM_SWFSoundMixer *mixer = create(192000, 2);
    if (!(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    allowance = 0;
    short output[1024];
    size_t written = 0;
    if (!(fvm_swf_sound_mixer_read(mixer, 512, output, 1024, &written) == FVM_OK && written == 1024 && !active(mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_stop_all(mixer) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mixer_close(mixer);
    allowance = -1;
    return 0;
}
