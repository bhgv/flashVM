/* OS: Windows. Poll one final SAPI transcript without waiting for microphone input. */
#include "../windows.hpp" /* Import native event/result ownership. */
#ifdef _WIN32             /* Isolate native SAPI event access. */


/* Retain a recognized transcript until the caller supplies sufficient UTF-8 storage. */
FVM_Result fvm_windows_stt_poll(void *opaque, char *text, size_t capacity, size_t *required, /* Query mode never consumes the pending result. */
                                int *final)                                                  /* Query mode never consumes the pending result. */
{                                                                                            /* Begin native transcript retrieval. */
    FVM_WindowsStt *state =                                                                  /* Recover this session's event queue and pending text. */
        static_cast<FVM_WindowsStt *>(opaque);                                               /* Recover this session's event queue and pending text. */
    if (!state->pending)                                                                     /* Retrieve a native event only if no earlier transcript is retained. */
    {                                                                                        /* Inspect at most one final-recognition event. */
        SPEVENT event;                                                                       /* Receive a referenced native recognition result. */
        ULONG fetched = 0;                                                                   /* Distinguish an empty queue from a populated event. */
        HRESULT result =                                                                     /* Poll the real SAPI event queue without waiting. */
            state->context->GetEvents(1, &event, &fetched);                                  /* Poll the real SAPI event queue without waiting. */
        if (FAILED(result))                                                                  /* Preserve native queue failures. */
        {                                                                                    /* Begin conditional branch: Preserve native queue failures. */
            return FVM_ERROR;                                                                /* Preserve native queue failures. */
        } /* End conditional branch: Preserve native queue failures. */
        if (!fetched)             /* Report no transcript yet without blocking. */
        {                         /* Begin conditional branch: Report no transcript yet without blocking. */
            return FVM_NOT_FOUND; /* Report no transcript yet without blocking. */
        } /* End conditional branch: Report no transcript yet without blocking. */
        ISpRecoResult *recognition = reinterpret_cast<ISpRecoResult *>(                                       /* Take the reference attached to the requested recognition event. */
                                                                       event.lParam);                         /* Take the reference attached to the requested recognition event. */
        if (event.eEventId != SPEI_RECOGNITION || event.elParamType != SPET_LPARAM_IS_OBJECT || !recognition) /* Require the exact event shape selected at open. */
        {                                                                                                     /* Begin conditional branch: Require the exact event shape selected at open. */
            return FVM_ERROR;                                                                                 /* Require the exact event shape selected at open. */
        } /* End conditional branch: Require the exact event shape selected at open. */
        result = recognition->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &state->pending, /* Obtain actual recognized dictation as owned UTF-16 text. */
                                      0);                                                          /* Obtain actual recognized dictation as owned UTF-16 text. */
        recognition->Release();                                                                    /* Release the native event's COM result reference. */
        if (FAILED(result) || !state->pending)                                                     /* Do not expose an absent native transcript. */
        {                                                                                          /* Begin conditional branch: Do not expose an absent native transcript. */
            return FVM_ERROR;                                                                      /* Do not expose an absent native transcript. */
        } /* End conditional branch: Do not expose an absent native transcript. */
    } /* Finish event-to-transcript conversion. */
    int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, state->pending, -1, 0, 0, 0, /* Query strict UTF-8 bytes including NUL. */
                                    0);                                                         /* Query strict UTF-8 bytes including NUL. */
    if (bytes <= 0)                                                                             /* Preserve invalid native Unicode rather than publishing corrupted text. */
    {                                                                                           /* Begin conditional branch: Preserve invalid native Unicode rather than publishing corrupted text. */
        return FVM_ERROR;                                                                       /* Preserve invalid native Unicode rather than publishing corrupted text. */
    } /* End conditional branch: Preserve invalid native Unicode rather than publishing corrupted text. */
    *required = (size_t)bytes; /* Report sufficient caller storage precisely. */
    *final = 1;                /* This backend intentionally requests only final dictation results. */
    if (!capacity)             /* Keep pending text for the following sized copy. */
    {                          /* Begin conditional branch: Keep pending text for the following sized copy. */
        return FVM_OK;         /* Keep pending text for the following sized copy. */
    } /* End conditional branch: Keep pending text for the following sized copy. */
    if (capacity < *required)        /* Preserve pending text after an undersized destination. */
    {                                /* Begin conditional branch: Preserve pending text after an undersized destination. */
        return FVM_INVALID_ARGUMENT; /* Preserve pending text after an undersized destination. */
    } /* End conditional branch: Preserve pending text after an undersized destination. */
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, state->pending, -1, text, bytes, 0, 0)) /* Copy actual recognized text into host-owned memory. */
    {                                                                                               /* Begin conditional branch: Copy actual recognized text into host-owned memory. */
        return FVM_ERROR;                                                                           /* Copy actual recognized text into host-owned memory. */
    } /* End conditional branch: Copy actual recognized text into host-owned memory. */
    CoTaskMemFree(state->pending); /* Release the native transcript only after successful copying. */
    state->pending = 0;            /* Permit retrieval of the next recognition event. */
    return FVM_OK;                 /* The caller now owns its copied transcript bytes. */
} /* Finish native transcript polling. */
#endif /* Finish Windows-only implementation. */
