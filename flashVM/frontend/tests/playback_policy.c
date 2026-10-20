/* Verify command transitions and validation without VM, SDL or filesystem startup. */
#include "../common/playback_internal.h"
#include <assert.h>
#include <string.h>
/* A callback marker is sufficient to validate an initially paused descriptor. */
static FVM_DesktopPlaybackCommand observe(const FVM_DesktopPlaybackState *state, void *data)
{
    (void)state;
    (void)data;
    return FVM_DESKTOP_PLAYBACK_KEEP;
}
int main(void)
{
    FVM_DesktopPlaybackControl control;
    FVM_DesktopPlaybackSession session, before;
    assert(!fvm_desktop_playback_control_init(0));
    assert(fvm_desktop_playback_control_init(&control));
    assert(control.struct_size == sizeof(control) && !control.initially_paused && !control.notify &&
           !control.userdata);
    assert(fvm_desktop_playback_session_init(&session, &control));
    assert(fvm_desktop_playback_can_tick(&session));
    assert(fvm_desktop_playback_apply(&session, FVM_DESKTOP_PLAYBACK_PAUSE) &&
           !fvm_desktop_playback_can_tick(&session));
    assert(fvm_desktop_playback_apply(&session, FVM_DESKTOP_PLAYBACK_STEP) &&
           fvm_desktop_playback_can_tick(&session));
    assert(fvm_desktop_playback_apply(&session, FVM_DESKTOP_PLAYBACK_STEP) &&
           session.step_pending == 1);
    fvm_desktop_playback_consume_step(&session);
    assert(session.paused && !fvm_desktop_playback_can_tick(&session));
    memcpy(&before, &session, sizeof(session)); /* Include padding in the rejection snapshot. */
    assert(!fvm_desktop_playback_apply(&session, (FVM_DesktopPlaybackCommand)-1));
    assert(!memcmp(&before, &session, sizeof(session)));
    assert(fvm_desktop_playback_apply(&session, FVM_DESKTOP_PLAYBACK_RESUME) &&
           fvm_desktop_playback_can_tick(&session));
    assert(fvm_desktop_playback_apply(&session, FVM_DESKTOP_PLAYBACK_STOP));
    assert(fvm_desktop_playback_apply(&session, FVM_DESKTOP_PLAYBACK_RESUME) &&
           !fvm_desktop_playback_can_tick(&session));
    memcpy(&before, &session, sizeof(session)); /* Include padding in the rejection snapshot. */
    --control.struct_size;
    assert(!fvm_desktop_playback_session_init(&session, &control) &&
           !memcmp(&before, &session, sizeof(session)));
    assert(fvm_desktop_playback_control_init(&control));
    control.initially_paused = 1;
    assert(!fvm_desktop_playback_session_init(&session, &control));
    control.notify = observe;
    assert(fvm_desktop_playback_session_init(&session, &control) && session.paused);
    control.initially_paused = 2;
    memcpy(&before, &session, sizeof(session)); /* Include padding in the rejection snapshot. */
    assert(!fvm_desktop_playback_session_init(&session, &control) &&
           !memcmp(&before, &session, sizeof(session)));
    return 0;
}
