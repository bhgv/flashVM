/* OS: Windows. Stop and release native voice ownership on its creating thread. */
#include "../windows.hpp" /* Import COM state and voice interfaces. */
#ifdef _WIN32 /* Isolate Windows native cleanup. */
/* Balance complete and partial native voice initialization. */
void fvm_windows_tts_close(void *opaque) /* Native cleanup must not race with speak. */
{ /* Begin voice cleanup. */
    FVM_WindowsTts *state = static_cast<FVM_WindowsTts *>(opaque); /* Recover owned native state. */
    if (state->voice) /* Release only an actually created voice. */
    { /* Stop speech before releasing the interface. */
        state->voice->Speak(0, SPF_PURGEBEFORESPEAK, 0); /* Request immediate purge of this voice queue. */
        state->voice->Release(); /* Release the actual COM voice reference. */
    } /* Finish voice-interface cleanup. */
    if (state->com) /* Balance this session's successful STA initialization. */
        CoUninitialize(); /* Balance this session's successful STA initialization. */
    delete state; /* Release unmanaged ownership last. */
} /* Finish native output cleanup. */
#endif /* Finish Windows-only implementation. */
