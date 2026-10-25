/* OS: Linux. Bind optional real static Vosk recognition and explicit ALSA microphone capture. */
#include "linux_vosk.hpp" /* Import native callbacks only when actual SDKs are selected. */
#include <string.h>       /* Clear incompatible/unsupported provider tables. */


/* Require explicit installed model/device configuration; never download or guess a model. */
FVM_Result fvm_stt_linux_backend(const FVM_LinuxSttConfig *config, /* Configuration remains host-owned through native opening. */
                                 FVM_SttBackend *out)              /* Configuration remains host-owned through native opening. */
{                                                                  /* Begin explicit native recognizer selection. */
    if (!out)                                                      /* Require a writable provider table. */
    {                                                              /* Begin conditional branch: Require a writable provider table. */
        return FVM_INVALID_ARGUMENT;                               /* Require a writable provider table. */
    } /* End conditional branch: Require a writable provider table. */
    memset(out, 0, sizeof(*out));                                                                           /* Clear stale callbacks on all failures. */
    if (!config || config->struct_size != sizeof(*config) || !config->model_path || !*config->model_path || /* Require complete model and microphone configuration. */
        !config->alsa_device || !*config->alsa_device)                                                      /* Require complete model and microphone configuration. */
    {                                                                                                       /* Begin scope: Require complete model and microphone configuration. */
        return FVM_INVALID_ARGUMENT;                                                                        /* Require complete model and microphone configuration. */
    } /* End scope: Require complete model and microphone configuration. */
#if defined(__linux__) && !defined(__ANDROID__) && /* Bind only actual statically linked SDKs. */ \
    defined(FVM_HAS_LINUX_STT)                     /* Bind only actual statically linked SDKs. */
    out->struct_size = sizeof(*out);               /* Identify the complete recognition-provider layout. */
    out->abi_version = FVM_SPEECH_ABI_VERSION;     /* Identify compatible speech callbacks. */
    out->os_name = "Linux/ALSA + Vosk";            /* Label the actual capture and recognition libraries. */
    out->userdata =                                /* Borrow stable host configuration for explicit native open. */
        const_cast<FVM_LinuxSttConfig *>(config);  /* Borrow stable host configuration for explicit native open. */
    out->open = fvm_linux_stt_open;                /* Bind real model loading and microphone opening. */
    out->poll = fvm_linux_stt_poll;                /* Bind real bounded capture and transcript processing. */
    out->close = fvm_linux_stt_close;              /* Bind real microphone/model cleanup. */
    return FVM_OK;                                 /* No capture occurs until the host explicitly opens a session. */
#else                                              /* Keep the public API available without optional models/libraries. */
    return FVM_UNSUPPORTED; /* Do not return fabricated recognition without a real SDK. */
#endif                                             /* Finish actual SDK selection. */
} /* Finish explicit Linux recognition binding. */
