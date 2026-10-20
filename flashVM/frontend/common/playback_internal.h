/* Private C89 command state shared by policy tests and the owner-loop integration. */
#ifndef FVM_DESKTOP_PLAYBACK_INTERNAL_H
#define FVM_DESKTOP_PLAYBACK_INTERNAL_H /* Guard private scheduling declarations. */
#include "playback.h"
#define FVM_DESKTOP_PLAYBACK_WAIT_MS 10U /* Bound owner command polling without busy spinning. */
/* No managed roots live here: each notify uses the player-owned pinned root. */
typedef struct FVM_DesktopPlaybackSession
{
    FVM_DesktopPlaybackControl policy; /* Shallow immutable copy; userdata remains borrowed. */
    double start_ms;           /* Monotonic epoch captured at the first observer callback. */
    int clock_started;         /* Distinguish a valid zero epoch from an uninitialized clock. */
    unsigned long total_ticks; /* Successful presented ticks across root replacement. */
    int paused;       /* Pause requested independently of the movie's own play/stop state. */
    int step_pending; /* At most one outstanding permit; repeated STEP commands coalesce. */
    int stopped;      /* Terminal disposition; subsequent commands cannot resurrect playback. */
} FVM_DesktopPlaybackSession;
/* Validate without startup; invalid input preserves the required session output. */
int fvm_desktop_playback_session_init(FVM_DesktopPlaybackSession *,
                                      const FVM_DesktopPlaybackControl *);
/* Apply one known command atomically; an unknown command leaves the state unchanged. */
int fvm_desktop_playback_apply(FVM_DesktopPlaybackSession *, FVM_DesktopPlaybackCommand);
/* Test whether exactly one ordinary host timeline advance is currently permitted. */
int fvm_desktop_playback_can_tick(const FVM_DesktopPlaybackSession *);
/* Consume a step permit immediately after a successful timeline advance. */
void fvm_desktop_playback_consume_step(FVM_DesktopPlaybackSession *);
#endif
