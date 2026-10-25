/* Allocation-free playback command policy; native windows/VMs remain in the owner runner. */
#include "playback_internal.h"
#include <string.h>


/* The initialized descriptor is usable as a running no-op observer. */
int fvm_desktop_playback_control_init(FVM_DesktopPlaybackControl *out)
{
    if (!out)
    {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    out->struct_size = sizeof(*out);
    return 1;
}


/* A paused policy without a callback cannot make progress, so reject it before startup. */
int fvm_desktop_playback_session_init(FVM_DesktopPlaybackSession *out, const FVM_DesktopPlaybackControl *policy)
{
    if (!out || !policy || policy->struct_size != sizeof(*policy) ||
        (policy->initially_paused != 0 && policy->initially_paused != 1) ||
        (policy->initially_paused && !policy->notify))
    {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    out->policy = *policy;
    out->paused = policy->initially_paused;
    return 1;
}


/* A command never queues arbitrary work or changes the running movie's own playing flag. */
int fvm_desktop_playback_apply(FVM_DesktopPlaybackSession *session, FVM_DesktopPlaybackCommand command)
{
    if (!session || (unsigned int)command > (unsigned int)FVM_DESKTOP_PLAYBACK_STOP)
    {
        return 0;
    }
    if (session->stopped)
    {
        return 1;
    }
    switch (command)
    {
    case FVM_DESKTOP_PLAYBACK_KEEP:
        break;
    case FVM_DESKTOP_PLAYBACK_PAUSE:
        session->paused = 1;
        session->step_pending = 0;
        break;
    case FVM_DESKTOP_PLAYBACK_RESUME:
        session->paused = 0;
        session->step_pending = 0;
        break;
    case FVM_DESKTOP_PLAYBACK_STEP:
        session->paused = 1;
        session->step_pending = 1;
        break;
    case FVM_DESKTOP_PLAYBACK_STOP:
        session->stopped = 1;
        session->paused = 1;
        session->step_pending = 0;
        break;
    }
    return 1;
}


/* Null is the identity policy used by all preexisting player entrypoints. */
int fvm_desktop_playback_can_tick(const FVM_DesktopPlaybackSession *session)
{
    return !session || (!session->stopped && (!session->paused || session->step_pending));
}


/* Step completion does not resume continuous playback. */
void fvm_desktop_playback_consume_step(FVM_DesktopPlaybackSession *session)
{
    if (session)
    {
        session->step_pending = 0;
    }
}
