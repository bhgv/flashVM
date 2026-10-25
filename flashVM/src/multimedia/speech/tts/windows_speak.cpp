/* OS: Windows. Convert strict UTF-8 and enqueue speech using ISpVoice. */
#include "../windows.hpp" /* Import native voice ownership. */
#ifdef _WIN32             /* Keep Windows calls isolated. */
#include <new>            /* Allocate a temporary UTF-16 buffer without exceptions. */


/* Submit plain text, never SAPI XML, and release temporary input after native copying. */
FVM_Result fvm_windows_tts_speak(void *opaque, const char *text)                 /* Accepted work may continue asynchronously. */
{                                                                                /* Begin actual speech submission. */
    FVM_WindowsTts *state = static_cast<FVM_WindowsTts *>(opaque);               /* Recover the thread-affine voice. */
    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, 0, /* Query strict UTF-16 capacity including NUL. */
                                     0);                                         /* Query strict UTF-16 capacity including NUL. */
    if (!length)                                                                 /* Reject malformed UTF-8 explicitly. */
    {                                                                            /* Begin conditional branch: Reject malformed UTF-8 explicitly. */
        return FVM_INVALID_ARGUMENT;                                             /* Reject malformed UTF-8 explicitly. */
    } /* End conditional branch: Reject malformed UTF-8 explicitly. */
    wchar_t *wide = new (std::nothrow) wchar_t[length]; /* Own temporary native text storage. */
    if (!wide)                                          /* Preserve allocation failure. */
    {                                                   /* Begin conditional branch: Preserve allocation failure. */
        return FVM_OUT_OF_MEMORY;                       /* Preserve allocation failure. */
    } /* End conditional branch: Preserve allocation failure. */
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, wide, /* Convert with the real Windows Unicode API. */
                             length))                                       /* Convert with the real Windows Unicode API. */
    {                                                                       /* Release temporary storage after unexpected conversion failure. */
        delete[] wide;                                                      /* Avoid leaking text buffers. */
        return FVM_INVALID_ARGUMENT;                                        /* Do not submit partially converted text. */
    } /* Finish conversion error handling. */
    HRESULT result = state->voice->Speak(wide, SPF_ASYNC | SPF_IS_NOT_XML, /* Queue real system speech without interpreting markup. */
                                         0);                               /* Queue real system speech without interpreting markup. */
    delete[] wide;                                                         /* SAPI copies the submitted asynchronous text. */
    return SUCCEEDED(result) ? FVM_OK : FVM_ERROR;                         /* Report enqueue acceptance, not audible completion. */
} /* Finish native text submission. */
#endif /* Finish Windows-only implementation. */
