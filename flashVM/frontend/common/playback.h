/* C89 owner-thread playback commands for embedding and IDE hosts; no SDL types in the ABI. */
#ifndef FVM_DESKTOP_PLAYBACK_H
#define FVM_DESKTOP_PLAYBACK_H /* Guard additive playback-control declarations. */
#include "source.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* One command per observation; commands do not directly execute VM instructions. */
    typedef enum FVM_DesktopPlaybackCommand
    {
        FVM_DESKTOP_PLAYBACK_KEEP = 0, /* Preserve current pause/step state. */
        FVM_DESKTOP_PLAYBACK_PAUSE =
            1, /* Freeze future host timeline ticks; keep owner services alive. */
        FVM_DESKTOP_PLAYBACK_RESUME =
            2, /* Resume at current time without catching up paused ticks. */
        FVM_DESKTOP_PLAYBACK_STEP = 3, /* Pause and permit exactly one future host timeline tick. */
        FVM_DESKTOP_PLAYBACK_STOP =
            4 /* End the complete source run successfully, including navigation. */
    } FVM_DesktopPlaybackCommand;
    /* Notifications are safe owner boundaries, never worker/audio-thread callbacks. */
    typedef enum FVM_DesktopPlaybackPhase
    {
        FVM_DESKTOP_PLAYBACK_READY =
            1, /* Imported/pinned root and window/audio exist, before first tick. */
        FVM_DESKTOP_PLAYBACK_TURN =
            2, /* Foreground owner turn before input/timers/resources and ticking. */
        FVM_DESKTOP_PLAYBACK_TICK =
            3 /* One successful timeline advance and presentation completed. */
    } FVM_DesktopPlaybackPhase;
    /* Stack snapshot and managed pointers are borrowed only during the notification. */
    typedef struct FVM_DesktopPlaybackState
    {
        unsigned long struct_size;      /* Exact size of this snapshot ABI. */
        FVM_DesktopPlaybackPhase phase; /* READY, TURN or TICK. */
        FVM_Context
            *context;     /* Borrowed owner context; never send to a worker or destroy it here. */
        FVM_Object *root; /* [GC:] Already pinned for this callback; host must not unpin it. */
        const char
            *source; /* Borrowed current root location; copy if needed after callback return. */
        unsigned long movie_ticks; /* Completed host ticks for the current imported root. */
        unsigned long total_ticks; /* Completed ticks across whole-root navigation in this run. */
        unsigned long
            presentations; /* Successful current-root draws, including paused refreshes. */
        int paused; /* Timeline paused; input/timers/network may still run and mutate the scene. */
        int step_pending; /* One step permit is waiting for an eligible foreground turn. */
    } FVM_DesktopPlaybackState;
    /* Read/query the live VM at this safe point. Return one command; unknown values fail the run.
     * No concurrent/reentrant runner, SDK shutdown, root replacement, VM destruction or C++ throw.
     * Do not retain borrowed pointers without the normal C API ownership/GC rules. */
    typedef FVM_DesktopPlaybackCommand (*FVM_DesktopPlaybackNotify)(
        const FVM_DesktopPlaybackState *, void *);
    /* Additive descriptor; existing source/options/loader layouts remain unchanged. */
    typedef struct FVM_DesktopPlaybackControl
    {
        unsigned long struct_size; /* Must equal sizeof this structure. */
        int initially_paused;      /* Zero or one; initial pause requires a callback that can
                                      resume/stop. */
        FVM_DesktopPlaybackNotify
            notify;     /* Optional borrowed owner callback until runner return. */
        void *userdata; /* Borrowed host state; never passed to workers or serialized. */
    } FVM_DesktopPlaybackControl;
    /* Initialize exact ABI size, running mode and null callback/userdata; no startup or allocation.
     */
    FVM_DESKTOP_API int fvm_desktop_playback_control_init(FVM_DesktopPlaybackControl *out);
#ifdef __cplusplus
}
#endif
#endif
