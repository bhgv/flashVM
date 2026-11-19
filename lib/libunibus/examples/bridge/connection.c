/* Copyright (c) 2026 bhgv
 * EN: Optional POSIX example backend connector; physical carrier selection in
 * the reusable libraries remains independent of TCP and this demo.
 * RU: POSIX backend connector примера; физический носитель reusable libraries
 * не ограничен TCP и этим demo.
 * standalone-reviewed: owned socket, bounded syscall timeout, no automatic reconnect.
 */
#include "bridge.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>


/* EN: port selects a loopback listener; return a newly owned connected fd or -1.
 * Close the fresh fd on every setup/connect failure. The 3-second socket options
 * bound individual I/O syscalls; partial progress can extend the overall RPC.
 * RU: port выбирает loopback listener; вернуть новый connected fd либо -1.
 * При ошибке настройки/connect закрыть fd. Timeout 3 с относится к отдельному
 * syscall; частичный прогресс может продлить полный RPC.
 */
int bridge_connect(unsigned port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0); /* Only this new open-file description is configured. */
    struct sockaddr_in address; /* No native struct is transmitted as protocol data. */
    struct timeval timeout = {3, 0}; /* Explicit teaching policy, not a universal Unibus deadline. */
    if (fd < 0)
    {
        return -1;
    }
    memset(&address, 0, sizeof(address)); /* Initialize padding and unused address fields. */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* Demo backend is local; stdin/stdout carry Unibus traffic. */
    address.sin_port = htons(port);
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) ||
        connect(fd, (struct sockaddr *)&address, sizeof(address)))
    {
        int error = errno; /* Cleanup must not replace the failing connect/setup diagnostic. */
        close(fd); /* A failed connector never transfers a partially configured descriptor. */
        errno = error;
        return -1;
    }
    return fd; /* bridge_worker_start consumes this ownership even if thread startup fails. */
}
