#include "../network_port.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
struct FVM_PortSocket
{
    SOCKET fd;
};
static int fvm_wsa_ready(void)
{
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}
static int bounded_connect(SOCKET fd, const struct sockaddr *address, int length)
{
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
FVM_PortSocket *fvm_port_tcp_connect(const char *hostname, unsigned short port)
{
    struct addrinfo hints, *addresses, *it;
    char service[8];
    SOCKET fd = INVALID_SOCKET;
    FVM_PortSocket *result;
    if (!hostname || !port || !fvm_wsa_ready())
        return NULL;
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
long fvm_port_socket_send(FVM_PortSocket *s, const void *p, size_t n)
{
    int r;
    if (!s || (!p && n))
        return -1;
    r = send(s->fd, (const char *)p, n > 2147483647U ? 2147483647 : (int)n, 0);
    return (long)r;
}
long fvm_port_socket_recv(FVM_PortSocket *s, void *p, size_t n)
{
    int r;
    if (!s || (!p && n))
        return -1;
    r = recv(s->fd, (char *)p, n > 2147483647U ? 2147483647 : (int)n, 0);
    return (long)r;
}
void fvm_port_socket_close(FVM_PortSocket *s)
{
    if (s)
    {
        closesocket(s->fd);
        free(s);
        WSACleanup();
    }
}

FVM_PortSocket *fvm_port_tcp_listen(const char *host, unsigned short port, unsigned short *actual)
{
    struct addrinfo hints, *addresses, *it;
    struct sockaddr_storage bound;
    int length;
    FVM_PortSocket *result;
    char service[8];
    SOCKET fd;
    int one;
    if (!host || !actual || !fvm_wsa_ready())
        return NULL;
    sprintf(service, "%u", (unsigned int)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(host, service, &hints, &addresses))
    {
        WSACleanup();
        return NULL;
    }
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
FVM_PortSocket *fvm_port_tcp_accept(FVM_PortSocket *listener, unsigned long ms)
{
    fd_set readset;
    struct timeval timeout;
    SOCKET fd;
    FVM_PortSocket *result;
    if (!listener)
        return NULL;
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
int fvm_port_socket_timeout(FVM_PortSocket *s, unsigned long ms)
{
    DWORD timeout = (DWORD)ms;
    return s &&
           !setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) &&
           !setsockopt(s->fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
}
int fvm_port_secure_random(unsigned char *bytes, size_t size)
{
    HMODULE module;
    typedef BOOLEAN(WINAPI * RandomFunction)(void *, ULONG);
    RandomFunction random_function;
    int result;
    if ((!bytes && size) || size > 0xffffffffUL)
        return 0;
    module = LoadLibraryA("advapi32.dll");
    if (!module)
        return 0;
    random_function = (RandomFunction)GetProcAddress(module, "SystemFunction036");
    result = random_function && random_function(bytes, (ULONG)size);
    FreeLibrary(module);
    return result;
}
