/* OS: Linux. Configure a Speech Dispatcher bridge without guessing a daemon or spawning it. */
#include "linux_ssip.hpp"                       /* Import native callbacks only on Linux. */
#include <string.h>                             /* Clear tables and inspect the explicitly selected socket path. */
#if defined(__linux__) && !defined(__ANDROID__) /* Obtain the real Unix socket path bound. */
#include <sys/un.h>                             /* Declare sockaddr_un and its target-specific path capacity. */
#endif                                          /* Finish platform-only includes. */
/* Borrow an explicit Unix socket path for each subsequent native open. */
FVM_Result                                 /* The path is configuration, never speech text or a shell command. */
fvm_tts_linux_backend(const char *path,    /* The path is configuration, never speech text or a shell command. */
                      FVM_TtsBackend *out) /* The path is configuration, never speech text or a shell command. */
{                                          /* Begin explicit daemon bridge setup. */
    if (!out)                              /* Require a writable provider table. */
    {                                      /* Begin conditional branch: Require a writable provider table. */
        return FVM_INVALID_ARGUMENT;       /* Require a writable provider table. */
    } /* End conditional branch: Require a writable provider table. */
    memset(out, 0, sizeof(*out));    /* Leave no stale callbacks on any failure. */
    if (!path || !*path)             /* Require an explicit service endpoint. */
    {                                /* Begin conditional branch: Require an explicit service endpoint. */
        return FVM_INVALID_ARGUMENT; /* Require an explicit service endpoint. */
    } /* End conditional branch: Require an explicit service endpoint. */
#if defined(__linux__) && !defined(__ANDROID__)   /* Bind the real Linux socket implementation. */
    struct sockaddr_un address;                   /* Use the actual kernel ABI path capacity. */
    if (strlen(path) >= sizeof(address.sun_path)) /* Reject truncation rather than connecting elsewhere. */
    {                                             /* Begin conditional branch: Reject truncation rather than connecting elsewhere. */
        return FVM_INVALID_ARGUMENT;              /* Reject truncation rather than connecting elsewhere. */
    } /* End conditional branch: Reject truncation rather than connecting elsewhere. */
    out->struct_size = sizeof(*out);               /* Identify the exact provider layout. */
    out->abi_version = FVM_SPEECH_ABI_VERSION;     /* Identify speech callback compatibility. */
    out->os_name = "Linux/Speech Dispatcher SSIP"; /* Label the actual system speech service. */
    out->userdata = const_cast<char *>(path);      /* Borrow the path until native open copies it into sockaddr_un. */
    out->open = fvm_linux_tts_open;                /* Bind actual Unix socket connection and protocol setup. */
    out->speak = fvm_linux_tts_speak;              /* Bind actual SSIP synthesis/playback requests. */
    out->stop = fvm_linux_tts_stop;                /* Bind client-scoped cancellation. */
    out->close = fvm_linux_tts_close;              /* Bind deterministic native cleanup. */
    return FVM_OK;                                 /* No connection or audible speech occurs during factory setup. */
#else                                              /* Preserve the function on other targets. */
    return FVM_UNSUPPORTED; /* Require that target's real provider instead. */
#endif                                             /* Finish Linux target selection. */
} /* Finish explicit Speech Dispatcher setup. */
