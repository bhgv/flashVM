/* OS: provider-selected. Explicitly open one unmanaged TTS session. */
#include "../session.hpp" /* Import copied provider ownership. */
#include <new>            /* Allocate without C++ exceptions. */


/* Publish only a successfully opened native speech session. */
FVM_Result fvm_tts_open(const FVM_TtsBackend *backend, /* Host configuration is borrowed during open. */
                        FVM_TtsSession **out)          /* Host configuration is borrowed during open. */
{                                                      /* Begin explicit speech opening. */
    if (!out)                                          /* Require a writable session slot. */
    {                                                  /* Begin conditional branch: Require a writable session slot. */
        return FVM_INVALID_ARGUMENT;                   /* Require a writable session slot. */
    } /* End conditional branch: Require a writable session slot. */
    *out = 0;                                                                                                     /* Clear stale handles before validation. */
    if (!backend || backend->struct_size != sizeof(*backend) || backend->abi_version != FVM_SPEECH_ABI_VERSION || /* Require a complete compatible provider. */
        !backend->open || !backend->speak || !backend->stop || !backend->close)                                   /* Require a complete compatible provider. */
    {                                                                                                             /* Begin scope: Require a complete compatible provider. */
        return FVM_INVALID_ARGUMENT;                                                                              /* Require a complete compatible provider. */
    } /* End scope: Require a complete compatible provider. */
    FVM_TtsSession *session = new (std::nothrow) FVM_TtsSession; /* Allocate outside the VM collector. */
    if (!session)                                                /* Do not open native resources after wrapper failure. */
    {                                                            /* Begin conditional branch: Do not open native resources after wrapper failure. */
        return FVM_OUT_OF_MEMORY;                                /* Do not open native resources after wrapper failure. */
    } /* End conditional branch: Do not open native resources after wrapper failure. */
    session->backend = *backend;                                            /* Snapshot callbacks without copying host-owned configuration. */
    session->native = 0;                                                    /* Establish partial-open safety. */
    FVM_Result result = backend->open(backend->userdata, &session->native); /* Explicitly open the real provider. */
    if (result != FVM_OK || !session->native)                               /* Reject failed or empty successful native handles. */
    {                                                                       /* Release only the unpublished wrapper; failed callbacks clean partial native state. */
        delete session;                                                     /* Avoid calling close on an unsuccessful open. */
        return result == FVM_OK ? FVM_ERROR : result;                       /* Preserve real failure categories. */
    } /* Finish unsuccessful opening. */
    *out = session; /* Transfer complete native/session ownership to the caller. */
    return FVM_OK;  /* Report an opened speech provider. */
} /* Finish speech opening. */
