/* Connect actual root StartSound controls to the optional real SDL playback queue. */
#include "audio.h"
#include "flashvm/movieclip.h"
#include <string.h>

/* Synchronous root frame-entry delivery occurs before that frame's DoAction. */
static FVM_Result sound_event(FVM_Context *context, FVM_Object *clip, size_t ordinal, void *raw)
{
    FVM_DesktopAudio *audio = (FVM_DesktopAudio *)raw;
    FVM_Result result;
    (void)clip;
    result = fvm_swf_sound_mixer_submit(audio->mixer, context, ordinal);
    if (result == FVM_OK)
        ++audio->events;
    return result;
}

/* Acquire device/mixer/producer before registering the callback; cleanup tolerates partial state.
 */
FVM_Result fvm_desktop_audio_open(FVM_Context *context, FVM_Object *clip, FVM_DesktopAudio *audio)
{
    FVM_AudioQueueBackend backend;
    FVM_SWFSoundMixerConfig geometry;
    FVM_Result result;
    size_t count = 0;
    (void)clip;
    memset(audio, 0, sizeof(*audio));
    result = fvm_swf_sound_event_count(context, &count);
    if (result != FVM_OK || !count)
        return result;
    result = fvm_audio_sdl_queue_open(FVM_DESKTOP_AUDIO_RATE, FVM_DESKTOP_AUDIO_CHANNELS,
                                      FVM_DESKTOP_AUDIO_BLOCK, &audio->device, &backend);
    if (result != FVM_OK)
        return result;
    geometry.struct_size = sizeof(geometry);
    geometry.sample_rate = FVM_DESKTOP_AUDIO_RATE;
    geometry.channels = FVM_DESKTOP_AUDIO_CHANNELS;
    geometry.voice_capacity = FVM_DESKTOP_AUDIO_VOICES;
    geometry.flags = FVM_SWF_SOUND_MIXER_CONVERT;
    result = fvm_swf_sound_mixer_create_configured(&geometry, &audio->mixer);
    if (result == FVM_OK)
        result = fvm_swf_sound_output_create(audio->mixer, &backend, FVM_DESKTOP_AUDIO_BLOCK,
                                             FVM_DESKTOP_AUDIO_TARGET, &audio->output);
    if (result == FVM_OK)
        result = fvm_movieclip_set_sound_callback(context, sound_event, audio);
    return result;
}

/* Do not derive video time from the software FIFO: hardware latency is not available here. */
FVM_Result fvm_desktop_audio_tick(FVM_DesktopAudio *audio)
{
    unsigned int attempt;
    FVM_Result result;
    if (!audio->output)
        return FVM_OK;
    for (attempt = 0; attempt < FVM_DESKTOP_AUDIO_PUMPS; ++attempt)
    {
        result = fvm_swf_sound_output_pump(audio->output, 0);
        if (result == FVM_NOT_FOUND)
            break;
        if (result != FVM_OK)
            return result;
    }
    return fvm_swf_sound_output_pause(audio->output, 0);
}

/* Callback userdata remains live until context destruction; this function never frees its owner. */
void fvm_desktop_audio_close(FVM_DesktopAudio *audio)
{
    if (audio->output)
        fvm_swf_sound_output_stop(audio->output);
    fvm_swf_sound_output_close(audio->output);
    fvm_swf_sound_mixer_close(audio->mixer);
    fvm_audio_sdl_queue_close(audio->device);
    memset(audio, 0, sizeof(*audio));
}
