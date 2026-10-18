/* Interpreter/JIT root events feed a converted mixer, copied FIFO and dequeue-clock API. */
#define FVM_MOVIECLIP_SOUND_FIXTURE_ONLY
#include "movieclip_sound.cpp"
#include "swf_sound_output_fixture.hpp"
static void output_timeline(FVM_ExecutionMode mode)
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK &&
           fvm_standard_install(context) == FVM_OK);
    Bytes swf = sound_movie();
    assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
    FVM_Object *clip = 0;
    assert(fvm_movieclip_load_swf(context, swf.begin(), swf.size(), "root", &clip) == FVM_OK);
    assert(fvm_gc_pin(context, clip) == FVM_OK &&
           fvm_execution_set_mode(context, mode, 0) == FVM_OK);
    Delivery delivery;
    delivery.source = &swf;
    delivery.mixer = conversion_fixture::create(48000, 2);
    assert(fvm_movieclip_set_sound_callback(context, receive, &delivery) == FVM_OK);
    output_fixture::Device device;
    FVM_AudioQueueBackend backend = output_fixture::backend(device);
    FVM_SWFSoundOutput *output = 0;
    assert(fvm_swf_sound_output_create(delivery.mixer, &backend, 4, 8, &output) == FVM_OK);
    assert(fvm_movieclip_advance(context, clip, 1) == FVM_OK && delivery.ordinals.size() == 2);
    size_t accepted = 0;
    assert(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 4);
    const short expected[] = {-32768, -32768, 27208, 27208, 32512, 32512, 0, 0};
    assert(device.values == 8 && !std::memcmp(device.history, expected, sizeof(expected)));
    assert(fvm_swf_sound_output_pause(output, 0) == FVM_OK);
    output_fixture::consume(device, 4);
    assert(output_fixture::info(output).played_frames == 4);
    seek(context, clip, 3, true);
    assert(fvm_movieclip_advance(context, clip, 1) == FVM_OK && delivery.ordinals[2] == 3);
    assert(fvm_movieclip_advance(context, clip, 1) == FVM_OK && delivery.ordinals[3] == 4);
    FVM_ExecutionStats stats;
    stats.struct_size = sizeof(stats);
    assert(fvm_execution_get_stats(context, &stats) == FVM_OK);
    if (mode == FVM_EXECUTION_JIT)
        assert(stats.native_entries > 0);
    else
        assert(stats.interpreter_entries > 0 && !stats.native_entries);
    fvm_gc_unpin(context, clip);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    assert(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 4);
    assert(!std::memcmp(device.history + 8, expected, sizeof(expected)));
    assert(fvm_swf_sound_output_stop(output) == FVM_OK);
    fvm_swf_sound_output_close(output);
    fvm_swf_sound_mixer_close(delivery.mixer);
}
int main()
{
    output_timeline(FVM_EXECUTION_INTERPRETER);
    if (fvm_jit_available())
        output_timeline(FVM_EXECUTION_JIT);
    return 0;
}
