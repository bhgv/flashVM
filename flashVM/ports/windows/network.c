/* FlashVM: Portable platform adapter interface or implementation: network.
 * File: ports/windows/network.c.
 */

#include "../network_port.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* Define FVM_PortSocket struct and its representation. */
struct FVM_PortSocket
{
    SOCKET fd;
};

/* Compute and return wsa ready. */
static int fvm_wsa_ready(void)
{
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

/* Compute and return bounded connect. */
static int bounded_connect(SOCKET fd, const struct sockaddr *address, int length)
{

    /* Initialization / Инициализация локального состояния. */
    u_long mode;
    int result, error, error_size;
    fd_set writable;
    struct timeval timeout;
    mode = 1;
    if (ioctlsocket(fd, FIONBIO, &mode))
        return 0;
    result = connect(fd, address, length);
    if (result && WSAGetLastError() == WSAEWOULDBLOCK)
    {
        FD_ZERO(&writable);
        FD_SET(fd, &writable);
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        result = select(0, NULL, &writable, NULL, &timeout);
        error = 0;
        error_size = sizeof(error);
        if (result <= 0 || getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&error, &error_size) ||
            error)
            return 0;
    }
    else if (result)
        return 0;
    mode = 0;
    return ioctlsocket(fd, FIONBIO, &mode) == 0;
}

/* Compute and return port tcp connect. */
FVM_PortSocket *fvm_port_tcp_connect(const char *hostname, unsigned short port)
{

    /* Initialization / Инициализация локального состояния. */
    struct addrinfo hints, *addresses, *it;
    char service[8];
    SOCKET fd = INVALID_SOCKET;
    FVM_PortSocket *result;
    if (!hostname || !port || !fvm_wsa_ready())
        return NULL;

    /* Computation / Вычисление и обновление состояния. */
    sprintf(service, "%u", (unsigned int)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(hostname, service, &hints, &addresses))
    {
        WSACleanup();
        return NULL;
    }
    for (it = addresses; it; it = it->ai_next)
    {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd == INVALID_SOCKET)
            continue;
        if (bounded_connect(fd, it->ai_addr, (int)it->ai_addrlen))
            break;
        closesocket(fd);
        fd = INVALID_SOCKET;
    }
    freeaddrinfo(addresses);
    if (fd == INVALID_SOCKET)
    {
        WSACleanup();
        return NULL;
    }
    result = (FVM_PortSocket *)malloc(sizeof(*result));
    if (!result)
    {
        closesocket(fd);
        WSACleanup();
        return NULL;
    }
    result->fd = fd;
    return result;
}

/* Send port socket. */
long fvm_port_socket_send(FVM_PortSocket *s, const void *p, size_t n)
{
    int r;
    if (!s || (!p && n))
        return -1;
    r = send(s->fd, (const char *)p, n > 2147483647U ? 2147483647 : (int)n, 0);
    return (long)r;
}

/* Compute and return port socket recv. */
long fvm_port_socket_recv(FVM_PortSocket *s, void *p, size_t n)
{
    int r;
    if (!s || (!p && n))
        return -1;
    r = recv(s->fd, (char *)p, n > 2147483647U ? 2147483647 : (int)n, 0);
    return (long)r;
}

/* Close port socket. */
void fvm_port_socket_close(FVM_PortSocket *s)
{
    if (s)
    {
        closesocket(s->fd);
        free(s);
        WSACleanup();
    }
}

/* Compute and return port tcp listen. */
FVM_PortSocket *fvm_port_tcp_listen(const char *host, unsigned short port, unsigned short *actual)
{

    /* Initialization / Инициализация локального состояния. */
    struct addrinfo hints, *addresses, *it;
    struct sockaddr_storage bound;
    int length;
    FVM_PortSocket *result;
    char service[8];
    SOCKET fd;
    int one;
    if (!host || !actual || !fvm_wsa_ready())
        return NULL;

    /* Computation / Вычисление и обновление состояния. */
    sprintf(service, "%u", (unsigned int)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(host, service, &hints, &addresses))
    {
        WSACleanup();
        return NULL;
    }

    /* Computation / Вычисление и обновление состояния. */
    fd = INVALID_SOCKET;
    one = 1;
    for (it = addresses; it; it = it->ai_next)
    {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd == INVALID_SOCKET)
            continue;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
        if (!bind(fd, it->ai_addr, (int)it->ai_addrlen) && !listen(fd, 32))
            break;
        closesocket(fd);
        fd = INVALID_SOCKET;
    }
    freeaddrinfo(addresses);
    if (fd == INVALID_SOCKET)
    {
        WSACleanup();
        return NULL;
    }
    length = sizeof(bound);
    if (getsockname(fd, (struct sockaddr *)&bound, &length))
    {
        closesocket(fd);
        WSACleanup();
        return NULL;
    }

    /* Computation / Вычисление и обновление состояния. */
    *actual = ntohs(bound.ss_family == AF_INET ? ((struct sockaddr_in *)&bound)->sin_port
                                               : ((struct sockaddr_in6 *)&bound)->sin6_port);
    result = (FVM_PortSocket *)malloc(sizeof(*result));
    if (!result)
    {
        closesocket(fd);
        WSACleanup();
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
    SOCKET fd;
    FVM_PortSocket *result;
    if (!listener)
        return NULL;

    /* Computation / Вычисление и обновление состояния. */
    FD_ZERO(&readset);
    FD_SET(listener->fd, &readset);
    timeout.tv_sec = (long)(ms / 1000);
    timeout.tv_usec = (long)(ms % 1000) * 1000;
    if (select(0, &readset, NULL, NULL, &timeout) <= 0)
        return NULL;
    if (!fvm_wsa_ready())
        return NULL;
    fd = accept(listener->fd, NULL, NULL);
    if (fd == INVALID_SOCKET)
    {
        WSACleanup();
        return NULL;
    }
    result = (FVM_PortSocket *)malloc(sizeof(*result));
    if (!result)
    {
        closesocket(fd);
        WSACleanup();
        return NULL;
    }
    result->fd = fd;
    return result;
}

/* Compute and return port socket timeout. */
int fvm_port_socket_timeout(FVM_PortSocket *s, unsigned long ms)
{
    DWORD timeout = (DWORD)ms;
    return s &&
           !setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) &&
           !setsockopt(s->fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
}

/* Compute and return port secure random. */
int fvm_port_secure_random(unsigned char *bytes, size_t size)
{
    HMODULE module;
    typedef BOOLEAN(WINAPI * RandomFunction)(void *, ULONG);

    /* Initialization / Инициализация локального состояния. */
    RandomFunction random_function;
    int result;
    if ((!bytes && size) || size > 0xffffffffUL)
        return 0;
    module = LoadLibraryA("advapi32.dll");
    if (!module)
        return 0;

    /* Computation / Вычисление и обновление состояния. */
    random_function = (RandomFunction)GetProcAddress(module, "SystemFunction036");
    result = random_function && random_function(bytes, (ULONG)size);
    FreeLibrary(module);
    return result;
}
