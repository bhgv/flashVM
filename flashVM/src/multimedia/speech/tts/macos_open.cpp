/* OS: macOS. Open the configured real Speech Synthesis voice. */
#include "macos.hpp" /* Import native channel ownership. */
#ifdef __APPLE__ /* Compile only with the actual Apple SDK. */
#include <new> /* Allocate ownership without exceptions. */
/* Create one independent native voice channel without speaking yet. */
FVM_Result fvm_macos_tts_open(void *unused, void **out) /* Use the system's configured voice/language. */
{ /* Begin native voice opening. */
    (void)unused; /* No shared mutable provider configuration is needed. */
    *out = 0; /* Publish only complete native ownership. */
    FVM_MacosTts *state = new (std::nothrow) FVM_MacosTts; /* Allocate unmanaged channel ownership. */
    if (!state) /* Preserve allocation failure before framework calls. */
        return FVM_OUT_OF_MEMORY; /* Preserve allocation failure before framework calls. */
    state->channel = 0; /* Establish partial-initialization safety. */
    if (NewSpeechChannel(0, &state->channel) != noErr) /* Open a real system Speech Synthesis channel. */
    { /* Release unavailable native state. */
        delete state; /* No usable channel was transferred. */
        return FVM_UNSUPPORTED; /* Report an absent/unusable native voice. */
    } /* Finish channel-open failure handling. */
    *out = state; /* Transfer the actual native channel to the common session. */
    return FVM_OK; /* Speech requires a later explicit text submission. */
} /* Finish native voice opening. */
#endif /* Finish Apple-only implementation. */
