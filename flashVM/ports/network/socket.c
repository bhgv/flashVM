/* POSIX/Winsock nonblocking TCP with monotonic deadlines and no process signal changes. */
#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1 /* Expose SO_NOSIGPIPE on Darwin. */
#endif
#endif
#include "socket.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET FVM_NativeSocket;       /* Preserve pointer-sized Windows handles. */
#define FVM_NET_INVALID INVALID_SOCKET /* Winsock invalid-handle sentinel. */
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
typedef int FVM_NativeSocket; /* POSIX descriptors are signed integers. */
#define FVM_NET_INVALID (-1) /* POSIX invalid-descriptor sentinel. */
#endif
/* Every handle retains one total deadline across all partial I/O, not a fresh timeout per byte. */
struct FVM_NetSocket
{
    FVM_NativeSocket handle; /* Owned descriptor, invalid while trying the next address. */
    double start;            /* Monotonic milliseconds at request entry. */
    FVM_NetControl control;  /* Copied policy; userdata remains borrowed through close. */
    unsigned long total;     /* Caller-selected total I/O deadline interval. */
#ifdef _WIN32
    int initialized; /* This handle owns a WSAStartup reference. */
#endif
};
/* Monotonic millisecond counter. Windows subtraction below tolerates one 32-bit wrap. */
static int net_now(double *out)
{
#ifdef _WIN32
    *out = (double)GetTickCount();
    return 1;
#else
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value))
        return 0;
    *out =
        (double)value.tv_sec * FVM_NET_MILLISECONDS + (double)value.tv_nsec / FVM_NET_NANOSECONDS;
    return 1;
#endif
}
/* Compute remaining time, failing closed when the clock is unavailable. */
static double net_remaining(FVM_NetSocket *s, unsigned long budget)
{
    double now, elapsed;
    if ((s->control.cancelled && s->control.cancelled(s->control.userdata)) || !net_now(&now))
        return 0;
    elapsed = now - s->start;
#ifdef _WIN32
    if (elapsed < 0)
        elapsed += (double)ULONG_MAX + 1.0;
#endif
    if (elapsed < 0 || elapsed >= (double)budget)
        return 0;
    return (double)budget - elapsed;
}
/* Retry only interrupt/would-block results; all other errors are terminal. */
static int net_retry(void)
{
#ifdef _WIN32
    int e = WSAGetLastError();
    return e == WSAEINTR || e == WSAEWOULDBLOCK || e == WSAEINPROGRESS;
#else
    return errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS;
#endif
}
/* Readiness never implies successful connect: callers still inspect SO_ERROR. */
static int net_wait(FVM_NetSocket *s, int writing, unsigned long budget)
{
    fd_set read_set, write_set, error_set;
    struct timeval time;
    double remaining;
    int status;
#ifndef _WIN32
    if (s->handle < 0 || s->handle >= FD_SETSIZE)
        return 0;
#endif
    for (;;)
    {
        remaining = net_remaining(s, budget);
        if (remaining <= 0)
            return 0;
        if (s->control.cancelled && remaining > FVM_NET_POLL_MS)
            remaining = FVM_NET_POLL_MS;
        time.tv_sec = (long)(remaining / FVM_NET_MILLISECONDS);
        time.tv_usec =
            (long)((remaining - time.tv_sec * FVM_NET_MILLISECONDS) * FVM_NET_MICROSECONDS);
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_ZERO(&error_set);
        if (writing)
            FD_SET(s->handle, &write_set);
        else
            FD_SET(s->handle, &read_set);
        FD_SET(s->handle, &error_set);
#ifdef _WIN32
        status = select(0, &read_set, &write_set, &error_set, &time);
#else
        status = select(s->handle + 1, &read_set, &write_set, &error_set, &time);
#endif
        if (status > 0)
            return 1;
        if (status == 0)
            continue;
        if (!net_retry())
            return 0;
    }
}
/* Drop a descriptor without affecting the per-request resolver/WSA ownership. */
static void net_drop(FVM_NetSocket *s)
{
    if (s->handle == FVM_NET_INVALID)
        return;
#ifdef _WIN32
    closesocket(s->handle);
#else
    close(s->handle);
#endif
    s->handle = FVM_NET_INVALID;
}
/* Configure asynchronous I/O and suppress SIGPIPE locally, never via a global handler. */
static int net_configure(FVM_NetSocket *s)
{
#ifdef _WIN32
    u_long enabled = 1;
    return ioctlsocket(s->handle, FIONBIO, &enabled) == 0;
#else
    int flags = fcntl(s->handle, F_GETFL, 0);
#ifdef SO_NOSIGPIPE
    int enabled = 1;
    if (setsockopt(s->handle, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)))
        return 0;
