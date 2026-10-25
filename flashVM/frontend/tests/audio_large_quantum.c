/* Exercise the production worker when the native SDK reports a larger block than requested. */
#define fvm_audio_sdl_queue_info fixture_queue_info
#include "../common/audio.c"

#undef fvm_audio_sdl_queue_info


/* Keep the real device while changing only its reported scheduling quantum for this fixture. */
FVM_Result fvm_audio_sdl_queue_info(const FVM_SDLAudioQueue *, FVM_SDLAudioQueueInfo *);


FVM_Result fixture_queue_info(const FVM_SDLAudioQueue *device, FVM_SDLAudioQueueInfo *info)
{
    FVM_Result result = fvm_audio_sdl_queue_info(device, info);
    if (result == FVM_OK)
    {
        info->device_buffer_frames = FVM_TEST_NATIVE_QUANTUM;
    }
    return result;
}
