/* Actual interpreter/JIT timeline delivery prepares mono source into a 48 kHz stereo host mixer. */
#define FVM_MOVIECLIP_SOUND_FIXTURE_ONLY
#include "movieclip_sound.cpp"
static void converted_timeline(FVM_ExecutionMode mode)
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
    Delivery state;
    state.source = &swf;
    FVM_SWFSoundMixerConfig output = {sizeof(output), 48000, 2, 4, FVM_SWF_SOUND_MIXER_CONVERT};
    assert(fvm_swf_sound_mixer_create_configured(&output, &state.mixer) == FVM_OK);
    assert(fvm_movieclip_set_sound_callback(context, receive, &state) == FVM_OK);
    assert(fvm_movieclip_advance(context, clip, 1) == FVM_OK && state.ordinals.size() == 2);
    short pcm[8];
    size_t written = 0;
    assert(fvm_swf_sound_mixer_read(state.mixer, 4, pcm, 8, &written) == FVM_OK && written == 8);
    const short expected[] = {-32768, -32768, 27208, 27208, 32512, 32512, 0, 0};
    assert(!std::memcmp(pcm, expected, sizeof(expected)) && !voices(state));
    seek(context, clip, 3, true);
    assert(fvm_movieclip_advance(context, clip, 1) == FVM_OK && state.ordinals[2] == 3);
    assert(fvm_movieclip_advance(context, clip, 1) == FVM_OK && state.ordinals[3] == 4);
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
    assert(fvm_swf_sound_mixer_read(state.mixer, 4, pcm, 8, &written) == FVM_OK &&
           !std::memcmp(pcm, expected, sizeof(expected)));
    fvm_swf_sound_mixer_close(state.mixer);
}
int main()
{
    converted_timeline(FVM_EXECUTION_INTERPRETER);
    if (fvm_jit_available())
        converted_timeline(FVM_EXECUTION_JIT);
    return 0;
}