#endif
    if (flags < 0 || fcntl(s->handle, F_SETFL, flags | O_NONBLOCK) < 0)
        return 0;
    flags = fcntl(s->handle, F_GETFD, 0);
    return flags >= 0 && fcntl(s->handle, F_SETFD, flags | FD_CLOEXEC) == 0;
#endif
}
/* Resolve each request independently; no global socket manager or cached host pointers. */
FVM_NetSocket *fvm_net_open_controlled(const char *host, const char *port, unsigned long connect_ms,
                                       unsigned long total_ms, const FVM_NetControl *control)
{
    FVM_NetSocket *s;
    struct addrinfo hints, *addresses = 0, *address;
    unsigned int count = 0;
    int status, error;
#ifdef _WIN32
    WSADATA data;
    int error_size;
#else
    socklen_t error_size;
#endif
    if (!host || !*host || !port || !*port || !connect_ms || !total_ms || connect_ms > total_ms ||
        total_ms > FVM_NET_MAX_MS)
        return 0;
    s = (FVM_NetSocket *)calloc(1, sizeof(*s));
    if (!s)
        return 0;
    s->handle = FVM_NET_INVALID;
    s->total = total_ms;
    if (control)
        s->control = *control;
    if (s->control.cancelled && s->control.cancelled(s->control.userdata))
    {
        free(s);
        return 0;
    }
#ifdef _WIN32
    if (WSAStartup(FVM_NET_WINSOCK_VERSION, &data))
    {
        free(s);
        return 0;
    }
    s->initialized = 1;
#endif
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (!net_now(&s->start) || getaddrinfo(host, port, &hints, &addresses))
        goto failed;
    for (address = addresses; address && count < FVM_NET_MAX_ADDRESSES;
         address = address->ai_next, ++count)
    {
        if (net_remaining(s, connect_ms) <= 0)
            break;
        s->handle = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (s->handle == FVM_NET_INVALID)
            continue;
        if (!net_configure(s))
        {
            net_drop(s);
            continue;
        }
        status = connect(s->handle, address->ai_addr, (int)address->ai_addrlen);
        if (status && (!net_retry() || !net_wait(s, 1, connect_ms)))
        {
            net_drop(s);
            continue;
        }
        error = 0;
        error_size = sizeof(error);
        if (getsockopt(s->handle, SOL_SOCKET, SO_ERROR, (char *)&error, &error_size) || error)
        {
            net_drop(s);
            continue;
        }
        freeaddrinfo(addresses);
        return s;
    }
failed:
    if (addresses)
        freeaddrinfo(addresses);
    fvm_net_close(s);
    return 0;
}
/* Preserve the original non-cancellable adapter ABI and its default behavior. */
FVM_NetSocket *fvm_net_open(const char *host, const char *port, unsigned long connect_ms,
                            unsigned long total_ms)
{
    return fvm_net_open_controlled(host, port, connect_ms, total_ms, 0);
}
/* Deliver only bytes actually received, with EOF distinct from socket failure. */
int fvm_net_read(FVM_NetSocket *s, void *bytes, size_t size, size_t *received)
{
    int count;
    if (!s || !bytes || !size || !received)
        return 0;
    if (size > INT_MAX)
        size = INT_MAX;
    for (;;)
    {
        if (!net_wait(s, 0, s->total))
            return 0;
        count = (int)recv(s->handle, (char *)bytes, (int)size, 0);
        if (count >= 0)
        {
            *received = (size_t)count;
            return 1;
        }
        if (!net_retry())
            return 0;
    }
}
/* A partial send is normal; the HTTP/TLS caller advances its own immutable input slice. */
int fvm_net_write(FVM_NetSocket *s, const void *bytes, size_t size, size_t *sent)
{
    int count, flags = 0;
    if (!s || !bytes || !size || !sent)
        return 0;
    if (size > INT_MAX)
        size = INT_MAX;
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif
    for (;;)
    {
        if (!net_wait(s, 1, s->total))
            return 0;
        count = (int)send(s->handle, (const char *)bytes, (int)size, flags);
        if (count > 0)
        {
            *sent = (size_t)count;
            return 1;
        }
        if (!count || !net_retry())
            return 0;
    }
}
/* Check a deadline without restarting it or touching socket readiness. */
int fvm_net_active(FVM_NetSocket *s)
{
    return s && s->handle != FVM_NET_INVALID && net_remaining(s, s->total) > 0;
}
/* Balance exactly this request's resources; never call process-wide socket cleanup otherwise. */
void fvm_net_close(FVM_NetSocket *s)
{
    if (!s)
        return;
    net_drop(s);
#ifdef _WIN32
    if (s->initialized)
        WSACleanup();
#endif
    free(s);
}
