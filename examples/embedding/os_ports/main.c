/* C89 embedding example: read native pointer state and attempt a TCP connect.
 * Run against a server you control; the connect call is BLOCKING.
 * This example intentionally makes no network connection during tests. */
#include "input_port.h"
#include "network_port.h"
#include <stdio.h>
int main(void) {
    FVM_PortMouseState mouse;
    if (fvm_port_mouse_poll(&mouse)) {
        printf("Pointer: %ld,%ld buttons=%u\n", mouse.x, mouse.y, mouse.buttons);
    } else {
        puts("Native pointer polling unavailable; use host input dispatch");
    }
    /* Example when a local TCP server is available:
     * FVM_PortSocket *s = fvm_port_tcp_connect("127.0.0.1", 8080);
     * if (s) { fvm_port_socket_send(s, "ping", 4); fvm_port_socket_close(s); }
     */
    return 0;
}
