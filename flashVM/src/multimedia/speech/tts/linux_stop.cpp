/* OS: Linux/Android. Cancel only this SSIP connection's real speech output. */
#include "linux_ssip.hpp" /* Import client-scoped protocol helpers. */
#if defined(__linux__)    /* Isolate actual Linux speech service operations. */


/* Cancel queued/current messages without terminating the service. */
FVM_Result fvm_linux_tts_stop(void *opaque)                              /* Do not send global cancellation affecting other applications. */
{                                                                        /* Begin native cancellation. */
    FVM_LinuxTts *state = static_cast<FVM_LinuxTts *>(opaque);           /* Recover this client's service connection. */
    const char command[] = "CANCEL SELF\r\n";                            /* Restrict cancellation to the selected session. */
    int code = fvm_ssip_send(state, command, sizeof(command) - 1)        /* Ask the actual daemon to stop its output for this client. */
                   ? fvm_ssip_reply(state)                               /* Ask the actual daemon to stop its output for this client. */
                   : 0;                                                  /* Ask the actual daemon to stop its output for this client. */
    return code >= FVM_SSIP_SUCCESS_MIN && code < FVM_SSIP_SUCCESS_LIMIT /* Preserve daemon rejection or connection failure. */
               ? FVM_OK                                                  /* Preserve daemon rejection or connection failure. */
               : FVM_ERROR;                                              /* Preserve daemon rejection or connection failure. */
} /* Finish client-scoped cancellation. */
#endif /* Finish Linux-only implementation. */
