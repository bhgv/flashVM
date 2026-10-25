#include <stdio.h>
#include <stdlib.h>
#include "network_port.h"
#include "input_port.h"


int main(void)
{
    FVM_PortMouseState state;
    if (!(fvm_port_tcp_connect((const char *)0, 80) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_port_tcp_connect("localhost", 0) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_port_socket_send(0, "x", 1) == -1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_port_socket_recv(0, &state, 1) == -1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_port_mouse_poll(0) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_port_android_touch_poll(0, 0, 0, 0, 0) == -1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
