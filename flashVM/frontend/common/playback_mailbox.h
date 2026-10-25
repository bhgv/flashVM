/* C89 command handoff and copied playback observations for native IDE/controller threads. */
#ifndef FVM_DESKTOP_PLAYBACK_MAILBOX_H
#define FVM_DESKTOP_PLAYBACK_MAILBOX_H /* Guard additive mailbox declarations. */
#include "playback.h"

#define FVM_DESKTOP_PLAYBACK_MAILBOX_MAX_HANDLES 64 /* Bound owned native mutexes independently of transfer jobs. */
#ifdef __cplusplus
extern "C"
{
#endif
/* Opaque native storage; no VM object or SDL type crosses this boundary. */
typedef struct FVM_DesktopPlaybackMailbox FVM_DesktopPlaybackMailbox;


/* Caller-owned value copy; all fields remain usable after the mailbox/runner is released. */
typedef struct FVM_DesktopPlaybackSnapshot
{
    unsigned long struct_size;          /* Must equal sizeof this structure, initialized below. */
    FVM_DesktopPlaybackPhase phase;     /* Most recently observed owner boundary. */
    unsigned long movie_ticks;          /* Current-root completed host advances at that boundary. */
    unsigned long total_ticks;          /* Completed advances across root navigation. */
    unsigned long presentations;        /* Current-root completed draws, including paused redraws. */
    double elapsed_ms;                  /* Owner monotonic clock since first READY, excluding startup fetch. */
    int paused;                         /* Observed timeline state before the callback's returned command is applied. */
    int step_pending;                   /* Observed single-step permit, not a queue of requested advances. */
    FVM_DesktopPlaybackCommand pending; /* Current native command awaiting an owner observation. */
    int stop_requested;                 /* Terminal stop latch; neither KEEP nor RESUME clears it. */
    char source[FVM_DESKTOP_MAX_LOCATION + 1]; /* Copied NUL-terminated source, never a borrowed pointer. */
} FVM_DesktopPlaybackSnapshot;


/* Fill exact ABI size and zero fields; return 1/0 for nonnull/null, no allocation or startup.
     */
FVM_DESKTOP_API int fvm_desktop_playback_snapshot_init(FVM_DesktopPlaybackSnapshot *out);


/* Allocate one native mutex and bounded scalar/text storage, without starting a
     * VM/window/thread. out is required and unchanged on failure; OK, INVALID_ARGUMENT,
     * OUT_OF_MEMORY or ERROR (native mutex failure/quota). Do not race creation/release with
     * process SDK shutdown. */
FVM_DESKTOP_API FVM_Result fvm_desktop_playback_mailbox_create(FVM_DesktopPlaybackMailbox **out);


/* Release after the runner and every posting/polling thread have finished; null is OK.
     * No concurrent use/release, no automatic worker joining and no live VM pointers to unpin. */
FVM_DESKTOP_API FVM_Result fvm_desktop_playback_mailbox_release(FVM_DesktopPlaybackMailbox *mailbox);


/* Thread-safe latest-intent mailbox, not an instruction queue: PAUSE/RESUME/STEP replace
     * a pending nonterminal command; KEEP does nothing. Repeated STEP requests coalesce.
     * STOP overrides pending intent and remains terminal. Non-STOP commands after STOP return
     * ERROR without changing it. Invalid handle/command returns INVALID_ARGUMENT. No allocation. */
FVM_DESKTOP_API FVM_Result fvm_desktop_playback_mailbox_post(FVM_DesktopPlaybackMailbox *mailbox,
                                                             FVM_DesktopPlaybackCommand command);


/* Atomically copy the last observation plus current pending/stop state. Required exact-size
     * out is unchanged on INVALID_ARGUMENT, NOT_FOUND (no observation yet) or lock ERROR.
     * No VM access; safe on another thread, and safe to retain the resulting value afterwards. */
FVM_DESKTOP_API FVM_Result fvm_desktop_playback_mailbox_poll(FVM_DesktopPlaybackMailbox *mailbox,
                                                             FVM_DesktopPlaybackSnapshot *out);


/* Return the terminal latch; null or native lock failure returns 1 (fail closed).
     * Reading does not consume STOP. Safe on provider/controller threads; no VM calls. */
FVM_DESKTOP_API int fvm_desktop_playback_mailbox_stop_requested(FVM_DesktopPlaybackMailbox *mailbox);


/* Owner-thread adapter for control.notify; userdata is this mailbox. Copies bounded native
     * observations then returns latest intent. Never stores/dereferences context/root. Null or
     * malformed input fails closed with STOP. Bind one mailbox to one synchronous source run. */
FVM_DESKTOP_API FVM_DesktopPlaybackCommand fvm_desktop_playback_mailbox_notify(const FVM_DesktopPlaybackState *state,
                                                                               void *userdata);


/* Owner-only load-monitor adapter: return terminal cancellation without consuming STOP.
     * Use the same mailbox as control.notify to stop startup and blocking provider waits too.
     * PAUSE/RESUME/STEP stay pending until a playback notification; no VM work occurs here. */
FVM_DESKTOP_API int fvm_desktop_playback_mailbox_load_notify(const FVM_DesktopLoadEvent *event, void *userdata);


/* Live native handles protect SDK shutdown, independently of active transfer jobs. */
FVM_DESKTOP_API unsigned int fvm_desktop_playback_mailbox_active(void);
#ifdef __cplusplus
}
#endif
#endif
