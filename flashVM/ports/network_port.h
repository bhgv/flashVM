/* FlashVM: Portable platform adapter interface or implementation: network port.
 * File: ports/network_port.h.
 */

#ifndef FVM_NETWORK_PORT_H
/* Define FVM NETWORK PORT H constant or compilation switch. */
#define FVM_NETWORK_PORT_H
#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct FVM_PortSocket FVM_PortSocket;
    /* TCP client. Blocking calls; caller must not invoke on the VM/render thread. */
    FVM_PortSocket *fvm_port_tcp_connect(const char *hostname, unsigned short port);
    /* Create a listening TCP socket and report its bound port. */
    FVM_PortSocket *fvm_port_tcp_listen(const char *, unsigned short, unsigned short *);
    /* Accept polls for milliseconds; NULL means no connection/error. */
    FVM_PortSocket *fvm_port_tcp_accept(FVM_PortSocket *, unsigned long);
    /* Implement port socket timeout for this subsystem. */
    int fvm_port_socket_timeout(FVM_PortSocket *, unsigned long);
    /* Implement port secure random for this subsystem. */
    int fvm_port_secure_random(unsigned char *, size_t);
    /* Return number of bytes transferred, 0 on EOF (recv), -1 on error. */
    /* Send port socket. */
    long fvm_port_socket_send(FVM_PortSocket *, const void *, size_t);
    /* Implement port socket recv for this subsystem. */
    long fvm_port_socket_recv(FVM_PortSocket *, void *, size_t);
    /* Close port socket. */
    void fvm_port_socket_close(FVM_PortSocket *);
#ifdef __cplusplus
}
#endif
#endif
