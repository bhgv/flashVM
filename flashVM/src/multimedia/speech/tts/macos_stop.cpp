/* OS: macOS. Stop actual speech on the selected voice channel. */
#include "macos.hpp" /* Import native channel and stop APIs. */
#ifdef __APPLE__     /* Isolate real Apple framework operations. */


/* Cancel only this session's native output. */
FVM_Result fvm_macos_tts_stop(void *opaque)                          /* Keep the native channel alive for subsequent text. */
{                                                                    /* Begin channel cancellation. */
    FVM_MacosTts *state = static_cast<FVM_MacosTts *>(opaque);       /* Recover the selected native channel. */
    return StopSpeech(state->channel) == noErr ? FVM_OK : FVM_ERROR; /* Invoke real system speech cancellation. */
} /* Finish channel cancellation. */
#endif /* Finish Apple-only implementation. */
