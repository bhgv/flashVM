/* Protocol host: real player/VM/raster, deterministic clock and observed audio policy.
 * No native audio device, Activity or GPU acceptance is claimed. */
#define fvm_desktop_window_active capture_base_active
#include "update_capture.c"

#undef fvm_desktop_window_active
#include "../common/audio.h"


/* A bounded background interval falls entirely within the final-frame hold. */
int fvm_desktop_window_active(FVM_DesktopWindow *window)
{
    return window == &view && !(milliseconds >= 10 && milliseconds < 40);
}


/* Native output sentinel is never dereferenced; only host policy decisions are tested. */
FVM_Result fvm_desktop_audio_open(FVM_Context *c, FVM_Object *clip, FVM_DesktopAudio *audio)
{
    (void)c;
    (void)clip;
    audio->output = (FVM_SWFSoundOutput *)audio;
    audio->device = (FVM_SDLAudioQueue *)audio; /* Boolean sentinel; no SDK operation uses it. */
    return FVM_OK;
}


/* Any attempt to resume audio in the background makes the actual player fail. */
FVM_Result fvm_desktop_audio_tick(FVM_DesktopAudio *audio)
{
    (void)audio;
    fprintf(stderr, "TICK %.0f\n", milliseconds);
    return fvm_desktop_window_active(&view) ? FVM_OK : FVM_ERROR;
}


/* Require an explicit pause throughout suspension, not silent omission of all audio policy. */
FVM_Result fvm_desktop_audio_pause(FVM_DesktopAudio *audio, int paused)
{
    (void)audio;
    fprintf(stderr, "PAUSE %.0f\n", milliseconds);
    return paused && !fvm_desktop_window_active(&view) ? FVM_OK : FVM_ERROR;
}


/* Release only the fake marker; this fixture owns no mixer/thread/device. */
void fvm_desktop_audio_close(FVM_DesktopAudio *audio)
{
    audio->output = NULL;
    audio->device = NULL;
}


/* This legacy-runner fixture must never request an initial control pause. */
FVM_Result fvm_desktop_audio_open_paused(FVM_Context *c, FVM_Object *clip, FVM_DesktopAudio *audio, int paused)
{
    return paused ? FVM_ERROR : fvm_desktop_audio_open(c, clip, audio);
}


/* Preserve the fixture's explicit foreground/background assertions through the additive API. */
FVM_Result fvm_desktop_audio_tick_paused(FVM_DesktopAudio *audio, int paused)
{
    return paused ? FVM_ERROR : fvm_desktop_audio_tick(audio);
}


/* Unload is outside this tail-policy fixture; reject unexpected use rather than emulate a mixer. */
FVM_Result fvm_desktop_audio_clear(FVM_Context *c, FVM_DesktopAudio *audio)
{
    (void)c;
    (void)audio;
    return FVM_ERROR;
}


/* This presenter-only movie has no root stream; require ordinary core indexing. */
FVM_Result fvm_desktop_audio_validate(FVM_Context *context)
{
    size_t count = 0;
    FVM_Result result = fvm_swf_stream_sound_count(context, &count);
    return result != FVM_OK ? result : count ? FVM_UNSUPPORTED : FVM_OK;
}


/* No stream/VM ownership is kept by this fake native endpoint. */
FVM_Result fvm_desktop_audio_timeline(FVM_Context *context, FVM_Object *clip, FVM_DesktopAudio *audio)
{
    (void)context;
    (void)clip;
    (void)audio;
    return FVM_OK;
}


/* Ordinary tail-pause fixture has no queued stream work. */
FVM_Result fvm_desktop_audio_stream_pending(FVM_DesktopAudio *audio, int *pending)
{
    (void)audio;
    if (!pending)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *pending = 0;
    return FVM_OK;
}
