#include "../network_port.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
struct FVM_PortSocket { SOCKET fd; };
static int fvm_wsa_ready(void) {
    WSADATA data;
    return WSAStartup(MAKEWORD(2,2), &data) == 0;
}
FVM_PortSocket *fvm_port_tcp_connect(const char *hostname, unsigned short port) {
    struct addrinfo hints, *addresses, *it;
    char service[8];
    SOCKET fd = INVALID_SOCKET;
    FVM_PortSocket *result;
    if (!hostname || !port || !fvm_wsa_ready()) return NULL;
    sprintf(service, "%u", (unsigned int)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(hostname, service, &hints, &addresses)) { WSACleanup(); return NULL; }
    for (it = addresses; it; it = it->ai_next) {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd == INVALID_SOCKET) continue;
        if (connect(fd, it->ai_addr, (int)it->ai_addrlen) == 0) break;
        closesocket(fd); fd = INVALID_SOCKET;
    }
    freeaddrinfo(addresses);
    if (fd == INVALID_SOCKET) { WSACleanup(); return NULL; }
    result = (FVM_PortSocket *)malloc(sizeof(*result));
    if (!result) { closesocket(fd); WSACleanup(); return NULL; }
    result->fd = fd;
    return result;
}
long fvm_port_socket_send(FVM_PortSocket *s, const void *p, size_t n) {
    int r;
    if (!s || (!p && n)) return -1;
    r = send(s->fd, (const char *)p, n > 2147483647U ? 2147483647 : (int)n, 0);
    return (long)r;
}
long fvm_port_socket_recv(FVM_PortSocket *s, void *p, size_t n) {
    int r;
    if (!s || (!p && n)) return -1;
    r = recv(s->fd, (char *)p, n > 2147483647U ? 2147483647 : (int)n, 0);
    return (long)r;
}
void fvm_port_socket_close(FVM_PortSocket *s) {
    if (s) { closesocket(s->fd); free(s); WSACleanup(); }
}
