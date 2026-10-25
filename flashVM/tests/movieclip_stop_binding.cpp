/* Same-frame StopSounds cancels prepared voices; failed clear preserves pending PCM until retry. */
#define FVM_MOVIECLIP_SOUND_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_sound.cpp"
#include "swf_sound_output_fixture.hpp"
#include "flashvm/audio_stop.h"


struct Control
{
    FVM_SWFSoundOutput *output;
    unsigned calls;


    Control() : output(0), calls(0)
    {
    }
};


static FVM_Result stop_output(void *raw)
{
    Control *control = (Control *)raw;
    ++control->calls;
    return fvm_swf_sound_output_stop(control->output);
}


static Bytes controlled_movie()
{
    Bytes tags, pcm, none, stop_action;
    u16(pcm, 7);
    pcm.push_back(12);
    u32(pcm, 2);
    pcm.push_back(0);
    pcm.push_back(255);
    tag(tags, 14, pcm);
    stop_action.push_back(9);
    stop_action.push_back(0);
    sound_event(tags, 0);
    tag(tags, 12, stop_action);
    tag(tags, 1, none);
    sound_event(tags, 0);
    tag(tags, 12, action(2));
    tag(tags, 1, none);
    tag(tags, 12, stop_action);
    tag(tags, 1, none);
    sound_event(tags, 0);
    tag(tags, 12, action(4));
    tag(tags, 1, none);
    tag(tags, 0, none);
    return movie(tags, 4);
}


static void run_control(FVM_ExecutionMode mode)
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
    Bytes swf = controlled_movie();
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
    FVM_AudioQueueBackend table = output_fixture::backend(device);
    Control control;
    if (!(fvm_swf_sound_output_create(delivery.mixer, &table, 4, 8, &control.output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_AudioStopBinding *binding = 0;
    if (!(fvm_audio_stop_binding_create(context, stop_output, &control, &binding) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t accepted = 0;
    if (!(fvm_swf_sound_output_pump(control.output, &accepted) == FVM_OK && device.queued == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && delivery.ordinals.size() == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(control.calls == 1 && !device.queued && !voices(delivery)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && voices(delivery) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.failure = 1;
    if (!(fvm_swf_sound_output_pump(control.output, &accepted) == FVM_ERROR && !accepted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(output_fixture::info(control.output).pending_frames == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.failure = 4;
    if (!(fvm_audio_stop_all(context) == FVM_ERROR && control.calls == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(output_fixture::info(control.output).pending_frames == 4 && device.paused))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.failure = 0;
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && control.calls == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!output_fixture::info(control.output).pending_frames && !device.queued))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && voices(delivery) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t before = device.values;
    if (!(fvm_swf_sound_output_pump(control.output, &accepted) == FVM_OK && accepted == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const short expected[] = {-32768, -32768, 27208, 27208, 32512, 32512, 0, 0};
    if (!(!std::memcmp(device.history + before, expected, sizeof(expected))))
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
    if (!(mode == FVM_EXECUTION_JIT ? stats.native_entries > 0
                                    : stats.interpreter_entries > 0 && !stats.native_entries))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_gc_unpin(context, clip);
    fvm_context_destroy(context); /* The still-bound control must stop before its host-owned mixer/device disappear. */
    if (!(control.calls == 4 && !device.queued && !output_fixture::info(control.output).submitted_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_output_close(control.output);
    fvm_swf_sound_mixer_close(delivery.mixer);
    fvm_runtime_destroy(runtime);
}


int main()
{
    run_control(FVM_EXECUTION_INTERPRETER);
    if (fvm_jit_available())
    {
        run_control(FVM_EXECUTION_JIT);
    }
    return 0;
}
