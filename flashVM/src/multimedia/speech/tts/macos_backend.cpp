/* OS: macOS. Bind actual Speech Synthesis functions without opening a channel. */
#include "macos.hpp" /* Import native callbacks only for Apple targets. */
/* Initialize the real system speech table without speaking. */
FVM_Result fvm_tts_macos_backend(FVM_TtsBackend *out) /* Called by the validated current-target factory. */
{ /* Begin native provider binding. */
#ifdef __APPLE__ /* Bind only a target with the real Apple SDK. */
    out->struct_size = sizeof(*out); /* Identify the complete provider layout. */
    out->abi_version = FVM_SPEECH_ABI_VERSION; /* Identify the replaceable speech ABI. */
    out->os_name = "macOS/Speech Synthesis"; /* Label the actual native voice service. */
    out->open = fvm_macos_tts_open; /* Bind actual channel creation. */
    out->speak = fvm_macos_tts_speak; /* Bind actual Unicode synthesis/playback. */
    out->stop = fvm_macos_tts_stop; /* Bind channel-scoped cancellation. */
    out->close = fvm_macos_tts_close; /* Bind actual native channel disposal. */
    return FVM_OK; /* Native resources remain deferred until explicit open. */
#else /* Preserve cross-target internal linkage. */
    (void)out; /* The validated public factory owns clearing the output. */
    return FVM_UNSUPPORTED; /* Do not emulate Apple speech on another system. */
#endif /* Finish Apple target selection. */
} /* Finish native provider binding. */
