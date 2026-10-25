/* OS: Linux/Android. Connect to an explicit real Speech Dispatcher socket without starting a daemon. */
#include "linux_ssip.hpp" /* Import native protocol and ownership helpers. */
#if defined(__linux__)    /* Isolate actual Linux system calls. */
#include <new>            /* Allocate native state without exceptions. */
#include <string.h>       /* Initialize and copy the already-validated socket path. */
#include <sys/socket.h>   /* Open and configure a Unix-domain stream socket. */
#include <sys/time.h>     /* Configure bounded blocking I/O timeouts. */
#include <sys/un.h>       /* Use the native Unix socket address layout. */
#include <unistd.h>       /* Close failed descriptors. */


/* Connect and identify one real SSIP client, with full partial-failure cleanup. */
FVM_Result fvm_unix_tts_open(void *opaque, void **out)     /* The borrowed socket path is copied during this call only. */
{                                                          /* Begin explicit native speech-service access. */
    *out = 0;                                              /* Publish no partial connection. */
    FVM_LinuxTts *state = new (std::nothrow) FVM_LinuxTts; /* Own a single native connection. */
    if (!state)                                            /* Do not open a descriptor after allocation failure. */
    {                                                      /* Begin conditional branch: Do not open a descriptor after allocation failure. */
        return FVM_OUT_OF_MEMORY;                          /* Do not open a descriptor after allocation failure. */
    } /* End conditional branch: Do not open a descriptor after allocation failure. */
    state->failed = false;                       /* Start with a known protocol boundary. */
    state->fd = socket(AF_UNIX, SOCK_STREAM, 0); /* Create the actual local service socket. */
    if (state->fd < 0)                           /* Preserve OS socket creation failure. */
    {                                            /* Release unpublished ownership. */
        delete state;                            /* No descriptor exists to close. */
        return FVM_ERROR;                        /* Report the actual system failure. */
    } /* Finish descriptor-creation error handling. */
    struct timeval timeout;                                                                                     /* Bound each blocking protocol I/O operation. */
    timeout.tv_sec = FVM_SSIP_IO_TIMEOUT_SECONDS;                                                               /* Use the documented finite native wait bound. */
    timeout.tv_usec = 0;                                                                                        /* No fractional timeout component is needed. */
    struct sockaddr_un address;                                                                                 /* Build either filesystem or abstract native addressing. */
    socklen_t address_length = 0;                                                                               /* Preserve the namespace's exact binary address length. */
    FVM_Result address_result = fvm_ssip_address(static_cast<const FVM_SpeechUnixEndpoint *>(opaque), &address, /* Never measure an abstract name with strlen. */
                                                 &address_length);                                              /* Never measure an abstract name with strlen. */
    if (address_result != FVM_OK)                                                                               /* Protect direct callback use as well as validated factory calls. */
    {                                                                                                           /* Release partial native socket ownership. */
        close(state->fd);                                                                                       /* Drop the newly created actual socket. */
        delete state;                                                                                           /* Release unpublished connection state. */
        return address_result;                                                                                  /* Preserve malformed endpoint arguments. */
    } /* Finish endpoint validation cleanup. */
    if (setsockopt(state->fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < /* Configure and connect the actual system endpoint. */
            0 ||                                                                    /* Configure and connect the actual system endpoint. */
        setsockopt(state->fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < /* Configure and connect the actual system endpoint. */
            0 ||                                                                    /* Configure and connect the actual system endpoint. */
        connect(state->fd, reinterpret_cast<struct sockaddr *>(&address),           /* Configure and connect the actual system endpoint. */
                address_length) <                                                   /* Configure and connect the actual system endpoint. */
            0)                                                                      /* Configure and connect the actual system endpoint. */
    {                                                                               /* Clean socket setup or unavailable-service failure. */
        close(state->fd);                                                           /* Release the owned descriptor without protocol commands. */
        delete state;                                                               /* Release native ownership. */
        return FVM_ERROR;                                                           /* An explicit endpoint was requested but could not be opened. */
    } /* Finish connection failure handling. */
    const char identity[] =                                         /* Identify this connection without injecting host-controlled protocol tokens. */
        "SET SELF CLIENT_NAME flashvm:tts:default\r\n";             /* Identify this connection without injecting host-controlled protocol tokens. */
    int code = fvm_ssip_send(state, identity, sizeof(identity) - 1) /* Send and read the actual service handshake. */
                   ? fvm_ssip_reply(state)                          /* Send and read the actual service handshake. */
                   : 0;                                             /* Send and read the actual service handshake. */
    if (code < FVM_SSIP_SUCCESS_MIN ||                              /* Require service acceptance before publishing the session. */
        code >= FVM_SSIP_SUCCESS_LIMIT)                             /* Require service acceptance before publishing the session. */
    {                                                               /* Clean a rejected or malformed handshake. */
        close(state->fd);                                           /* Drop the unusable connection immediately. */
        delete state;                                               /* Release native ownership. */
        return FVM_ERROR;                                           /* Do not present a failed handshake as usable speech. */
    } /* Finish protocol-open failure handling. */
    *out = state;  /* Transfer a fully connected speech-client state. */
    return FVM_OK; /* No text is synthesized until explicit speak. */
} /* Finish native SSIP opening. */
#endif /* Finish Linux-only implementation. */
