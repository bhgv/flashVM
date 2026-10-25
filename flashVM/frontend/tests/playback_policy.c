/* Verify command transitions and validation without VM, SDL or filesystem startup. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/playback_internal.h"
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
    if (!(!fvm_desktop_playback_control_init(0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_playback_control_init(&control)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(control.struct_size == sizeof(control) && !control.initially_paused && !control.notify && !control.userdata))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_playback_session_init(&session, &control)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_playback_can_tick(&session)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_playback_apply(&session, FVM_DESKTOP_PLAYBACK_PAUSE) && !fvm_desktop_playback_can_tick(&session)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_playback_apply(&session, FVM_DESKTOP_PLAYBACK_STEP) && fvm_desktop_playback_can_tick(&session)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_playback_apply(&session, FVM_DESKTOP_PLAYBACK_STEP) && session.step_pending == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_desktop_playback_consume_step(&session);
    if (!(session.paused && !fvm_desktop_playback_can_tick(&session)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memcpy(&before, &session, sizeof(session)); /* Include padding in the rejection snapshot. */
    if (!(!fvm_desktop_playback_apply(&session, (FVM_DesktopPlaybackCommand)-1)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(&before, &session, sizeof(session))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_playback_apply(&session, FVM_DESKTOP_PLAYBACK_RESUME) && fvm_desktop_playback_can_tick(&session)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_playback_apply(&session, FVM_DESKTOP_PLAYBACK_STOP)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_playback_apply(&session, FVM_DESKTOP_PLAYBACK_RESUME) &&
          !fvm_desktop_playback_can_tick(&session)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memcpy(&before, &session, sizeof(session)); /* Include padding in the rejection snapshot. */
    --control.struct_size;
    if (!(!fvm_desktop_playback_session_init(&session, &control) && !memcmp(&before, &session, sizeof(session))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_playback_control_init(&control)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    control.initially_paused = 1;
    if (!(!fvm_desktop_playback_session_init(&session, &control)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    control.notify = observe;
    if (!(fvm_desktop_playback_session_init(&session, &control) && session.paused))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    control.initially_paused = 2;
    memcpy(&before, &session, sizeof(session)); /* Include padding in the rejection snapshot. */
    if (!(!fvm_desktop_playback_session_init(&session, &control) && !memcmp(&before, &session, sizeof(session))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
