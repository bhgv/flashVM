#define _POSIX_C_SOURCE 200112L
#include "../network_port.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
struct FVM_PortSocket { int fd; };
FVM_PortSocket *fvm_port_tcp_connect(const char *hostname, unsigned short port) {
    struct addrinfo hints, *addresses, *it;
    char service[8];
    int fd = -1;
    FVM_PortSocket *result;
    if (!hostname || !port) return NULL;
    sprintf(service, "%u", (unsigned int)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(hostname, service, &hints, &addresses) != 0) return NULL;
    for (it = addresses; it; it = it->ai_next) {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, it->ai_addr, it->ai_addrlen) == 0) break;
        close(fd); fd = -1;
    }
    freeaddrinfo(addresses);
    if (fd < 0) return NULL;
    result = (FVM_PortSocket *)malloc(sizeof(*result));
    if (!result) { close(fd); return NULL; }
    result->fd = fd;
    return result;
}
long fvm_port_socket_send(FVM_PortSocket *s, const void *p, size_t n) {
    ssize_t r;
    if (!s || (!p && n)) return -1;
    r = send(s->fd, p, n, 0);
    return r < 0 ? -1 : (long)r;
}
long fvm_port_socket_recv(FVM_PortSocket *s, void *p, size_t n) {
    ssize_t r;
    if (!s || (!p && n)) return -1;
    r = recv(s->fd, p, n, 0);
    return r < 0 ? -1 : (long)r;
}
void fvm_port_socket_close(FVM_PortSocket *s) {
    if (s) { close(s->fd); free(s); }
}
