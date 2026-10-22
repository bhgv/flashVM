/* Compile production C89 paths directly; deterministic tests own serialization without a worker. */
#include "../common/audio.c"
/* Run the real native-only stream/event scheduling step. */
FVM_Result fvm_test_mixed_refill(FVM_DesktopAudio *audio)
{
    return refill_stream(audio);
}
/* Install real owner callbacks against a caller-prepared native FIFO fixture. */
FVM_Result fvm_test_mixed_install(FVM_Context *context, FVM_DesktopAudio *audio)
{
    FVM_Result result = fvm_movieclip_set_sound_callback(context, sound_event, audio);
    if (result == FVM_OK)
        result = fvm_audio_stop_binding_create(context, stop_context_audio, audio,
                                                &audio->stop_binding);
    return result;
}
