/* Interpreter/JIT root events feed a converted mixer, copied FIFO and dequeue-clock API. */
#define FVM_MOVIECLIP_SOUND_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_sound.cpp"
#include "swf_sound_output_fixture.hpp"


static void output_timeline(FVM_ExecutionMode mode)
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK && fvm_standard_install(context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes swf = sound_movie();
    if (!(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *clip = 0;
    if (!(fvm_movieclip_load_swf(context, swf.begin(), swf.size(), "root", &clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(context, clip) == FVM_OK && fvm_execution_set_mode(context, mode, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Delivery delivery;
    delivery.source = &swf;
    delivery.mixer = conversion_fixture::create(48000, 2);
    if (!(fvm_movieclip_set_sound_callback(context, receive, &delivery) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    output_fixture::Device device;
    FVM_AudioQueueBackend backend = output_fixture::backend(device);
    FVM_SWFSoundOutput *output = 0;
    if (!(fvm_swf_sound_output_create(delivery.mixer, &backend, 4, 8, &output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && delivery.ordinals.size() == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t accepted = 0;
    if (!(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const short expected[] = {-32768, -32768, 27208, 27208, 32512, 32512, 0, 0};
    if (!(device.values == 8 && !std::memcmp(device.history, expected, sizeof(expected))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pause(output, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    output_fixture::consume(device, 4);
    if (!(output_fixture::info(output).played_frames == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    seek(context, clip, 3, true);
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && delivery.ordinals[2] == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && delivery.ordinals[3] == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_ExecutionStats stats;
    stats.struct_size = sizeof(stats);
    if (!(fvm_execution_get_stats(context, &stats) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (mode == FVM_EXECUTION_JIT)
    {
        if (!(stats.native_entries > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(stats.interpreter_entries > 0 && !stats.native_entries))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_gc_unpin(context, clip);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    if (!(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::memcmp(device.history + 8, expected, sizeof(expected))))
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
    fvm_swf_sound_mixer_close(delivery.mixer);
}


int main()
{
    output_timeline(FVM_EXECUTION_INTERPRETER);
    if (fvm_jit_available())
    {
        output_timeline(FVM_EXECUTION_JIT);
    }
    return 0;
}
