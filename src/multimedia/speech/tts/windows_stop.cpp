/* OS: Windows. Purge the selected SAPI voice queue. */
#include "../windows.hpp" /* Import the native voice interface. */
#ifdef _WIN32 /* Compile only against the actual Windows SDK. */
/* Stop current/pending speech while keeping the voice available. */
FVM_Result fvm_windows_tts_stop(void *opaque) /* Require the creating COM thread. */
{ /* Begin native cancellation. */
    FVM_WindowsTts *state = static_cast<FVM_WindowsTts *>(opaque); /* Recover this voice instance. */
    return SUCCEEDED(state->voice->Speak(0, SPF_PURGEBEFORESPEAK, 0)) ? FVM_OK /* Purge actual speech with SAPI. */
                                                                      : FVM_ERROR; /* Purge actual speech with SAPI. */
} /* Finish native cancellation. */
#endif /* Finish Windows-only implementation. */
