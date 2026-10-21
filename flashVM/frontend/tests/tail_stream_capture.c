/* Deterministic completion protocol through the real player loop, without an audio device. */
#define fvm_desktop_audio_stream_pending tail_base_pending
#define fvm_desktop_window_events tail_base_events
#include "tail_capture.c"
#undef fvm_desktop_audio_stream_pending
#undef fvm_desktop_window_events
#include <stdlib.h>
/* Simulate ready, stalled and failed producer observations after the final frame deadline. */
FVM_Result fvm_desktop_audio_stream_pending(FVM_DesktopAudio *audio, int *pending)
{
    const char *mode = getenv("FVM_TEST_TAIL_MODE");
    (void)audio;
    if (!mode || !pending) return FVM_INVALID_ARGUMENT;
    fprintf(stderr,"PENDING %.0f\n",milliseconds);
    if (!strcmp(mode,"error")) return FVM_ERROR;
    *pending = strcmp(mode,"success") || milliseconds < 180;
    return FVM_OK;
}
/* A native close during completion must not wait for either drain or timeout. */
int fvm_desktop_window_events(FVM_DesktopWindow *window,int timeout)
{
    const char *mode = getenv("FVM_TEST_TAIL_MODE");
    int result = tail_base_events(window,timeout);
    return mode && !strcmp(mode,"close") && milliseconds >= 130 ? 0 : result;
}
