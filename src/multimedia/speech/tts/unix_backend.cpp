/* OS: Linux/Android. Bind a real SSIP service through explicit binary-safe Unix socket addressing. */
#include "linux_ssip.hpp" /* Import actual native connector and protocol callbacks. */
#include <string.h> /* Clear unsupported/incompatible provider tables. */
/* Validate an endpoint without connecting or starting any speech service. */
FVM_Result /* The host retains endpoint/name storage through native opening. */
fvm_tts_unix_backend(const FVM_SpeechUnixEndpoint *endpoint, /* The host retains endpoint/name storage through native opening. */
                     FVM_TtsBackend *out) /* The host retains endpoint/name storage through native opening. */
{ /* Begin explicit Unix service selection. */
    if (!out) /* Require writable provider storage. */
        return FVM_INVALID_ARGUMENT; /* Require writable provider storage. */
    memset(out, 0, sizeof(*out)); /* Clear stale callbacks before all failures. */
#if defined(__linux__) /* Linux and Android implement the actual required socket namespace semantics. */
    struct sockaddr_un address; /* Validate the actual target's native address capacity. */
    socklen_t length = 0; /* Receive the exact native binary address length. */
    FVM_Result result = fvm_ssip_address(endpoint, &address, /* Reject invalid namespace/length or truncated filesystem names. */
                                         &length); /* Reject invalid namespace/length or truncated filesystem names. */
    if (result != FVM_OK) /* Do not bind callbacks for malformed host configuration. */
        return result; /* Do not bind callbacks for malformed host configuration. */
    out->struct_size = sizeof(*out); /* Identify the complete speech-provider layout. */
    out->abi_version = FVM_SPEECH_ABI_VERSION; /* Identify compatible speech callback semantics. */
#ifdef __ANDROID__ /* Label the actual Android socket service configuration. */
    out->os_name = /* This is an explicit installed service, not Android's standard TTS Binder API. */
        "Android/SSIP Unix socket"; /* This is an explicit installed service, not Android's standard TTS Binder API. */
#else /* Label the actual Linux service configuration. */
    out->os_name = "Linux/SSIP Unix socket"; /* The daemon must actually speak SSIP at the chosen endpoint. */
#endif /* Finish target labeling. */
    out->userdata = const_cast<FVM_SpeechUnixEndpoint *>(endpoint); /* Borrow persistent host address configuration. */
    out->open = fvm_unix_tts_open; /* Bind actual binary-safe native connection and service handshake. */
    out->speak = fvm_linux_tts_speak; /* Reuse actual SSIP text framing and synthesis requests. */
    out->stop = fvm_linux_tts_stop; /* Reuse client-scoped real service cancellation. */
    out->close = fvm_linux_tts_close; /* Reuse actual native socket cleanup. */
    return FVM_OK; /* Native resources open only after a later explicit session open. */
#else /* Preserve the public ABI on targets without these abstract namespace semantics. */
    (void)endpoint; /* No binary endpoint is dereferenced on an unsupported OS. */
    return FVM_UNSUPPORTED; /* Use a real target-specific speech provider instead. */
#endif /* Finish Linux/Android selection. */
} /* Finish explicit Unix speech backend selection. */
