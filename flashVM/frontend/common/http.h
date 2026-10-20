/* Optional libcurl HTTP(S) provider; the core VM and default host build remain independent. */
#ifndef FVM_DESKTOP_HTTP_H
#define FVM_DESKTOP_HTTP_H
#include "source.h"
#include "../../ports/network/socket.h"
#include "network.h"
#include "transfer.h"
#define FVM_DESKTOP_HTTP_CONNECT_MS 10000L /* Bound connection setup. */
#define FVM_DESKTOP_HTTP_TOTAL_MS 30000L   /* Bound the synchronous transfer. */
#define FVM_DESKTOP_HTTP_SUCCESS_MIN 200L  /* First accepted response status. */
#define FVM_DESKTOP_HTTP_SUCCESS_END 300L  /* First non-success response status. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* FVM_DesktopFetch implementation, available with FVM_PLAYER_HTTP=ON only.
     * HTTP(S) GET, verified TLS, bounded time, no redirects. Resolve against base;
     * return canonical provenance, stream into the bounded sink and balance this
     * call's curl-global reference. userdata must be null. Existing host curl users
     * retain their own global references and coordinate initialization/thread rules. */
    FVM_Result fvm_desktop_http_fetch(const char *reference, const char *base,
                                      FVM_DesktopWrite write, void *write_userdata, char *resolved,
                                      size_t capacity, void *userdata);
    /* Linked libcurl protocol bits; no easy handle, connection or global-init reference. */
    unsigned int fvm_desktop_http_capabilities(void);
    /* Controlled counterpart: userdata is null or a borrowed FVM_NetControl pointer.
     * Cancellation is cooperative through libcurl progress and sink callbacks; resolver
     * implementations may still block. ERROR and unchanged provenance on cancellation. */
    FVM_Result fvm_desktop_http_fetch_controlled(const char *reference, const char *base,
                                                 FVM_DesktopWrite write, void *write_userdata,
                                                 char *resolved, size_t capacity, void *userdata);
    /* Worker-safe adapter for transfer_begin. Native userdata is optional socket options;
     * libcurl userdata must be null and requires a thread-safe libcurl global API. */
    FVM_Result fvm_desktop_http_transfer_fetch(const char *reference, const char *base,
                                               FVM_DesktopWrite write, void *write_userdata,
                                               char *resolved, size_t capacity,
                                               const FVM_NetControl *control, void *userdata);
    /* Report whether the linked SDK supports worker-safe HTTP global initialization. */
    int fvm_desktop_http_background_supported(void);
#ifdef __cplusplus
}
#endif
#endif
