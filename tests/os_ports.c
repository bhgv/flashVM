#include "network_port.h"
#include "input_port.h"
#include <assert.h>
int main(void) {
    FVM_PortMouseState state;
    assert(fvm_port_tcp_connect((const char *)0, 80) == 0);
    assert(fvm_port_tcp_connect("localhost", 0) == 0);
    assert(fvm_port_socket_send(0, "x", 1) == -1);
    assert(fvm_port_socket_recv(0, &state, 1) == -1);
    assert(fvm_port_mouse_poll(0) == 0);
    assert(fvm_port_android_touch_poll(0, 0, 0, 0, 0) == -1);
    return 0;
}
