/* Optional verified TLS adapter layered on the same deadline-aware native TCP connection. */
#ifndef FVM_NET_TLS_H
#define FVM_NET_TLS_H
#include "socket.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Opaque per-request OpenSSL objects; no process-wide context or trust-store mutation. */
    typedef struct FVM_NetTLS FVM_NetTLS;
    /* Borrow socket/host only during this call; returned TLS handle borrows socket until close.
     * ca_file null selects OpenSSL default trust paths; otherwise load only that PEM CA file.
     * Verify peer chain, DNS/IP identity, TLS >= 1.2 and DNS SNI; null means failure. */
    FVM_NetTLS *fvm_net_tls_open(FVM_NetSocket *socket, const char *host, int numeric,
                                 const char *ca_file);
    /* Counted partial binary I/O. Return 1 on success, 0 on TLS/native/deadline failure.
     * Only a verified TLS close_notify produces a successful zero-length read. */
    int fvm_net_tls_read(FVM_NetTLS *tls, void *bytes, size_t size, size_t *received);
    int fvm_net_tls_write(FVM_NetTLS *tls, const void *bytes, size_t size, size_t *sent);
    /* Release TLS objects without closing the borrowed TCP handle or global OpenSSL cleanup. */
    void fvm_net_tls_close(FVM_NetTLS *tls);
#ifdef __cplusplus
}
#endif
#endif
