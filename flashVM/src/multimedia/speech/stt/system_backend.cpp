/* OS: current compilation target. Recognition requires a real installed provider. */
#include "../session.hpp" /* Import native recognition table initializers. */
#include "macos.hpp"      /* Import optional actual macOS recognition callbacks. */
#include <string.h>       /* Clear unsupported callback tables. */


/* Fill callbacks without activating recognition or requesting microphone permissions. */
FVM_Result fvm_stt_system_backend(FVM_SttBackend *out) /* Explicit open owns the capture boundary. */
{                                                      /* Begin native recognizer selection. */
    if (!out)                                          /* Require writable output storage. */
    {                                                  /* Begin conditional branch: Require writable output storage. */
        return FVM_INVALID_ARGUMENT;                   /* Require writable output storage. */
    } /* End conditional branch: Require writable output storage. */
    memset(out, 0, sizeof(*out));                      /* Publish no stale recognizer on unsupported targets. */
#ifdef _WIN32                                          /* Bind the actual installed Windows dictation engine. */
    return fvm_stt_windows_backend(out);               /* SAPI opens capture only during explicit session open. */
#elif defined(__APPLE__) && defined(FVM_HAS_MACOS_STT) /* Bind the actual compiled Apple Speech framework. */
    out->struct_size = sizeof(*out);              /* Identify the complete recognition-provider layout. */
    out->abi_version = FVM_SPEECH_ABI_VERSION;    /* Identify compatible speech callbacks. */
    out->os_name = "macOS/Speech + AVFoundation"; /* Label actual native capture and recognition services. */
    out->open = fvm_macos_stt_open;               /* Bind explicit authorized native capture. */
    out->poll = fvm_macos_stt_poll;               /* Bind actual synchronized transcript retrieval. */
    out->close = fvm_macos_stt_close;             /* Bind real capture/task shutdown. */
    return FVM_OK;                                /* Factory setup itself never opens the microphone. */
#else                                                  /* Linux and Android require explicit model/service or application configuration. */
    return FVM_UNSUPPORTED; /* Hosts can install a real platform-specific recognition table. */
#endif                                                 /* Finish native provider selection. */
} /* Finish native STT factory. */
