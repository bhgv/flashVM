/* OS: Windows SAPI. Build a table without creating voices or activating dictation. */
#include "../windows.hpp" /* Import native callbacks behind Windows guards. */
/* Initialize a real Windows provider or report unsupported on another target. */
FVM_Result fvm_stt_windows_backend(FVM_SttBackend *out) /* Called by the validated system factory. */
{ /* Begin native callback binding. */
#ifdef _WIN32 /* Bind only callbacks compiled against the actual Windows SDK. */
    out->struct_size = sizeof(*out); /* Identify the complete provider layout. */
    out->abi_version = FVM_SPEECH_ABI_VERSION; /* Identify the replaceable speech ABI. */
    out->os_name = "Windows/SAPI"; /* Label the actual native service. */
    out->open = fvm_windows_stt_open; /* Bind the actual SAPI open operation. */
    out->poll = fvm_windows_stt_poll; /* Bind the actual SAPI poll operation. */
    out->close = fvm_windows_stt_close; /* Bind the actual SAPI close operation. */
    return FVM_OK; /* Native opening remains an explicit later operation. */
#else /* Keep internal cross-target linkage predictable. */
    (void)out; /* The public factory has already cleared the table. */
    return FVM_UNSUPPORTED; /* Do not emulate SAPI on another platform. */
#endif /* Finish Windows selection. */
} /* Finish callback binding. */
