/* C89 owner-thread loading observation and cooperative stop policy for embedded players. */
#ifndef FVM_DESKTOP_LOAD_MONITOR_H
#define FVM_DESKTOP_LOAD_MONITOR_H
#include "background.h"
#define FVM_DESKTOP_LOAD_ROOT 1U      /* Network startup or whole-root replacement. */
#define FVM_DESKTOP_LOAD_CHILD 2U     /* Network child MovieClip content. */
#define FVM_DESKTOP_LOAD_VARIABLES 3U /* Network form variables. */
#define FVM_DESKTOP_LOAD_BEGIN 1U     /* Before launching the worker; may stop without I/O. */
#define FVM_DESKTOP_LOAD_WAIT 2U      /* Owner heartbeat while awaiting immutable bytes. */
#define FVM_DESKTOP_LOAD_FETCHED 3U   /* Fetch succeeded; does not promise VM import success. */
#define FVM_DESKTOP_LOAD_FAILED 4U    /* Begin/provider/sink failure; no partial import. */
#define FVM_DESKTOP_LOAD_CANCELLED                                                                 \
    5U                                /* Resource abandoned; worker joined before runner returns.  \
                                       */
#define FVM_DESKTOP_LOAD_TIMED_OUT 6U /* Cooperative deadline won; no partial import. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Stack event and strings are borrowed only for the notify call. Contains no VM pointers. */
    typedef struct FVM_DesktopLoadEvent
    {
        unsigned long id;   /* One-based network request ordinal within this source run. */
        unsigned int kind;  /* LOAD_ROOT, LOAD_CHILD or LOAD_VARIABLES. */
        unsigned int phase; /* One LOAD_BEGIN/WAIT/FETCHED/FAILED/CANCELLED/TIMED_OUT value. */
        size_t
            received; /* Provider-delivered body bytes observed so far, not wire Content-Length. */
        FVM_Result result;     /* NOT_FOUND for BEGIN/WAIT; fetch result otherwise. */
        const char *reference; /* Copied request URL/reference, nonempty. */
        const char *base;      /* Optional copied request base; null if none. */
        const char *location;  /* Canonical location only during FETCHED; otherwise null. */
    } FVM_DesktopLoadEvent;
    /* Called on runner's owner thread outside VM callbacks, never by the worker. Return nonzero
     * at BEGIN/WAIT to stop the whole source run successfully. Terminal returns are ignored.
     * Do not enter VM, recurse a runner, destroy SDK/provider state or throw across C. Host-owned
     * progress UI is permitted; do not steal events belonging to the player's active SDL window. */
    typedef int (*FVM_DesktopLoadNotify)(const FVM_DesktopLoadEvent *event, void *userdata);
    /* Additive policy: existing playback/loader structs and old runners remain unchanged. */
    typedef struct FVM_DesktopLoadMonitor
    {
        unsigned int
            asynchronous; /* 0 waits at safe points, 1 keeps child/form timeline running. */
        unsigned long
            timeout_ms; /* Per-network-job total budget; zero disables, bounded as jobs. */
        FVM_DesktopLoadNotify notify; /* Optional borrowed owner callback through runner return. */
        void *userdata; /* Optional borrowed observer state, never passed to workers. */
    } FVM_DesktopLoadMonitor;
    /* Initialize async delivery, no deadline/callback; 1 success, 0 null, no startup. */
    FVM_DESKTOP_API int fvm_desktop_load_monitor_init(FVM_DesktopLoadMonitor *out);
    /* Synchronous whole-player call with owner observation, including startup before any window.
     * Required loader/monitor validated before any callback/worker/window. Local reads are not
     * observed. Exactly BEGIN then zero or more WAIT then one terminal notice per started request.
     * FETCHED means bytes validated by provider, not imported SWF/form. Stop at BEGIN/WAIT prevents
     * import, cancels/joins and returns 0 with null error. Failed/timeout remains playback error 1.
     * Close/frame bound may abandon a pending job (CANCELLED). All borrowed data lives until
     * return. DNS/noncooperative providers may delay join. No concurrent/reentrant player runs. */
    FVM_DESKTOP_API int fvm_desktop_run_source_monitored(const FVM_DesktopOptions *options,
                                                         const FVM_DesktopSource *source,
                                                         const FVM_DesktopBackgroundLoader *loader,
                                                         const FVM_DesktopLoadMonitor *monitor,
                                                         const char **error);
    /* Default-provider playlist adapter. IDs restart for each source; stop ends the playlist.
     * Without a compiled provider local playback remains available and URLs fail normally. */
    FVM_DESKTOP_API int fvm_desktop_run_monitored(const FVM_DesktopOptions *options,
                                                  const FVM_DesktopLoadMonitor *monitor,
                                                  const char **error);
#ifdef __cplusplus
}
#endif
#endif
