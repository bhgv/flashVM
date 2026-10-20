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
    (void)c; (void)clip;
    audio->output = (FVM_SWFSoundOutput *)audio;
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
}
