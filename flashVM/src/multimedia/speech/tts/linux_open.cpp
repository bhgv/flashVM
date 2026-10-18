/* OS: Linux. Preserve the filesystem-path convenience factory while using exact native addressing. */
#include "linux_ssip.hpp" /* Share binary-safe Unix endpoint opening. */
#if defined(__linux__) /* Compile the actual Unix service bridge on Linux-derived targets. */
#include <string.h> /* Measure only a filesystem path, never an abstract name. */
/* Adapt the legacy explicit path configuration to the common native Unix connector. */
FVM_Result fvm_linux_tts_open(void *opaque, /* Borrow the validated filesystem path during this call only. */
                              void **out) /* Borrow the validated filesystem path during this call only. */
{ /* Begin filesystem convenience opening. */
    FVM_SpeechUnixEndpoint endpoint; /* Keep the temporary typed address alive through synchronous open. */
    endpoint.struct_size = sizeof(endpoint); /* Identify the exact typed endpoint layout. */
    endpoint.name_space = FVM_SPEECH_UNIX_FILESYSTEM; /* Preserve the original path-only factory semantics. */
    endpoint.name = opaque; /* Borrow the explicit filesystem path from factory userdata. */
    endpoint.length = /* strlen is valid only for this path convenience API. */
        strlen(static_cast<const char *>(opaque)); /* strlen is valid only for this path convenience API. */
    return fvm_unix_tts_open(&endpoint, out); /* Build the correct native length and connect synchronously. */
} /* Finish filesystem convenience opening. */
#endif /* Finish Linux-derived implementation. */
