/* OS: current compilation target. Select only a real available native speech provider. */
#include "../session.hpp" /* Import target-specific table initializers. */
#include <string.h> /* Clear unsupported provider tables. */
/* Fill callbacks without speaking, starting a daemon or opening a microphone. */
FVM_Result fvm_tts_system_backend(FVM_TtsBackend *out) /* Native resources are deferred until explicit open. */
{ /* Begin provider selection. */
    if (!out) /* Require writable table storage. */
        return FVM_INVALID_ARGUMENT; /* Require writable table storage. */
    memset(out, 0, sizeof(*out)); /* Never publish stale callbacks after unsupported selection. */
#if defined(_WIN32) /* Bind actual SAPI on Windows. */
    return fvm_tts_windows_backend(out); /* Use the target's installed system voice. */
#elif defined(__APPLE__) /* Bind actual Speech Synthesis on macOS. */
    return fvm_tts_macos_backend(out); /* Use the target's configured system voice. */
#else /* Linux needs an explicit daemon socket; Android needs its application Context. */
    return FVM_UNSUPPORTED; /* Let the host select/install its real provider explicitly. */
#endif /* Finish target selection. */
} /* Finish native TTS factory. */
