/* C89 source descriptors and bounded synchronous transport for the shared player. */
#ifndef FVM_DESKTOP_SOURCE_H
#define FVM_DESKTOP_SOURCE_H
#include "options.h"
#define FVM_DESKTOP_SOURCE_MEMORY                                                                  \
    1U                             /* Borrow exact binary bytes from the embedding application.    \
                                    */
#define FVM_DESKTOP_SOURCE_FILE 2U /* Read a local path through the SDL UTF-8 adapter. */
#define FVM_DESKTOP_SOURCE_URL 3U  /* Fetch with an explicitly supplied host transport. */
#define FVM_DESKTOP_MAX_LOCATION 4096U      /* Maximum source/reference/canonical location bytes. */
#define FVM_DESKTOP_FETCH_INITIAL 4096U     /* First bounded accumulation allocation. */
#define FVM_DESKTOP_URL_ARGUMENTS 3U        /* URL, target, flags supplied by the VM service. */
#define FVM_DESKTOP_URL_URL 0U              /* Counted URL operand. */
#define FVM_DESKTOP_URL_TARGET 1U           /* Counted target operand. */
#define FVM_DESKTOP_URL_FLAGS 2U            /* Numeric AVM1 request policy. */
#define FVM_DESKTOP_URL_METHOD_MASK 3U      /* Low AVM1 method-selection bits. */
#define FVM_DESKTOP_LOAD_GLOBAL_MIN_ARGS 2U /* URL and target in the global form. */
#define FVM_DESKTOP_LOAD_GLOBAL_FULL_ARGS 3U    /* Global form with explicit method. */
#define FVM_DESKTOP_LOAD_METHOD_INDEX 1U        /* Receiver-form method selector argument. */
#define FVM_DESKTOP_LOAD_GLOBAL_METHOD_INDEX 2U /* Global-form method selector argument. */
#define FVM_DESKTOP_LOAD_TARGET_INDEX 1U        /* Global-form target argument. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* A single source, never a C string when kind is MEMORY. */
    typedef struct FVM_DesktopSource
    {
        unsigned int kind;    /* Exactly one SOURCE_* selector. */
        const void *bytes;    /* Borrowed MEMORY input; null for FILE/URL. */
        size_t size;          /* Exact MEMORY byte length; zero for FILE/URL. */
        const char *location; /* Borrowed name, local path or URL; nonempty and bounded. */
    } FVM_DesktopSource;
    /* Feed arbitrary binary chunks synchronously; stop immediately on non-OK status. */
    typedef FVM_Result (*FVM_DesktopWrite)(const void *bytes, size_t size, void *userdata);
    /* Host transport: resolve reference against optional base, fetch through write, then
     * write the effective absolute location to resolved[capacity], including its NUL.
     * No pointer is retained. Do not run VM/SDL, destroy host state or recurse playback.
     * Return actual transport status; partial data is discarded on any failure. */
    typedef FVM_Result (*FVM_DesktopFetch)(const char *reference, const char *base,
                                           FVM_DesktopWrite write, void *write_userdata,
                                           char *resolved, size_t capacity, void *userdata);
    /* Caller-owned policy; no VM-managed objects or hidden global registration. */
    typedef struct FVM_DesktopLoader
    {
        FVM_DesktopFetch fetch;  /* Optional transport; no URL fetch without one. */
        void *userdata;          /* Borrowed through synchronous run_source return. */
        unsigned long max_loads; /* 1 disables SWF requests; 2..MAX_LOADS bounds all resources. */
    } FVM_DesktopLoader;
    /* Set no transport and one allowed startup movie; null output returns zero. */
    FVM_DESKTOP_API int fvm_desktop_loader_init(FVM_DesktopLoader *out);
    /* Run a source with deferred MovieClip and loadVariables requests. Playlist is ignored.
     * Same UI-thread, scalar options, static error and 0/1/2 status rules as memory run.
     * Caller storage remains borrowed/immutable until return; no concurrent/reentrant runs.
     * GetURL2 LOAD_TARGET resolves clip paths; LOAD_VARIABLES assigns decoded form fields.
     * GET/no-method only; POST/browser navigation are unsupported. Child import uses the
     * strict current SWF subset. Native FIFO targets are pinned until safe-point delivery.
     * Memory defaults to the executable directory, URL to each loaded movie's canonical URL.
     * An absolute memory source location explicitly overrides platform path discovery. */
    FVM_DESKTOP_API int fvm_desktop_run_source(const FVM_DesktopOptions *options,
                                               const FVM_DesktopSource *source,
                                               const FVM_DesktopLoader *loader, const char **error);
#ifdef __cplusplus
}
#endif
#endif
