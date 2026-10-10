#ifndef FVM_NETWORK_PORT_H
#define FVM_NETWORK_PORT_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct FVM_PortSocket FVM_PortSocket;
/* TCP client. Blocking calls; caller must not invoke on the VM/render thread. */
FVM_PortSocket *fvm_port_tcp_connect(const char *hostname, unsigned short port);
/* Return number of bytes transferred, 0 on EOF (recv), -1 on error. */
long fvm_port_socket_send(FVM_PortSocket *, const void *, size_t);
long fvm_port_socket_recv(FVM_PortSocket *, void *, size_t);
void fvm_port_socket_close(FVM_PortSocket *);
#ifdef __cplusplus
}
#endif
#endif
