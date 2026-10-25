/* FlashVM: Portable platform adapter interface or implementation: network.
 * File: ports/android/network.c.
 */

#define _POSIX_C_SOURCE 200112L
#include "../network_port.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/* Bionic's kernel networking headers spell inline as C99 even for a C89 caller. */
#if !defined(__cplusplus) && !defined(inline)
#define inline __inline__ /* Confine the compiler spelling to SDK headers, not project C89 code. */
#define FVM_ANDROID_NETWORK_INLINE_COMPAT
#endif
#include <netinet/in.h> /* Declare IPv4/IPv6 sockaddr layouts explicitly on Bionic. */
#include <arpa/inet.h>  /* Declare network byte-order conversion without transitive headers. */

#ifdef FVM_ANDROID_NETWORK_INLINE_COMPAT
#undef inline
#undef FVM_ANDROID_NETWORK_INLINE_COMPAT
#endif
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>


/* Define FVM_PortSocket struct and its representation. */
struct FVM_PortSocket
{
    int fd;
};

/* Socket connect is bounded independently of DNS resolution. Restore blocking
   mode before returning; callers set their own send/receive timeouts. */


/* Compute and return bounded connect. */
static int bounded_connect(int fd, const struct sockaddr *address, socklen_t length)
{

    /* Initialization / Инициализация локального состояния. */
    int flags, result, error;
    socklen_t error_size;
    fd_set writable;
    struct timeval timeout;
    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        return 0;
    }
    result = connect(fd, address, length);
    if (result && errno == EINPROGRESS)
    {
        if (fd >= FD_SETSIZE)
        {
            return 0;
        }
        FD_ZERO(&writable);
        FD_SET(fd, &writable);
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        result = select(fd + 1, NULL, &writable, NULL, &timeout);
        error = 0;
        error_size = sizeof(error);
        if (result <= 0 || getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &error_size) || error)
        {
            return 0;
        }
    }
    else if (result)
    {
        return 0;
    }
    return fcntl(fd, F_SETFL, flags) == 0;
}


/* Compute and return port tcp connect. */
FVM_PortSocket *fvm_port_tcp_connect(const char *hostname, unsigned short port)
{


    /* Initialization / Инициализация локального состояния. */
    struct addrinfo hints, *addresses, *it;
    char service[8];
    int fd = -1;
    FVM_PortSocket *result;
    if (!hostname || !port)
    {
        return NULL;
    }


    /* Computation / Вычисление и обновление состояния. */
    sprintf(service, "%u", (unsigned int)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(hostname, service, &hints, &addresses) != 0)
    {
        return NULL;
    }
    for (it = addresses; it; it = it->ai_next)
    {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0)
        {
            continue;
        }
        if (bounded_connect(fd, it->ai_addr, it->ai_addrlen))
        {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(addresses);
    if (fd < 0)
    {
        return NULL;
    }
    result = (FVM_PortSocket *)malloc(sizeof(*result));
    if (!result)
    {
        close(fd);
        return NULL;
    }
    result->fd = fd;
    return result;
}


/* Send port socket. */
long fvm_port_socket_send(FVM_PortSocket *s, const void *p, size_t n)
{
    ssize_t r;
    if (!s || (!p && n))
    {
        return -1;
    }
    do
    {
#ifdef MSG_NOSIGNAL
        r = send(s->fd, p, n, MSG_NOSIGNAL);
#else
        r = send(s->fd, p, n, 0);
#endif
    } while (r < 0 && errno == EINTR);
    return r < 0 ? -1 : (long)r;
}


/* Compute and return port socket recv. */
long fvm_port_socket_recv(FVM_PortSocket *s, void *p, size_t n)
{
    ssize_t r;
    if (!s || (!p && n))
    {
        return -1;
    }
    do
    {
        r = recv(s->fd, p, n, 0);
    } while (r < 0 && errno == EINTR);
    return r < 0 ? -1 : (long)r;
}


/* Close port socket. */
void fvm_port_socket_close(FVM_PortSocket *s)
{
    if (s)
    {
        close(s->fd);
        free(s);
    }
}


/* Compute and return port tcp listen. */
FVM_PortSocket *fvm_port_tcp_listen(const char *host, unsigned short port, unsigned short *actual)
{


    /* Initialization / Инициализация локального состояния. */
    struct addrinfo hints, *addresses, *it;
    struct sockaddr_storage bound;
    socklen_t length;
    FVM_PortSocket *result;
    char service[8];
    int fd, one;
    if (!host || !actual)
    {
        return NULL;
    }


    /* Computation / Вычисление и обновление состояния. */
    sprintf(service, "%u", (unsigned int)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(host, service, &hints, &addresses))
    {
        return NULL;
    }

    /* Computation / Вычисление и обновление состояния. */
    fd = -1;
    one = 1;
    for (it = addresses; it; it = it->ai_next)
    {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0)
        {
            continue;
        }
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        if (!bind(fd, it->ai_addr, it->ai_addrlen) && !listen(fd, 32))
        {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(addresses);
    if (fd < 0)
    {
        return NULL;
    }
    length = sizeof(bound);
    if (getsockname(fd, (struct sockaddr *)&bound, &length))
    {
        close(fd);
        return NULL;
    }

    /* Computation / Вычисление и обновление состояния. */
    *actual = ntohs(bound.ss_family == AF_INET ? ((struct sockaddr_in *)&bound)->sin_port
                                               : ((struct sockaddr_in6 *)&bound)->sin6_port);
    result = (FVM_PortSocket *)malloc(sizeof(*result));
    if (!result)
    {
        close(fd);
        return NULL;
    }
    result->fd = fd;
    return result;
}


/* Compute and return port tcp accept. */
FVM_PortSocket *fvm_port_tcp_accept(FVM_PortSocket *listener, unsigned long ms)
{

    /* Initialization / Инициализация локального состояния. */
    fd_set readset;
    struct timeval timeout;
    int fd;
    FVM_PortSocket *result;
    if (!listener || listener->fd >= FD_SETSIZE)
    {
        return NULL;
    }


    /* Computation / Вычисление и обновление состояния. */
    FD_ZERO(&readset);
    FD_SET(listener->fd, &readset);
    timeout.tv_sec = (long)(ms / 1000);
    timeout.tv_usec = (long)(ms % 1000) * 1000;
    if (select(listener->fd + 1, &readset, NULL, NULL, &timeout) <= 0)
    {
        return NULL;
    }
    fd = accept(listener->fd, NULL, NULL);
    if (fd < 0)
    {
        return NULL;
    }
    result = (FVM_PortSocket *)malloc(sizeof(*result));
    if (!result)
    {
        close(fd);
        return NULL;
    }
    result->fd = fd;
    return result;
}


/* Compute and return port socket timeout. */
int fvm_port_socket_timeout(FVM_PortSocket *s, unsigned long ms)
{
    struct timeval timeout;

    /* Computation / Вычисление и обновление состояния. */
    timeout.tv_sec = (long)(ms / 1000);
    timeout.tv_usec = (long)(ms % 1000) * 1000;
    return s && !setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) &&
           !setsockopt(s->fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}


/* Compute and return port secure random. */
int fvm_port_secure_random(unsigned char *bytes, size_t size)
{

    /* Initialization / Инициализация локального состояния. */
    FILE *file;
    size_t count;
    if (!bytes && size)
    {
        return 0;
    }
    file = fopen("/dev/urandom", "rb");
    if (!file)
    {
        return 0;
    }

    /* Computation / Вычисление и обновление состояния. */
    count = fread(bytes, 1, size, file);
    fclose(file);
    return count == size;
}
