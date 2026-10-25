/* Producer preparation OOM, exclusive empty ownership and allocation-free pumping after VM
 * teardown. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "swf_sound_output_fixture.hpp"

using namespace output_fixture;


int main()
{
    FVM_SWFSoundMixer *mixer = conversion_fixture::create(48000, 2);
    Device device;
    FVM_AudioQueueBackend table = backend(device);
    FVM_SWFSoundOutput *output = 0;
    bool failed = false, passed = false;
    for (long boundary = 0; boundary < 12 && !passed; ++boundary)
    {
        output = (FVM_SWFSoundOutput *)1;
        allowance = boundary;
        FVM_Result result = fvm_swf_sound_output_create(mixer, &table, 4, 8, &output);
        allowance = -1;
        if (result == FVM_OUT_OF_MEMORY)
        {
            failed = true;
            if (!(!output))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(result == FVM_OK && output))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            passed = true;
            fvm_swf_sound_output_close(output);
        }
    }
    if (!(failed && passed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.queued = 1;
    if (!(fvm_swf_sound_output_create(mixer, &table, 4, 8, &output) == FVM_INVALID_ARGUMENT && !output &&
          device.queued == 1 && device.paused))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.queued = 0;
    for (unsigned scenario = 0; scenario < 8; ++scenario)
    {
        FVM_AudioQueueBackend invalid = table;
        if (scenario == 0)
        {
            invalid.struct_size = 0;
        }
        if (scenario == 1)
        {
            invalid.abi_version = 0;
        }
        if (scenario == 2)
        {
            invalid.sample_rate = 44100;
        }
        if (scenario == 3)
        {
            invalid.channels = 1;
        }
        if (scenario == 4)
        {
            invalid.write = 0;
        }
        if (scenario == 5)
        {
            invalid.pause = 0;
        }
        if (scenario == 6)
        {
            invalid.clear = 0;
        }
        if (scenario == 7)
        {
            invalid.queued = 0;
        }
        if (!(fvm_swf_sound_output_create(mixer, &invalid, 4, 8, &output) == FVM_INVALID_ARGUMENT && !output))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_swf_sound_output_create(mixer, &table, 0, 8, &output) == FVM_INVALID_ARGUMENT && !output))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_create(mixer, &table, 9, 8, &output) == FVM_INVALID_ARGUMENT && !output))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_create(mixer, &table, 4, FVM_SWF_SOUND_OUTPUT_MAX_QUEUE + 1, &output) ==
              FVM_INVALID_ARGUMENT &&
          !output))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (int thrown = 1; thrown <= 2; ++thrown)
    {
        device.thrown = thrown;
        if (!(fvm_swf_sound_output_create(mixer, &table, 4, 8, &output) ==
              (thrown == 1 ? FVM_OUT_OF_MEMORY : FVM_ERROR)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!output))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        device.thrown = 0;
    }
    if (!(fvm_swf_sound_output_create(mixer, &table, 4, 8, &output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.output = output;
    device.reenter = true;
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
    conversion_fixture::Samples samples;
    samples.push_back(-32768);
    samples.push_back(32512);
    conversion_fixture::Bytes swf = conversion_fixture::movie(samples, 3, 1);
    if (!(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    allowance = 0;
    size_t accepted = 0;
    if (!(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(info(output).queued_frames == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pause(output, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    consume(device, 8);
    if (!(info(output).played_frames == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop(output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_output_close(output);
    fvm_swf_sound_mixer_close(mixer);
    allowance = -1;
    return 0;
}
