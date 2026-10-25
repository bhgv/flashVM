/* Native nonblocking TCP adapter, independent of SDL, VM state and HTTP parsing. */
#ifndef FVM_NET_SOCKET_H
#define FVM_NET_SOCKET_H
#define FVM_NET_POLL_MS                                                                                                \
    50UL                               /* Maximum readiness slice before checking cooperative cancellation.  \
                              */
#define FVM_NET_CONNECT_MS 10000UL     /* Default TCP connection budget. */
#define FVM_NET_TOTAL_MS 30000UL       /* Default connection and transfer budget. */
#define FVM_NET_MAX_MS 120000UL        /* Maximum caller-selected synchronous budget. */
#define FVM_NET_MAX_ADDRESSES 16U      /* Bound resolver-result connection attempts. */
#define FVM_NET_MILLISECONDS 1000      /* Milliseconds per second. */
#define FVM_NET_MICROSECONDS 1000      /* Microseconds per millisecond. */
#define FVM_NET_NANOSECONDS 1000000.0  /* Nanoseconds per millisecond. */
#define FVM_NET_WINSOCK_VERSION 0x0202 /* Require Winsock 2.2. */
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif
/* Borrowed callback state; cancellation callbacks must be thread-safe and must not
     * enter the VM, SDL UI, destroy socket state or throw across the C boundary. */
typedef struct FVM_NetControl
{
    int (*cancelled)(void *userdata); /* Nonzero requests cancellation; null disables it. */
    void *userdata;                   /* Borrowed until the controlled synchronous request returns. */
} FVM_NetControl;
/* Opaque native socket; owns one balanced Winsock reference on Windows. */
typedef struct FVM_NetSocket FVM_NetSocket;


/* Resolve synchronously, then connect nonblocking. DNS cannot be interrupted by this API.
     * Return null on any failure; no state is retained. Time budgets include elapsed DNS time. */
FVM_NetSocket *fvm_net_open(const char *host, const char *port, unsigned long connect_ms, unsigned long total_ms);


/* Same ownership as open, with copied cooperative cancellation policy. The callback
     * is checked around DNS, connect and at most every POLL_MS while waiting for I/O.
     * It cannot interrupt synchronous getaddrinfo or caller/library code. */
FVM_NetSocket *fvm_net_open_controlled(const char *host,
                                       const char *port,
                                       unsigned long connect_ms,
                                       unsigned long total_ms,
                                       const FVM_NetControl *control);


/* Read/write a partial chunk. Return 1 on success, 0 on error/timeout; read size 0 means EOF.
     * Positive requested size and nonnull arguments required. Never deliver VM callbacks here. */
int fvm_net_read(FVM_NetSocket *socket, void *bytes, size_t size, size_t *received);


int fvm_net_write(FVM_NetSocket *socket, const void *bytes, size_t size, size_t *sent);


/* Return 1 while the original total deadline remains; useful for buffered TLS operations. */
int fvm_net_active(FVM_NetSocket *socket);


/* Close the owned socket and release its platform state; null is harmless. */
void fvm_net_close(FVM_NetSocket *socket);
#ifdef __cplusplus
}
#endif
#endif
