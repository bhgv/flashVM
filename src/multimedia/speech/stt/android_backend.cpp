/* OS: Android. Bind the supplied real platform speech helper with explicit application state. */
#include "../android.hpp" /* Import private JNI callbacks behind Android guards. */
#include <string.h> /* Clear incompatible/unsupported provider tables. */
/* Borrow host-owned VM, application Context and helper-class global references. */
FVM_Result fvm_stt_android_backend(const FVM_AndroidSpeechConfig *config, /* Keep config/references alive through all sessions. */
                                   FVM_SttBackend *out) /* Keep config/references alive through all sessions. */
{ /* Begin Android provider setup. */
    if (!out) /* Require writable provider storage. */
        return FVM_INVALID_ARGUMENT; /* Require writable provider storage. */
    memset(out, 0, sizeof(*out)); /* Clear stale callbacks before all validation. */
    if (!config || config->struct_size != sizeof(*config) || !config->java_vm || !config->application_context || /* Require complete host-owned JNI configuration. */
        !config->service_class) /* Require complete host-owned JNI configuration. */
        return FVM_INVALID_ARGUMENT; /* Require complete host-owned JNI configuration. */
#ifdef __ANDROID__ /* Bind only actual NDK/JVM operations. */
    out->struct_size = sizeof(*out); /* Identify the native provider ABI. */
    out->abi_version = FVM_SPEECH_ABI_VERSION; /* Identify compatible speech callbacks. */
    out->os_name = "Android/SpeechRecognizer"; /* Label the real platform service. */
    out->userdata = const_cast<FVM_AndroidSpeechConfig *>(config); /* Borrow persistent host configuration. */
    out->open = fvm_android_stt_open; /* Bind explicit native/Java service opening. */
    out->poll = fvm_android_stt_poll; /* Bind the real Java service poll bridge. */
    out->close = fvm_android_speech_close; /* Bind actual native service shutdown. */
    return FVM_OK; /* No service or microphone is activated during factory setup. */
#else /* Preserve a callable public ABI on other systems. */
    return FVM_UNSUPPORTED; /* Require that target's own provider instead of reinterpreting JNI pointers. */
#endif /* Finish Android selection. */
} /* Finish Android provider setup. */
