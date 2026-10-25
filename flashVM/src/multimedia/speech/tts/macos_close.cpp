/* OS: macOS. Stop and dispose real Speech Synthesis resources. */
#include "macos.hpp" /* Import native channel ownership. */
#ifdef __APPLE__     /* Isolate Apple resource cleanup. */


/* Stop output before releasing the owned channel. */
void fvm_macos_tts_close(void *opaque)                         /* Close on the same host thread used for the session. */
{                                                              /* Begin native voice cleanup. */
    FVM_MacosTts *state = static_cast<FVM_MacosTts *>(opaque); /* Recover owned native state. */
    StopSpeech(state->channel);                                /* Request native cancellation before disposal. */
    DisposeSpeechChannel(state->channel);                      /* Release the actual system voice channel. */
    delete state;                                              /* Release unmanaged ownership last. */
} /* Finish native channel cleanup. */
#endif /* Finish Apple-only implementation. */
