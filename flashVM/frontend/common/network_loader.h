/* Explicit default-provider HTTP policy for source/playlist playback, independent of CLI parsing.
 */
#ifndef FVM_DESKTOP_NETWORK_LOADER_H
#define FVM_DESKTOP_NETWORK_LOADER_H
#include "http_policy.h"
#include "load_monitor.h"

#ifdef __cplusplus
extern "C"
{
#endif


/* Host-level initializer is available even without a network provider; 1 success, 0 null.
     * Defaults: no redirects, native default connect/total budgets and null trust filename. */
FVM_DESKTOP_API int fvm_desktop_network_policy_init(FVM_DesktopHTTPPolicy *out);


/* Select compiled native/curl policy adapter explicitly. Required policy/monitor validated
     * before callbacks/I/O/VM/window startup. Policy, ca_file and observer userdata remain
     * immutable/borrowed until synchronous return. The same policy applies to startup, root
     * replacement, child/form loads. Final canonical locations become per-movie bases. max_loads
     * counts logical resources, while each resource's max_redirects and total_ms separately bound
     * its HTTP hops. No provider: local files/ memory still work, URL input fails normally. Monitor
     * stop ends successfully; failure stays 1. Old synchronous/background/async runners remain
     * unchanged; no concurrent/reentrant runs. */
FVM_DESKTOP_API int fvm_desktop_run_source_network(const FVM_DesktopOptions *options,
                                                   const FVM_DesktopSource *source,
                                                   const FVM_DesktopHTTPPolicy *policy,
                                                   const FVM_DesktopLoadMonitor *monitor,
                                                   const char **error);


/* Playlist counterpart; IDs restart for each source, host stop ends the whole playlist. */
FVM_DESKTOP_API int fvm_desktop_run_network(const FVM_DesktopOptions *options,
                                            const FVM_DesktopHTTPPolicy *policy,
                                            const FVM_DesktopLoadMonitor *monitor,
                                            const char **error);
#ifdef __cplusplus
}
#endif
#endif
