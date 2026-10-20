/* Bounded native command mailbox: a mutex protects scalar/text copies, never a VM heap reference.
 */
#define SDL_MAIN_HANDLED
#include "playback_mailbox_internal.h"
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
/* Native handles prevent process SDK teardown while controller mutex storage is still owned. */
static SDL_atomic_t mailbox_handles;
/* One instance belongs to one run; other threads may post/poll while its owner notifies. */
struct FVM_DesktopPlaybackMailbox
{
    SDL_mutex *mutex; /* Owned native synchronization, released only after callers quiesce. */
    FVM_DesktopPlaybackSnapshot snapshot; /* Entirely native copy, including bounded source text. */
    FVM_DesktopPlaybackCommand
        pending;  /* Latest unconsumed intent, with terminal STOP precedence. */
    int observed; /* Distinguish no owner notification from a valid zero-tick READY observation. */
    int stopped;  /* Sticky terminal command; an instance is deliberately not reset for another run.
                   */
};
/* Initialize caller-owned output without any native or VM startup. */
int fvm_desktop_playback_snapshot_init(FVM_DesktopPlaybackSnapshot *out)
{
    if (!out)
        return 0;
    memset(out, 0, sizeof(*out));
    out->struct_size = sizeof(*out);
    return 1;
}
/* Reserve quota first, rolling it back on every allocation/native-mutex failure. */
FVM_Result fvm_desktop_playback_mailbox_create(FVM_DesktopPlaybackMailbox **out)
{
    FVM_DesktopPlaybackMailbox *mailbox;
    int handles;
    if (!out)
        return FVM_INVALID_ARGUMENT;
    do
    {
        handles = SDL_AtomicGet(&mailbox_handles);
        if (handles >= FVM_DESKTOP_PLAYBACK_MAILBOX_MAX_HANDLES)
            return FVM_ERROR;
    } while (!SDL_AtomicCAS(&mailbox_handles, handles, handles + 1));
    mailbox = (FVM_DesktopPlaybackMailbox *)calloc(1, sizeof(*mailbox));
    if (!mailbox)
    {
        SDL_AtomicAdd(&mailbox_handles, -1);
        return FVM_OUT_OF_MEMORY;
    }
    mailbox->mutex = SDL_CreateMutex();
    if (!mailbox->mutex)
    {
        free(mailbox);
        SDL_AtomicAdd(&mailbox_handles, -1);
        return FVM_ERROR;
    }
    fvm_desktop_playback_snapshot_init(&mailbox->snapshot);
    *out = mailbox;
    return FVM_OK;
}
/* Lifetime is explicit: the host joins its controller before disposing this opaque object. */
FVM_Result fvm_desktop_playback_mailbox_release(FVM_DesktopPlaybackMailbox *mailbox)
{
    if (!mailbox)
        return FVM_OK;
    SDL_DestroyMutex(mailbox->mutex);
    free(mailbox);
    SDL_AtomicAdd(&mailbox_handles, -1);
    return FVM_OK;
}
/* Publish intent without entering the VM or invoking any user callback under a lock. */
FVM_Result fvm_desktop_playback_mailbox_post(FVM_DesktopPlaybackMailbox *mailbox,
                                             FVM_DesktopPlaybackCommand command)
{
    FVM_Result result = FVM_OK;
    if (!mailbox || (unsigned int)command > (unsigned int)FVM_DESKTOP_PLAYBACK_STOP)
        return FVM_INVALID_ARGUMENT;
    if (SDL_LockMutex(mailbox->mutex) != 0)
        return FVM_ERROR;
    if (mailbox->stopped && command != FVM_DESKTOP_PLAYBACK_STOP)
        result = FVM_ERROR;
    else if (command == FVM_DESKTOP_PLAYBACK_STOP)
    {
        mailbox->stopped = 1;
        mailbox->pending = command;
    }
    else if (command != FVM_DESKTOP_PLAYBACK_KEEP)
        mailbox->pending = command;
    SDL_UnlockMutex(mailbox->mutex);
    return result;
}
/* Copy one consistent observation, overlaying current native intent rather than stale intent. */
FVM_Result fvm_desktop_playback_mailbox_poll(FVM_DesktopPlaybackMailbox *mailbox,
                                             FVM_DesktopPlaybackSnapshot *out)
{
    FVM_Result result = FVM_OK;
    if (!mailbox || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    if (SDL_LockMutex(mailbox->mutex) != 0)
        return FVM_ERROR;
    if (!mailbox->observed)
        result = FVM_NOT_FOUND;
    else
    {
        *out = mailbox->snapshot;
        out->pending = mailbox->pending;
        out->stop_requested = mailbox->stopped;
    }
    SDL_UnlockMutex(mailbox->mutex);
    return result;
}
/* Stop remains readable during startup and blocking fetch waits, before any root snapshot exists.
 */
int fvm_desktop_playback_mailbox_stop_requested(FVM_DesktopPlaybackMailbox *mailbox)
{
    int stopped;
    if (!mailbox || SDL_LockMutex(mailbox->mutex) != 0)
        return 1;
    stopped = mailbox->stopped;
    SDL_UnlockMutex(mailbox->mutex);
    return stopped;
}
/* No SDK initialization is needed to inspect the native handle count. */
unsigned int fvm_desktop_playback_mailbox_active(void)
{
    return (unsigned int)SDL_AtomicGet(&mailbox_handles);
}
/* Validate and bound the borrowed text before locking; only scalars/text survive this function. */
FVM_DesktopPlaybackCommand
fvm_desktop_playback_mailbox_publish(FVM_DesktopPlaybackMailbox *mailbox,
                                     const FVM_DesktopPlaybackState *state)
{
    size_t length;
    FVM_DesktopPlaybackCommand command;
    if (!mailbox || !state || state->struct_size != sizeof(*state) || !state->source ||
        state->phase < FVM_DESKTOP_PLAYBACK_READY || state->phase > FVM_DESKTOP_PLAYBACK_TICK)
        return FVM_DESKTOP_PLAYBACK_STOP;
    for (length = 0; length <= FVM_DESKTOP_MAX_LOCATION && state->source[length]; ++length)
    {
        /* Do not call unbounded strlen on a borrowed source supplied by a custom owner. */
    }
    if (!length || length > FVM_DESKTOP_MAX_LOCATION || SDL_LockMutex(mailbox->mutex) != 0)
        return FVM_DESKTOP_PLAYBACK_STOP;
    mailbox->snapshot.phase = state->phase;
    mailbox->snapshot.movie_ticks = state->movie_ticks;
    mailbox->snapshot.total_ticks = state->total_ticks;
    mailbox->snapshot.presentations = state->presentations;
    mailbox->snapshot.elapsed_ms = state->elapsed_ms;
    mailbox->snapshot.paused = state->paused;
    mailbox->snapshot.step_pending = state->step_pending;
    memset(mailbox->snapshot.source, 0, sizeof(mailbox->snapshot.source));
    memcpy(mailbox->snapshot.source, state->source, length);
    mailbox->observed = 1;
    command = mailbox->pending;
    if (!mailbox->stopped)
        mailbox->pending = FVM_DESKTOP_PLAYBACK_KEEP;
    SDL_UnlockMutex(mailbox->mutex);
    return command;
}

/* Publish only on the native owner; controller threads see the independent copy through poll. */
FVM_DesktopPlaybackCommand
fvm_desktop_playback_mailbox_notify(const FVM_DesktopPlaybackState *state, void *userdata)
{
    return fvm_desktop_playback_mailbox_publish((FVM_DesktopPlaybackMailbox *)userdata, state);
}
/* Cancellation is available even before READY and during a cooperative blocking fetch wait. */
int fvm_desktop_playback_mailbox_load_notify(const FVM_DesktopLoadEvent *event, void *userdata)
{
    if (!event)
        return 1;
    return fvm_desktop_playback_mailbox_stop_requested((FVM_DesktopPlaybackMailbox *)userdata);
}
