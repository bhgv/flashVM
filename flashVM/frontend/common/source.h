/* C89 source descriptors and bounded synchronous transport for the shared player. */
#ifndef FVM_DESKTOP_SOURCE_H
#define FVM_DESKTOP_SOURCE_H
#include "options.h"
#define FVM_DESKTOP_SOURCE_MEMORY                                                                  \
    1U                             /* Borrow exact binary bytes from the embedding application.    \
                                    */
#define FVM_DESKTOP_SOURCE_FILE 2U /* Read a local path through the SDL UTF-8 adapter. */
#define FVM_DESKTOP_SOURCE_URL 3U  /* Fetch with an explicitly supplied host transport. */
#define FVM_DESKTOP_MAX_LOCATION 4096U  /* Maximum source/reference/canonical location bytes. */
#define FVM_DESKTOP_MAX_LOADS 1024UL    /* Bound sequential movie replacement, including startup. */
#define FVM_DESKTOP_FETCH_INITIAL 4096U /* First bounded accumulation allocation. */
#define FVM_DESKTOP_URL_ARGUMENTS 3U    /* URL, target, flags supplied by the VM service. */
#define FVM_DESKTOP_URL_URL 0U          /* Counted URL operand. */
#define FVM_DESKTOP_URL_TARGET 1U       /* Counted target operand. */
#define FVM_DESKTOP_URL_FLAGS 2U        /* Numeric AVM1 request policy. */
#define FVM_DESKTOP_URL_METHOD_MASK 3U  /* Low AVM1 method-selection bits. */
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
                                       FVM_DesktopWrite write, void *write_userdata, char *resolved,
                                       size_t capacity, void *userdata);
/* Caller-owned policy; no VM-managed objects or hidden global registration. */
typedef struct FVM_DesktopLoader
{
    FVM_DesktopFetch fetch;  /* Optional transport; no URL fetch without one. */
    void *userdata;          /* Borrowed through synchronous run_source return. */
    unsigned long max_loads; /* 1 disables SWF navigation; 2..MAX_LOADS bounds total movies. */
} FVM_DesktopLoader;
#ifdef __cplusplus
extern "C"
{
#endif
    /* Set no transport and one allowed startup movie; null output returns zero. */
    int fvm_desktop_loader_init(FVM_DesktopLoader *out);
    /* Run one source and optional deferred root replacements. Playlist fields are ignored.
     * Same UI-thread, scalar options, static error and 0/1/2 status contract as memory run.
     * Source/policy/paths/data remain borrowed and immutable until return. No parallel/reentrant
     * runs. Navigation supports GetURL2 LOAD_TARGET to _root/_level0, GET/no-method only;
     * child clips, browser navigation, POST and loadVariables are explicitly unsupported. */
    int fvm_desktop_run_source(const FVM_DesktopOptions *options, const FVM_DesktopSource *source,
                               const FVM_DesktopLoader *loader, const char **error);
#ifdef __cplusplus
}
#endif
#endif
