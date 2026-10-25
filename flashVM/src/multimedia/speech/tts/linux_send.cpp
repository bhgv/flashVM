/* OS: Linux/Android. Send a complete SSIP protocol fragment without delivering SIGPIPE to the host. */
#include "linux_ssip.hpp" /* Import native connection ownership. */
#if defined(__linux__)    /* Isolate Unix socket APIs. */
#include <errno.h>        /* Retry interrupted system calls. */
#include <sys/socket.h>   /* Declare send and MSG_NOSIGNAL. */


/* Write every requested byte or mark the connection unusable. */
bool fvm_ssip_send(FVM_LinuxTts *state, const char *data, /* Borrow input bytes until synchronous completion. */
                   size_t size)                           /* Borrow input bytes until synchronous completion. */
{                                                         /* Begin complete-fragment transmission. */
    if (state->failed)                                    /* Do not reuse uncertain protocol state. */
    {                                                     /* Begin conditional branch: Do not reuse uncertain protocol state. */
        return false;                                     /* Do not reuse uncertain protocol state. */
    } /* End conditional branch: Do not reuse uncertain protocol state. */
    while (size)                                   /* Handle partial socket writes correctly. */
    {                                              /* Begin one actual system write. */
        ssize_t sent = send(state->fd, data, size, /* Suppress process-level SIGPIPE on a disconnected speech daemon. */
                            MSG_NOSIGNAL);         /* Suppress process-level SIGPIPE on a disconnected speech daemon. */
        if (sent < 0 && errno == EINTR)            /* Retry only an interrupted system call. */
        {                                          /* Begin conditional branch: Retry only an interrupted system call. */
            continue;                              /* Retry only an interrupted system call. */
        } /* End conditional branch: Retry only an interrupted system call. */
        if (sent <= 0)            /* Treat timeout, disconnection and hard failures as terminal. */
        {                         /* Poison the uncertain protocol position. */
            state->failed = true; /* Require close/reopen after failed transmission. */
            return false;         /* Preserve native I/O failure for the public callback. */
        } /* Finish failed-send handling. */
        data += sent;         /* Advance past bytes actually accepted by the kernel. */
        size -= (size_t)sent; /* Send only the remaining fragment. */
    } /* Finish all partial writes. */
    return true; /* All protocol bytes reached the local socket. */
} /* Finish complete-fragment transmission. */
#endif /* Finish Linux-only implementation. */
