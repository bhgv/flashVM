/* OS: Linux/Android. Cancel this client's speech and release its real Unix socket. */
#include "linux_ssip.hpp" /* Import connection ownership and cancellation. */
#if defined(__linux__)    /* Isolate native descriptor cleanup. */
#include <unistd.h>       /* Close the actual Unix-domain socket. */


/* Close complete native state without reusing an already failed protocol stream. */
void fvm_linux_tts_close(void *opaque)                         /* Cleanup cannot report cancellation/close errors through this void callback. */
{                                                              /* Begin native speech cleanup. */
    FVM_LinuxTts *state = static_cast<FVM_LinuxTts *>(opaque); /* Recover owned daemon state. */
    if (!state->failed)                                        /* Request client-scoped cancellation before orderly disconnect. */
    {                                                          /* Begin conditional branch: Request client-scoped cancellation before orderly disconnect. */
        fvm_linux_tts_stop(state);                             /* Request client-scoped cancellation before orderly disconnect. */
    } /* End conditional branch: Request client-scoped cancellation before orderly disconnect. */
    close(state->fd); /* Release the real local service connection even if cancellation failed. */
    delete state;     /* Release unmanaged connection ownership. */
} /* Finish native service cleanup. */
#endif /* Finish Linux-only implementation. */
