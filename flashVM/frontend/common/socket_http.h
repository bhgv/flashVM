/* Optional native TCP HTTP provider: no libcurl, SDL initialization or VM callbacks. */
#ifndef FVM_SOCKET_HTTP_H
#define FVM_SOCKET_HTTP_H
#include "source.h"
#include "network.h"
#include "transfer.h"
#include "../../ports/network/socket.h"
#define FVM_SOCKET_BUFFER 8192U          /* Native read and body-forwarding buffer bytes. */
#define FVM_SOCKET_LINE 8192U            /* Maximum status/header/chunk line bytes. */
#define FVM_SOCKET_HEADERS 32768U        /* Aggregate informational/header/trailer byte budget. */
#define FVM_SOCKET_FIELDS 256U           /* Aggregate field count, including trailers. */
#define FVM_SOCKET_INFORMATIONAL 8U      /* Maximum interim responses before the final response. */
#define FVM_SOCKET_CHUNKS 65536UL        /* Bound tiny-chunk CPU overhead. */
#define FVM_SOCKET_BODY_MAX 67108864UL   /* Maximum decoded resource bytes, including forms. */
#define FVM_SOCKET_STATUS_MIN 200        /* First successful response status. */
#define FVM_SOCKET_STATUS_END 300        /* First unsuccessful final response status. */
#define FVM_SOCKET_STATUS_CONTINUE 100   /* First informational response status. */
#define FVM_SOCKET_STATUS_SWITCH 101     /* Protocol switching is unsupported. */
#define FVM_SOCKET_STATUS_EMPTY 204      /* A response without a payload. */
#define FVM_SOCKET_STATUS_RESET 205      /* A successful response with zero content. */
#define FVM_SOCKET_REQUEST_OVERHEAD 128U /* Fixed request text beyond target and Host. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Caller-owned per-fetch limits; no pointer is retained after the synchronous call. */
    typedef struct FVM_SocketHTTPOptions
    {
        unsigned long
            connect_ms;         /* TCP budget; DNS itself remains a synchronous OS resolver call. */
        unsigned long total_ms; /* One monotonic budget across connection, headers and body. */
        const char *ca_file;    /* Borrowed PEM CA file for HTTPS; null selects OpenSSL defaults. */
    } FVM_SocketHTTPOptions;
    /* Initialize defaults without starting sockets; null returns zero. */
    int fvm_desktop_socket_options_init(FVM_SocketHTTPOptions *out);
    /* FVM_DesktopFetch callback; userdata is null for defaults or points to options above.
     * HTTP GET only, no proxy/redirect/auth/cookies/content decoding. HTTPS is unsupported
     * without a TLS build. Resolved is published only on success; sink may see partial bytes
     * before a later error, so discard partial input on any non-OK result. A failing sink's
     * exact result is returned and no more bytes are delivered. Same synchronous host-thread
     * restrictions as FVM_DesktopFetch. No VM objects or global networking registration. */
    FVM_Result fvm_desktop_socket_fetch(const char *reference, const char *base,
                                        FVM_DesktopWrite write, void *write_userdata,
                                        char *resolved, size_t capacity, void *userdata);
    /* Native provider build bits, without connection, DNS, trust file access or TLS startup. */
    unsigned int fvm_desktop_socket_capabilities(void);
    /* Additive controlled policy keeps the original options structure ABI unchanged. */
    typedef struct FVM_SocketHTTPRequest
    {
        FVM_SocketHTTPOptions options; /* Initialized scalar deadlines and borrowed CA filename. */
        FVM_NetControl
            control; /* Copied cancellation callback; caller owns its thread-safe state. */
    } FVM_SocketHTTPRequest;
    /* Initialize the whole controlled request; null returns zero, no startup occurs. */
    int fvm_desktop_socket_request_init(FVM_SocketHTTPRequest *out);
    /* Same fetch/partial-data rules as socket_fetch. userdata is null for defaults or
     * points to FVM_SocketHTTPRequest. Cancellation returns ERROR and never publishes
     * provenance; DNS/CA/sink code itself cannot be forcibly interrupted. */
    FVM_Result fvm_desktop_socket_fetch_controlled(const char *reference, const char *base,
                                                   FVM_DesktopWrite write, void *write_userdata,
                                                   char *resolved, size_t capacity, void *userdata);
    /* Worker-safe adapter for transfer_begin. Native userdata is optional socket options;
     * libcurl userdata must be null and requires a thread-safe libcurl global API. */
    FVM_Result fvm_desktop_socket_transfer_fetch(const char *reference, const char *base,
                                                 FVM_DesktopWrite write, void *write_userdata,
                                                 char *resolved, size_t capacity,
                                                 const FVM_NetControl *control, void *userdata);
#ifdef __cplusplus
}
#endif
#endif
